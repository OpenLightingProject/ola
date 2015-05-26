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
 * JaRuleWidgetImpl.cpp
 * The implementation of the Ja Rule Widget.
 * Copyright (C) 2015 Simon Newton
 */

#include "plugins/usbdmx/JaRuleWidgetImpl.h"

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/rdm/DiscoveryAgent.h>
#include <ola/rdm/QueueingRDMController.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UID.h>
#include <ola/util/Utils.h>
#include <ola/strings/Format.h>
#include <ola/util/SequenceNumber.h>
#include <string.h>

#include <memory>
#include <vector>

#include "plugins/usbdmx/JaRuleEndpoint.h"
#include "plugins/usbdmx/JaRuleWidget.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::NewSingleCallback;
using ola::io::ByteString;
using ola::rdm::DiscoveryAgent;
using ola::rdm::DiscoverableQueueingRDMController;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::RunRDMCallback;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::rdm::RDMStatusCode;
using ola::strings::ToHex;
using std::auto_ptr;
using std::vector;

JaRuleWidgetImpl::JaRuleWidgetImpl(ola::io::SelectServerInterface *ss,
                                   AsyncronousLibUsbAdaptor *adaptor,
                                   libusb_device *device,
                                   const UID &controller_uid)
    : m_endpoint(ss, adaptor, device),
      m_in_shutdown(false),
      m_dmx_in_progress(false),
      m_dmx_queued(false),
      m_dmx_callback(NewCallback(this, &JaRuleWidgetImpl::DMXComplete)),
      m_discovery_agent(this),
      m_our_uid(controller_uid) {
}

JaRuleWidgetImpl::~JaRuleWidgetImpl() {
  m_in_shutdown = true;
  m_discovery_agent.Abort();
  m_endpoint.CancelAll();
  delete m_dmx_callback;
}

bool JaRuleWidgetImpl::Init() {
  return m_endpoint.Init();
}

void JaRuleWidgetImpl::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  OLA_INFO << "Full discovery triggered";
  m_discovery_agent.StartFullDiscovery(
      NewSingleCallback(this, &JaRuleWidgetImpl::DiscoveryComplete,
      callback));
}

void JaRuleWidgetImpl::RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
  OLA_INFO << "Incremental discovery triggered";
  m_discovery_agent.StartIncrementalDiscovery(
      NewSingleCallback(this, &JaRuleWidgetImpl::DiscoveryComplete,
      callback));
}

void JaRuleWidgetImpl::SendRDMRequest(RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  ByteString frame;
  if (!RDMCommandSerializer::Pack(*request, &frame)) {
    RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    delete request;
    return;
  }

  m_endpoint.SendCommand(
      GetCommandFromRequest(request), frame.data(), frame.size(),
      NewSingleCallback(this, &JaRuleWidgetImpl::RDMComplete,
                        static_cast<const RDMRequest*>(request), on_complete));
}

void JaRuleWidgetImpl::MuteDevice(const UID &target,
                                  MuteDeviceCallback *mute_complete) {
  auto_ptr<RDMRequest> request(
      ola::rdm::NewMuteRequest(m_our_uid, target,
                               m_transaction_number.Next()));

  ByteString frame;
  RDMCommandSerializer::Pack(*request, &frame);
  m_endpoint.SendCommand(
      JaRuleEndpoint::RDM_REQUEST, frame.data(), frame.size(),
      NewSingleCallback(this, &JaRuleWidgetImpl::MuteDeviceComplete,
                        mute_complete));
}

void JaRuleWidgetImpl::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  auto_ptr<RDMRequest> request(
      ola::rdm::NewUnMuteRequest(m_our_uid, UID::AllDevices(),
                                 m_transaction_number.Next()));

  ByteString frame;
  RDMCommandSerializer::Pack(*request, &frame);
  m_endpoint.SendCommand(
      JaRuleEndpoint::RDM_BROADCAST_REQUEST, frame.data(), frame.size(),
      NewSingleCallback(this, &JaRuleWidgetImpl::UnMuteDeviceComplete,
                        unmute_complete));
}

void JaRuleWidgetImpl::Branch(const UID &lower,
                              const UID &upper,
                              BranchCallback *branch_complete) {
  auto_ptr<RDMRequest> request(
      ola::rdm::NewDiscoveryUniqueBranchRequest(m_our_uid, lower, upper,
                                                m_transaction_number.Next()));

  ByteString frame;
  RDMCommandSerializer::Pack(*request, &frame);
  OLA_INFO << "Sending RDM DUB: " << lower << " - " << upper;
  m_endpoint.SendCommand(
      JaRuleEndpoint::RDM_DUB, frame.data(), frame.size(),
      NewSingleCallback(this, &JaRuleWidgetImpl::DUBComplete, branch_complete));
}

bool JaRuleWidgetImpl::SendDMX(const DmxBuffer &buffer) {
  if (m_dmx_in_progress) {
    m_dmx = buffer;
    m_dmx_queued = true;
  } else {
    m_dmx_in_progress = true;
    m_endpoint.SendCommand(JaRuleEndpoint::TX_DMX, buffer.GetRaw(),
                           buffer.Size(), m_dmx_callback);
  }
  return true;
}

void JaRuleWidgetImpl::ResetDevice() {
  m_endpoint.SendCommand(JaRuleEndpoint::RESET_DEVICE, NULL, 0, NULL);
}

void JaRuleWidgetImpl::CheckStatusFlags(uint8_t flags) {
  if (flags & JaRuleEndpoint::LOGS_PENDING_FLAG) {
    OLA_INFO << "Logs pending!";
  }
  if (flags & JaRuleEndpoint::FLAGS_CHANGED_FLAG) {
    OLA_INFO << "Flags changed!";
  }
  if (flags & JaRuleEndpoint::MSG_TRUNCATED_FLAG) {
    OLA_INFO << "Message truncated";
  }
}

void JaRuleWidgetImpl::DMXComplete(
    OLA_UNUSED JaRuleEndpoint::CommandResult result,
    OLA_UNUSED uint8_t return_code,
    uint8_t status_flags,
    OLA_UNUSED const ola::io::ByteString &payload) {
  CheckStatusFlags(status_flags);
  // We ignore status and return_code, since DMX is streaming.
  if (m_dmx_queued && !m_in_shutdown) {
    m_endpoint.SendCommand(JaRuleEndpoint::TX_DMX, m_dmx.GetRaw(),
                           m_dmx.Size(), m_dmx_callback);
    m_dmx_queued = false;
  } else {
    m_dmx_in_progress = false;
  }
}

void JaRuleWidgetImpl::MuteDeviceComplete(MuteDeviceCallback *mute_complete,
                                          JaRuleEndpoint::CommandResult result,
                                          uint8_t return_code,
                                          uint8_t status_flags,
                                          const ola::io::ByteString &payload) {
  CheckStatusFlags(status_flags);
  bool muted_ok = false;
  if (result == JaRuleEndpoint::COMMAND_COMPLETED_OK &&
      return_code == RC_OK &&
      payload.size() > sizeof(GetSetTiming)) {
    // Skip the timing data & the start code.
    ola::rdm::RDMStatusCode status_code = rdm::RDM_INVALID_RESPONSE;
    auto_ptr<RDMResponse> response(RDMResponse::InflateFromData(
          payload.substr(sizeof(GetSetTiming) + 1), &status_code));

    // TODO(simon): I guess we could ack timer the MUTE. Handle this case
    // someday.
    muted_ok = (
        status_code == rdm::RDM_COMPLETED_OK &&
        response.get() &&
        response->CommandClass() == RDMCommand::DISCOVER_COMMAND_RESPONSE &&
        response->ResponseType() == rdm::RDM_ACK);
  }
  mute_complete->Run(muted_ok);
}

void JaRuleWidgetImpl::UnMuteDeviceComplete(
    UnMuteDeviceCallback *unmute_complete,
    OLA_UNUSED JaRuleEndpoint::CommandResult result,
    OLA_UNUSED uint8_t return_code,
    OLA_UNUSED uint8_t status_flags,
    OLA_UNUSED const ola::io::ByteString &payload) {
  CheckStatusFlags(status_flags);
  // TODO(simon): At some point we need to account for failures here.
  unmute_complete->Run();
}


void JaRuleWidgetImpl::DUBComplete(BranchCallback *callback,
                                   JaRuleEndpoint::CommandResult result,
                                   uint8_t return_code,
                                   uint8_t status_flags,
                                   const ola::io::ByteString &payload) {
  CheckStatusFlags(status_flags);
  ByteString discovery_data;
  if (payload.size() >= sizeof(DUBTiming)) {
    discovery_data = payload.substr(sizeof(DUBTiming));
  }
  if (result == JaRuleEndpoint::COMMAND_COMPLETED_OK && return_code == RC_OK) {
    callback->Run(discovery_data.data(), discovery_data.size());
  } else {
    callback->Run(NULL, 0);
  }
}

void JaRuleWidgetImpl::RDMComplete(const ola::rdm::RDMRequest *request_ptr,
                                   ola::rdm::RDMCallback *callback,
                                   JaRuleEndpoint::CommandResult result,
                                   uint8_t return_code,
                                   uint8_t status_flags,
                                   const ola::io::ByteString &payload) {
  CheckStatusFlags(status_flags);
  auto_ptr<const RDMRequest> request(request_ptr);
  ola::rdm::RDMFrames frames;

  if (result != JaRuleEndpoint::COMMAND_COMPLETED_OK) {
    RunRDMCallback(callback, rdm::RDM_FAILED_TO_SEND);
  }

  JaRuleEndpoint::CommandClass command = GetCommandFromRequest(request.get());
  ola::rdm::RDMStatusCode status_code = rdm::RDM_INVALID_RESPONSE;
  ola::rdm::RDMResponse *response = NULL;

  if (command == JaRuleEndpoint::RDM_DUB && return_code == RC_OK) {
    if (payload.size() > sizeof(DUBTiming)) {
      DUBTiming timing;
      memcpy(reinterpret_cast<uint8_t*>(&timing),
             payload.data(), sizeof(timing));
      OLA_INFO << "Start time " << (timing.start / 10.0)
               << "uS, End: " << (timing.end / 10.0) << "uS";

      ola::rdm::RDMFrame frame(payload.substr(sizeof(DUBTiming)));
      frame.timing.response_time = 100 * timing.start;
      frame.timing.data_time = 100 * (timing.end - timing.start);
      frames.push_back(frame);
    }
    status_code = rdm::RDM_DUB_RESPONSE;
  } else if (command == JaRuleEndpoint::RDM_BROADCAST_REQUEST &&
             return_code == RC_OK) {
    status_code = rdm::RDM_WAS_BROADCAST;
  } else if (command == JaRuleEndpoint::RDM_BROADCAST_REQUEST &&
             return_code == RC_RDM_BCAST_RESPONSE) {
    if (payload.size() > sizeof(GetSetTiming)) {
      response = UnpackRDMResponse(
          request.get(), payload.substr(sizeof(GetSetTiming)),
          &status_code);
    }
  } else if (command == JaRuleEndpoint::RDM_REQUEST &&
             return_code == RC_OK) {
    if (payload.size() > sizeof(GetSetTiming)) {
      GetSetTiming timing;
      memcpy(reinterpret_cast<uint8_t*>(&timing),
             payload.data(), sizeof(timing));
      OLA_INFO << "Response time " << (timing.break_start / 10.0)
               << "uS, Break: "
               << (timing.mark_start - timing.break_start) / 10.0
               << "uS, Mark: " << (timing.mark_end - timing.mark_start) / 10.0
               << "uS";
      response = UnpackRDMResponse(
          request.get(), payload.substr(sizeof(GetSetTiming)),
          &status_code);

      ola::rdm::RDMFrame frame(payload.substr(sizeof(GetSetTiming)));
      frame.timing.response_time = 100 * timing.break_start;
      frame.timing.break_time = 100 * (timing.mark_start - timing.break_start);
      frame.timing.mark_time = 100 * (timing.mark_end - timing.mark_start);
      frames.push_back(frame);
    }
  } else if (return_code == RC_RDM_TIMEOUT) {
    status_code = rdm::RDM_TIMEOUT;
  } else if (return_code == RC_TX_ERROR || return_code == RC_BUFFER_FULL) {
    status_code = rdm::RDM_FAILED_TO_SEND;
  } else {
    OLA_WARN << "Unknown Ja Rule RDM RC: " << ToHex(return_code);
    status_code = rdm::RDM_FAILED_TO_SEND;
  }

  RDMReply reply(status_code, response, frames);
  callback->Run(&reply);
}

ola::rdm::RDMResponse* JaRuleWidgetImpl::UnpackRDMResponse(
    const RDMRequest *request,
    const ByteString &payload,
    ola::rdm::RDMStatusCode *status_code) {
  if (payload.empty() || payload[0] != RDMCommand::START_CODE) {
    *status_code = rdm::RDM_INVALID_RESPONSE;
    return NULL;
  }

  return ola::rdm::RDMResponse::InflateFromData(
      payload.data() + 1, payload.size() - 1, status_code, request);
}

void JaRuleWidgetImpl::DiscoveryComplete(RDMDiscoveryCallback *callback,
                                         OLA_UNUSED bool ok,
                                         const UIDSet& uids) {
  m_uids = uids;
  if (callback) {
    callback->Run(m_uids);
  }
}

JaRuleEndpoint::CommandClass JaRuleWidgetImpl::GetCommandFromRequest(
      const ola::rdm::RDMRequest *request) {
  if (request->IsDUB()) {
    return JaRuleEndpoint::RDM_DUB;
  }
  return request->DestinationUID().IsBroadcast() ?
      JaRuleEndpoint::RDM_BROADCAST_REQUEST :
      JaRuleEndpoint::RDM_REQUEST;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
