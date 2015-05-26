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

#include "tools/ja-rule/JaRuleWidgetImpl.h"

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
#include <ola/util/SequenceNumber.h>

#include <memory>
#include <string>
#include <vector>

#include "tools/ja-rule/JaRuleEndpoint.h"
#include "tools/ja-rule/JaRuleWidget.h"

using ola::NewSingleCallback;
using ola::rdm::DiscoveryAgent;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::DiscoverableQueueingRDMController;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::strings::ToHex;
using std::auto_ptr;
using std::string;

JaRuleWidgetImpl::JaRuleWidgetImpl(JaRuleEndpoint *device,
                                   const UID &controller_uid)
    : m_device(device),
      m_discovery_agent(this),
      m_our_uid(controller_uid),
      m_rdm_callback(NULL),
      m_mute_callback(NULL),
      m_unmute_callback(NULL),
      m_branch_callback(NULL) {
  device->SetHandler(this);
}

JaRuleWidgetImpl::~JaRuleWidgetImpl() {
  m_discovery_agent.Abort();
  m_device->SetHandler(NULL);
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


void JaRuleWidgetImpl::SendRDMRequest(RDMRequest *request_ptr,
                                      ola::rdm::RDMCallback *on_complete) {
  auto_ptr<RDMRequest> request(request_ptr);
  if (!CheckForDevice()) {
    ola::rdm::RunRDMCallback(on_complete, ola::rdm::RDM_FAILED_TO_SEND);
    return;
  }

  m_rdm_callback = on_complete;

  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  m_device->SendMessage(JaRuleEndpoint::RDM_REQUEST, data, rdm_length);
}

void JaRuleWidgetImpl::MuteDevice(const UID &target,
                                  MuteDeviceCallback *mute_complete) {
  if (!CheckForDevice()) {
    mute_complete->Run(false);
    return;
  }

  auto_ptr<RDMRequest> request(
      ola::rdm::NewMuteRequest(m_our_uid, target,
                               m_transaction_number.Next()));

  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  m_device->SendMessage(JaRuleEndpoint::RDM_REQUEST, data, rdm_length);

  m_mute_callback = mute_complete;
}

void JaRuleWidgetImpl::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  if (!CheckForDevice()) {
    unmute_complete->Run();
    return;
  }

  auto_ptr<RDMRequest> request(
      ola::rdm::NewUnMuteRequest(m_our_uid, UID::AllDevices(),
                                 m_transaction_number.Next()));

  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  m_device->SendMessage(JaRuleEndpoint::RDM_REQUEST, data, rdm_length);

  m_unmute_callback = unmute_complete;
}

void JaRuleWidgetImpl::Branch(const UID &lower,
                              const UID &upper,
                              BranchCallback *branch_complete) {
  if (!CheckForDevice()) {
    branch_complete->Run(NULL, 0);
    return;
  }

  auto_ptr<RDMRequest> request(
      ola::rdm::NewDiscoveryUniqueBranchRequest(m_our_uid, lower, upper,
                                                m_transaction_number.Next()));
  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  OLA_INFO << "Sending DUB (" << lower << ", " << upper << ")";
  m_device->SendMessage(JaRuleEndpoint::RDM_DUB, data, rdm_length);

  m_branch_callback = branch_complete;
}

void JaRuleWidgetImpl::ResetDevice() {
  m_device->SendMessage(JaRuleEndpoint::RESET_DEVICE, NULL, 0);
}

void JaRuleWidgetImpl::NewMessage(const Message &message) {
  OLA_INFO << "Got message with command "
           << static_cast<int>(message.command);

  switch (message.command) {
    case JaRuleEndpoint::RDM_DUB:
      HandleDUBResponse(message);
      break;
    case JaRuleEndpoint::RDM_REQUEST:
      HandleRDM(message);
      break;
    case JaRuleEndpoint::RESET_DEVICE:
      PrintAck(message);
      break;
    default:
      OLA_WARN << "Unknown command: " << ToHex(message.command);
  }

  if (message.flags & LOGS_PENDING_FLAG) {
    OLA_INFO << "Logs pending!";
  }
  if (message.flags & FLAGS_CHANGED_FLAG) {
    OLA_INFO << "Flags changed!";
  }
  if (message.flags & MSG_TRUNCATED_FLAG) {
    OLA_INFO << "Message truncated";
  }
}


bool JaRuleWidgetImpl::CheckForDevice() const {
  if (!m_device) {
    OLA_INFO << "Device not present";
  }
  return m_device != NULL;
}

void JaRuleWidgetImpl::PrintAck(const Message& message) {
  OLA_INFO << "ACK (" << static_cast<int>(message.return_code)
           << "): payload_size: " << message.payload_size;
}

void JaRuleWidgetImpl::HandleDUBResponse(const Message& message) {
  if (m_branch_callback) {
    const uint8_t *data = NULL;
    unsigned int size = 0;
    if (message.payload && message.payload_size > 1) {
      data = message.payload + 1;
      size = message.payload_size - 1;
    }
    BranchCallback *callback = m_branch_callback;
    m_branch_callback = NULL;
    callback->Run(data, size);
  }
}

void JaRuleWidgetImpl::HandleRDM(const Message& message) {
  if (m_unmute_callback) {
    UnMuteDeviceCallback *callback = m_unmute_callback;
    m_unmute_callback = NULL;
    callback->Run();
    return;
  }

  if (m_mute_callback) {
    // TODO(simon): inflate the actual RDM response here. Right now we treat
    // any response as good.
    bool ok = message.payload_size > 1;
    MuteDeviceCallback *callback = m_mute_callback;
    m_mute_callback = NULL;
    callback->Run(ok);
  }
}

void JaRuleWidgetImpl::DiscoveryComplete(RDMDiscoveryCallback *callback,
                                         OLA_UNUSED bool ok,
                                         const UIDSet& uids) {
  OLA_DEBUG << "Discovery complete: " << uids;
  m_uids = uids;
  if (callback) {
    callback->Run(m_uids);
  }
}
