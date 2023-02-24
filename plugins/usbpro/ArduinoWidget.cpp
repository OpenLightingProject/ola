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
 * ArduinoWidget.cpp
 * The Arduino RGB Mixer widget.
 * Copyright (C) 2011 Simon Newton
 */

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "ola/io/ByteString.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/strings/Format.h"
#include "plugins/usbpro/ArduinoRGBDevice.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/ArduinoWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RunRDMCallback;
using std::auto_ptr;
using std::ostringstream;
using std::string;
using std::vector;

const uint8_t ArduinoWidgetImpl::RDM_REQUEST_LABEL = 'R';

const uint8_t ArduinoWidgetImpl::RESPONSE_OK = 0;
const uint8_t ArduinoWidgetImpl::RESPONSE_WAS_BROADCAST = 1;
const uint8_t ArduinoWidgetImpl::RESPONSE_FAILED = 2;
const uint8_t ArduinoWidgetImpl::RESPONSE_FAILED_CHECKSUM = 3;
const uint8_t ArduinoWidgetImpl::RESPONSE_INVALID_DESTINATION = 4;
const uint8_t ArduinoWidgetImpl::RESPONSE_INVALID_COMMAND = 5;


/*
 * New ArduinoWidget device
 * @param descriptor the ConnectedDescriptor for this widget.
 * @param esta_id the ESTA id.
 * @param serial the 4 byte serial which forms part of the UID
 */
ArduinoWidgetImpl::ArduinoWidgetImpl(
    ola::io::ConnectedDescriptor *descriptor,
    uint16_t esta_id,
    uint32_t serial)
    : BaseUsbProWidget(descriptor),
      m_transaction_id(0),
      m_uid(esta_id, serial),
      m_rdm_request_callback(NULL) {
}


/**
 * Clean up
 */
ArduinoWidgetImpl::~ArduinoWidgetImpl() {
  Stop();
}



/**
 * Stop the widget
 */
void ArduinoWidgetImpl::Stop() {
  // timeout any existing message
  if (m_rdm_request_callback) {
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
  }
}


/**
 * Handle an RDM request by passing it through to the Arduino
 */
void ArduinoWidgetImpl::SendRDMRequest(RDMRequest *request_ptr,
                                       ola::rdm::RDMCallback *on_complete) {
  auto_ptr<RDMRequest> request(request_ptr);
  if (request->CommandClass() == ola::rdm::RDMCommand::DISCOVER_COMMAND) {
    RunRDMCallback(on_complete, ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED);
    return;
  }

  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  request->SetTransactionNumber(m_transaction_id++);
  request->SetPortId(1);

  ola::io::ByteString data;
  if (!RDMCommandSerializer::PackWithStartCode(*request, &data)) {
    OLA_WARN << "Failed to pack message, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  m_rdm_request_callback = on_complete;
  m_pending_request.reset(request.release());
  if (SendMessage(RDM_REQUEST_LABEL, data.data(), data.size())) {
    return;
  }
  m_rdm_request_callback = NULL;
  m_pending_request.reset();
  RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
}


/**
 * Called when a new packet arrives
 */
void ArduinoWidgetImpl::HandleMessage(uint8_t label,
                                      const uint8_t *data,
                                      unsigned int length) {
  switch (label) {
    case RDM_REQUEST_LABEL:
      HandleRDMResponse(data, length);
      break;
    default:
      OLA_WARN << "Unknown label: " << strings::ToHex(label);
  }
}


/**
 * Handle a RDM response.
 */
void ArduinoWidgetImpl::HandleRDMResponse(const uint8_t *data,
                                          unsigned int length) {
  if (m_rdm_request_callback == NULL) {
    OLA_FATAL << "Got a response but no callback to run!";
    return;
  }

  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  std::auto_ptr<const ola::rdm::RDMRequest> request(
      m_pending_request.release());

  if (length == 0) {
    // invalid response
    RunRDMCallback(callback, ola::rdm::RDM_INVALID_RESPONSE);
    return;
  }

  if (data[0]) {
    switch (data[0]) {
      case RESPONSE_WAS_BROADCAST:
        RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
        return;
      case RESPONSE_FAILED:
        break;
      case RESPONSE_FAILED_CHECKSUM:
        OLA_WARN << "USB Device reports checksum mismatch";
        break;
      case RESPONSE_INVALID_DESTINATION:
        OLA_WARN << "USB Device reports invalid destination";
        break;
      case RESPONSE_INVALID_COMMAND:
        OLA_WARN << "USB Device reports invalid command";
        break;
      default:
        OLA_WARN << "Invalid response code from USB device: "
                 << static_cast<int>(data[0]);
    }
    RunRDMCallback(callback, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  // response status was RESPONSE_OK
  if (length == 1) {
    // invalid response
    OLA_WARN << "RDM Response was too short";
    RunRDMCallback(callback, ola::rdm::RDM_INVALID_RESPONSE);
    return;
  }

  if (data[1] != ola::rdm::START_CODE) {
    OLA_WARN << "Wrong start code, was " << strings::ToHex(data[1])
             << " required "
             << strings::ToHex(ola::rdm::START_CODE);
    RunRDMCallback(callback, ola::rdm::RDM_INVALID_RESPONSE);
    return;
  }

  rdm::RDMFrame frame(data + 1, length - 1);
  auto_ptr<RDMReply> reply(RDMReply::FromFrame(frame, request.get()));
  callback->Run(reply.get());
}


/**
 * Return the UID Set to the client
 */
void ArduinoWidgetImpl::GetUidSet(ola::rdm::RDMDiscoveryCallback *callback) {
  ola::rdm::UIDSet uid_set;
  uid_set.AddUID(m_uid);
  callback->Run(uid_set);
}


/**
 * ArduinoWidget Constructor
 */
ArduinoWidget::ArduinoWidget(ola::io::ConnectedDescriptor *descriptor,
                             uint16_t esta_id,
                             uint32_t serial,
                             unsigned int queue_size) {
  m_impl = new ArduinoWidgetImpl(descriptor, esta_id, serial);
  m_controller = new ola::rdm::DiscoverableQueueingRDMController(m_impl,
                                                                 queue_size);
}


ArduinoWidget::~ArduinoWidget() {
  // delete the controller after the impl because the controller owns the
  // callback
  delete m_impl;
  delete m_controller;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
