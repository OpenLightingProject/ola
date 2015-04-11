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
      m_discovery_agent(this),
      m_our_uid(controller_uid),
      m_rdm_callback(NULL),
      m_rdm_request(NULL),
      m_mute_callback(NULL),
      m_unmute_callback(NULL),
      m_branch_callback(NULL) {
  m_endpoint.SetHandler(this);
}

JaRuleWidgetImpl::~JaRuleWidgetImpl() {
  m_discovery_agent.Abort();
  m_endpoint.SetHandler(NULL);
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
  m_rdm_callback = on_complete;
  m_rdm_request = request;

  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);

  JaRuleEndpoint::Command command;
  if (request->IsDUB()) {
    command = JaRuleEndpoint::RDM_DUB;
  } else {
    command = request->DestinationUID().IsBroadcast() ?
              JaRuleEndpoint::RDM_BROADCAST_REQUEST :
              JaRuleEndpoint::RDM_REQUEST;
  }
  m_endpoint.SendMessage(command, data, rdm_length);
}

void JaRuleWidgetImpl::MuteDevice(const UID &target,
                                  MuteDeviceCallback *mute_complete) {
  auto_ptr<RDMRequest> request(
      ola::rdm::NewMuteRequest(m_our_uid, target,
                               m_transaction_number.Next()));

  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  m_endpoint.SendMessage(JaRuleEndpoint::RDM_REQUEST, data, rdm_length);

  m_mute_callback = mute_complete;
}

void JaRuleWidgetImpl::UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
  auto_ptr<RDMRequest> request(
      ola::rdm::NewUnMuteRequest(m_our_uid, UID::AllDevices(),
                                 m_transaction_number.Next()));

  unsigned int rdm_length = RDMCommandSerializer::RequiredSize(*request);
  uint8_t data[rdm_length];
  RDMCommandSerializer::Pack(*request, data, &rdm_length);
  m_endpoint.SendMessage(JaRuleEndpoint::RDM_BROADCAST_REQUEST, data,
                         rdm_length);

  m_unmute_callback = unmute_complete;
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
  m_endpoint.SendMessage(JaRuleEndpoint::RDM_DUB, data, rdm_length);

  m_branch_callback = branch_complete;
}

bool JaRuleWidgetImpl::SendDMX(const DmxBuffer &buffer) {
  // TODO(simon): We should figure out throttling here.
  m_endpoint.SendMessage(JaRuleEndpoint::TX_DMX, buffer.GetRaw(),
                         buffer.Size());
  return true;
}

void JaRuleWidgetImpl::ResetDevice() {
  m_endpoint.SendMessage(JaRuleEndpoint::RESET_DEVICE, NULL, 0);
}

void JaRuleWidgetImpl::NewMessage(const Message &message) {
  OLA_INFO << "Got message " << ToHex(message.command) << ", RC "
           << ToHex(message.return_code);
  switch (message.command) {
    case JaRuleEndpoint::TX_DMX:
      // Ignore for now.
      // TODO(simon): handle this.
      break;
    case JaRuleEndpoint::RDM_DUB:
      HandleDUBResponse(message);
      break;
    case JaRuleEndpoint::RDM_REQUEST:
    case JaRuleEndpoint::RDM_BROADCAST_REQUEST:
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

void JaRuleWidgetImpl::PrintAck(const Message &message) {
  OLA_INFO << "ACK (" << static_cast<int>(message.return_code)
           << "): payload_size: " << message.payload_size;
}

void JaRuleWidgetImpl::HandleDUBResponse(const Message &message) {
  const uint8_t *data = NULL;
  unsigned int size = 0;
  if (message.payload && message.payload_size > 1) {
    data = message.payload + 1;
    size = message.payload_size - 1;
  }

  if (m_branch_callback) {
    BranchCallback *callback = m_branch_callback;
    m_branch_callback = NULL;
    callback->Run(data, size);
  } else if (m_rdm_callback) {
    vector<string> packets;
    RDMCallback *callback = m_rdm_callback;
    m_rdm_callback = NULL;
    auto_ptr<const RDMRequest> request(m_rdm_request);
    m_rdm_request = NULL;
    string packet;
    if (data) {
      packet.assign(reinterpret_cast<const char*>(data), size);
      packets.push_back(packet);
    }
    callback->Run(
        message.return_code == RC_RX_TIMEOUT ?
            rdm::RDM_TIMEOUT :
            rdm::RDM_DUB_RESPONSE,
        NULL, packets);
  }
}

void JaRuleWidgetImpl::HandleRDM(const Message &message) {
  if (m_unmute_callback) {
    // TODO(simon): At some point we need to account for failures here.
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

  if (m_rdm_callback) {
    HandleRDMResponse(message);
  }
}

void JaRuleWidgetImpl::HandleRDMResponse(const Message &message) {
  vector<string> packets;
  RDMCallback *callback = m_rdm_callback;
  m_rdm_callback = NULL;
  auto_ptr<const RDMRequest> request(m_rdm_request);
  m_rdm_request = NULL;

  if (message.payload == 0) {
    callback->Run(rdm::RDM_FAILED_TO_SEND, NULL, packets);
    return;
  }

  uint8_t rc = message.return_code;

  ola::rdm::rdm_response_code response_code = rdm::RDM_INVALID_RESPONSE;
  ola::rdm::RDMResponse *response = NULL;

  if (message.command == JaRuleEndpoint::RDM_BROADCAST_REQUEST) {
    switch (rc) {
      case RC_OK:
        response = UnpackRDMResponse(
            request.get(), message.payload + 1, message.payload_size - 1,
            &response_code);
        break;
      case RC_RX_TIMEOUT:
        response_code = rdm::RDM_WAS_BROADCAST;
        break;
      case RC_TX_ERROR:
        response_code = rdm::RDM_FAILED_TO_SEND;
        break;
      default:
        OLA_WARN << "Unknown Ja Rule RDM RC: " << ToHex(rc);
        response_code = rdm::RDM_FAILED_TO_SEND;
        break;
    }
  } else {
    switch (rc) {
      case RC_OK:
        response = UnpackRDMResponse(
            request.get(), message.payload + 1, message.payload_size - 1,
            &response_code);
        break;
      case RC_TX_ERROR:
        response_code = rdm::RDM_FAILED_TO_SEND;
        break;
      case RC_RX_TIMEOUT:
        response_code = rdm::RDM_TIMEOUT;
        break;
      default:
        OLA_WARN << "Unknown Ja Rule RDM RC: " << ToHex(rc);
        response_code = rdm::RDM_FAILED_TO_SEND;
        break;
    }
  }
  callback->Run(response_code, response, packets);
}

ola::rdm::RDMResponse* JaRuleWidgetImpl::UnpackRDMResponse(
    const RDMRequest *request,
    const uint8_t *data,
    unsigned int length,
    ola::rdm::rdm_response_code *response_code) {

  // TODO(simon): remove this.
  ola::strings::FormatData(&std::cout, data, length);

  if (length <= 1 || data[0] != RDMCommand::START_CODE) {
    *response_code = rdm::RDM_INVALID_RESPONSE;
    return NULL;
  }

  return ola::rdm::RDMResponse::InflateFromData(
      data + 1, length - 1, response_code, request);
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
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
