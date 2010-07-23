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
 * DmxTriDevice.h
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
#include "plugins/usbpro/DmxTriDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {

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
DmxTriDevice::DmxTriDevice(const ola::PluginAdaptor *plugin_adaptor,
                           ola::AbstractPlugin *owner,
                           const string &name,
                           UsbWidget *widget,
                           uint16_t esta_id,
                           uint16_t device_id,
                           uint32_t serial):
    UsbDevice(owner, name, widget),
    m_plugin_adaptor(plugin_adaptor),
    m_rdm_timeout_id(ola::network::INVALID_TIMEOUT),
    m_uid_count(0),
    m_rdm_request_pending(false),
    m_last_esta_id(UID::ALL_MANUFACTURERS) {
  std::stringstream str;
  str << std::hex << esta_id << "-" << device_id << "-" <<
    NetworkToHost(serial);
  m_device_id = str.str();
  DmxTriOutputPort *output_port = new DmxTriOutputPort(this);
  AddPort(output_port);
  m_widget->SetMessageHandler(this);
}


/*
 * Shutdown
 */
DmxTriDevice::~DmxTriDevice() {
  // delete all outstanding requests
  while (!m_pending_requests.empty()) {
    const RDMRequest *request = m_pending_requests.front();
    delete request;
    m_pending_requests.pop();
  }
}


/*
 * Kick off the RDM discovery process
 */
bool DmxTriDevice::StartHook() {
  RunRDMDiscovery();
  return true;
}


/*
 * Remove the rdm timeout if it's still running
 */
void DmxTriDevice::PrePortStop() {
  if (m_rdm_timeout_id != ola::network::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_rdm_timeout_id);
    m_rdm_timeout_id = ola::network::INVALID_TIMEOUT;
  }
}


/*
 * Send a dmx msg
 * @returns true if we sent ok, false otherwise
 */
bool DmxTriDevice::SendDMX(const DmxBuffer &buffer) const {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  if (!IsEnabled())
    return true;

  widget_dmx.start_code = 0;
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(widget_dmx.dmx, &length);
  return m_widget->SendMessage(UsbWidget::DMX_LABEL,
                               length + 1,
                               reinterpret_cast<uint8_t*>(&widget_dmx));
}


/*
 * Handle an RDM Request, ownership of the request object is transferred to us.
 */
bool DmxTriDevice::HandleRDMRequest(const ola::rdm::RDMRequest *request) {
  // if we can't find this UID, fail now. While in discovery mode the
  // m_uid_index_map will be clear so skip the check in this case.
  const UID &dest_uid = request->DestinationUID();
  if (!dest_uid.IsBroadcast() &&
      !InDiscoveryMode() &&
      m_uid_index_map.find(dest_uid) == m_uid_index_map.end()) {
    delete request;
    return false;
  }
  m_pending_requests.push(request);
  MaybeSendRDMRequest();
  return true;
}


/*
 * Kick off the discovery process if it's not already running
 */
void DmxTriDevice::RunRDMDiscovery() {
  if (InDiscoveryMode())
    // process already running
    return;

  if (!SendDiscoveryStart()) {
    OLA_WARN << "Failed to begin RDM discovery";
    return;
  }

  // setup a stat every RDM_STATUS_INTERVAL_MS until we're done
  m_rdm_timeout_id = m_plugin_adaptor->RegisterRepeatingTimeout(
      RDM_STATUS_INTERVAL_MS,
      NewClosure(this, &DmxTriDevice::CheckDiscoveryStatus));
}


/*
 * Check the status of the RDM discovery process.
 * This is called periodically while discovery is running
 */
bool DmxTriDevice::CheckDiscoveryStatus() {
  if (!IsEnabled())
    return false;
  return SendDiscoveryStat();
}


/*
 * Handle a message received from the widget
 */
void DmxTriDevice::HandleMessage(UsbWidget* widget,
                                 uint8_t label,
                                 unsigned int length,
                                 const uint8_t *data) {
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
        OLA_WARN << "Unknown DMX-TRI CI: " << static_cast<int>(command_id);
    }
  } else {
    OLA_INFO << "DMX-TRI got response " << static_cast<int>(label);
  }
  (void) widget;
}


/*
 * Return true if discovery is running
 */
bool DmxTriDevice::InDiscoveryMode() const {
  return (m_rdm_timeout_id != ola::network::INVALID_TIMEOUT ||
          m_uid_count);
}


/*
 * Send a DiscoAuto message to begin the discovery process.
 */
bool DmxTriDevice::SendDiscoveryStart() {
  uint8_t command_id = DISCOVER_AUTO_COMMAND_ID;

  return m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                               sizeof(command_id),
                               &command_id);
}


/*
 * Send a DiscoAuto message to begin the discovery process.
 */
void DmxTriDevice::FetchNextUID() {
  if (!m_uid_count)
    return;

  OLA_INFO << "fetching index  " << static_cast<int>(m_uid_count);
  uint8_t data[] = {REMOTE_UID_COMMAND_ID, m_uid_count};
  m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                        sizeof(data),
                        data);
}


/*
 * Send a SetFilter command
 */
bool DmxTriDevice::SendSetFilter(uint16_t esta_id) {
  uint8_t data[] = {SET_FILTER_COMMAND_ID, esta_id >> 8, esta_id & 0xff};
  return m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                               sizeof(data),
                               reinterpret_cast<uint8_t*>(&data));
}


/*
 * Send a DiscoStat message to begin the discovery process.
 */
bool DmxTriDevice::SendDiscoveryStat() {
  uint8_t command_id = DISCOVER_STATUS_COMMAND_ID;

  return m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                               sizeof(command_id),
                               &command_id);
}


/*
 * If we're not in discovery mode, send the next request. This will call
 * SetFilter and defer the send if it's a broadcast UID.
 */
void DmxTriDevice::MaybeSendRDMRequest() {
  if (!IsEnabled())
    return;

  if (InDiscoveryMode() || m_pending_requests.empty() || m_rdm_request_pending)
    return;

  m_rdm_request_pending = true;
  const RDMRequest *request = m_pending_requests.front();
  if (request->DestinationUID().IsBroadcast() &&
      request->DestinationUID().ManufacturerId() != m_last_esta_id) {
    SendSetFilter(request->DestinationUID().ManufacturerId());
  } else {
    DispatchNextRequest();
  }
}


/*
 * Send the next RDM request, this assumes that SetFilter has been called
 */
void DmxTriDevice::DispatchNextRequest() {
  const RDMRequest *request = m_pending_requests.front();
  if (request->ParamId() == ola::rdm::PID_QUEUED_MESSAGE &&
      request->CommandClass() == RDMCommand::GET_COMMAND) {
    // these are special
    if (request->ParamDataSize())
      DispatchQueuedGet(request);
    else
      OLA_WARN << "Missing param data in queued message get";
    return;
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
    return;
  }

  if (request->DestinationUID().IsBroadcast()) {
    message.index = 0;
  } else {
    map<UID, uint8_t>::const_iterator iter =
      m_uid_index_map.find(request->DestinationUID());
    if (iter == m_uid_index_map.end()) {
      OLA_WARN << request->DestinationUID() << " not found in uid map";
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

  /*
  uint8_t *foo = reinterpret_cast<uint8_t*>(&message);
  for(unsigned int i = 0; i < size; i++) {
    OLA_INFO << std::hex << (int) *foo++;
  }
  */
  m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                        size,
                        reinterpret_cast<uint8_t*>(&message));
}


/*
 * Send a queued get message
 */
void DmxTriDevice::DispatchQueuedGet(const ola::rdm::RDMRequest* request) {
  map<UID, uint8_t>::const_iterator iter =
    m_uid_index_map.find(request->DestinationUID());
  if (iter == m_uid_index_map.end()) {
    OLA_WARN << request->DestinationUID() << " not found in uid map";
    return;
  }
  uint8_t data[] = {QUEUED_GET_COMMAND_ID,
                    iter->second,
                    request->ParamData()[0]};

  m_widget->SendMessage(EXTENDED_COMMAND_LABEL,
                        sizeof(data),
                        reinterpret_cast<uint8_t*>(&data));
}


/*
 * Stop the discovery process
 */
void DmxTriDevice::StopDiscovery() {
  if (m_rdm_timeout_id != ola::network::INVALID_TIMEOUT) {
    m_plugin_adaptor->RemoveTimeout(m_rdm_timeout_id);
    m_rdm_timeout_id = ola::network::INVALID_TIMEOUT;
  }
}


/*
 * Handle the response from calling DiscoAuto
 */
void DmxTriDevice::HandleDiscoveryAutoResponse(uint8_t return_code,
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
void DmxTriDevice::HandleDiscoverStatResponse(uint8_t return_code,
                                              const uint8_t *data,
                                              unsigned int length) {
  switch (return_code) {
    case EC_NO_ERROR:
      break;
    case EC_RESPONSE_MUTE:
      OLA_WARN << "Failed to mute device, aborting discovery";
      StopDiscovery();
      return;
    case EC_RESPONSE_DISCOVERY:
      OLA_WARN <<
        "Duplicated or erroneous device detected, aborting discovery";
      StopDiscovery();
      return;
    case EC_RESPONSE_UNEXPECTED:
      OLA_INFO << "Got an unexpected RDM response during discovery";
      break;
    default:
      OLA_WARN << "DMX_TRI discovery returned error " <<
        static_cast<int>(return_code);
      StopDiscovery();
      return;
  }

  if (length < 1) {
    OLA_WARN << "DiscoStat command too short, was " << length;
    return;
  }

  if (data[1] == 0) {
    OLA_DEBUG << "Discovery process has completed, " <<
      static_cast<int>(data[0]) << " devices found";
    StopDiscovery();
    m_uid_count = data[0];
    m_uid_index_map.clear();
    if (m_uid_count)
      FetchNextUID();
  }
}


/*
 * Handle the response to a RemoteGet command
 */
void DmxTriDevice::HandleRemoteUIDResponse(uint8_t return_code,
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
    UIDSet uid_set;
    map<UID, uint8_t>::iterator iter = m_uid_index_map.begin();
    for (; iter != m_uid_index_map.end(); ++iter) {
      uid_set.AddUID(iter->first);
    }
    GetOutputPort(0)->NewUIDList(uid_set);

    // start sending rdm commands again
    MaybeSendRDMRequest();
  }
}


/*
 * Handle the response to a RemoteGet command
 */
void DmxTriDevice::HandleRemoteRDMResponse(uint8_t return_code,
                                           const uint8_t *data,
                                           unsigned int length) {
  OLA_WARN << "got get response! " << static_cast<int>(return_code) <<
    " length " << length;
  const RDMRequest *request = m_pending_requests.front();
  if (return_code == EC_NO_ERROR || return_code == EC_RESPONSE_WAIT) {
    ola::rdm::RDMResponse *response;
    if (request->CommandClass() == RDMCommand::GET_COMMAND) {
      response = new ola::rdm::RDMGetResponse(
          request->DestinationUID(),
          request->SourceUID(),
          request->TransactionNumber(),
          ola::rdm::ACK,
          // this is a hack, there is no way to expose # of queues messages
          return_code == EC_RESPONSE_WAIT ? 1 : 0,
          request->SubDevice(),
          request->ParamId(),
          data,
          length);
    } else {
      response = new ola::rdm::RDMSetResponse(
          request->DestinationUID(),
          request->SourceUID(),
          request->TransactionNumber(),
          ola::rdm::ACK,
          // this is a hack, there is no way to expose # of queues messages
          return_code == EC_RESPONSE_WAIT ? 1 : 0,
          request->SubDevice(),
          request->ParamId(),
          data,
          length);
    }
    GetOutputPort(0)->HandleRDMResponse(response);
  } else {
    OLA_WARN << "Response was returned with 0x" << std::hex <<
      static_cast<int>(return_code);
  }
  delete request;
  m_pending_requests.pop();
  m_rdm_request_pending = false;
}


/*
 * Handle the response to a QueuedGet command
 */
void DmxTriDevice::HandleQueuedGetResponse(uint8_t return_code,
                                           const uint8_t *data,
                                           unsigned int length) {
  OLA_INFO << "got queued message response";
}


/*
 * Handle a setfilter response
 */
void DmxTriDevice::HandleSetFilterResponse(uint8_t return_code,
                                           const uint8_t *data,
                                           unsigned int length) {
  if (return_code == EC_NO_ERROR) {
    m_last_esta_id =
      m_pending_requests.front()->DestinationUID().ManufacturerId();
    DispatchNextRequest();
  } else {
    OLA_WARN << "SetFilter returned " << static_cast<int>(return_code) <<
      ", we have no option but to drop the rdm request";
    delete m_pending_requests.front();
    m_pending_requests.pop();
    m_rdm_request_pending = false;
    MaybeSendRDMRequest();
  }
  (void) data;
  (void) length;
}
}  // usbpro
}  // plugin
}  // ola
