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
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/DmxTriWidget.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::map;
using std::string;
using ola::network::NetworkToHost;
using ola::network::HostToNetwork;
using ola::rdm::RDMCommand;
using ola::rdm::RDMRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;


/*
 * New DMX TRI device
 */
DmxTriWidgetImpl::DmxTriWidgetImpl(ola::network::SelectServerInterface *ss,
                                   UsbWidgetInterface *widget):
    m_ss(ss),
    m_widget(widget),
    m_rdm_timeout_id(ola::network::INVALID_TIMEOUT),
    m_uid_count(0),
    m_last_esta_id(UID::ALL_MANUFACTURERS),
    m_uid_set_callback(NULL),
    m_discovery_callback(NULL),
    m_rdm_request_callback(NULL),
    m_pending_request(NULL),
    m_transaction_number(0) {
  m_widget->SetMessageHandler(
      NewCallback(this, &DmxTriWidgetImpl::HandleMessage));
}


/*
 * Shutdown
 */
DmxTriWidgetImpl::~DmxTriWidgetImpl() {
  Stop();

  // timeout any existing message
  if (m_rdm_request_callback)
    m_rdm_request_callback->Run(ola::rdm::RDM_TIMEOUT, NULL);
  if (m_pending_request)
    delete m_pending_request;

  if (m_uid_set_callback)
    delete m_uid_set_callback;

  if (m_discovery_callback)
    delete m_discovery_callback;
}


/**
 * Set the callback used when the UIDSet changes
 */
void DmxTriWidgetImpl::SetUIDListCallback(
    ola::Callback1<void, const ola::rdm::UIDSet&> *callback) {
  if (m_uid_set_callback)
    delete m_uid_set_callback;
  m_uid_set_callback = callback;
}


/**
 * Set the callback to be run when discovery completes
 */
void DmxTriWidgetImpl::SetDiscoveryCallback(ola::Callback0<void> *callback) {
  if (m_discovery_callback)
    delete m_discovery_callback;
  m_discovery_callback = callback;
}


/**
 * Stop the rdm discovery process if it's running
 */
void DmxTriWidgetImpl::Stop() {
  if (m_rdm_timeout_id != ola::network::INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(m_rdm_timeout_id);
    m_rdm_timeout_id = ola::network::INVALID_TIMEOUT;
  }
}


/*
 * Send a DMX message
 * @returns true if we sent ok, false otherwise
 */
bool DmxTriWidgetImpl::SendDMX(const DmxBuffer &buffer) const {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  widget_dmx.start_code = 0;
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(widget_dmx.dmx, &length);
  return m_widget->SendMessage(UsbWidget::DMX_LABEL,
                               reinterpret_cast<uint8_t*>(&widget_dmx),
                               length + 1);
}


/*
 * Send an RDM request. Because this is wrapped in a QueueingRDMController it's
 * only going to be called one-at-a-time.
 */
void DmxTriWidgetImpl::SendRequest(const ola::rdm::RDMRequest *request,
                                   ola::rdm::RDMCallback *on_complete) {
  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
    delete request;
    return;
  }

  // if we can't find this UID, fail now.
  const UID &dest_uid = request->DestinationUID();
  if (!dest_uid.IsBroadcast() &&
      m_uid_index_map.find(dest_uid) == m_uid_index_map.end()) {
    on_complete->Run(ola::rdm::RDM_UNKNOWN_UID, NULL);
    delete request;
    return;
  }

  if (request->DestinationUID().IsBroadcast() &&
      request->DestinationUID().ManufacturerId() != m_last_esta_id) {
    uint16_t esta_id = request->DestinationUID().ManufacturerId();
    uint8_t data[] = {SET_FILTER_COMMAND_ID, esta_id >> 8, esta_id & 0xff};
    m_pending_request = request;
    m_rdm_request_callback = on_complete;
    bool r = m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                                   reinterpret_cast<uint8_t*>(&data),
                                   sizeof(data));
    if (!r) {
      OLA_INFO << "Failed to send set filter, aborting request";
      delete request;
      m_pending_request = NULL;
      m_rdm_request_callback = NULL;
      on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
    }
  } else {
    DispatchRequest(request, on_complete);
  }
}


/*
 * Kick off the discovery process if it's not already running
 */
void DmxTriWidgetImpl::RunRDMDiscovery() {
  if (InDiscoveryMode())
    // process already running
    return;

  if (!SendDiscoveryStart()) {
    OLA_WARN << "Failed to begin RDM discovery";
    return;
  }

  // setup a stat every RDM_STATUS_INTERVAL_MS until we're done
  m_rdm_timeout_id = m_ss->RegisterRepeatingTimeout(
      RDM_STATUS_INTERVAL_MS,
      NewCallback(this, &DmxTriWidgetImpl::CheckDiscoveryStatus));
}


/**
 * Call the UIDSet handler with the latest UID list.
 */
void DmxTriWidgetImpl::SendUIDUpdate() {
  UIDSet uid_set;
  map<UID, uint8_t>::iterator iter = m_uid_index_map.begin();
  for (; iter != m_uid_index_map.end(); ++iter) {
    uid_set.AddUID(iter->first);
  }
  if (m_uid_set_callback)
    m_uid_set_callback->Run(uid_set);
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
  return (m_rdm_timeout_id != ola::network::INVALID_TIMEOUT ||
          m_uid_count);
}


/*
 * Send a DiscoAuto message to begin the discovery process.
 */
bool DmxTriWidgetImpl::SendDiscoveryStart() {
  uint8_t command_id = DISCOVER_AUTO_COMMAND_ID;

  return m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
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
  m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                        data,
                        sizeof(data));
}


/*
 * Send a DiscoStat message to begin the discovery process.
 */
bool DmxTriWidgetImpl::SendDiscoveryStat() {
  uint8_t command_id = DISCOVER_STATUS_COMMAND_ID;

  return m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                               &command_id,
                               sizeof(command_id));
}


/*
 * Send the next RDM request, this assumes that SetFilter has been called
 */
void DmxTriWidgetImpl::DispatchRequest(const ola::rdm::RDMRequest *request,
                                       ola::rdm::RDMCallback *callback) {
  if (request->ParamId() == ola::rdm::PID_QUEUED_MESSAGE &&
      request->CommandClass() == RDMCommand::GET_COMMAND) {
    // these are special
    if (request->ParamDataSize()) {
      DispatchQueuedGet(request, callback);
    } else {
      OLA_WARN << "Missing param data in queued message get";
      delete request;
      callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
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
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
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
      callback->Run(ola::rdm::RDM_UNKNOWN_UID, NULL);
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
  bool r = m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                                 reinterpret_cast<uint8_t*>(&message),
                                 size);
  if (!r) {
    m_pending_request = NULL;
    delete request;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
  }
}


/*
 * Send a queued get message
 */
void DmxTriWidgetImpl::DispatchQueuedGet(const ola::rdm::RDMRequest* request,
                                         ola::rdm::RDMCallback *callback) {
  map<UID, uint8_t>::const_iterator iter =
    m_uid_index_map.find(request->DestinationUID());
  if (iter == m_uid_index_map.end()) {
    OLA_WARN << request->DestinationUID() << " not found in uid map";
    delete request;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
    return;
  }
  uint8_t data[] = {QUEUED_GET_COMMAND_ID,
                    iter->second,
                    request->ParamData()[0]};

  m_pending_request = request;
  bool r = m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                                 reinterpret_cast<uint8_t*>(&data),
                                 sizeof(data));

  if (!r) {
    m_pending_request = NULL;
    delete request;
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
  }
}


/*
 * Stop the discovery process
 */
void DmxTriWidgetImpl::StopDiscovery() {
  if (m_rdm_timeout_id != ola::network::INVALID_TIMEOUT) {
    m_ss->RemoveTimeout(m_rdm_timeout_id);
    m_rdm_timeout_id = ola::network::INVALID_TIMEOUT;
  }
}


/*
 * Handle the response from calling DiscoAuto
 */
void DmxTriWidgetImpl::HandleDiscoveryAutoResponse(uint8_t return_code,
                                                   const uint8_t *data,
                                                   unsigned int length) {
  if (return_code != EC_NO_ERROR) {
    OLA_WARN << "DMX_TRI discovery returned error " <<
      static_cast<int>(return_code);
    StopDiscovery();
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
        if (m_discovery_callback)
          m_discovery_callback->Run();
        SendUIDUpdate();
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
    if (m_discovery_callback)
      m_discovery_callback->Run();
    SendUIDUpdate();
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
    // notify the universe
    if (m_discovery_callback)
      m_discovery_callback->Run();

    SendUIDUpdate();
  }
}


/*
 * Handle the response to a Remote Get/Set command
 */
void DmxTriWidgetImpl::HandleRemoteRDMResponse(uint8_t return_code,
                                               const uint8_t *data,
                                               unsigned int length) {
  const ola::rdm::RDMRequest *request = m_pending_request;
  m_pending_request = NULL;
  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;

  OLA_INFO << "Received RDM response with code 0x" <<
    std::hex << static_cast<int>(return_code) << ", " << std::dec << length <<
    " bytes, param " << std::hex << request->ParamId();

  if (callback == NULL || request == NULL) {
    OLA_FATAL << "Got a response but missing callback or request object!";
    return;
  }
  ola::rdm::RDMResponse *response = NULL;
  ola::rdm::rdm_request_status status;

  if (return_code == EC_NO_ERROR) {
    if (request->DestinationUID().IsBroadcast()) {
      status = ola::rdm::RDM_WAS_BROADCAST;
    } else {
      status = ola::rdm::RDM_COMPLETED_OK;
      response = ola::rdm::GetResponseWithData(
          request,
          data,
          length);
    }
  } else if (return_code == EC_RESPONSE_TIME ||
             return_code == EC_RESPONSE_WAIT ||
             return_code == EC_RESPONSE_MORE) {
    ola::rdm::rdm_response_type type = ola::rdm::ACK;
    if (return_code == EC_RESPONSE_TIME)
      type = ola::rdm::ACK_TIMER;
    else if (return_code == EC_RESPONSE_MORE)
      type = ola::rdm::ACK_OVERFLOW;

    response = ola::rdm::GetResponseWithData(
        request,
        data,
        length,
        type,
        // this is a hack, there is no way to expose # of queues messages
        return_code == EC_RESPONSE_WAIT ? 1 : 0);

    status = ola::rdm::RDM_COMPLETED_OK;
  } else if (return_code == EC_RESPONSE_TIME) {
    status = ola::rdm::RDM_TIMEOUT;

  } else if (return_code >= EC_UNKNOWN_PID &&
             return_code <= EC_PROXY_BUFFER_FULL) {
    // avoid compiler warnings
    ola::rdm::rdm_nack_reason reason = ola::rdm::NR_UNKNOWN_PID;

    switch (return_code) {
      case EC_UNKNOWN_PID:
        reason = ola::rdm::NR_UNKNOWN_PID;
        break;
      case EC_FORMAT_ERROR:
        reason = ola::rdm::NR_FORMAT_ERROR;
        break;
      case EC_HARDWARE_FAULT:
        reason = ola::rdm::NR_HARDWARE_FAULT;
        break;
      case EC_PROXY_REJECT:
        reason = ola::rdm::NR_PROXY_REJECT;
        break;
      case EC_WRITE_PROTECT:
        reason = ola::rdm::NR_WRITE_PROTECT;
        break;
      case EC_UNSUPPORTED_COMMAND_CLASS:
        reason = ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS;
        break;
      case EC_OUT_OF_RANGE:
        reason = ola::rdm::NR_DATA_OUT_OF_RANGE;
        break;
      case EC_BUFFER_FULL:
        reason = ola::rdm::NR_BUFFER_FULL;
        break;
      case EC_FRAME_OVERFLOW:
        reason = ola::rdm::NR_PACKET_SIZE_UNSUPPORTED;
        break;
      case EC_SUBDEVICE_UNKNOWN:
        reason = ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE;
        break;
      case EC_PROXY_BUFFER_FULL:
        reason = ola::rdm::NR_PROXY_BUFFER_FULL;
        break;
    }
    response = ola::rdm::NackWithReason(request, reason);
    status = ola::rdm::RDM_COMPLETED_OK;
  } else if (return_code == EC_RESPONSE_NONE) {
    OLA_INFO << "Response timed out";
    status = ola::rdm::RDM_TIMEOUT;
  } else {
    // TODO(simonn): Expand the error codes here
    OLA_WARN << "Response was returned with 0x" << std::hex <<
      static_cast<int>(return_code);
    status = ola::rdm::RDM_INVALID_RESPONSE;
  }
  delete request;
  callback->Run(status, response);
}


/*
 * Handle the response to a QueuedGet command
 */
void DmxTriWidgetImpl::HandleQueuedGetResponse(uint8_t return_code,
                                           const uint8_t *data,
                                           unsigned int length) {
  OLA_INFO << "got queued message response";
  // TODO(simon): implement this
  (void) return_code;
  (void) data;
  (void) length;
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
    if (callback)
      callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL);
  }
  (void) data;
  (void) length;
}


/**
 * DmxTriWidget Constructor
 */
DmxTriWidget::DmxTriWidget(ola::network::SelectServerInterface *ss,
                           UsbWidgetInterface *widget,
                           unsigned int queue_size):
    m_impl(ss, widget),
    m_controller(&m_impl, queue_size) {
  m_impl.SetDiscoveryCallback(
      NewCallback(this, &DmxTriWidget::ResumeRDMCommands));
}
}  // usbpro
}  // plugin
}  // ola
