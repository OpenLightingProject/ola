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
 * DmxterWidgetImpl.h
 * The Goddard Design Dmxter RDM and miniDmxter
 * Copyright (C) 2010 Simon Newton
 */

#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/DmxterWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::rdm::RDMRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::string;

const uint8_t DmxterWidgetImpl::RDM_REQUEST_LABEL = 0x80;
const uint8_t DmxterWidgetImpl::RDM_BCAST_REQUEST_LABEL = 0x81;
const uint8_t DmxterWidgetImpl::TOD_LABEL = 0x82;
const uint8_t DmxterWidgetImpl::DISCOVERY_BRANCH_LABEL = 0x83;
const uint8_t DmxterWidgetImpl::FULL_DISCOVERY_LABEL = 0x84;
const uint8_t DmxterWidgetImpl::INCREMENTAL_DISCOVERY_LABEL = 0x85;
const uint8_t DmxterWidgetImpl::SHUTDOWN_LABAEL = 0xf0;


/*
 * New DMXter device
 * @param ss A pointer to a SelectServerInterface
 * @param widget the underlying UsbWidget
 * @param esta_id the ESTA id, should normally be GODDARD Design
 * @param serial the 4 byte serial which forms part of the UID
 */
DmxterWidgetImpl::DmxterWidgetImpl(ola::network::SelectServerInterface *ss,
                                   UsbWidgetInterface *widget,
                                   uint16_t esta_id,
                                   uint32_t serial):
    m_uid(esta_id, serial),
    m_widget(widget),
    m_ss(ss),
    m_discovery_callback(NULL),
    m_pending_request(NULL),
    m_rdm_request_callback(NULL),
    m_transaction_number(0) {
  m_widget->SetMessageHandler(
      NewCallback(this, &DmxterWidgetImpl::HandleMessage));
}


/**
 * Clean up
 */
DmxterWidgetImpl::~DmxterWidgetImpl() {
  // timeout any existing message
  std::vector<std::string> packets;
  if (m_rdm_request_callback)
    m_rdm_request_callback->Run(ola::rdm::RDM_TIMEOUT, NULL, packets);

  if (m_pending_request)
    delete m_pending_request;
}


/**
 * Called when a new packet arrives
 */
void DmxterWidgetImpl::HandleMessage(uint8_t label,
                                     const uint8_t *data,
                                     unsigned int length) {
  switch (label) {
    case TOD_LABEL:
      HandleTodResponse(data, length);
      break;
    case RDM_REQUEST_LABEL:
      HandleRDMResponse(data, length);
      break;
    case RDM_BCAST_REQUEST_LABEL:
      HandleBroadcastRDMResponse(data, length);
      break;
    case SHUTDOWN_LABAEL:
      HandleShutdown(data, length);
      break;
    default:
      OLA_WARN << "Unknown label: 0x" << std::hex <<
        static_cast<int>(label);
  }
}


/**
 * Send an RDM request. By wrapping this in a QueueingRDMController, we ensure
 * that this is only called one-at-a-time.
 * @param request the RDMRequest object
 * @param on_complete the callback to run when the request completes or fails
 */
void DmxterWidgetImpl::SendRDMRequest(const RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  std::vector<std::string> packets;
  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    delete request;
    return;
  }

  unsigned int data_size = request->Size();  // add in the start code
  uint8_t *data = new uint8_t[data_size + 1];
  data[0] = ola::rdm::RDMCommand::START_CODE;

  bool r = request->PackWithControllerParams(data + 1,
                                             &data_size,
                                             m_uid,
                                             m_transaction_number++,
                                             1);
  if (r) {
    uint8_t label = request->DestinationUID().IsBroadcast() ?
      RDM_BCAST_REQUEST_LABEL : RDM_REQUEST_LABEL;

    m_rdm_request_callback = on_complete;
    m_pending_request = request;
    if (m_widget->SendMessage(label, data, data_size + 1)) {
      delete[] data;
      return;
    }
  } else {
    OLA_WARN << "Failed to pack message, dropping request";
  }
  m_rdm_request_callback = NULL;
  m_pending_request = NULL;
  delete[] data;
  delete request;
  on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
}


/**
 * Trigger full RDM discovery for the widget.
 */
bool DmxterWidgetImpl::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_discovery_callback = callback;
  if (!m_widget->SendMessage(FULL_DISCOVERY_LABEL, NULL, 0)) {
    OLA_WARN << "Failed to send full dmxter discovery command";
    delete callback;
    m_discovery_callback = NULL;
    return false;
  }
  return true;
}


/**
 * Trigger incremental RDM discovery for the widget.
 */
bool DmxterWidgetImpl::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_discovery_callback = callback;
  if (!m_widget->SendMessage(INCREMENTAL_DISCOVERY_LABEL, NULL, 0)) {
    OLA_WARN << "Failed to send incremental dmxter discovery command";
    m_discovery_callback = NULL;
    delete callback;
    return false;
  }
  return true;
}


/**
 * Run the UID Set callback with the current list of UIDs
 */
void DmxterWidgetImpl::SendUIDUpdate() {
  if (m_discovery_callback) {
    m_discovery_callback->Run(m_uids);
    m_discovery_callback = NULL;
  }
}


/**
 * Send a TOD request to the widget
 */
void DmxterWidgetImpl::SendTodRequest() {
  m_widget->SendMessage(TOD_LABEL, NULL, 0);
  OLA_INFO << "Sent TOD request";
}


/**
 * Handle a TOD response
 */
void DmxterWidgetImpl::HandleTodResponse(const uint8_t *data,
                                         unsigned int length) {
  (void) data;
  if (length % UID::UID_SIZE) {
    OLA_WARN << "Response length " << length << " not divisible by " <<
      static_cast<int>(ola::rdm::UID::UID_SIZE) << ", ignoring packet";
    return;
  }

  m_uids.Clear();
  for (unsigned int i = 0; i < length; i+= UID::UID_SIZE) {
    UID uid(data + i);
    OLA_INFO << "added " << uid.ToString();
    m_uids.AddUID(uid);
  }
  SendUIDUpdate();
}


/**
 * Handle a RDM response.
 */
void DmxterWidgetImpl::HandleRDMResponse(const uint8_t *data,
                                         unsigned int length) {
  std::vector<std::string> packets;
  if (m_rdm_request_callback == NULL) {
    OLA_FATAL << "Got a response but no callback to run!";
    return;
  }

  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  const ola::rdm::RDMRequest *request = m_pending_request;
  m_pending_request = NULL;

  if (length < 2) {
    OLA_WARN << "Invalid RDM response from the widget";
    callback->Run(ola::rdm::RDM_INVALID_RESPONSE, NULL, packets);
    delete request;
    return;
  }

  uint8_t version = data[0];
  uint8_t response_code = data[1];

  if (version != 0) {
    OLA_WARN << "Unknown version # in widget response: " <<
      static_cast<int>(version);
    callback->Run(ola::rdm::RDM_INVALID_RESPONSE, NULL, packets);
    delete request;
    return;
  }

  ola::rdm::rdm_response_code code = ola::rdm::RDM_COMPLETED_OK;
  switch (response_code) {
    case RC_CHECKSUM_ERROR:
      code = ola::rdm::RDM_CHECKSUM_INCORRECT;
      break;
    case RC_FRAMING_ERROR:
    case RC_FRAMING_ERROR2:
    case RC_BAD_STARTCODE:
      code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case RC_BAD_SUB_STARTCODE:
      code = ola::rdm::RDM_WRONG_SUB_START_CODE;
      break;
    case RC_WRONG_PDL:
    case RC_BAD_PDL:
      code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case RC_PACKET_TOO_SHORT:
      code = ola::rdm::RDM_PACKET_TOO_SHORT;
      break;
    case RC_PACKET_TOO_LONG:
      code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case RC_PHYSICAL_LENGTH_MISMATCH:
      code = ola::rdm::RDM_PACKET_LENGTH_MISMATCH;
      break;
    case RC_PDL_LENGTH_MISMATCH:
      code = ola::rdm::RDM_PARAM_LENGTH_MISMATCH;
      break;
    case RC_TRANSACTION_MISMATCH:
      code = ola::rdm::RDM_TRANSACTION_MISMATCH;
      break;
    case RC_BAD_RESPONSE_TYPE:
      code = ola::rdm::RDM_INVALID_RESPONSE_TYPE;
      break;
    case RC_IDLE_LEVEL:
    case RC_GOOD_LEVEL:
    case RC_BAD_LEVEL:
    case RC_BROADCAST:
    case RC_VENDORCAST:
      OLA_INFO << "Got response code " << static_cast<int>(response_code);
      code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case RC_GOOD_RESPONSE:
    case RC_ACK_TIMER:
    case RC_ACK_OVERFLOW:
    case RC_NACK:
    case RC_NACK_UNKNOWN_PID:
    case RC_NACK_FORMAT_ERROR:
    case RC_NACK_HARDWARE_FAULT:
    case RC_NACK_PROXY_REJECT:
    case RC_NACK_WRITE_PROECT:
    case RC_NACK_COMMAND_CLASS:
    case RC_NACK_DATA_RANGE:
    case RC_NACK_BUFFER_FULL:
    case RC_NACK_PACKET_SIZE:
    case RC_NACK_SUB_DEVICE_RANGE:
    case RC_NACK_PROXY_QUEUE_BUFFER_FULL:
      code = ola::rdm::RDM_COMPLETED_OK;
      break;
    case RC_TIMED_OUT:
      OLA_INFO << "Request timed out";
      code = ola::rdm::RDM_TIMEOUT;
      break;
    case RC_SUBDEVICE_MISMATCH:
      code = ola::rdm::RDM_SUB_DEVICE_MISMATCH;
      break;
    case RC_SRC_UID_MISMATCH:
      code = ola::rdm::RDM_SRC_UID_MISMATCH;
      break;
    case RC_DEST_UID_MISMATCH:
      code = ola::rdm::RDM_DEST_UID_MISMATCH;
      break;
    case RC_COMMAND_CLASS_MISMATCH:
      code = ola::rdm::RDM_COMMAND_CLASS_MISMATCH;
      break;
    case RC_PARAM_ID_MISMATCH:
      // this should *hopefully* be caught higher up the stack
      code = ola::rdm::RDM_COMPLETED_OK;
      break;
    default:
      OLA_WARN << "Unknown response code " << static_cast<int>(response_code);
      code = ola::rdm::RDM_INVALID_RESPONSE;
  }

  string packet;
  if (length > 3)
    packet.assign(reinterpret_cast<const char*>(data + 3), length - 3);
  packets.push_back(packet);

  if (code == ola::rdm::RDM_COMPLETED_OK) {
    ola::rdm::RDMResponse *response = ola::rdm::RDMResponse::InflateFromData(
        packet,
        m_pending_request,
        &code);
    if (response)
      callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);
    else
      callback->Run(ola::rdm::RDM_INVALID_RESPONSE, NULL, packets);
  } else {
    callback->Run(code, NULL, packets);
  }
  delete request;
}


/**
 * Handle a broadcast response
 */
void DmxterWidgetImpl::HandleBroadcastRDMResponse(const uint8_t *data,
                                                  unsigned int length) {
  if (m_rdm_request_callback == NULL) {
    OLA_FATAL << "Got a response but no callback to run!";
    return;
  }

  if (length != 0 || data != NULL) {
    OLA_WARN << "Got strange broadcast response, length was " << length <<
      ", data was " << data;
  }
  std::vector<std::string> packets;
  m_rdm_request_callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
  m_rdm_request_callback = NULL;
}


/**
 * Handle a shutdown message
 */
void DmxterWidgetImpl::HandleShutdown(const uint8_t *data,
                                      unsigned int length) {
  if (length || data) {
    OLA_WARN << "Invalid shutdown message, length was " << length;
  } else {
    OLA_INFO << "Received shutdown message from Dmxter";
    // this closed socket will be detected the the ss, which will then
    // invoke the on_close callback, removing the device.
    m_widget->CloseSocket();
  }
}


/**
 * DmxterWidget Constructor
 */
DmxterWidget::DmxterWidget(ola::network::SelectServerInterface *ss,
                           UsbWidgetInterface *widget,
                           uint16_t esta_id,
                           uint32_t serial,
                           unsigned int queue_size):
    m_impl(ss, widget, esta_id, serial),
    m_controller(&m_impl, queue_size) {
}
}  // usbpro
}  // plugin
}  // ola
