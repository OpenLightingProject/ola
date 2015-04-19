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

#include <memory>
#include <string>
#include <vector>

#include "plugins/usbdmx/JaRuleEndpoint.h"
#include "plugins/usbdmx/JaRuleWidget.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::NewSingleCallback;
using ola::rdm::DiscoveryAgent;
using ola::rdm::DiscoverableQueueingRDMController;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::rdm::rdm_response_code;
using ola::strings::ToHex;
using std::auto_ptr;
using std::string;
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

void JaRuleWidgetImpl::SendRDMRequest(const RDMRequest *request,
                                      ola::rdm::RDMCallback *on_complete) {
  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);

  m_endpoint.SendCommand(
      GetCommandFromRequest(request), data, rdm_length,
      NewSingleCallback(this, &JaRuleWidgetImpl::RDMComplete,
                        request, on_complete));
}

void JaRuleWidgetImpl::MuteDevice(const UID &target,
                                  MuteDeviceCallback *mute_complete) {
  auto_ptr<RDMRequest> request(
      ola::rdm::NewMuteRequest(m_our_uid, target,
                               m_transaction_number.Next()));

  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  m_endpoint.SendCommand(
      JaRuleEndpoint::RDM_REQUEST, data, rdm_length,
      NewSingleCallback(this, &JaRuleWidgetImpl::MuteDeviceComplete,
                        mute_complete));
}

void JaRuleWidgetImpl::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  auto_ptr<RDMRequest> request(
      ola::rdm::NewUnMuteRequest(m_our_uid, UID::AllDevices(),
                                 m_transaction_number.Next()));

  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  m_endpoint.SendCommand(
      JaRuleEndpoint::RDM_BROADCAST_REQUEST, data, rdm_length,
      NewSingleCallback(this, &JaRuleWidgetImpl::UnMuteDeviceComplete,
                        unmute_complete));
}

void JaRuleWidgetImpl::Branch(const UID &lower,
                              const UID &upper,
                              BranchCallback *branch_complete) {
  auto_ptr<RDMRequest> request(
      ola::rdm::NewDiscoveryUniqueBranchRequest(m_our_uid, lower, upper,
                                                m_transaction_number.Next()));
  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  OLA_INFO << "Sending RDM DUB: " << lower << " - " << upper;
  m_endpoint.SendCommand(
      JaRuleEndpoint::RDM_DUB, data, rdm_length,
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
    OLA_UNUSED const std::string &payload) {
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
                                          const std::string &payload) {
  CheckStatusFlags(status_flags);
  bool muted_ok = false;
  if (result == JaRuleEndpoint::COMMAND_COMPLETED_OK &&
      return_code == RC_OK &&
      payload.size() > sizeof(GetSetTiming)) {
    // Skip the timing data & the start code.
    string mute_data = payload.substr(sizeof(GetSetTiming) + 1);
    ola::rdm::rdm_response_code response_code = rdm::RDM_INVALID_RESPONSE;
    auto_ptr<RDMResponse> response(RDMResponse::InflateFromData(
          reinterpret_cast<const uint8_t*>(mute_data.data()),
          mute_data.size(), &response_code));

    // TODO(simon): I guess we could ack timer the MUTE. Handle this case
    // someday.
    muted_ok = (
        response_code == rdm::RDM_COMPLETED_OK &&
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
    OLA_UNUSED const std::string &payload) {
  CheckStatusFlags(status_flags);
  // TODO(simon): At some point we need to account for failures here.
  unmute_complete->Run();
}


void JaRuleWidgetImpl::DUBComplete(BranchCallback *callback,
                                   JaRuleEndpoint::CommandResult result,
                                   uint8_t return_code,
                                   uint8_t status_flags,
                                   const std::string &payload) {
  CheckStatusFlags(status_flags);
  string discovery_data;
  if (payload.size() >= sizeof(DUBTiming)) {
    discovery_data = payload.substr(sizeof(DUBTiming));
  }
  if (result == JaRuleEndpoint::COMMAND_COMPLETED_OK &&
      return_code == RC_OK) {
    callback->Run(reinterpret_cast<const uint8_t*>(discovery_data.data()),
                  discovery_data.size());
  } else {
    callback->Run(NULL, 0);
  }
}

void JaRuleWidgetImpl::RDMComplete(const ola::rdm::RDMRequest *request_ptr,
                                   ola::rdm::RDMCallback *callback,
                                   JaRuleEndpoint::CommandResult result,
                                   uint8_t return_code,
                                   uint8_t status_flags,
                                   const std::string &payload) {
  CheckStatusFlags(status_flags);
  vector<string> packets;
  auto_ptr<const RDMRequest> request(request_ptr);

  if (result != JaRuleEndpoint::COMMAND_COMPLETED_OK) {
    callback->Run(rdm::RDM_FAILED_TO_SEND, NULL, packets);
  }

  JaRuleEndpoint::CommandClass command = GetCommandFromRequest(request.get());
  ola::rdm::rdm_response_code response_code = rdm::RDM_INVALID_RESPONSE;
  ola::rdm::RDMResponse *response = NULL;

  if (command == JaRuleEndpoint::RDM_DUB &&
      return_code == RC_OK) {
    if (payload.size() > sizeof(DUBTiming)) {
      packets.push_back(payload.substr(sizeof(DUBTiming)));
    }
    response_code = rdm::RDM_DUB_RESPONSE;
  } else if (command == JaRuleEndpoint::RDM_BROADCAST_REQUEST &&
             return_code == RC_OK) {
    response_code = rdm::RDM_WAS_BROADCAST;
  } else if (command == JaRuleEndpoint::RDM_BROADCAST_REQUEST &&
             return_code == RC_RDM_BCAST_RESPONSE) {
    if (payload.size() > sizeof(GetSetTiming)) {
      response = UnpackRDMResponse(
          request.get(), payload.substr(sizeof(GetSetTiming)),
          &response_code);
    }
  } else if (command == JaRuleEndpoint::RDM_REQUEST &&
             return_code == RC_OK) {
    if (payload.size() > sizeof(GetSetTiming)) {
      GetSetTiming timing;
      memcpy(reinterpret_cast<uint8_t*>(&timing),
             reinterpret_cast<const uint8_t*>(payload.data()),
             sizeof(timing));
      OLA_INFO << "Response time " << (timing.break_start / 10.0)
               << "uS, Break: "
               << (timing.mark_start - timing.break_start) / 10.0
               << "uS, Mark: " << (timing.mark_end - timing.mark_start) / 10.0
               << "uS";
      response = UnpackRDMResponse(
          request.get(), payload.substr(sizeof(GetSetTiming)),
          &response_code);
    }
  } else if (return_code == RC_RDM_TIMEOUT) {
    response_code = rdm::RDM_TIMEOUT;
  } else if (return_code == RC_TX_ERROR || return_code == RC_BUFFER_FULL) {
    response_code = rdm::RDM_FAILED_TO_SEND;
  } else {
    OLA_WARN << "Unknown Ja Rule RDM RC: " << ToHex(return_code);
    response_code = rdm::RDM_FAILED_TO_SEND;
  }

  callback->Run(response_code, response, packets);
}

ola::rdm::RDMResponse* JaRuleWidgetImpl::UnpackRDMResponse(
    const RDMRequest *request,
    const string &payload,
    ola::rdm::rdm_response_code *response_code) {
  if (payload.empty() ||
      static_cast<uint8_t>(payload[0]) != RDMCommand::START_CODE) {
    *response_code = rdm::RDM_INVALID_RESPONSE;
    return NULL;
  }

  return ola::rdm::RDMResponse::InflateFromData(
      reinterpret_cast<const uint8_t*>(payload.data() + 1), payload.size() - 1,
      response_code, request);
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
