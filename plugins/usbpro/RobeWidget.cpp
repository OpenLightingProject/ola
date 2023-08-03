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
 * RobeWidget.cpp
 * Read and Write to a Robe USB Widget.
 * Copyright (C) 2011 Simon Newton
 */

#include <string.h>
#include <memory>
#include <string>
#include <vector>
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/io/ByteString.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/strings/Format.h"
#include "plugins/usbpro/BaseRobeWidget.h"
#include "plugins/usbpro/RobeWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using ola::io::ByteString;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMReply;
using ola::rdm::RunRDMCallback;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::unique_ptr;
using std::string;
using std::vector;

// The DMX frames have an extra 4 bytes at the end
const int RobeWidgetImpl::DMX_FRAME_DATA_SIZE = DMX_UNIVERSE_SIZE + 4;

RobeWidgetImpl::RobeWidgetImpl(ola::io::ConnectedDescriptor *descriptor,
                               const ola::rdm::UID &uid)
    : BaseRobeWidget(descriptor),
      m_rdm_request_callback(NULL),
      m_mute_callback(NULL),
      m_unmute_callback(NULL),
      m_branch_callback(NULL),
      m_discovery_agent(this),
      m_dmx_callback(),
      m_pending_request(),
      m_uid(uid),
      m_transaction_number(0) {
}


/**
 * Stop the widget.
 */
void RobeWidgetImpl::Stop() {
  if (m_rdm_request_callback) {
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
  }

  m_discovery_agent.Abort();
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
void RobeWidgetImpl::SendRDMRequest(RDMRequest *request_ptr,
                                    ola::rdm::RDMCallback *on_complete) {
  unique_ptr<RDMRequest> request(request_ptr);
  if (m_rdm_request_callback) {
    OLA_FATAL << "Previous request hasn't completed yet, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  // prepare the buffer for the RDM data, we don't need to include the start
  // code. We need to include 4 bytes at the end, these bytes can be any value.
  ByteString data;

  unsigned int this_transaction_number = m_transaction_number++;
  unsigned int port_id = 1;

  request->SetSourceUID(m_uid);
  request->SetTransactionNumber(this_transaction_number);
  request->SetPortId(port_id);

  if (!RDMCommandSerializer::Pack(*request, &data)) {
    OLA_WARN << "Failed to pack message, dropping request";
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  // Append the extra padding bytes, which can be set to any value.
  data.append(RDM_PADDING_BYTES, 0);

  OLA_DEBUG << "Sending RDM command. CC: "
            << strings::ToHex(request->CommandClass()) << ", PID "
            << strings::ToHex(request->ParamId()) << ", TN: "
            << this_transaction_number;

  m_rdm_request_callback = on_complete;
  m_pending_request.reset(request.release());

  const uint8_t label = m_pending_request->IsDUB() ?
      RDM_DISCOVERY : RDM_REQUEST;
  bool sent_ok = SendMessage(label, data.data(), data.size());

  if (!sent_ok) {
    m_rdm_request_callback = NULL;
    m_pending_request.reset();
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
  }
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
 * @param mute_complete the callback to run once the mute request
 * completes.
 */
void RobeWidgetImpl::MuteDevice(const UID &target,
                                MuteDeviceCallback *mute_complete) {
  unique_ptr<RDMRequest> mute_request(
      ola::rdm::NewMuteRequest(m_uid, target, m_transaction_number++));
  OLA_DEBUG << "Muting " << target;
  if (PackAndSendRDMRequest(RDM_REQUEST, mute_request.get()))
    m_mute_callback = mute_complete;
  else
    mute_complete->Run(false);
}


/**
 * Unmute all responders
 * @param unmute_complete the callback to run once the unmute request
 * completes.
 */
void RobeWidgetImpl::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  unique_ptr<RDMRequest> unmute_request(
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
  unique_ptr<RDMRequest> branch_request(
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
      OLA_INFO << "Unknown message from Robe widget "
               << strings::ToHex(label);
  }
}


/**
 * Handle a RDM response
 */
void RobeWidgetImpl::HandleRDMResponse(const uint8_t *data,
                                       unsigned int length) {
  OLA_DEBUG << "Got RDM Response from Robe Widget, length " << length;
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
  unique_ptr<const RDMRequest> request(m_pending_request.release());

  // this was a broadcast request
  if (request->DestinationUID().IsBroadcast()) {
    RunRDMCallback(callback, ola::rdm::RDM_WAS_BROADCAST);
    return;
  }

  if (length == RDM_PADDING_BYTES) {
    // this indicates that no request was received
    RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
    return;
  }

  // The widget response data doesn't contain a start code so we prepend it.
  rdm::RDMFrame frame(data, length, rdm::RDMFrame::Options(true));
  unique_ptr<RDMReply> reply(RDMReply::FromFrame(frame, request.get()));
  callback->Run(reply.get());
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
  } else if (m_rdm_request_callback) {
    ola::rdm::RDMCallback *callback = m_rdm_request_callback;
    m_rdm_request_callback = NULL;
    unique_ptr<const RDMRequest> request(m_pending_request.release());

    if (length <= RDM_PADDING_BYTES) {
      // this indicates that no request was received
      RunRDMCallback(callback, ola::rdm::RDM_TIMEOUT);
    } else {
      unique_ptr<RDMReply> reply(RDMReply::DUBReply(
        rdm::RDMFrame(data, length - RDM_PADDING_BYTES)));
      callback->Run(reply.get());
    }
  } else {
    OLA_WARN << "Got response to DUB but no callbacks defined!";
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
  ByteString frame;
  if (!RDMCommandSerializer::Pack(*request, &frame)) {
    return false;
  }
  frame.append(RDM_PADDING_BYTES, 0);
  return SendMessage(label, frame.data(), frame.size());
}


/**
 * RobeWidget Constructor
 */
RobeWidget::RobeWidget(ola::io::ConnectedDescriptor *descriptor,
                       const ola::rdm::UID &uid,
                       unsigned int queue_size) {
  m_impl = new RobeWidgetImpl(descriptor, uid);
  m_controller = new ola::rdm::DiscoverableQueueingRDMController(m_impl,
                                                                 queue_size);
}


RobeWidget::~RobeWidget() {
  // delete the controller after the impl because the controller owns the
  // callback
  delete m_impl;
  delete m_controller;
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
