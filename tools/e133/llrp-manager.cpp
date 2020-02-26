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
 * llrp-manager.cpp
 * Run a very simple E1.33 LLRP Manager.
 * Copyright (C) 2020 Peter Newman
 */

#include <ola/Callback.h>
#include <ola/acn/ACNPort.h>
#include <ola/acn/CID.h>
#include <ola/base/Flags.h>
#include <ola/base/Init.h>
#include <ola/base/SysExits.h>
#include <ola/io/SelectServer.h>
#include <ola/network/Interface.h>
#include <ola/network/InterfacePicker.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/MACAddress.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/Socket.h>
#include <ola/rdm/PidStoreHelper.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/RDMReply.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>
#include <ola/util/SequenceNumber.h>
#include <ola/DmxBuffer.h>
#include <ola/Logging.h>

#include <algorithm>
#include <string>
#include <memory>
#include <vector>

#include "libs/acn/HeaderSet.h"
#include "libs/acn/LLRPHeader.h"
#include "libs/acn/LLRPInflator.h"
#include "libs/acn/LLRPPDU.h"
#include "libs/acn/LLRPProbeReplyInflator.h"
#include "libs/acn/LLRPProbeReplyPDU.h"
#include "libs/acn/LLRPProbeRequestPDU.h"
#include "libs/acn/PreamblePacker.h"
#include "libs/acn/RDMInflator.h"
#include "libs/acn/RDMPDU.h"
#include "libs/acn/RootHeader.h"
#include "libs/acn/RootInflator.h"
#include "libs/acn/RootSender.h"
#include "libs/acn/Transport.h"
#include "libs/acn/UDPTransport.h"

using std::string;
using std::vector;
using std::auto_ptr;

using ola::acn::CID;
using ola::acn::IncomingUDPTransport;
using ola::acn::LLRPHeader;
using ola::acn::LLRPProbeReplyInflator;
using ola::acn::LLRPProbeReplyPDU;
using ola::acn::LLRPProbeRequestPDU;
using ola::acn::OutgoingUDPTransport;
using ola::acn::OutgoingUDPTransportImpl;
using ola::acn::RootHeader;
using ola::network::Interface;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::MACAddress;
using ola::network::NetworkToHost;
using ola::rdm::PidStoreHelper;
using ola::rdm::RDMGetRequest;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::UID;
using ola::rdm::UIDSet;

DEFINE_string(manager_uid, "7a70:00000002", "The UID of the manager.");
DEFINE_default_bool(set, false, "Send a set rather than a get.");
DEFINE_default_bool(allow_loopback, false, "Include the loopback interface.");
DEFINE_s_string(interface, i, "",
                "The interface name (e.g. eth0) or IP address of the network "
                "interface to use for LLRP messages.");

auto_ptr<ola::network::InterfacePicker> picker(
  ola::network::InterfacePicker::NewPicker());
ola::network::Interface m_interface;
ola::network::UDPSocket m_socket;
uint8_t *m_recv_buffer;
std::auto_ptr<UID> manager_uid;
std::auto_ptr<PidStoreHelper> m_pid_helper;
ola::SequenceNumber<uint32_t> m_llrp_transaction_number_sequence;
ola::SequenceNumber<uint8_t> m_rdm_transaction_number_sequence;

std::string pid_name;
vector<string> rdm_inputs;

ola::acn::PreamblePacker m_packer;
CID cid = CID::Generate();
ola::acn::RootSender m_root_sender(cid, true);

bool CheckCIDAddressedToUs(const CID destination_cid) {
  return (destination_cid == CID::LLRPBroadcastCID() ||
          destination_cid == cid);
}

void SendLLRPProbeRequest() {
  LLRPHeader llrp_header = LLRPHeader(CID::LLRPBroadcastCID(),
                                      m_llrp_transaction_number_sequence.Next());

  IPV4Address *target_address = IPV4Address::FromString("239.255.250.133");

  OutgoingUDPTransportImpl transport_impl = OutgoingUDPTransportImpl(&m_socket, &m_packer);
  OutgoingUDPTransport transport(&transport_impl,
                                 *target_address,
                                 ola::acn::LLRP_PORT);

  LLRPProbeRequestPDU probe_request(
      LLRPProbeRequestPDU::VECTOR_PROBE_REQUEST_DATA,
      *UID::FromString("0000:00000000"),
      *UID::FromString("ffff:ffffffff"),
      false,
      false,
      UIDSet());

  ola::acn::LLRPPDU pdu(ola::acn::VECTOR_LLRP_PROBE_REQUEST, llrp_header, &probe_request);

  m_root_sender.SendPDU(ola::acn::VECTOR_ROOT_LLRP, pdu, &transport);
  OLA_DEBUG << "Sent PDU";
}

void HandleLLRPProbeReply(
    const ola::acn::HeaderSet *headers,
    const LLRPProbeReplyInflator::LLRPProbeReply &reply) {
  OLA_DEBUG << "Potentially handling probe reply from " << reply.uid;

  const LLRPHeader llrp_header = headers->GetLLRPHeader();
  if (!CheckCIDAddressedToUs(llrp_header.DestinationCid())) {
    OLA_INFO << "Ignoring probe request as it's not addressed to us or the LLRP broadcast CID";
    return;
  }

  const RootHeader root_header = headers->GetRootHeader();

  OLA_DEBUG << "Source CID: " << root_header.GetCid();
  OLA_DEBUG << "TN: " << llrp_header.TransactionNumber();

  LLRPHeader rdm_llrp_header = LLRPHeader(root_header.GetCid(),
                                          m_llrp_transaction_number_sequence.Next());

  IPV4Address *target_address = IPV4Address::FromString("239.255.250.133");

  OutgoingUDPTransportImpl transport_impl = OutgoingUDPTransportImpl(&m_socket, &m_packer);
  OutgoingUDPTransport transport(&transport_impl,
                                 *target_address,
                                 ola::acn::LLRP_PORT);

  bool is_set = FLAGS_set;

  // get the pid descriptor
  const ola::rdm::PidDescriptor *pid_descriptor = m_pid_helper->GetDescriptor(
      pid_name,
      reply.uid.ManufacturerId());

  uint16_t pid_value;
  if (!pid_descriptor &&
      (ola::PrefixedHexStringToInt(pid_name, &pid_value) ||
       ola::StringToInt(pid_name, &pid_value))) {
    pid_descriptor = m_pid_helper->GetDescriptor(
        pid_value,
        reply.uid.ManufacturerId());
  }

  if (!pid_descriptor) {
    std::cout << "Unknown PID: " << pid_name << std::endl;
    std::cout << "Use --list-pids to list the available PIDs." << std::endl;
    return;
  }

  const ola::messaging::Descriptor *descriptor = NULL;
  if (is_set) {
    descriptor = pid_descriptor->SetRequest();
  } else {
    descriptor = pid_descriptor->GetRequest();
  }

  if (!descriptor) {
    std::cout << (is_set ? "SET" : "GET") << " command not supported for "
      << pid_name << std::endl;
    return;
  }

  // attempt to build the message
  auto_ptr<const ola::messaging::Message> message(m_pid_helper->BuildMessage(
      descriptor,
      rdm_inputs));

  if (!message.get()) {
    std::cout << m_pid_helper->SchemaAsString(descriptor);
    return;
  }

  unsigned int param_data_length;
  const uint8_t *param_data = m_pid_helper->SerializeMessage(
      message.get(),
      &param_data_length);

  RDMRequest *request;
  if (is_set) {
   request = new RDMSetRequest(
      *manager_uid,
      reply.uid,
      m_rdm_transaction_number_sequence.Next(),  // transaction #
      1,  // port id
      0,  // sub device
      pid_descriptor->Value(),  // param id
      param_data,  // data
      param_data_length);  // data length
  } else {
   request = new RDMGetRequest(
      *manager_uid,
      reply.uid,
      m_rdm_transaction_number_sequence.Next(),  // transaction #
      1,  // port id
      0,  // sub device
      pid_descriptor->Value(),  // param id
      param_data,  // data
      param_data_length);  // data length
  }

  ola::io::ByteString raw_reply;
  ola::rdm::RDMCommandSerializer::Pack(*request, &raw_reply);

  ola::acn::RDMPDU rdm_reply(raw_reply);

  ola::acn::LLRPPDU pdu(ola::acn::VECTOR_LLRP_RDM_CMD, rdm_llrp_header, &rdm_reply);

  m_root_sender.SendPDU(ola::acn::VECTOR_ROOT_LLRP, pdu, &transport);
  OLA_DEBUG << "Sent PDU";
}

/**
 * Handle an ACK response
 */
void HandleAckResponse(uint16_t manufacturer_id,
                       bool is_set,
                       uint16_t pid,
                       const uint8_t *data,
                       unsigned int length) {
  const ola::rdm::PidDescriptor *pid_descriptor = m_pid_helper->GetDescriptor(
      pid,
      manufacturer_id);

  if (!pid_descriptor) {
    OLA_WARN << "Unknown PID: " << pid << ".";
    return;
  }

  const ola::messaging::Descriptor *descriptor = NULL;
  if (is_set) {
    descriptor = pid_descriptor->SetResponse();
  } else {
    descriptor = pid_descriptor->GetResponse();
  }

  if (!descriptor) {
    OLA_WARN << "Unknown response message: " << (is_set ? "SET" : "GET") <<
        " " << pid_descriptor->Name();
    return;
  }

  auto_ptr<const ola::messaging::Message> message(
      m_pid_helper->DeserializeMessage(descriptor, data, length));

  if (!message.get()) {
    OLA_WARN << "Unable to inflate RDM response";
    return;
  }

  std::cout << m_pid_helper->PrettyPrintMessage(manufacturer_id,
                                               is_set,
                                               pid,
                                               message.get());
}

void HandleRDM(
    const ola::acn::HeaderSet *headers,
    const string &raw_response) {
  IPV4SocketAddress target = headers->GetTransportHeader().Source();
  OLA_INFO << "Got RDM response from " << target;

  if (!CheckCIDAddressedToUs(headers->GetLLRPHeader().DestinationCid())) {
    OLA_INFO << "Ignoring RDM response as it's not addressed to us or the LLRP broadcast CID";
    return;
  }

  ola::rdm::RDMStatusCode status_code;
  // attempt to unpack as a request
  ola::rdm::RDMResponse *response = ola::rdm::RDMResponse::InflateFromData(
    reinterpret_cast<const uint8_t*>(raw_response.data()),
    raw_response.size(),
    &status_code);

  OLA_DEBUG << "Got status code " << ola::rdm::StatusCodeToString(status_code);

  if (!response) {
    OLA_WARN << "Failed to unpack LLRP RDM message, ignoring request.";
    return;
  } else {
    OLA_DEBUG << "Got RDM response " << response->ToString();
  }

  if (!response->DestinationUID().DirectedToUID(*manager_uid)) {
    OLA_WARN << "Destination UID " << response->DestinationUID() << " was not "
             << "directed to us";
    return;
  }

  OLA_INFO << "Got RDM response from " << response->SourceUID();
  if (response->ResponseType() == ola::rdm::RDM_ACK) {
    HandleAckResponse(response->SourceUID().ManufacturerId(),
                      (response->CommandClass() ==
                           ola::rdm::RDMCommand::SET_COMMAND_RESPONSE),
                      response->ParamId(),
                      response->ParamData(),
                      response->ParamDataSize());
  } else if (response->ResponseType() == ola::rdm::RDM_NACK_REASON) {
    uint16_t nack_reason;
    if (response->ParamDataSize() != sizeof(nack_reason)) {
      OLA_WARN << "Invalid NACK reason size of " << response->ParamDataSize();
    } else {
      memcpy(reinterpret_cast<uint8_t*>(&nack_reason), response->ParamData(),
             sizeof(nack_reason));
      nack_reason = NetworkToHost(nack_reason);
      std::cout << "Request NACKed: " <<
        ola::rdm::NackReasonToString(nack_reason) << std::endl;
    }
  } else {
    OLA_WARN << "Unknown RDM response type "
             << ola::strings::ToHex(response->ResponseType());
  }
}

int main(int argc, char* argv[]) {
  ola::AppInit(&argc, argv, "[options]", "Run a very simple E1.33 LLRP Manager.");

  if (argc >= 2) {
    pid_name = argv[1];

    // split out rdm message params from the pid name (ignore program name)
    rdm_inputs.resize(argc - 2);
    for(int i = 0; i < argc - 2; i++) {
      rdm_inputs[i] = argv[i+2];
    }
    OLA_DEBUG << "Parsed RDM";
  } else {
    OLA_INFO << "No RDM to parse";
  }

  m_pid_helper.reset(new PidStoreHelper(""));
  m_pid_helper->Init();

  manager_uid.reset(UID::FromString(FLAGS_manager_uid));
  if (!manager_uid.get()) {
    OLA_WARN << "Invalid UID: " << FLAGS_manager_uid;
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  } else {
    OLA_INFO << "Started LLRP Manager with UID " << *manager_uid;
  }

  ola::io::SelectServer ss;

  if (!m_socket.Init()) {
    return false;
  }
  std::cout << "Init!" << std::endl;

  std::cout << "Using CID " << cid << std::endl;

  IPV4Address *addr = IPV4Address::FromString("239.255.250.134");

  if (!m_socket.Bind(IPV4SocketAddress(*addr,
                                       ola::acn::LLRP_PORT))) {
    return false;
  }
  std::cout << "Bind!" << std::endl;

  const std::string m_preferred_ip = FLAGS_interface;
  ola::network::InterfacePicker::Options options;
  options.include_loopback = FLAGS_allow_loopback;
  if (!picker->ChooseInterface(&m_interface, m_preferred_ip, options)) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  std::cout << "IF " << m_interface << std::endl;

  // If we enable multicast loopback, we can test two bits of software on the
  // same machine, but we get, and must ignore, all our own requests too
  if (!m_socket.JoinMulticast(m_interface.ip_address, *addr, true)) {
    OLA_WARN << "Failed to join multicast group " << addr;
    return false;
  }

  ola::acn::RootInflator root_inflator;
  ola::acn::LLRPInflator llrp_inflator;
  ola::acn::LLRPProbeReplyInflator llrp_probe_reply_inflator;
  llrp_probe_reply_inflator.SetLLRPProbeReplyHandler(
      ola::NewCallback(&HandleLLRPProbeReply));
  ola::acn::RDMInflator llrp_rdm_inflator(ola::acn::VECTOR_LLRP_RDM_CMD);
  llrp_rdm_inflator.SetGenericRDMHandler(
      ola::NewCallback(&HandleRDM));

  // setup all the inflators
  root_inflator.AddInflator(&llrp_inflator);
  llrp_inflator.AddInflator(&llrp_probe_reply_inflator);
  llrp_inflator.AddInflator(&llrp_rdm_inflator);

  IncomingUDPTransport m_incoming_udp_transport(&m_socket, &root_inflator);
  m_socket.SetOnData(ola::NewCallback(&m_incoming_udp_transport,
                                      &IncomingUDPTransport::Receive));
  ss.AddReadDescriptor(&m_socket);

  // TODO(Peter): Add the ability to filter on UID or UID+CID to avoid the probing

  // TODO(Peter): Send this three times
  // TODO(Peter): Deal with known UID list etc and proper discovery
  SendLLRPProbeRequest();
  ss.Run();

  return 0;
}
