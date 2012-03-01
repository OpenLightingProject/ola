/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DmxTriWidget.h
 * The Jese DMX TRI device.
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/DmxTriWidget.h"
#include "plugins/usbpro/BaseUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::map;
using std::string;
using ola::network::NetworkToHost;
using ola::network::HostToNetwork;
using ola::rdm::RDMCommand;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;


/*
 * New DMX TRI Widget
 */
DmxTriWidgetImpl::DmxTriWidgetImpl(
    ola::thread::SchedulerInterface *scheduler,
    ola::network::ConnectedDescriptor *descriptor,
    bool use_raw_rdm)
    : BaseUsbProWidget(descriptor),
      m_scheduler(scheduler),
      m_rdm_timeout_id(ola::thread::INVALID_TIMEOUT),
      m_uid_count(0),
      m_last_esta_id(UID::ALL_MANUFACTURERS),
      m_use_raw_rdm(use_raw_rdm),
      m_discovery_callback(NULL),
      m_rdm_request_callback(NULL),
      m_pending_request(NULL),
      m_transaction_number(0) {
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
  if (m_rdm_timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_scheduler->RemoveTimeout(m_rdm_timeout_id);
    m_rdm_timeout_id = ola::thread::INVALID_TIMEOUT;
  }

  // timeout any existing message
  if (m_rdm_request_callback) {
    std::vector<string> packets;
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    callback->Run(ola::rdm::RDM_TIMEOUT, NULL, packets);
  }

  if (m_pending_request) {
    delete m_pending_request;
    m_pending_request = NULL;
  }

  if (m_discovery_callback) {
    RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
  }
}


/*
 * Send an RDM request. Because this is wrapped in a QueueingRDMController it's
 * only going to be called one-at-a-time.
 */
void DmxTriWidgetImpl::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  std::vector<string> packets;
  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    delete request;
    return;
  }

  // if we can't find this UID, fail now.
  const UID &dest_uid = request->DestinationUID();
  if (!dest_uid.IsBroadcast() &&
      m_uid_index_map.find(dest_uid) == m_uid_index_map.end()) {
    on_complete->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
    delete request;
    return;
  }

  if (m_use_raw_rdm) {
    SendRawRDMRequest(request, on_complete);
    return;
  }

  if (request->DestinationUID().IsBroadcast() &&
      request->DestinationUID().ManufacturerId() != m_last_esta_id) {
    uint16_t esta_id = request->DestinationUID().ManufacturerId();
    uint8_t data[] = {SET_FILTER_COMMAND_ID, esta_id >> 8, esta_id & 0xff};
    m_pending_request = request;
    m_rdm_request_callback = on_complete;
    bool r = SendMessage(EXTENDED_COMMAND_LABEL,
                         reinterpret_cast<uint8_t*>(&data),
                         sizeof(data));
    if (!r) {
      OLA_INFO << "Failed to send set filter, aborting request";
      delete request;
      m_pending_request = NULL;
      m_rdm_request_callback = NULL;
      on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    }
  } else {
    DispatchRequest(request, on_complete);
  }
}


/**
 * Start the discovery process
 */
void DmxTriWidgetImpl::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  RunRDMDiscovery(callback);
}


/**
 * Start incremental discovery, which on the TRI is the same as full.
 */
void DmxTriWidgetImpl::RunIncrementalDiscovery(
    RDMDiscoveryCallback *callback) {
  RunRDMDiscovery(callback);
}


/*
 * Kick off the discovery process if it's not already running
 */
void DmxTriWidgetImpl::RunRDMDiscovery(RDMDiscoveryCallback *callback) {
  if (m_discovery_callback) {
    OLA_FATAL << "Call to RunFullDiscovery while discovery is already running"
      << ", the DiscoverableQueueingRDMController has broken!";
    // the best we can do is run the callback now
    RunDiscoveryCallback(callback);
    return;
  }

  m_discovery_callback = callback;
  if (!SendDiscoveryStart()) {
    OLA_WARN << "Failed to begin RDM discovery";
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
    return;
  }

  // setup a stat every RDM_STATUS_INTERVAL_MS until we're done
  m_rdm_timeout_id = m_scheduler->RegisterRepeatingTimeout(
      RDM_STATUS_INTERVAL_MS,
      NewCallback(this, &DmxTriWidgetImpl::CheckDiscoveryStatus));
}


/**
 * Call the UIDSet handler with the latest UID list.
 */
void DmxTriWidgetImpl::RunDiscoveryCallback(RDMDiscoveryCallback *callback) {
  if (!callback)
    return;

  UIDSet uid_set;
  map<UID, uint8_t>::iterator iter = m_uid_index_map.begin();
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
  return SendDiscoveryStat();
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

    switch (command_id) {
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
        OLA_WARN << "Unknown DMX-TRI CI: 0x" << std::hex <<
          static_cast<int>(command_id);
    }
  } else {
    OLA_INFO << "DMX-TRI got response " << static_cast<int>(label);
  }
}


/*
 * Return true if discovery is running
 */
bool DmxTriWidgetImpl::InDiscoveryMode() const {
  return (m_rdm_timeout_id != ola::thread::INVALID_TIMEOUT ||
          m_uid_count);
}


/*
 * Send a DiscoAuto message to begin the discovery process.
 */
bool DmxTriWidgetImpl::SendDiscoveryStart() {
  uint8_t command_id = DISCOVER_AUTO_COMMAND_ID;

  return SendMessage(EXTENDED_COMMAND_LABEL,
                     &command_id,
                     sizeof(command_id));
}


/*
 * Send a DiscoAuto message to begin the discovery process.
 */
void DmxTriWidgetImpl::FetchNextUID() {
  if (!m_uid_count)
    return;

  OLA_INFO << "Fetching index  " << static_cast<int>(m_uid_count);
  uint8_t data[] = {REMOTE_UID_COMMAND_ID, m_uid_count};
  SendMessage(EXTENDED_COMMAND_LABEL,
              data,
              sizeof(data));
}


/*
 * Send a DiscoStat message to begin the discovery process.
 */
bool DmxTriWidgetImpl::SendDiscoveryStat() {
  uint8_t command_id = DISCOVER_STATUS_COMMAND_ID;

  return SendMessage(EXTENDED_COMMAND_LABEL,
                     &command_id,
                     sizeof(command_id));
}


/**
 * Send a raw RDM command, bypassing all the handling the RDM-TRI does.
 */
void DmxTriWidgetImpl::SendRawRDMRequest(
    const ola::rdm::RDMRequest *raw_request,
    ola::rdm::RDMCallback *callback) {
  std::vector<string> packets;
  // add two bytes for the command & option field
  const ola::rdm::RDMRequest *request =
    raw_request->DuplicateWithControllerParams(
        raw_request->SourceUID(),
        m_transaction_number,
        1);  // port id is always 1
  delete raw_request;

  unsigned int packet_size = request->Size();
  uint8_t *send_buffer = new uint8_t[packet_size + 2];

  send_buffer[0] = RAW_RDM_COMMAND_ID;
  send_buffer[1] = 0;

  if (!request->Pack(send_buffer + 2, &packet_size)) {
    OLA_WARN << "Failed to pack RDM request";
    delete[] send_buffer;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }

  OLA_INFO << "Sending raw request to " << request->DestinationUID() <<
    " with command " << std::hex << request->CommandClass() << " and param " <<
    std::hex << request->ParamId();

  m_pending_request = request;
  m_rdm_request_callback = callback;
  bool r = SendMessage(EXTENDED_COMMAND_LABEL,
                       send_buffer,
                       packet_size + 2);
  delete[] send_buffer;
  if (r) {
    m_transaction_number++;
  } else {
    m_pending_request = NULL;
    m_rdm_request_callback = NULL;
    delete request;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
  }
}


/*
 * Send the next RDM request, this assumes that SetFilter has been called
 */
void DmxTriWidgetImpl::DispatchRequest(const ola::rdm::RDMRequest *request,
                                       ola::rdm::RDMCallback *callback) {
  std::vector<string> packets;
  if (request->ParamId() == ola::rdm::PID_QUEUED_MESSAGE &&
      request->CommandClass() == RDMCommand::GET_COMMAND) {
    // these are special
    if (request->ParamDataSize()) {
      DispatchQueuedGet(request, callback);
      return;
    } else {
      OLA_WARN << "Missing param data in queued message get";
      delete request;
      callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
      return;
    }
  }

  struct rdm_message {
    uint8_t command;
    uint8_t index;
    uint16_t sub_device;
    uint16_t param_id;
    uint8_t data[ola::rdm::RDMCommand::MAX_PARAM_DATA_LENGTH];
  } __attribute__((packed));

  rdm_message message;

  if (request->CommandClass() == RDMCommand::GET_COMMAND) {
    message.command = REMOTE_GET_COMMAND_ID;
  } else if (request->CommandClass() == RDMCommand::SET_COMMAND) {
    message.command = REMOTE_SET_COMMAND_ID;
  } else {
    OLA_WARN << "Request was not get or set: " <<
      static_cast<int>(request->CommandClass());
    delete request;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }

  if (request->DestinationUID().IsBroadcast()) {
    message.index = 0;
  } else {
    map<UID, uint8_t>::const_iterator iter =
      m_uid_index_map.find(request->DestinationUID());
    if (iter == m_uid_index_map.end()) {
      OLA_WARN << request->DestinationUID() << " not found in uid map";
      delete request;
      callback->Run(ola::rdm::RDM_UNKNOWN_UID, NULL, packets);
      return;
    }
    message.index = iter->second;
  }
  message.sub_device = HostToNetwork(request->SubDevice());
  message.param_id = HostToNetwork(request->ParamId());
  if (request->ParamDataSize())
    memcpy(message.data,
           request->ParamData(),
           request->ParamDataSize());

  unsigned int size = sizeof(message) -
    ola::rdm::RDMCommand::MAX_PARAM_DATA_LENGTH + request->ParamDataSize();

  OLA_INFO << "Sending request to " << request->DestinationUID() <<
    " with command " << std::hex << request->CommandClass() << " and param " <<
    std::hex << request->ParamId();

  m_pending_request = request;
  m_rdm_request_callback = callback;
  bool r = SendMessage(EXTENDED_COMMAND_LABEL,
                       reinterpret_cast<uint8_t*>(&message),
                       size);
  if (!r) {
    m_pending_request = NULL;
    m_rdm_request_callback = NULL;
    delete request;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
  }
}


/*
 * Send a queued get message
 */
void DmxTriWidgetImpl::DispatchQueuedGet(const ola::rdm::RDMRequest* request,
                                         ola::rdm::RDMCallback *callback) {
  std::vector<string> packets;
  map<UID, uint8_t>::const_iterator iter =
    m_uid_index_map.find(request->DestinationUID());
  if (iter == m_uid_index_map.end()) {
    OLA_WARN << request->DestinationUID() << " not found in uid map";
    delete request;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }
  uint8_t data[] = {QUEUED_GET_COMMAND_ID,
                    iter->second,
                    request->ParamData()[0]};

  m_pending_request = request;
  m_rdm_request_callback = callback;
  bool r = SendMessage(EXTENDED_COMMAND_LABEL,
                       reinterpret_cast<uint8_t*>(&data),
                       sizeof(data));

  if (!r) {
    m_pending_request = NULL;
    m_rdm_request_callback = NULL;
    delete request;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
  }
}


/*
 * Stop the discovery process
 */
void DmxTriWidgetImpl::StopDiscovery() {
  if (m_rdm_timeout_id != ola::thread::INVALID_TIMEOUT) {
    m_scheduler->RemoveTimeout(m_rdm_timeout_id);
    m_rdm_timeout_id = ola::thread::INVALID_TIMEOUT;
  }
}


/*
 * Handle the response from calling DiscoAuto
 */
void DmxTriWidgetImpl::HandleDiscoveryAutoResponse(uint8_t return_code,
                                                   const uint8_t *data,
                                                   unsigned int length) {
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
  (void) data;
  (void) length;
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
      OLA_DEBUG << "Discovery process has completed, " <<
        static_cast<int>(data[0]) << " devices found";
      StopDiscovery();
      m_uid_count = data[0];
      m_uid_index_map.clear();
      if (m_uid_count) {
        FetchNextUID();
      } else {
        RDMDiscoveryCallback *callback = m_discovery_callback;
        m_discovery_callback= NULL;
        RunDiscoveryCallback(callback);
      }
    }
  } else {
    // These are all fatal
    switch (return_code) {
      case EC_RESPONSE_MUTE:
        OLA_WARN << "Failed to mute device, aborting discovery";
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
    FetchNextUID();
  } else {
    RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    RunDiscoveryCallback(callback);
  }
}


/*
 * Handle the response to a raw RDM command
 */
void DmxTriWidgetImpl::HandleRawRDMResponse(uint8_t return_code,
                                            const uint8_t *data,
                                            unsigned int length) {
  OLA_INFO << "got raw RDM response with code: " << std::hex <<
    static_cast<int>(return_code) << ", length: " << std::dec << length;

  const ola::rdm::RDMRequest *request = m_pending_request;
  m_pending_request = NULL;
  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;

  if (callback == NULL || request == NULL) {
    OLA_FATAL << "Got a response but missing callback or request object!";
    return;
  }

  if (return_code == EC_UNKNOWN_COMMAND) {
    // This means raw mode isn't supported and we should fall back to the
    // default mode
    m_use_raw_rdm = false;
    OLA_WARN <<
      "Raw RDM mode not supported, switching back to the managed RDM mode";
    SendRDMRequest(request, callback);
    return;
  }

  std::vector<string> packets;
  packets.push_back(string(reinterpret_cast<const char*>(data), length));
  const UID &dest_uid = request->DestinationUID();

  if (dest_uid.IsBroadcast()) {
    if (return_code != EC_RESPONSE_NONE) {
      OLA_WARN << "Unexpected response to broadcast message";
    }
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
    delete request;
    return;
  }

  if (return_code == EC_RESPONSE_NONE) {
    callback->Run(ola::rdm::RDM_TIMEOUT, NULL, packets);
    return;
  }

  ola::rdm::rdm_response_code code = ola::rdm::RDM_COMPLETED_OK;
  ola::rdm::RDMResponse *response =
    ola::rdm::RDMResponse::InflateFromData(data, length, &code, request);
  delete request;
  callback->Run(code, response, packets);
}


/*
 * Handle the response to a Remote Get/Set command
 */
void DmxTriWidgetImpl::HandleRemoteRDMResponse(uint8_t return_code,
                                               const uint8_t *data,
                                               unsigned int length) {
  if (m_pending_request == NULL) {
    OLA_FATAL << "Got a response but missing callback or request object!";
    return;
  }

  OLA_INFO << "Received RDM response with code 0x" <<
    std::hex << static_cast<int>(return_code) << ", " << std::dec << length <<
    " bytes, param " << std::hex << m_pending_request->ParamId();

  HandleGenericRDMResponse(return_code,
                           m_pending_request->ParamId(),
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
    delete m_pending_request;
    m_pending_request = NULL;
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    if (callback) {
      std::vector<string> packets;
      callback->Run(ola::rdm::RDM_INVALID_RESPONSE, NULL, packets);
    }
    return;
  }

  uint16_t pid;
  memcpy(reinterpret_cast<uint8_t*>(&pid), data, sizeof(pid));
  pid = NetworkToHost(pid);

  data += 2;
  length -= 2;

  OLA_INFO << "Received queued message response with code 0x" <<
    std::hex << static_cast<int>(return_code) << ", " << std::dec << length <<
    " bytes, param " << std::hex << pid;

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
  const ola::rdm::RDMRequest *request = m_pending_request;
  m_pending_request = NULL;
  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;

  if (callback == NULL || request == NULL) {
    OLA_FATAL << "Got a response but missing callback or request object!";
    return;
  }

  ola::rdm::RDMResponse *response = NULL;
  ola::rdm::rdm_response_code code = ola::rdm::RDM_COMPLETED_OK;
  ola::rdm::rdm_nack_reason reason;

  if (ReturnCodeToNackReason(return_code, &reason)) {
    response = ola::rdm::NackWithReason(request, reason);
    code = ola::rdm::RDM_COMPLETED_OK;
  } else if (return_code == EC_NO_ERROR) {
    if (request->DestinationUID().IsBroadcast()) {
      code = ola::rdm::RDM_WAS_BROADCAST;
    } else {
      code = ola::rdm::RDM_COMPLETED_OK;
      response = ola::rdm::GetResponseWithPid(
          request,
          pid,
          data,
          length);
    }
  } else if (return_code == EC_RESPONSE_TIME) {
    response = ola::rdm::GetResponseWithPid(request,
                                            pid,
                                            data,
                                            length,
                                            ola::rdm::RDM_ACK_TIMER);
  } else if (return_code == EC_RESPONSE_WAIT) {
    // this is a hack, there is no way to expose # of queued messages
    response = ola::rdm::GetResponseWithPid(request,
                                            pid,
                                            data,
                                            length,
                                            ola::rdm::RDM_ACK,
                                            1);
  } else if (return_code == EC_RESPONSE_MORE) {
    response = ola::rdm::GetResponseWithPid(request,
                                            pid,
                                            data,
                                            length,
                                            ola::rdm::ACK_OVERFLOW);
  } else if (!TriToOlaReturnCode(return_code, &code)) {
    OLA_WARN << "Response was returned with 0x" << std::hex <<
      static_cast<int>(return_code);
    code = ola::rdm::RDM_INVALID_RESPONSE;
  }
  delete request;
  std::vector<string> packets;
  // Unfortunately we don't get to see the raw response here, which limits the
  // use of the TRI for testing. For testing use the raw mode.
  callback->Run(code, response, packets);
}

/*
 * Handle a setfilter response
 */
void DmxTriWidgetImpl::HandleSetFilterResponse(uint8_t return_code,
                                               const uint8_t *data,
                                               unsigned int length) {
  if (!m_pending_request) {
    OLA_WARN << "Set filter response but no RDM message to send!";
    return;
  }

  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  const ola::rdm::RDMRequest *request = m_pending_request;
  m_pending_request = NULL;

  if (return_code == EC_NO_ERROR) {
    m_last_esta_id = request->DestinationUID().ManufacturerId();
    DispatchRequest(request, callback);
  } else {
    OLA_WARN << "SetFilter returned " << static_cast<int>(return_code) <<
      ", we have no option but to drop the rdm request";
    delete request;
    if (callback) {
      std::vector<string> packets;
      callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    }
  }
  (void) data;
  (void) length;
}


/**
 * Convert a DMX TRI return code to the appropriate rdm_response_code
 * @return true if this was a matching code, false otherwise
 */
bool DmxTriWidgetImpl::TriToOlaReturnCode(
    uint8_t return_code,
    ola::rdm::rdm_response_code *code) {
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
    default:
      return false;
  }
  return true;
}


/**
 * DmxTriWidget Constructor
 */
DmxTriWidget::DmxTriWidget(ola::thread::SchedulerInterface *scheduler,
                           ola::network::ConnectedDescriptor *descriptor,
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
}  // usbpro
}  // plugin
}  // ola
