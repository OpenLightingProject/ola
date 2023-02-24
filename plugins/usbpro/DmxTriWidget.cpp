/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DmxTriWidget.cpp
 * The Jese DMX TRI device.
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/io/ByteString.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/stl/STLUtils.h"
#include "ola/strings/Format.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/DmxTriWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::io::ByteString;
using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RunRDMCallback;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::strings::ToHex;
using std::auto_ptr;
using std::map;
using std::string;
using std::vector;

/*
 * New DMX TRI Widget
 */
DmxTriWidgetImpl::DmxTriWidgetImpl(
    ola::thread::SchedulerInterface *scheduler,
    ola::io::ConnectedDescriptor *descriptor,
    bool use_raw_rdm)
    : BaseUsbProWidget(descriptor),
      m_scheduler(scheduler),
      m_uid_count(0),
      m_last_esta_id(UID::ALL_MANUFACTURERS),
      m_use_raw_rdm(use_raw_rdm),
      m_disc_stat_timeout_id(ola::thread::INVALID_TIMEOUT),
      m_discovery_callback(NULL),
      m_discovery_state(NO_DISCOVERY_ACTION),
      m_rdm_request_callback(NULL),
      m_pending_rdm_request(NULL),
      m_transaction_number(0),
      m_last_command(RESERVED_COMMAND_ID),
      m_expected_command(RESERVED_COMMAND_ID) {
}


/*
 * Shutdown
 */
DmxTriWidgetImpl::~DmxTriWidgetImpl() {
  Stop();
}


/**
 * Stop the rdm discovery process if it's running
 */
void DmxTriWidgetImpl::Stop() {
  if (m_disc_stat_timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_scheduler->RemoveTimeout(m_disc_stat_timeout_id);
    m_disc_stat_timeout_id = ola::thread::INVALID_TIMEOUT;
  }

  // timeout any existing message
  if (m_rdm_request_callback) {
    HandleRDMError(ola::rdm::RDM_TIMEOUT);
  }

  if (m_discovery_callback) {
    RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
  }
}


/**
 * Send a DMX frame. This may queue the frame if there is a transaction
 * pending.
 */
bool DmxTriWidgetImpl::SendDMX(const DmxBuffer &buffer) {
  // Update the buffer, if there is already a frame pending, we send take the
  // latest one.
  if (m_outgoing_dmx.Size()) {
    OLA_INFO << "TRI widget dropping frame";
  }
  m_outgoing_dmx = buffer;
  MaybeSendNextRequest();
  return true;
}


/*
 * Send an RDM request. Because this is wrapped in a QueueingRDMController it's
 * only going to be called one-at-a-time. This will queue the RDM request if
 * there is a transaction pending.
 */
void DmxTriWidgetImpl::SendRDMRequest(RDMRequest *request_ptr,
                                      ola::rdm::RDMCallback *on_complete) {
  auto_ptr<RDMRequest> request(request_ptr);

  if (request->CommandClass() == RDMCommand::DISCOVER_COMMAND &&
      !m_use_raw_rdm) {
    RunRDMCallback(on_complete, ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED);
    return;
  }

  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  // store pointers
  m_pending_rdm_request.reset(request.release());
  m_rdm_request_callback = on_complete;
  MaybeSendNextRequest();
}


/**
 * Start the discovery process
 */
void DmxTriWidgetImpl::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  // Right now there is no difference between full and incremental discovery.
  RunIncrementalDiscovery(callback);
}


/**
 * Start incremental discovery, which on the TRI is the same as full.
 */
void DmxTriWidgetImpl::RunIncrementalDiscovery(
    RDMDiscoveryCallback *callback) {
  if (m_discovery_callback) {
    OLA_FATAL << "Call to RunFullDiscovery while discovery is already running"
      << ", the DiscoverableQueueingRDMController has broken!";
    // the best we can do is run the callback now
    RunDiscoveryCallback(callback);
    return;
  }

  m_discovery_callback = callback;
  m_discovery_state = DISCOVER_AUTO_REQUIRED;
  MaybeSendNextRequest();
}


/**
 * Send the outstanding DMXBuffer.
 */
void DmxTriWidgetImpl::SendDMXBuffer() {
  // CommandID, Options, Start Code + data
  uint8_t send_buffer[3 + DMX_UNIVERSE_SIZE];
  send_buffer[0] = SINGLE_TX_COMMAND_ID;
  send_buffer[1] = 0;  // wait for command completion
  send_buffer[2] = DMX512_START_CODE;

  unsigned int length = DMX_UNIVERSE_SIZE;
  m_outgoing_dmx.Get(send_buffer + 3, &length);
  m_outgoing_dmx.Reset();
  SendCommandToTRI(EXTENDED_COMMAND_LABEL, send_buffer, length + 3);
}


/**
 * Send the queued up RDM command.
 */
void DmxTriWidgetImpl::SendQueuedRDMCommand() {
  // If we can't find this UID, fail now.
  const UID &dest_uid = m_pending_rdm_request->DestinationUID();
  if (!dest_uid.IsBroadcast() && !STLContains(m_uid_index_map, dest_uid)) {
    HandleRDMError(ola::rdm::RDM_UNKNOWN_UID);
    return;
  }

  if (m_use_raw_rdm) {
    SendRawRDMRequest();
    return;
  }

  if (dest_uid.IsBroadcast() && dest_uid.ManufacturerId() != m_last_esta_id) {
    // we have to send a Set filter command
    uint16_t esta_id = dest_uid.ManufacturerId();
    uint8_t data[] = {SET_FILTER_COMMAND_ID,
                      static_cast<uint8_t>(esta_id >> 8),
                      static_cast<uint8_t>(esta_id & 0xff)
                     };
    if (!SendCommandToTRI(EXTENDED_COMMAND_LABEL,
                          reinterpret_cast<uint8_t*>(&data),
                          sizeof(data))) {
      OLA_INFO << "Failed to send set filter, aborting request";
      HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
    }
    return;
  } else {
    DispatchRequest();
  }
}


/**
 * Call the UIDSet handler with the latest UID list.
 */
void DmxTriWidgetImpl::RunDiscoveryCallback(RDMDiscoveryCallback *callback) {
  if (!callback)
    return;

  UIDSet uid_set;
  UIDToIndexMap::iterator iter = m_uid_index_map.begin();
  for (; iter != m_uid_index_map.end(); ++iter) {
    uid_set.AddUID(iter->first);
  }
  callback->Run(uid_set);
}


/*
 * Check the status of the RDM discovery process.
 * This is called periodically while discovery is running
 */
bool DmxTriWidgetImpl::CheckDiscoveryStatus() {
  m_discovery_state = DISCOVER_STATUS_REQUIRED;
  MaybeSendNextRequest();
  return true;
}


/*
 * Handle a message received from the widget
 */
void DmxTriWidgetImpl::HandleMessage(uint8_t label,
                                     const uint8_t *data,
                                     unsigned int length) {
  if (label == EXTENDED_COMMAND_LABEL) {
    if (length < DATA_OFFSET) {
      OLA_WARN << "DMX-TRI frame too small";
      return;
    }

    uint8_t command_id = data[0];
    uint8_t return_code = data[1];
    length -= DATA_OFFSET;
    data = length ? data + DATA_OFFSET: NULL;

    if (command_id != m_expected_command) {
      OLA_WARN << "Received an unexpected command response, expected "
               << ToHex(m_expected_command) << ", got " << ToHex(command_id);
    }
    m_last_command = m_expected_command;
    m_expected_command = RESERVED_COMMAND_ID;

    switch (command_id) {
      case SINGLE_TX_COMMAND_ID:
        HandleSingleTXResponse(return_code);
        break;
      case DISCOVER_AUTO_COMMAND_ID:
        HandleDiscoveryAutoResponse(return_code, data, length);
        break;
      case DISCOVER_STATUS_COMMAND_ID:
        HandleDiscoverStatResponse(return_code, data, length);
        break;
      case REMOTE_UID_COMMAND_ID:
        HandleRemoteUIDResponse(return_code, data, length);
        break;
      case RAW_RDM_COMMAND_ID:
        HandleRawRDMResponse(return_code, data, length);
        break;
      case REMOTE_GET_COMMAND_ID:
        HandleRemoteRDMResponse(return_code, data, length);
        break;
      case REMOTE_SET_COMMAND_ID:
        HandleRemoteRDMResponse(return_code, data, length);
        break;
      case QUEUED_GET_COMMAND_ID:
        HandleQueuedGetResponse(return_code, data, length);
        break;
      case SET_FILTER_COMMAND_ID:
        HandleSetFilterResponse(return_code, data, length);
        break;
      default:
        OLA_WARN << "Unknown DMX-TRI CI: " << ToHex(command_id);
    }
  } else {
    OLA_INFO << "DMX-TRI got response " << static_cast<int>(label);
  }
}


/*
 * Send a DiscoAuto message to begin the discovery process.
 */
void DmxTriWidgetImpl::SendDiscoveryAuto() {
  m_discovery_state = NO_DISCOVERY_ACTION;
  uint8_t command_id = DISCOVER_AUTO_COMMAND_ID;
  if (!SendCommandToTRI(EXTENDED_COMMAND_LABEL, &command_id,
                        sizeof(command_id))) {
    OLA_WARN << "Unable to begin RDM discovery";
    RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
  } else {
    // setup a stat every RDM_STATUS_INTERVAL_MS until we're done
    m_disc_stat_timeout_id = m_scheduler->RegisterRepeatingTimeout(
        RDM_STATUS_INTERVAL_MS,
        NewCallback(this, &DmxTriWidgetImpl::CheckDiscoveryStatus));
  }
}


/*
 * Send a RemoteUID message to fetch UID in TRI register.
 */
void DmxTriWidgetImpl::FetchNextUID() {
  m_discovery_state = NO_DISCOVERY_ACTION;
  if (!m_uid_count)
    return;

  OLA_INFO << "Fetching index  " << static_cast<int>(m_uid_count);
  uint8_t data[] = {REMOTE_UID_COMMAND_ID, m_uid_count};
  SendCommandToTRI(EXTENDED_COMMAND_LABEL, data, sizeof(data));
}

/*
 * Send a DiscoStat message to get status of the discovery process.
 */
void DmxTriWidgetImpl::SendDiscoveryStat() {
  m_discovery_state = NO_DISCOVERY_ACTION;
  uint8_t command = DISCOVER_STATUS_COMMAND_ID;
  if (!SendCommandToTRI(EXTENDED_COMMAND_LABEL, &command, sizeof(command))) {
    RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
  }
}


/**
 * Send a raw RDM command, bypassing all the handling the RDM-TRI does.
 */
void DmxTriWidgetImpl::SendRawRDMRequest() {
  m_pending_rdm_request->SetTransactionNumber(m_transaction_number);
  m_pending_rdm_request->SetPortId(1);  // port id is always 1

  // add two bytes for the command & option field
  ByteString data;
  data.push_back(RAW_RDM_COMMAND_ID);
  // a 2 means we don't wait for a break in the response.
  data.push_back(m_pending_rdm_request->IsDUB() ? 2 : 0);

  if (!RDMCommandSerializer::Pack(*m_pending_rdm_request, &data)) {
    OLA_WARN << "Failed to pack RDM request";
    HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  OLA_INFO << "Sending raw request to "
           << m_pending_rdm_request->DestinationUID()
           << " with command " << ToHex(m_pending_rdm_request->CommandClass())
           << " and param " << ToHex(m_pending_rdm_request->ParamId());

  if (SendCommandToTRI(EXTENDED_COMMAND_LABEL, data.data(), data.size())) {
    m_transaction_number++;
  } else {
    HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
  }
}


/*
 * Send the next RDM request, this assumes that SetFilter has been called
 */
void DmxTriWidgetImpl::DispatchRequest() {
  const ola::rdm::RDMRequest *request = m_pending_rdm_request.get();
  if (request->ParamId() == ola::rdm::PID_QUEUED_MESSAGE &&
      request->CommandClass() == RDMCommand::GET_COMMAND) {
    // these are special
    if (request->ParamDataSize()) {
      DispatchQueuedGet();
    } else {
      OLA_WARN << "Missing param data in queued message get";
      HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
    }
    return;
  }

  PACK(
  struct rdm_message {
    uint8_t command;
    uint8_t index;
    uint16_t sub_device;
    uint16_t param_id;
    uint8_t data[RDMCommandSerializer::MAX_PARAM_DATA_LENGTH];
  });

  rdm_message message;

  if (request->CommandClass() == RDMCommand::GET_COMMAND) {
    message.command = REMOTE_GET_COMMAND_ID;
  } else if (request->CommandClass() == RDMCommand::SET_COMMAND) {
    message.command = REMOTE_SET_COMMAND_ID;
  } else {
    OLA_WARN << "Request was not get or set: "
             << static_cast<int>(request->CommandClass());
    HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  if (request->DestinationUID().IsBroadcast()) {
    message.index = 0;
  } else {
    UIDToIndexMap::const_iterator iter =
      m_uid_index_map.find(request->DestinationUID());
    if (iter == m_uid_index_map.end()) {
      OLA_WARN << request->DestinationUID() << " not found in uid map";
      HandleRDMError(ola::rdm::RDM_UNKNOWN_UID);
      return;
    }
    message.index = iter->second;
  }
  message.sub_device = HostToNetwork(request->SubDevice());
  message.param_id = HostToNetwork(request->ParamId());
  if (request->ParamDataSize())
    memcpy(message.data, request->ParamData(), request->ParamDataSize());

  unsigned int size = sizeof(message) -
    RDMCommandSerializer::MAX_PARAM_DATA_LENGTH + request->ParamDataSize();

  OLA_INFO << "Sending request to " << request->DestinationUID()
           << " with command " << ToHex(request->CommandClass())
           << " and param " << ToHex(request->ParamId());

  bool r = SendCommandToTRI(EXTENDED_COMMAND_LABEL,
                            reinterpret_cast<uint8_t*>(&message),
                            size);
  if (!r) {
    HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
  }
}


/*
 * Send a queued get message
 */
void DmxTriWidgetImpl::DispatchQueuedGet() {
  UIDToIndexMap::const_iterator iter =
    m_uid_index_map.find(m_pending_rdm_request->DestinationUID());
  if (iter == m_uid_index_map.end()) {
    OLA_WARN << m_pending_rdm_request->DestinationUID()
             << " not found in uid map";
    HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }
  uint8_t data[3] = {QUEUED_GET_COMMAND_ID,
                     iter->second,
                     m_pending_rdm_request->ParamData()[0]};

  if (!SendCommandToTRI(EXTENDED_COMMAND_LABEL,
                        reinterpret_cast<uint8_t*>(&data),
                        arraysize(data))) {
    HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
  }
}


/*
 * Stop the discovery process
 */
void DmxTriWidgetImpl::StopDiscovery() {
  if (m_disc_stat_timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_scheduler->RemoveTimeout(m_disc_stat_timeout_id);
    m_disc_stat_timeout_id = ola::thread::INVALID_TIMEOUT;
  }
}


/**
 * Handle the response from Single TX.
 */
void DmxTriWidgetImpl::HandleSingleTXResponse(uint8_t return_code) {
  if (return_code != EC_NO_ERROR)
    OLA_WARN << "Error sending DMX data. TRI return code was "
             << ToHex(return_code);
  MaybeSendNextRequest();
}


/*
 * Handle the response from calling DiscoAuto
 */
void DmxTriWidgetImpl::HandleDiscoveryAutoResponse(uint8_t return_code,
                                                   const uint8_t*,
                                                   unsigned int) {
  if (return_code != EC_NO_ERROR) {
    if (return_code == EC_UNKNOWN_COMMAND)
      OLA_INFO << "This DMX-TRI doesn't support RDM";
    else
      OLA_WARN << "DMX_TRI discovery returned error " <<
        static_cast<int>(return_code);
    StopDiscovery();
    RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
  }
}


/*
 * Handle the response from calling discovery stat
 */
void DmxTriWidgetImpl::HandleDiscoverStatResponse(uint8_t return_code,
                                                  const uint8_t *data,
                                                  unsigned int length) {
  if (return_code == EC_NO_ERROR || return_code == EC_RESPONSE_UNEXPECTED) {
    if (return_code == EC_RESPONSE_UNEXPECTED)
      OLA_INFO << "Got an unexpected RDM response during discovery";

    if (length < 1) {
      // don't cancel the timer, try again after RDM_STATUS_INTERVAL_MS
      OLA_WARN << "DiscoStat command too short, was " << length;
      return;
    }

    if (data[1] == 0) {
      OLA_DEBUG << "Discovery process has completed, "
                << static_cast<int>(data[0]) << " devices found";
      StopDiscovery();
      m_uid_count = data[0];
      m_uid_index_map.clear();
      if (m_uid_count) {
        m_discovery_state = FETCH_UID_REQUIRED;
        MaybeSendNextRequest();
      } else {
        RDMDiscoveryCallback *callback = m_discovery_callback;
        m_discovery_callback = NULL;
        RunDiscoveryCallback(callback);
      }
    }
  } else {
    // These are all fatal
    switch (return_code) {
      case EC_RESPONSE_MUTE:
        OLA_WARN << "Unable to mute device, aborting discovery";
        break;
      case EC_RESPONSE_DISCOVERY:
        OLA_WARN <<
          "Duplicated or erroneous device detected, aborting discovery";
        break;
      default:
        OLA_WARN << "DMX_TRI discovery returned error " <<
          static_cast<int>(return_code);
        break;
    }
    // clear out the old map
    m_uid_index_map.clear();
    StopDiscovery();
    RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
  }
}


/*
 * Handle the response to a RemoteGet command
 */
void DmxTriWidgetImpl::HandleRemoteUIDResponse(uint8_t return_code,
                                               const uint8_t *data,
                                               unsigned int length) {
  if (!m_uid_count) {
    // not expecting any responses
    OLA_INFO << "Got an unexpected RemoteUID response";
    return;
  }

  if (return_code == EC_NO_ERROR) {
    if (length < ola::rdm::UID::UID_SIZE) {
      OLA_INFO << "Short RemoteUID response, was " << length;
    } else {
      const UID uid(data);
      m_uid_index_map[uid] = m_uid_count;
    }
  } else if (return_code == EC_CONSTRAINT) {
    // this is returned if the index is wrong
    OLA_INFO << "RemoteUID returned RC_Constraint, some how we botched the"
      << " discovery process, subtracting 1 and attempting to continue";
  } else {
    OLA_INFO << "RemoteUID returned " << static_cast<int>(return_code);
  }

  m_uid_count--;

  if (m_uid_count) {
    m_discovery_state = FETCH_UID_REQUIRED;
    MaybeSendNextRequest();
  } else {
    RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
  }
}


/*
 * Handle the response to a raw RDM command
 * data will be NULL, if length is 0.
 */
void DmxTriWidgetImpl::HandleRawRDMResponse(uint8_t return_code,
                                            const uint8_t *data,
                                            unsigned int length) {
  OLA_INFO << "got raw RDM response with code: " << ToHex(return_code)
           << ", length: " << length;

  auto_ptr<ola::rdm::RDMRequest> request(m_pending_rdm_request);
  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_pending_rdm_request.reset();
  m_rdm_request_callback = NULL;

  if (callback == NULL || request.get() == NULL) {
    OLA_FATAL << "Got a response but missing callback or request object!";
    return;
  }

  if (return_code == EC_UNKNOWN_COMMAND) {
    // This means raw mode isn't supported and we should fall back to the
    // default mode
    m_use_raw_rdm = false;
    OLA_WARN <<
      "Raw RDM mode not supported, switching back to the managed RDM mode";
    SendRDMRequest(request.release(), callback);
    return;
  }

  // handle responses to DUB commands
  if (request->IsDUB()) {
    if (return_code == EC_RESPONSE_NONE) {
      RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
    } else if (return_code == EC_NO_ERROR ||
               return_code == EC_RESPONSE_DISCOVERY) {
      rdm::RDMFrame frame(data, length);
      auto_ptr<RDMReply> reply(RDMReply::DUBReply(frame));
      callback->Run(reply.get());
    } else {
      OLA_WARN << "Un-handled DUB response " << ToHex(return_code);
      RunRDMCallback(callback, ola::rdm::RDM_INVALID_RESPONSE);
    }
    return;
  }

  const UID &dest_uid = request->DestinationUID();

  if (dest_uid.IsBroadcast()) {
    if (return_code != EC_RESPONSE_NONE) {
      OLA_WARN << "Unexpected response to broadcast message";
    }
    RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
    return;
  }

  if (return_code == EC_RESPONSE_NONE) {
    RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
    return;
  }

  rdm::RDMFrame::Options options;
  options.prepend_start_code = true;
  auto_ptr<RDMReply> reply(RDMReply::FromFrame(
        rdm::RDMFrame(data, length, options)));
  callback->Run(reply.get());
}


/*
 * Handle the response to a Remote Get/Set command
 */
void DmxTriWidgetImpl::HandleRemoteRDMResponse(uint8_t return_code,
                                               const uint8_t *data,
                                               unsigned int length) {
  if (m_pending_rdm_request.get() == NULL) {
    OLA_FATAL << "Got a response but missing callback or request object!";
    return;
  }

  OLA_INFO << "Received RDM response with code " << ToHex(return_code)
           << ", " << length << " bytes, param "
           << ToHex(m_pending_rdm_request->ParamId());

  HandleGenericRDMResponse(return_code,
                           m_pending_rdm_request->ParamId(),
                           data,
                           length);
}


/*
 * Handle the response to a QueuedGet command
 */
void DmxTriWidgetImpl::HandleQueuedGetResponse(uint8_t return_code,
                                               const uint8_t *data,
                                               unsigned int length) {
  if (length < 2) {
    OLA_FATAL << "Queued response too small, was " << length << " bytes";
    HandleRDMError(ola::rdm::RDM_INVALID_RESPONSE);
    return;
  }

  uint16_t pid;
  memcpy(reinterpret_cast<uint8_t*>(&pid), data, sizeof(pid));
  pid = NetworkToHost(pid);

  data += 2;
  length -= 2;

  OLA_INFO << "Received queued message response with code "
           << ToHex(return_code) << ", " << length << " bytes, param "
           << ToHex(pid);

  if (!length)
    data = NULL;
  HandleGenericRDMResponse(return_code, pid, data, length);
}


/*
 * Handle a RDM response for both queued and normal messages.
 */
void DmxTriWidgetImpl::HandleGenericRDMResponse(uint8_t return_code,
                                                uint16_t pid,
                                                const uint8_t *data,
                                                unsigned int length) {
  auto_ptr<const ola::rdm::RDMRequest> request(m_pending_rdm_request.release());
  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;

  if (callback == NULL || request.get() == NULL) {
    OLA_FATAL << "Got a response but missing callback or request object!";
    return;
  }

  ola::rdm::RDMResponse *response = NULL;
  ola::rdm::RDMStatusCode code = ola::rdm::RDM_COMPLETED_OK;
  ola::rdm::rdm_nack_reason reason;

  if (ReturnCodeToNackReason(return_code, &reason)) {
    response = ola::rdm::NackWithReason(request.get(), reason);
    code = ola::rdm::RDM_COMPLETED_OK;
  } else if (return_code == EC_NO_ERROR) {
    if (request->DestinationUID().IsBroadcast()) {
      code = ola::rdm::RDM_WAS_BROADCAST;
    } else {
      code = ola::rdm::RDM_COMPLETED_OK;
      response = ola::rdm::GetResponseWithPid(
          request.get(),
          pid,
          data,
          length);
    }
  } else if (return_code == EC_RESPONSE_TIME) {
    response = ola::rdm::GetResponseWithPid(request.get(),
                                            pid,
                                            data,
                                            length,
                                            ola::rdm::RDM_ACK_TIMER);
  } else if (return_code == EC_RESPONSE_WAIT) {
    // this is a hack, there is no way to expose # of queued messages
    response = ola::rdm::GetResponseWithPid(request.get(),
                                            pid,
                                            data,
                                            length,
                                            ola::rdm::RDM_ACK,
                                            1);
  } else if (return_code == EC_RESPONSE_MORE) {
    response = ola::rdm::GetResponseWithPid(request.get(),
                                            pid,
                                            data,
                                            length,
                                            ola::rdm::ACK_OVERFLOW);
  } else if (!TriToOlaReturnCode(return_code, &code)) {
    OLA_WARN << "Response was returned with " << ToHex(return_code);
    code = ola::rdm::RDM_INVALID_RESPONSE;
  }
  // Unfortunately we don't get to see the raw response here, which limits the
  // use of the TRI for testing. For testing use the raw mode.
  RDMReply reply(code, response);
  callback->Run(&reply);
}

/*
 * Handle a setfilter response
 */
void DmxTriWidgetImpl::HandleSetFilterResponse(uint8_t return_code,
                                               const uint8_t*, unsigned int) {
  if (!m_pending_rdm_request.get()) {
    OLA_WARN << "Set filter response but no RDM message to send!";
    return;
  }

  if (return_code == EC_NO_ERROR) {
    m_last_esta_id = m_pending_rdm_request->DestinationUID().ManufacturerId();
    DispatchRequest();
  } else {
    OLA_WARN << "SetFilter returned " << static_cast<int>(return_code) <<
      ", we have no option but to drop the rdm request";
    HandleRDMError(ola::rdm::RDM_FAILED_TO_SEND);
  }
}


/**
 * Return true if there is an outstanding transaction pending.
 */
bool DmxTriWidgetImpl::PendingTransaction() const {
  return m_expected_command != RESERVED_COMMAND_ID;
}

/**
 * Check if there is anything waiting to be sent.
 */
void DmxTriWidgetImpl::MaybeSendNextRequest() {
  // sending may fail, so we loop until there is nothing to do or there is a
  // transaction pending.
  bool first = true;
  while (true) {
    if (PendingTransaction()) {
      if (first)
        OLA_DEBUG << "Transaction in progress, delaying send";
      return;
    }
    first = false;

    if (m_outgoing_dmx.Size() && m_last_command != SINGLE_TX_COMMAND_ID) {
      // avoid starving out DMX frames
      SendDMXBuffer();
    } else if (m_pending_rdm_request.get()) {
      // there is an RDM command to send
      SendQueuedRDMCommand();
    } else if (m_discovery_state == DISCOVER_AUTO_REQUIRED) {
      SendDiscoveryAuto();
    } else if (m_discovery_state == DISCOVER_STATUS_REQUIRED) {
      SendDiscoveryStat();
    } else if (m_discovery_state == FETCH_UID_REQUIRED) {
      FetchNextUID();
    } else if (m_outgoing_dmx.Size()) {
      // there is a DMX frame to send
      SendDMXBuffer();
    } else {
      return;
    }
  }
}


/**
 * Return an error on the RDM callback and handle the clean up. This assumes
 * that m_pending_rdm_request and m_rdm_request_callback are non-null.
 */
void DmxTriWidgetImpl::HandleRDMError(ola::rdm::RDMStatusCode status_code) {
  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  m_pending_rdm_request.reset();
  if (callback) {
    RunRDMCallback(callback, status_code);
  }
}


/**
 * Send data to the TRI, and set the flag indicating a transaction is pending.
 */
bool DmxTriWidgetImpl::SendCommandToTRI(uint8_t label, const uint8_t *data,
                                        unsigned int length) {
  bool r = SendMessage(label, data, length);
  if (r && label == EXTENDED_COMMAND_LABEL && length) {
    OLA_DEBUG << "Sent command " << ToHex(data[0]);
    m_expected_command = data[0];
  }
  return r;
}

/**
 * Convert a DMX TRI return code to the appropriate RDMStatusCode
 * @return true if this was a matching code, false otherwise
 */
bool DmxTriWidgetImpl::TriToOlaReturnCode(
    uint8_t return_code,
    ola::rdm::RDMStatusCode *code) {
  switch (return_code) {
    case EC_RESPONSE_TRANSACTION:
      *code = ola::rdm::RDM_TRANSACTION_MISMATCH;
      break;
    case EC_RESPONSE_SUB_DEVICE:
      *code = ola::rdm::RDM_SUB_DEVICE_MISMATCH;
      break;
    case EC_RESPONSE_FORMAT:
      *code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case EC_RESPONSE_CHECKSUM:
      *code = ola::rdm::RDM_CHECKSUM_INCORRECT;
      break;
    case EC_RESPONSE_NONE:
      *code = ola::rdm::RDM_TIMEOUT;
      break;
    case EC_RESPONSE_IDENTITY:
      *code = ola::rdm::RDM_SRC_UID_MISMATCH;
      break;
    default:
      return false;
  }
  return true;
}


/**
 * Convert a DMX-TRI return code to Nack reason code if appropriate
 */
bool DmxTriWidgetImpl::ReturnCodeToNackReason(
    uint8_t return_code,
    ola::rdm::rdm_nack_reason *reason) {
  switch (return_code) {
    case EC_UNKNOWN_PID:
      *reason = ola::rdm::NR_UNKNOWN_PID;
      break;
    case EC_FORMAT_ERROR:
      *reason = ola::rdm::NR_FORMAT_ERROR;
      break;
    case EC_HARDWARE_FAULT:
      *reason = ola::rdm::NR_HARDWARE_FAULT;
      break;
    case EC_PROXY_REJECT:
      *reason = ola::rdm::NR_PROXY_REJECT;
      break;
    case EC_WRITE_PROTECT:
      *reason = ola::rdm::NR_WRITE_PROTECT;
      break;
    case EC_UNSUPPORTED_COMMAND_CLASS:
      *reason = ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS;
      break;
    case EC_OUT_OF_RANGE:
      *reason = ola::rdm::NR_DATA_OUT_OF_RANGE;
      break;
    case EC_BUFFER_FULL:
      *reason = ola::rdm::NR_BUFFER_FULL;
      break;
    case EC_FRAME_OVERFLOW:
      *reason = ola::rdm::NR_PACKET_SIZE_UNSUPPORTED;
      break;
    case EC_SUBDEVICE_UNKNOWN:
      *reason = ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE;
      break;
    case EC_PROXY_BUFFER_FULL:
      *reason = ola::rdm::NR_PROXY_BUFFER_FULL;
      break;
    case EC_ACTION_NOT_SUPPORTED:
      *reason = ola::rdm::NR_ACTION_NOT_SUPPORTED;
      break;
    case EC_ENDPOINT_NUMBER_INVALID:
      *reason = ola::rdm::NR_ENDPOINT_NUMBER_INVALID;
      break;
    case EC_INVALID_ENDPOINT_MODE:
      *reason = ola::rdm::NR_INVALID_ENDPOINT_MODE;
      break;
    case EC_UNKNOWN_UID:
      *reason = ola::rdm::NR_UNKNOWN_UID;
      break;
    case EC_UNKNOWN_SCOPE:
      *reason = ola::rdm::NR_UNKNOWN_SCOPE;
      break;
    case EC_INVALID_STATIC_CONFIG_TYPE:
      *reason = ola::rdm::NR_INVALID_STATIC_CONFIG_TYPE;
      break;
    case EC_INVALID_IPV4_ADDRESS:
      *reason = ola::rdm::NR_INVALID_IPV4_ADDRESS;
      break;
    case EC_INVALID_IPV6_ADDRESS:
      *reason = ola::rdm::NR_INVALID_IPV6_ADDRESS;
      break;
    case EC_INVALID_PORT:
      *reason = ola::rdm::NR_INVALID_PORT;
      break;
    default:
      return false;
  }
  return true;
}


/**
 * DmxTriWidget Constructor
 */
DmxTriWidget::DmxTriWidget(ola::thread::SchedulerInterface *scheduler,
                           ola::io::ConnectedDescriptor *descriptor,
                           unsigned int queue_size,
                           bool use_raw_rdm) {
  m_impl = new DmxTriWidgetImpl(scheduler, descriptor, use_raw_rdm);
  m_controller = new ola::rdm::DiscoverableQueueingRDMController(m_impl,
                                                                 queue_size);
}


DmxTriWidget::~DmxTriWidget() {
  // delete the controller after the impl because the controller owns the
  // callback
  delete m_impl;
  delete m_controller;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
