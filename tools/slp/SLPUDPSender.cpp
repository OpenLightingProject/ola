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
 * SLPUDPSender.h
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/io/BigEndianStream.h>
#include <ola/io/IOQueue.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>
#include <ola/network/NetworkUtils.h>

#include <set>
#include <string>

#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPUDPSender.h"
#include "tools/slp/SLPPacketConstants.h"

using ola::network::HostToNetwork;
using ola::network::IPV4SocketAddress;
using std::set;
using std::string;

namespace ola {
namespace slp {


SLPUDPSender::SLPUDPSender(ola::network::UDPSocket *socket)
    : m_udp_socket(socket),
      m_output_stream(&m_output),
      m_multicast_address(HostToNetwork(SLP_MULTICAST_ADDRESS)) {
}


void SLPUDPSender::SendServiceRequest(const IPV4SocketAddress &dest,
                                      xid_t xid,
                                      const set<IPV4Address> &pr_list,
                                      const string &service_type,
                                      const ScopeSet &scopes) {
  EmptyBuffer();
  SLPPacketBuilder::BuildServiceRequest(
      &m_output_stream, xid, dest.Host() == m_multicast_address, pr_list,
      service_type, scopes);
  OLA_INFO << "Sending SrvRqst for " << service_type;
  Send(dest);
}


void SLPUDPSender::SendServiceRequest(const IPV4SocketAddress &dest,
                                      xid_t xid,
                                      const string &service_type,
                                      const ScopeSet &scopes) {
  set<IPV4Address> pr_list;
  SendServiceRequest(dest, xid, pr_list, service_type, scopes);
}


void SLPUDPSender::SendServiceReply(const IPV4SocketAddress &dest,
                                    xid_t xid,
                                    uint16_t error_code,
                                    const URLEntries &urls) {
  EmptyBuffer();
  SLPPacketBuilder::BuildServiceReply(&m_output_stream, xid, error_code, urls);
  OLA_INFO << "Sending SrvRply with error " << error_code;
  Send(dest);
}


void SLPUDPSender::SendServiceReply(const IPV4SocketAddress &dest,
                                    xid_t xid,
                                    uint16_t error_code) {
  URLEntries urls;
  SendServiceReply(dest, xid, error_code, urls);
}


/**
 * We pass the scopes separately here since they may be a subset of what the
 * service was registered with, see Section 8.3
 */
void SLPUDPSender::SendServiceRegistration(const IPV4SocketAddress &dest,
                                           xid_t xid,
                                           bool fresh,
                                           const ScopeSet &scopes,
                                           const ServiceEntry &service) {
  EmptyBuffer();
  SLPPacketBuilder::BuildServiceRegistration(&m_output_stream, xid, fresh,
                                             scopes, service);
  OLA_INFO << "Sending SrvReg with for " << service.service_type();
  Send(dest);
}


/**
 * Similar to SendServiceRegistration, we pass the scopes separately here
 */
void SLPUDPSender::SendServiceDeRegistration(const IPV4SocketAddress &dest,
                                             xid_t xid,
                                             const ScopeSet &scopes,
                                             const ServiceEntry &service) {
  EmptyBuffer();
  SLPPacketBuilder::BuildServiceDeRegistration(&m_output_stream, xid, scopes,
                                               service);
  OLA_INFO << "Sending SrvDeReg with for " << service.service_type();
  Send(dest);
}


void SLPUDPSender::SendServiceAck(const IPV4SocketAddress &dest,
                                  xid_t xid,
                                  uint16_t error_code) {
  EmptyBuffer();
  SLPPacketBuilder::BuildServiceAck(&m_output_stream, xid, error_code);
  OLA_INFO << "Sending SrvAck to " << dest << ", xid " << xid
           << ", error " << error_code;
  Send(dest);
}


void SLPUDPSender::SendDAAdvert(const IPV4SocketAddress &dest,
                                xid_t xid,
                                uint16_t error_code,
                                uint32_t boot_timestamp,
                                const string &url,
                                const ScopeSet &scopes) {
  EmptyBuffer();
  SLPPacketBuilder::BuildDAAdvert(
      &m_output_stream, xid, dest.Host() == m_multicast_address, error_code,
      boot_timestamp, url, scopes);
  OLA_INFO << "Sending DAAdvert with url " << url;
  Send(dest);
}


void SLPUDPSender::SendSAAdvert(const IPV4SocketAddress &dest,
                                xid_t xid,
                                const string &url,
                                const ScopeSet &scopes) {
  EmptyBuffer();
  SLPPacketBuilder::BuildSAAdvert(
      &m_output_stream, xid, dest.Host() == m_multicast_address, url,
      scopes);
  OLA_INFO << "Sending SAAdvert with url " << url;
  Send(dest);
}


/**
 * Make sure the IOQueue is empty before we start building a new packet.
 */
void SLPUDPSender::EmptyBuffer() {
  if (m_output.Empty())
    return;

  OLA_WARN << "IOQueue not empty, previous packet wasn't sent";
  m_output.Pop(m_output.Size());
}


/**
 * Perform the send.
 */
void SLPUDPSender::Send(const IPV4SocketAddress &target) {
  m_udp_socket->SendTo(&m_output, target);
}
}  // slp
}  // ola
