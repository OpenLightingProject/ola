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
 * ArduinoRGBDevice.h
 * The Arduino RGB Mixer device.
 * Copyright (C) 2011 Simon Newton
 */

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/SelectServerInterface.h"
#include "ola/rdm/RDMCommand.h"
#include "olad/PortDecorators.h"
#include "plugins/usbpro/ArduinoRGBDevice.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;

const uint8_t ArduinoWidgetImpl::RDM_REQUEST_LABEL = 'R';

const uint8_t ArduinoWidgetImpl::RESPONSE_OK = 0;
const uint8_t ArduinoWidgetImpl::RESPONSE_WAS_BROADCAST = 1;
const uint8_t ArduinoWidgetImpl::RESPONSE_FAILED = 2;
const uint8_t ArduinoWidgetImpl::RESPONSE_FAILED_CHECKSUM = 3;
const uint8_t ArduinoWidgetImpl::RESONSE_INVALID_DESTINATION = 4;
const uint8_t ArduinoWidgetImpl::RESPONSE_INVALID_COMMAND = 5;


/*
 * New ArduinoWidget device
 * @param widget the underlying UsbWidget
 * @param esta_id the ESTA id.
 * @param serial the 4 byte serial which forms part of the UID
 */
ArduinoWidgetImpl::ArduinoWidgetImpl(UsbWidgetInterface *widget,
                                     uint16_t esta_id,
                                     uint32_t serial):
    m_transaction_id(0),
    m_uid(esta_id, serial),
    m_widget(widget),
    m_pending_request(NULL),
    m_rdm_request_callback(NULL) {
  m_widget->SetMessageHandler(
      NewCallback(this, &ArduinoWidgetImpl::HandleMessage));
}


/**
 * Clean up
 */
ArduinoWidgetImpl::~ArduinoWidgetImpl() {
  // timeout any existing message
  std::vector<std::string> packets;
  if (m_rdm_request_callback)
    m_rdm_request_callback->Run(ola::rdm::RDM_TIMEOUT, NULL, packets);

  if (m_pending_request)
    delete m_pending_request;
}


/*
 * Send a dmx msg
 * @returns true if we sent ok, false otherwise
 */
bool ArduinoWidgetImpl::SendDMX(const DmxBuffer &buffer) {
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
      OLA_WARN << "Unknown label: 0x" << std::hex <<
        static_cast<int>(label);
  }
}



/**
 * Handle an RDM request by passing it through to the Arduino
 */
void ArduinoWidgetImpl::SendRDMRequest(
    const ola::rdm::RDMRequest *request,
    ola::rdm::RDMCallback *on_complete) {

  std::vector<std::string> packets;
  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    delete request;
    return;
  }

  unsigned int data_size = request->Size();
  // allow an extra byte for the start code
  uint8_t *data = new uint8_t[data_size + 1];
  data[0] = ola::rdm::RDMCommand::START_CODE;

  if (request->PackWithControllerParams(data + 1,
                                        &data_size,
                                        request->SourceUID(),
                                        m_transaction_id++,
                                        1)) {
    data_size++;
    m_rdm_request_callback = on_complete;
    m_pending_request = request;
    if (m_widget->SendMessage(RDM_REQUEST_LABEL, data, data_size)) {
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


void ArduinoWidgetImpl::HandleRDMResponse(const uint8_t *data,
                                          unsigned int length) {
  std::vector<std::string> packets;
  if (m_rdm_request_callback == NULL) {
    OLA_FATAL << "Got a response but no callback to run!";
    return;
  }

  stringstream str;
  for (unsigned int i = 0; i < length; ++i) {
    str << std::hex << static_cast<int>(data[i]) << " ";
  }

  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  std::auto_ptr<const ola::rdm::RDMRequest> request(m_pending_request);
  m_pending_request = NULL;

  if (length == 0) {
    // invalid response
    callback->Run(ola::rdm::RDM_INVALID_RESPONSE, NULL, packets);
    return;
  }

  if (data[0]) {
    switch (data[0]) {
      case RESPONSE_WAS_BROADCAST:
        callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
        return;
      case RESPONSE_FAILED:
        break;
      case RESPONSE_FAILED_CHECKSUM:
        OLA_WARN << "USB Device reports checksum mismatch";
        break;
      case RESONSE_INVALID_DESTINATION:
        OLA_WARN << "USB Device reports invalid destination";
        break;
      case RESPONSE_INVALID_COMMAND:
        OLA_WARN << "USB Device reports invalid command";
        return;
      default:
        OLA_WARN << "Invalid response code from USB device: " <<
          static_cast<int>(data[0]);
    }
    callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }

  // response status was RESPONSE_OK
  if (length == 1) {
    // invalid response
    OLA_WARN << "RDM Response was too short";
    callback->Run(ola::rdm::RDM_INVALID_RESPONSE, NULL, packets);
    return;
  }

  if (data[1] != ola::rdm::RDMCommand::START_CODE) {
    OLA_WARN << "Wrong start code, was 0x" << std::hex <<
    static_cast<int>(data[1]) << " required 0x" <<
    static_cast<int>(ola::rdm::RDMCommand::START_CODE);
    callback->Run(ola::rdm::RDM_INVALID_RESPONSE, NULL, packets);
    return;
  }

  string packet;
  packet.assign(reinterpret_cast<const char*>(data + 2), length - 2);
  packets.push_back(packet);

  ola::rdm::rdm_response_code code;
  ola::rdm::RDMResponse *response = ola::rdm::RDMResponse::InflateFromData(
      packet,
      &code,
      request.get(),
      m_transaction_id - 1);

  if (response)
    callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);
  else
    callback->Run(code, NULL, packets);
}


/**
 * Return the UID Set to the client
 */
bool ArduinoWidgetImpl::GetUidSet(ola::rdm::RDMDiscoveryCallback *callback) {
  ola::rdm::UIDSet uid_set;
  uid_set.AddUID(m_uid);
  callback->Run(uid_set);
  return true;
}


/**
 * ArduinoWidget Constructor
 */
ArduinoWidget::ArduinoWidget(UsbWidgetInterface *widget,
                             uint16_t esta_id,
                             uint32_t serial,
                             unsigned int queue_size) {
  m_impl = new ArduinoWidgetImpl(widget, esta_id, serial);
  m_controller = new ola::rdm::DiscoverableQueueingRDMController(m_impl,
                                                                 queue_size);
}


ArduinoWidget::~ArduinoWidget() {
  // delete the controller after the impl because the controller owns the
  // callback
  delete m_impl;
  delete m_controller;
}
}  // usbpro
}  // plugin
}  // ola
