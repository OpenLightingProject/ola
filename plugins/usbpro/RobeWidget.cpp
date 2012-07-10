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
#include <memory>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/RobeWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;

// The DMX frames have an extra 4 bytes at the end
const int RobeWidgetImpl::DMX_FRAME_DATA_SIZE = DMX_UNIVERSE_SIZE + 4;

RobeWidgetImpl::RobeWidgetImpl(ola::io::ConnectedDescriptor *descriptor,
                               ola::thread::SchedulingExecutorInterface *ss,
                               const ola::rdm::UID &uid)
    : BaseRobeWidget(descriptor),
      m_ss(ss),
      m_rdm_request_callback(NULL),
      m_mute_callback(NULL),
      m_unmute_callback(NULL),
      m_branch_callback(NULL),
      m_discovery_agent(this),
      m_dmx_callback(NULL),
      m_pending_request(NULL),
      m_uid(uid),
      m_transaction_number(0) {
}


/**
 * Stop the widget.
 */
void RobeWidgetImpl::Stop() {
  std::vector<std::string> packets;
  if (m_rdm_request_callback) {
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    callback->Run(ola::rdm::RDM_TIMEOUT, NULL, packets);
  }

  m_discovery_agent.Abort();

  if (m_pending_request) {
    delete m_pending_request;
    m_pending_request = NULL;
  }
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
void RobeWidgetImpl::SendRDMRequest(const RDMRequest *request,
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
  OLA_DEBUG << "Sending RDM command. CC: 0x" << std::hex <<
    request->CommandClass() << ", PID 0x" << std::hex <<
    request->ParamId() << ", TN: " << this_transaction_number;
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
}


/**
 * Perform full discovery.
 */
void RobeWidgetImpl::RunFullDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  OLA_INFO << "Full discovery";
  m_discovery_agent.StartFullDiscovery(
    ola::NewSingleCallback(this,
                           &RobeWidgetImpl::DiscoveryComplete,
                           callback));
}


/**
 * Perform incremental discovery.
 */
void RobeWidgetImpl::RunIncrementalDiscovery(
    ola::rdm::RDMDiscoveryCallback *callback) {
  OLA_INFO << "Incremental discovery";
  m_discovery_agent.StartIncrementalDiscovery(
    ola::NewSingleCallback(this,
                           &RobeWidgetImpl::DiscoveryComplete,
                           callback));
}


/**
 * Change to receive mode.
 */
bool RobeWidgetImpl::ChangeToReceiveMode() {
  m_buffer.Reset();
  return SendMessage(DMX_IN_REQUEST, NULL, 0);
}


void RobeWidgetImpl::SetDmxCallback(Callback0<void> *callback) {
  m_dmx_callback.reset(callback);
}


/**
 * Mute a responder
 * @param target the UID to mute
 * @param MuteDeviceCallback the callback to run once the mute request
 * completes.
 */
void RobeWidgetImpl::MuteDevice(const UID &target,
                                MuteDeviceCallback *mute_complete) {
  auto_ptr<RDMRequest> mute_request(
      ola::rdm::NewMuteRequest(m_uid, target, m_transaction_number++));
  OLA_DEBUG << "Muting " << target;
  if (PackAndSendRDMRequest(RDM_REQUEST, mute_request.get()))
    m_mute_callback = mute_complete;
  else
    mute_complete->Run(false);
}


/**
 * Unmute all responders
 * @param UnMuteDeviceCallback the callback to run once the unmute request
 * completes.
 */
void RobeWidgetImpl::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  auto_ptr<RDMRequest> unmute_request(
      ola::rdm::NewUnMuteRequest(m_uid,
                                 ola::rdm::UID::AllDevices(),
                                 m_transaction_number++));
  OLA_DEBUG << "UnMuting all devices";
  if (PackAndSendRDMRequest(RDM_REQUEST, unmute_request.get())) {
    m_unmute_callback = unmute_complete;
  } else {
    OLA_WARN << "Failed to send Unmute all request";
    unmute_complete->Run();
  }
}


/**
 * Send a Discovery Unique Branch
 */
void RobeWidgetImpl::Branch(const UID &lower,
                            const UID &upper,
                            BranchCallback *callback) {
  auto_ptr<RDMRequest> branch_request(
      ola::rdm::NewDiscoveryUniqueBranchRequest(
          m_uid,
          lower,
          upper,
          m_transaction_number++));
  if (PackAndSendRDMRequest(RDM_DISCOVERY, branch_request.get()))
    m_branch_callback = callback;
  else
    callback->Run(NULL, 0);
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
    case BaseRobeWidget::RDM_DISCOVERY_RESPONSE:
      HandleDiscoveryResponse(data, length);
      return;
    case DMX_IN_RESPONSE:
      HandleDmxFrame(data, length);
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
  OLA_DEBUG << "Got RDM Response from Robe Widget";
  std::vector<std::string> packets;
  if (m_unmute_callback) {
    UnMuteDeviceCallback *callback = m_unmute_callback;
    m_unmute_callback = NULL;
    callback->Run();
    return;
  }

  if (m_mute_callback) {
    MuteDeviceCallback *callback = m_mute_callback;
    m_mute_callback = NULL;
    // TODO(simon): actually check the response here
    callback->Run(length > RDM_PADDING_BYTES);
    return;
  }

  if (m_rdm_request_callback == NULL) {
    OLA_FATAL << "Got a RDM response but no callback to run!";
    return;
  }
  ola::rdm::RDMCallback *callback = m_rdm_request_callback;
  m_rdm_request_callback = NULL;
  auto_ptr<const RDMRequest> request(m_pending_request);
  m_pending_request = NULL;

  // this was a broadcast request
  if (request->DestinationUID().IsBroadcast()) {
    callback->Run(ola::rdm::RDM_WAS_BROADCAST, NULL, packets);
    return;
  }

  if (length == RDM_PADDING_BYTES) {
    // this indicates that no request was recieved
    callback->Run(ola::rdm::RDM_TIMEOUT, NULL, packets);
    return;
  }

  string packet;
  packet.assign(reinterpret_cast<const char*>(data), length);
  packets.push_back(packet);

  // try to inflate
  ola::rdm::rdm_response_code response_code;
  RDMResponse *response = RDMResponse::InflateFromData(
      packet,
      &response_code,
      request.get());
  callback->Run(response_code, response, packets);
}


/**
 * Handle a response to a Discovery unique branch request
 */
void RobeWidgetImpl::HandleDiscoveryResponse(const uint8_t *data,
                                             unsigned int length) {
  if (m_branch_callback) {
    BranchCallback *callback = m_branch_callback;
    m_branch_callback = NULL;
    // there are always 4 bytes padded on the end of the response
    if (length <= RDM_PADDING_BYTES)
      callback->Run(NULL, 0);
    else
      callback->Run(data, length - RDM_PADDING_BYTES);
  } else {
    OLA_WARN << "Got response to DUB but no callback defined!";
  }
}


/**
 * Called when the discovery process finally completes
 * @param callback the callback passed to StartFullDiscovery or
 * StartIncrementalDiscovery that we should execute.
 * @param status true if discovery worked, false otherwise
 * @param uids the UIDSet of UIDs that were found.
 */
void RobeWidgetImpl::DiscoveryComplete(
    ola::rdm::RDMDiscoveryCallback *callback,
    bool status,
    const UIDSet &uids) {
  if (callback)
    callback->Run(uids);
  (void) status;
}


/**
 * Handle DMX data
 */
void RobeWidgetImpl::HandleDmxFrame(const uint8_t *data, unsigned int length) {
  m_buffer.Set(data, length);
  if (m_dmx_callback.get())
    m_dmx_callback->Run();
}


/**
 * Send a RDM request to the widget
 */
bool RobeWidgetImpl::PackAndSendRDMRequest(uint8_t label,
                                           const RDMRequest *request) {
  unsigned int length = request->Size();
  uint8_t data[length + RDM_PADDING_BYTES];
  memset(data, 0, length + RDM_PADDING_BYTES);
  request->Pack(data, &length);
  return SendMessage(label, data, length + RDM_PADDING_BYTES);
}


/**
 * RobeWidget Constructor
 */
RobeWidget::RobeWidget(ola::io::ConnectedDescriptor *descriptor,
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
