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
 * RobeWidget.cpp
 * Read and Write to a Robe USB Widget.
 * Copyright (C) 2011 Simon Newton
 */

#include <string.h>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/RobeWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::rdm::UID;

// The DMX frames have an extra 4 bytes at the end
const int RobeWidgetImpl::DMX_FRAME_DATA_SIZE = DMX_UNIVERSE_SIZE + 4;

RobeWidgetImpl::RobeWidgetImpl(ola::network::ConnectedDescriptor *descriptor,
                               ola::thread::SchedulingExecutorInterface *ss,
                               const ola::rdm::UID &uid)
    : BaseRobeWidget(descriptor),
      m_ss(ss),
      m_rdm_request_callback(NULL),
      m_pending_request(NULL),
      m_uid(uid),
      m_transaction_number(0) {
  ola::rdm::UID mock_uid(0x00a1, 0x00020020);
  m_uids.AddUID(mock_uid);
}


/**
 * Send DMX
 * @param buffer the DMX data
 */
bool RobeWidgetImpl::SendDMX(const DmxBuffer &buffer) {
  // the data is 512 + an extra 4 bytes
  uint8_t output_data[DMX_FRAME_DATA_SIZE];
  memset(output_data, 0, DMX_FRAME_DATA_SIZE);
  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(output_data, &length);
  return SendMessage(CHANNEL_A_OUT,
                     reinterpret_cast<uint8_t*>(&output_data),
                     length + 4);
}


/**
 * Send a RDM Message
 */
void RobeWidgetImpl::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *on_complete) {
  std::vector<string> packets;
  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    delete request;
    return;
  }

  // prepare the buffer for the RDM data, we don't need to include the start
  // code. We need to include 4 bytes at the end, these bytes can be any value.
  unsigned int data_size = request->Size() + RDM_PADDING_BYTES;
  uint8_t *data = new uint8_t[data_size];
  memset(data, 0, data_size);

  unsigned int this_transaction_number = m_transaction_number++;
  unsigned int port_id = 1;

  bool r = request->PackWithControllerParams(data,
                                             &data_size,
                                             m_uid,
                                             this_transaction_number,
                                             port_id);

  if (!r) {
    OLA_WARN << "Failed to pack message, dropping request";
    delete[] data;
    delete request;
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }

  m_rdm_request_callback = on_complete;
  // convert the request into one that matches this widget
  m_pending_request = request->DuplicateWithControllerParams(
      m_uid,
      this_transaction_number,
      port_id);
  delete request;
  if (!SendMessage(BaseRobeWidget::RDM_REQUEST, data, data_size +
                  RDM_PADDING_BYTES)) {
    m_rdm_request_callback = NULL;
    m_pending_request = NULL;
    delete[] data;
    delete request;
    on_complete->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }

  delete[] data;
  // sent ok
  if (m_pending_request->DestinationUID().IsBroadcast()) {
    m_rdm_request_callback = NULL;
    delete m_pending_request;
    m_pending_request = NULL;
    on_complete->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
  }
}


/**
 * Perform full discovery.
 */
bool RobeWidgetImpl::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  OLA_INFO << "Full discovery";
  if (callback)
    callback->Run(m_uids);
  return true;
}


/**
 * Perform incremental discovery.
 */
bool RobeWidgetImpl::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  OLA_INFO << "Incremental discovery";
  if (callback)
    callback->Run(m_uids);
  return true;
}


/**
 * Handle a Robe message
 */
void RobeWidgetImpl::HandleMessage(uint8_t label,
                                   const uint8_t *data,
                                   unsigned int length) {
  switch (label) {
    case BaseRobeWidget::RDM_RESPONSE:
      HandleRDMResponse(data, length);
      return;
    default:
      OLA_INFO << "Unknown message from Robe widget " << std::hex <<
        static_cast<unsigned int>(label);
  }
}


/**
 * Handle a RDM response
 */
void RobeWidgetImpl::HandleRDMResponse(const uint8_t *data,
                                       unsigned int length) {
  std::vector<std::string> packets;
  if (m_rdm_request_callback == NULL) {
    OLA_FATAL << "Got a RDM response but no callback to run!";
    return;
  }
  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  const ola::rdm::RDMRequest *request = m_pending_request;
  m_pending_request = NULL;

  if (length == RDM_PADDING_BYTES) {
    // this indicates that no request was recieved
    callback->Run(ola::rdm::RDM_TIMEOUT, NULL, packets);
    delete request;
    return;
  }

  string packet;
  packet.assign(reinterpret_cast<const char*>(data), length);
  packets.push_back(packet);

  // try to inflate
  ola::rdm::rdm_response_code response_code;
  ola::rdm::RDMResponse *response = ola::rdm::RDMResponse::InflateFromData(
      packet,
      &response_code,
      request);
  callback->Run(response_code, response, packets);
  delete request;
}


/**
 * RobeWidget Constructor
 */
RobeWidget::RobeWidget(ola::network::ConnectedDescriptor *descriptor,
                       ola::thread::SchedulingExecutorInterface *ss,
                       const ola::rdm::UID &uid,
                       unsigned int queue_size) {
  m_impl = new RobeWidgetImpl(descriptor, ss, uid);
  m_controller = new ola::rdm::DiscoverableQueueingRDMController(m_impl,
                                                                 queue_size);
}


RobeWidget::~RobeWidget() {
  // delete the controller after the impl because the controller owns the
  // callback
  delete m_impl;
  delete m_controller;
}
}  // usbpro
}  // plugin
}  // ola
