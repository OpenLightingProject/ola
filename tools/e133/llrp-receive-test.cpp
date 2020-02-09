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
 * llrp-receive-test.cpp
 * Run a very simple E1.33 LLRP Responder.
 * Copyright (C) 2020 Peter Newman
 */

#include <getopt.h>
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
#include "libs/acn/RootHeader.h"
#include "libs/acn/RootInflator.h"
#include "libs/acn/RootSender.h"
#include "libs/acn/Transport.h"
#include "libs/acn/UDPTransport.h"
#include "ola/Callback.h"
#include "ola/acn/ACNPort.h"
#include "ola/acn/CID.h"
#include "ola/base/Flags.h"
#include "ola/base/Init.h"
#include "ola/base/SysExits.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Interface.h"
#include "ola/network/InterfacePicker.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/MACAddress.h"
#include "ola/network/SocketAddress.h"
#include "ola/network/Socket.h"
#include "ola/rdm/UID.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"

using std::string;
using std::vector;
using std::auto_ptr;

using ola::acn::CID;
using ola::acn::IncomingUDPTransport;
using ola::acn::LLRPHeader;
using ola::acn::LLRPProbeReplyPDU;
using ola::acn::OutgoingUDPTransport;
using ola::acn::OutgoingUDPTransportImpl;
using ola::network::Interface;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::MACAddress;
using ola::rdm::UID;

DEFINE_string(uid, "7a70:00000001", "The UID of the responder.");

auto_ptr<ola::network::InterfacePicker> picker(
  ola::network::InterfacePicker::NewPicker());
ola::network::Interface m_interface;
const std::string m_preferred_ip;
ola::network::UDPSocket m_socket;
uint8_t *m_recv_buffer;
std::auto_ptr<UID> target_uid;

ola::acn::PreamblePacker m_packer;
ola::acn::CID cid = CID::Generate();
ola::acn::RootSender m_root_sender(cid, true);

bool CompareInterfaceMACs(Interface a, Interface b) {
  return a.hw_address < b.hw_address;
}

Interface FindLowestMAC() {
  // TODO(Peter): Get some clarification on whether we only care about active
  // interfaces, or any installed ones?
  // TODO(Peter): Work out what to do here if running on localhost only? Return
  // 00:00:00:00:00:00
  std::vector<Interface> interfaces = picker->GetInterfaces(false);
  std::vector<Interface>::iterator result = std::min_element(interfaces.begin(), interfaces.end(), CompareInterfaceMACs);
  return *result;
}

//void CheckData() {
//  std::cout << "Awaiting data..." << std::endl;
//  if (!m_recv_buffer)
//    m_recv_buffer = new uint8_t[512];
//
//  ssize_t size = 512;
//  ola::network::IPV4SocketAddress source;

//  if (!m_socket.RecvFrom(m_recv_buffer, &size, &source))
//    return;

//  std::cout << "Got " << size << " bytes" << std::endl;

//  ola::FormatData(&std::cout, m_recv_buffer, size);

//  ola::acn::HeaderSet header_set;
//  if (root_inflator.InflatePDUBlock(&header_set, &m_recv_buffer[16], size - 16)) {
//    std::cout << "Inflated" << std::endl;
//  } else {
//    std::cout << "Failed to inflate" << std::endl;
//  }
//}

void HandleLLRPProbeRequest(
    const ola::acn::HeaderSet *headers,
    const ola::rdm::UID &lower_uid,
    const ola::rdm::UID &upper_uid) {
  OLA_DEBUG << "Handling probe from " << lower_uid << " to " << upper_uid;

  const ola::acn::RootHeader root_header = headers->GetRootHeader();
  const ola::acn::LLRPHeader llrp_header = headers->GetLLRPHeader();

  OLA_DEBUG << "Source CID: " << root_header.GetCid();
  OLA_DEBUG << "TN: " << llrp_header.TransactionNumber();

  ola::acn::LLRPHeader reply_llrp_header = LLRPHeader(root_header.GetCid(), llrp_header.TransactionNumber());

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
  m_root_sender.SendPDU(ola::acn::VECTOR_ROOT_LLRP, pdu, &transport);
  OLA_DEBUG << "Sent PDU";
}

int main(int argc, char* argv[]) {
  ola::AppInit(&argc, argv, "[options]", "Run a very simple E1.33 LLRP Responder.");

  target_uid.reset(UID::FromString(FLAGS_uid));
  if (!target_uid.get()) {
    OLA_WARN << "Invalid UID: " << FLAGS_uid;
    ola::DisplayUsage();
    exit(ola::EXIT_USAGE);
  } else {
    OLA_INFO << "Started LLRP Responder with UID " << *target_uid;
  }

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

  if (!picker->ChooseInterface(&m_interface, m_preferred_ip)) {
    OLA_INFO << "Failed to find an interface";
    return false;
  }

  std::cout << "IF " << m_interface << std::endl;

  if (!m_socket.JoinMulticast(m_interface.ip_address, *addr)) {
    OLA_WARN << "Failed to join multicast group " << addr;
  }

  ola::acn::RootInflator root_inflator;
  ola::acn::LLRPInflator llrp_inflator;
  ola::acn::LLRPProbeRequestInflator llrp_probe_request_inflator;
  llrp_probe_request_inflator.SetLLRPProbeRequestHandler(
      ola::NewCallback(&HandleLLRPProbeRequest));

  // setup all the inflators
  root_inflator.AddInflator(&llrp_inflator);
  llrp_inflator.AddInflator(&llrp_probe_request_inflator);

  IncomingUDPTransport m_incoming_udp_transport(&m_socket, &root_inflator);
  m_socket.SetOnData(ola::NewCallback(&m_incoming_udp_transport,
                                      &IncomingUDPTransport::Receive));
//  m_socket.SetOnData(ola::NewCallback(&CheckData));
  ss.AddReadDescriptor(&m_socket);

  std::cout << "Pre run!" << std::endl;

  ss.Run();

  std::cout << "Run!" << std::endl;

  return 0;
}
