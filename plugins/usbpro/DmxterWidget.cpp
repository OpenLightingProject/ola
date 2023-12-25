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
 * DmxterWidget.cpp
 * The Goddard Design Dmxter RDM and miniDmxter
 * Copyright (C) 2010 Simon Newton
 */

#include <memory>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/io/ByteString.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/DmxterWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::io::ByteString;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RunRDMCallback;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;
using std::string;
using std::vector;

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
DmxterWidgetImpl::DmxterWidgetImpl(
    ola::io::ConnectedDescriptor *descriptor,
    uint16_t esta_id,
    uint32_t serial)
    : BaseUsbProWidget(descriptor),
      m_uid(esta_id, serial),
      m_discovery_callback(NULL),
      m_rdm_request_callback(NULL),
      m_transaction_number(0) {
}


/**
 * Clean up
 */
DmxterWidgetImpl::~DmxterWidgetImpl() {
  Stop();
}


/**
 * Stop the widget
 */
void DmxterWidgetImpl::Stop() {
  // timeout any existing message
  if (m_rdm_request_callback) {
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
  }

  if (m_discovery_callback) {
    ola::rdm::UIDSet uids;
    ola::rdm::RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    callback->Run(uids);
  }
}


/**
 * Send an RDM request. By wrapping this in a QueueingRDMController, we ensure
 * that this is only called one-at-a-time.
 * @param request_ptr the RDMRequest object
 * @param on_complete the callback to run when the request completes or fails
 */
void DmxterWidgetImpl::SendRDMRequest(RDMRequest *request_ptr,
                                      ola::rdm::RDMCallback *on_complete) {
  auto_ptr<RDMRequest> request(request_ptr);
  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  request->SetSourceUID(m_uid);
  request->SetTransactionNumber(m_transaction_number++);
  request->SetPortId(1);

  ByteString data;
  if (!RDMCommandSerializer::PackWithStartCode(*request, &data)) {
    OLA_WARN << "Failed to pack message, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  uint8_t label;
  if (request->IsDUB()) {
    label = DISCOVERY_BRANCH_LABEL;
  } else {
    label = request->DestinationUID().IsBroadcast() ?
      RDM_BCAST_REQUEST_LABEL : RDM_REQUEST_LABEL;
  }

  m_rdm_request_callback = on_complete;
  m_pending_request.reset(request.release());
  if (SendMessage(label, data.data(), data.size())) {
    return;
  }

  m_rdm_request_callback = NULL;
  m_pending_request.reset();
  RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
}


/**
 * Trigger full RDM discovery for the widget.
 */
void DmxterWidgetImpl::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_discovery_callback = callback;
  if (!SendMessage(FULL_DISCOVERY_LABEL, NULL, 0)) {
    OLA_WARN << "Failed to send full dmxter discovery command";
    m_discovery_callback = NULL;
    // return the existing set of UIDs
    callback->Run(m_uids);
  }
}


/**
 * Trigger incremental RDM discovery for the widget.
 */
void DmxterWidgetImpl::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  m_discovery_callback = callback;
  if (!SendMessage(INCREMENTAL_DISCOVERY_LABEL, NULL, 0)) {
    OLA_WARN << "Failed to send incremental dmxter discovery command";
    m_discovery_callback = NULL;
    // return the existing set of UIDs
    callback->Run(m_uids);
  }
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
    case DISCOVERY_BRANCH_LABEL:
      HandleRDMResponse(data, length);
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
  if (m_discovery_callback) {
    ola::rdm::RDMDiscoveryCallback *callback = m_discovery_callback;
    m_discovery_callback = NULL;
    callback->Run(m_uids);
  }
}


/**
 * Handle a RDM response.
 */
void DmxterWidgetImpl::HandleRDMResponse(const uint8_t *data,
                                         unsigned int length) {
  struct ResponseHeader {
    uint8_t version;
    uint8_t response_code;
  };

  if (m_rdm_request_callback == NULL) {
    OLA_FATAL << "Got a response but no callback to run!";
    return;
  }

  bool waiting_for_dub_response = m_pending_request->IsDUB();

  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  auto_ptr<const ola::rdm::RDMRequest> request(m_pending_request.release());

  if (length < sizeof(ResponseHeader)) {
    OLA_WARN << "Invalid RDM response from the widget";
    RunRDMCallback(callback, ola::rdm::RDM_INVALID_RESPONSE);
    return;
  }

  const ResponseHeader *header = reinterpret_cast<const ResponseHeader*>(data);
  if (header->version != 0) {
    OLA_WARN << "Unknown version # in widget response: "
             << static_cast<int>(header->version);
    RunRDMCallback(callback, ola::rdm::RDM_INVALID_RESPONSE);
    return;
  }

  ola::rdm::RDMStatusCode status_code = ola::rdm::RDM_COMPLETED_OK;
  switch (header->response_code) {
    case RC_CHECKSUM_ERROR:
      status_code = ola::rdm::RDM_CHECKSUM_INCORRECT;
      break;
    case RC_FRAMING_ERROR:
    case RC_FRAMING_ERROR2:
    case RC_BAD_STARTCODE:
      status_code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case RC_BAD_SUB_STARTCODE:
      status_code = ola::rdm::RDM_WRONG_SUB_START_CODE;
      break;
    case RC_WRONG_PDL:
    case RC_BAD_PDL:
      status_code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case RC_PACKET_TOO_SHORT:
      status_code = ola::rdm::RDM_PACKET_TOO_SHORT;
      break;
    case RC_PACKET_TOO_LONG:
      status_code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case RC_PHYSICAL_LENGTH_MISMATCH:
      status_code = ola::rdm::RDM_PACKET_LENGTH_MISMATCH;
      break;
    case RC_PDL_LENGTH_MISMATCH:
      status_code = ola::rdm::RDM_PARAM_LENGTH_MISMATCH;
      break;
    case RC_TRANSACTION_MISMATCH:
      status_code = ola::rdm::RDM_TRANSACTION_MISMATCH;
      break;
    case RC_BAD_RESPONSE_TYPE:
      status_code = ola::rdm::RDM_INVALID_RESPONSE_TYPE;
      break;
    case RC_GOOD_LEVEL:
      if (waiting_for_dub_response) {
        status_code = ola::rdm::RDM_DUB_RESPONSE;
      } else {
        OLA_INFO << "Got response code "
                 << static_cast<int>(header->response_code);
        status_code = ola::rdm::RDM_INVALID_RESPONSE;
      }
      break;
    case RC_BAD_LEVEL:
    case RC_BROADCAST:
    case RC_VENDORCAST:
      OLA_INFO << "Got response status_code "
               << static_cast<int>(header->response_code);
      status_code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    case RC_GOOD_RESPONSE:
    case RC_ACK_TIMER:
    case RC_ACK_OVERFLOW:
    case RC_NACK:
    case RC_NACK_UNKNOWN_PID:
    case RC_NACK_FORMAT_ERROR:
    case RC_NACK_HARDWARE_FAULT:
    case RC_NACK_PROXY_REJECT:
    case RC_NACK_WRITE_PROTECT:
    case RC_NACK_COMMAND_CLASS:
    case RC_NACK_DATA_RANGE:
    case RC_NACK_BUFFER_FULL:
    case RC_NACK_PACKET_SIZE:
    case RC_NACK_SUB_DEVICE_RANGE:
    case RC_NACK_PROXY_QUEUE_BUFFER_FULL:
      status_code = ola::rdm::RDM_COMPLETED_OK;
      break;
    case RC_IDLE_LEVEL:
    case RC_TIMED_OUT:
      OLA_INFO << "Request timed out";
      status_code = ola::rdm::RDM_TIMEOUT;
      break;
    case RC_SUBDEVICE_MISMATCH:
      status_code = ola::rdm::RDM_SUB_DEVICE_MISMATCH;
      break;
    case RC_SRC_UID_MISMATCH:
      status_code = ola::rdm::RDM_SRC_UID_MISMATCH;
      break;
    case RC_DEST_UID_MISMATCH:
      status_code = ola::rdm::RDM_DEST_UID_MISMATCH;
      break;
    case RC_COMMAND_CLASS_MISMATCH:
      status_code = ola::rdm::RDM_COMMAND_CLASS_MISMATCH;
      break;
    case RC_PARAM_ID_MISMATCH:
      // this should *hopefully* be caught higher up the stack
      status_code = ola::rdm::RDM_COMPLETED_OK;
      break;
    case RC_DATA_RECEIVED_NO_BREAK:
      OLA_INFO << "Got data with no break";
      status_code = ola::rdm::RDM_INVALID_RESPONSE;
      break;
    default:
      OLA_WARN << "Unknown response status_code "
               << static_cast<int>(header->response_code);
      status_code = ola::rdm::RDM_INVALID_RESPONSE;
  }

  data += sizeof(ResponseHeader);
  length -= sizeof(ResponseHeader);

  auto_ptr<RDMReply> reply;
  if (status_code == ola::rdm::RDM_COMPLETED_OK) {
    rdm::RDMFrame frame(data, length);
    reply.reset(RDMReply::FromFrame(frame, request.get()));
  } else {
    rdm::RDMFrames frames;
    if (length > 0) {
      frames.push_back(rdm::RDMFrame(data, length));
    }
    reply.reset(new RDMReply(status_code, NULL, frames));
  }
  callback->Run(reply.get());
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

  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
}


/**
 * Handle a shutdown message
 */
void DmxterWidgetImpl::HandleShutdown(const uint8_t *data,
                                      unsigned int length) {
  if (length || data) {
    OLA_WARN << "Invalid shutdown message, length was " << length;
  } else {
    OLA_INFO << "Received shutdown message from the Dmxter";
    // Run the on close handler which calls WidgetDetectorThread::FreeWidget.
    // This removes the descriptor from the SS and closes the FD.
    // This is the same behaviour as if the remote end closed the connection
    // i.e. the device was plugged.
    ola::io::ConnectedDescriptor::OnCloseCallback *on_close =
        GetDescriptor()->TransferOnClose();
    on_close->Run();
  }
}


/**
 * DmxterWidget Constructor
 */
DmxterWidget::DmxterWidget(ola::io::ConnectedDescriptor *descriptor,
                           uint16_t esta_id,
                           uint32_t serial,
                           unsigned int queue_size) {
  m_impl = new DmxterWidgetImpl(descriptor, esta_id, serial);
  m_controller = new ola::rdm::DiscoverableQueueingRDMController(m_impl,
                                                                 queue_size);
}

DmxterWidget::~DmxterWidget() {
  // delete the controller after the impl because the controller owns the
  // callback
  delete m_impl;
  delete m_controller;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
