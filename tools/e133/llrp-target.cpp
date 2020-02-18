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
 * llrp-target.cpp
 * Run a very simple E1.33 LLRP Target.
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
#include <ola/network/SocketAddress.h>
#include <ola/network/Socket.h>
#include <ola/rdm/DummyResponder.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/RDMHelper.h>
#include <ola/rdm/RDMReply.h>
#include <ola/rdm/RDMResponseCodes.h>
#include <ola/rdm/UID.h>
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
#include "libs/acn/LLRPProbeReplyPDU.h"
#include "libs/acn/LLRPProbeRequestInflator.h"
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
using ola::acn::LLRPProbeReplyPDU;
using ola::acn::LLRPProbeRequestInflator;
using ola::acn::OutgoingUDPTransport;
using ola::acn::OutgoingUDPTransportImpl;
using ola::acn::RootHeader;
using ola::network::Interface;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::MACAddress;
using ola::rdm::RDMReply;
using ola::rdm::RDMResponse;
using ola::rdm::UID;

DEFINE_string(uid, "7a70:00000001", "The UID of the target.");

auto_ptr<ola::network::InterfacePicker> picker(
  ola::network::InterfacePicker::NewPicker());
ola::network::Interface m_interface;
const std::string m_preferred_ip;
ola::network::UDPSocket m_socket;
uint8_t *m_recv_buffer;
std::auto_ptr<UID> target_uid;
std::auto_ptr<ola::rdm::DummyResponder> dummy_responder;

ola::acn::PreamblePacker m_packer;
CID cid = CID::Generate();
ola::acn::RootSender m_root_sender(cid, true);

bool CheckCIDAddressedToUs(const CID destination_cid) {
  return (destination_cid == CID::LLRPBroadcastCID() ||
          destination_cid == cid);
}

bool CompareInterfaceMACs(Interface a, Interface b) {
  return a.hw_address < b.hw_address;
}

Interface FindLowestMAC() {
  // TODO(Peter): Get some clarification on whether we only care about active
  // interfaces, or any installed ones?
  // TODO(Peter): Work out what to do here if running on localhost only? Return
  // 00:00:00:00:00:00
  vector<Interface> interfaces = picker->GetInterfaces(false);
  vector<Interface>::iterator result = std::min_element(interfaces.begin(),
                                                        interfaces.end(),
                                                        CompareInterfaceMACs);
  return *result;
}

void HandleLLRPProbeRequest(
    const ola::acn::HeaderSet *headers,
    const LLRPProbeRequestInflator::LLRPProbeRequest &request) {
  OLA_DEBUG << "Potentially handling probe from " << request.lower << " to "
            << request.upper;

  const LLRPHeader llrp_header = headers->GetLLRPHeader();
  if (!CheckCIDAddressedToUs(llrp_header.DestinationCid())) {
    OLA_INFO << "Ignoring probe request as it's not addressed to us or the LLRP broadcast CID";
    return;
  }

  if ((*target_uid < request.lower) || (*target_uid > request.upper)) {
    OLA_INFO << "Ignoring probe request as we are not in the target UID range";
    return;
  }

  OLA_DEBUG << "Known UIDs are: " << request.known_uids;

  if (request.known_uids.Contains(*target_uid)) {
    OLA_INFO << "Ignoring probe request as we are already in the known UID "
             << "list";
    return;
  }

  // TODO(Peter): Check the filter bits!

  const RootHeader root_header = headers->GetRootHeader();

  OLA_DEBUG << "Source CID: " << root_header.GetCid();
  OLA_DEBUG << "TN: " << llrp_header.TransactionNumber();

  LLRPHeader reply_llrp_header = LLRPHeader(root_header.GetCid(),
                                            llrp_header.TransactionNumber());

  IPV4Address *target_address = IPV4Address::FromString("239.255.250.134");

  OutgoingUDPTransportImpl transport_impl = OutgoingUDPTransportImpl(&m_socket, &m_packer);
  OutgoingUDPTransport transport(&transport_impl,
                                 *target_address,
                                 ola::acn::LLRP_PORT);

  LLRPProbeReplyPDU probe_reply(
      LLRPProbeReplyPDU::VECTOR_PROBE_REPLY_DATA,
      *target_uid,
      FindLowestMAC().hw_address,
      LLRPProbeReplyPDU::LLRP_COMPONENT_TYPE_NON_RDMNET);

  ola::acn::LLRPPDU pdu(ola::acn::VECTOR_LLRP_PROBE_REPLY, reply_llrp_header, &probe_reply);

  // TODO(Peter): Delay sending by 0 to LLRP_MAX_BACKOFF!

  m_root_sender.SendPDU(ola::acn::VECTOR_ROOT_LLRP, pdu, &transport);
  OLA_DEBUG << "Sent PDU";
}

void RDMRequestComplete(
    ola::acn::HeaderSet headers,
    ola::rdm::RDMReply *reply) {
  OLA_INFO << "Got RDM reply to send";
  OLA_DEBUG << reply->ToString();

  const RDMResponse *response = reply->Response();
  uint8_t response_type = response->ResponseType();

  if (response_type == ola::rdm::RDM_ACK_TIMER ||
      response_type == ola::rdm::ACK_OVERFLOW) {
    // Technically we shouldn't have even actioned the request but we can't
    // really do that in OLA, as we don't know what it might return until we've
    // done it
    OLA_DEBUG << "Got a disallowed ACK, mangling to NR_ACTION_NOT_SUPPORTED";
    response = NackWithReason(response, ola::rdm::NR_ACTION_NOT_SUPPORTED);
  } else {
    OLA_DEBUG << "Got an acceptable response type: "
              << (unsigned int)response_type;
  }

  const RootHeader root_header = headers.GetRootHeader();
  const LLRPHeader llrp_header = headers.GetLLRPHeader();

  OLA_DEBUG << "Source CID: " << root_header.GetCid();
  OLA_DEBUG << "TN: " << llrp_header.TransactionNumber();

  LLRPHeader reply_llrp_header = LLRPHeader(root_header.GetCid(),
                                            llrp_header.TransactionNumber());

  IPV4Address *target_address = IPV4Address::FromString("239.255.250.134");

  OutgoingUDPTransportImpl transport_impl = OutgoingUDPTransportImpl(&m_socket, &m_packer);
  OutgoingUDPTransport transport(&transport_impl,
                                 *target_address,
                                 ola::acn::LLRP_PORT);

  ola::io::ByteString raw_reply;
  ola::rdm::RDMCommandSerializer::Pack(*response, &raw_reply);

  ola::acn::RDMPDU rdm_reply(raw_reply);

  ola::acn::LLRPPDU pdu(ola::acn::VECTOR_LLRP_RDM_CMD, reply_llrp_header, &rdm_reply);

  m_root_sender.SendPDU(ola::acn::VECTOR_ROOT_LLRP, pdu, &transport);
  OLA_DEBUG << "Sent RDM PDU";
}

void HandleRDM(
    const ola::acn::HeaderSet *headers,
    const string &raw_request) {
  IPV4SocketAddress target = headers->GetTransportHeader().Source();
  OLA_INFO << "Got RDM request from " << target;

  if (!CheckCIDAddressedToUs(headers->GetLLRPHeader().DestinationCid())) {
    OLA_INFO << "Ignoring RDM request as it's not addressed to us or the LLRP broadcast CID";
    return;
  }

  // attempt to unpack as a request
  ola::rdm::RDMRequest *request = ola::rdm::RDMRequest::InflateFromData(
    reinterpret_cast<const uint8_t*>(raw_request.data()),
    raw_request.size());

  if (!request) {
    OLA_WARN << "Failed to unpack LLRP RDM message, ignoring request.";
    return;
  } else {
    OLA_DEBUG << "Got RDM request " << request->ToString();
  }

  if (!request->DestinationUID().DirectedToUID(*target_uid)) {
    OLA_WARN << "Destination UID " << request->DestinationUID() << " was not "
             << "directed to us";
    return;
  }

  if (!((request->SubDevice() == ola::rdm::ROOT_RDM_DEVICE) ||
        (request->SubDevice() == ola::rdm::ALL_RDM_SUBDEVICES))) {
    OLA_WARN << "Subdevice " << request->SubDevice() << " was not the root or "
             << "broadcast subdevice, NACKing";
    // Immediately send a NACK
    RDMReply reply(
        ola::rdm::RDM_COMPLETED_OK,
        NackWithReason(request, ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE));
    RDMRequestComplete(*headers, &reply);
  } else {
    // Dispatch the message to the responder
    dummy_responder->SendRDMRequest(
        request,
        ola::NewSingleCallback(&RDMRequestComplete,
                               *headers));
  }
}

int main(int argc, char* argv[]) {
  ola::AppInit(&argc, argv, "[options]", "Run a very simple E1.33 LLRP Target.");

  target_uid.reset(UID::FromString(FLAGS_uid));
  if (!target_uid.get()) {
    OLA_WARN << "Invalid UID: " << FLAGS_uid;
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  } else {
    OLA_INFO << "Started LLRP Responder with UID " << *target_uid;
  }

  dummy_responder.reset(new ola::rdm::DummyResponder(*target_uid));

  ola::io::SelectServer ss;

  if (!m_socket.Init()) {
    return false;
  }
  std::cout << "Init!" << std::endl;

  std::cout << "Using CID " << cid << std::endl;

  if (!m_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                                       ola::acn::LLRP_PORT))) {
    return false;
  }
  std::cout << "Bind!" << std::endl;

  IPV4Address *addr = IPV4Address::FromString("239.255.250.133");

  ola::network::InterfacePicker::Options options;
  options.include_loopback = false;
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
  ola::acn::LLRPProbeRequestInflator llrp_probe_request_inflator;
  llrp_probe_request_inflator.SetLLRPProbeRequestHandler(
      ola::NewCallback(&HandleLLRPProbeRequest));
  ola::acn::RDMInflator llrp_rdm_inflator(ola::acn::VECTOR_LLRP_RDM_CMD);
  llrp_rdm_inflator.SetGenericRDMHandler(
      ola::NewCallback(&HandleRDM));

  // setup all the inflators
  root_inflator.AddInflator(&llrp_inflator);
  llrp_inflator.AddInflator(&llrp_probe_request_inflator);
  llrp_inflator.AddInflator(&llrp_rdm_inflator);

  IncomingUDPTransport m_incoming_udp_transport(&m_socket, &root_inflator);
  m_socket.SetOnData(ola::NewCallback(&m_incoming_udp_transport,
                                      &IncomingUDPTransport::Receive));
  ss.AddReadDescriptor(&m_socket);

  ss.Run();

  return 0;
}
