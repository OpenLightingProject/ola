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
 * MockUdpSocket.cpp
 * This implements the UdpSocketInterface in a way that we can use it for
 * testing.
 * Copyright (C) 2010 Simon Newton
 */


#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <queue>

#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/artnet/MockUdpSocket.h"

using ola::network::HostToNetwork;
using ola::network::IPV4Address;


bool MockUdpSocket::Init() {
  m_init_called = true;
  return true;
}


bool MockUdpSocket::Bind(unsigned short port) {
  m_bound_to_port = true;
  m_port = port;
  return true;
}


bool MockUdpSocket::Close() {
  m_bound_to_port = false;
  return true;
}


int MockUdpSocket::ReadDescriptor() const { return 0; }
int MockUdpSocket::WriteDescriptor() const { return 0; }


ssize_t MockUdpSocket::SendTo(const uint8_t *buffer,
                              unsigned int size,
                              const struct sockaddr_in &destination) const {
  OLA_INFO << "sending packet of size " << size;
  CPPUNIT_ASSERT(m_expected_calls.size());
  expected_call call = m_expected_calls.front();

  /*
  unsigned int min_size = std::min(size, call.size);
  for (unsigned int i = 0; i < min_size; i++)
    OLA_INFO << i << ": " << (int) call.data[i] << ", " << (int) buffer[i] <<
      (call.data[i] != buffer[i] ? " !!!!" : "");
  */

  CPPUNIT_ASSERT_EQUAL(call.size, size);
  CPPUNIT_ASSERT_EQUAL(0, memcmp(call.data, buffer, size));
  CPPUNIT_ASSERT_EQUAL(call.destination.sin_addr.s_addr,
                       destination.sin_addr.s_addr);
  CPPUNIT_ASSERT_EQUAL(call.destination.sin_port, destination.sin_port);
  m_expected_calls.pop();
  return size;
}


ssize_t MockUdpSocket::SendTo(const uint8_t *buffer,
                              unsigned int size,
                              const ola::network::IPV4Address &ip_address,
                              unsigned short port) const {
  struct sockaddr_in destination;
  memset(&destination, 0x00, sizeof(destination));
  destination.sin_family = AF_INET;
  destination.sin_port = HostToNetwork(port);
  destination.sin_addr = ip_address.Address();
  return SendTo(buffer, size, destination);
}


bool MockUdpSocket::RecvFrom(uint8_t *buffer,
                             ssize_t *data_read,
                             struct sockaddr_in &source,
                             socklen_t &src_size) const {
  ssize_t length = std::min(m_available, *data_read);
  memcpy(buffer, m_buffer, length);
  *data_read = length;
  source.sin_addr = m_source.sin_addr;
  source.sin_port = m_source.sin_port;
  (void) src_size;

  buffer += length;
  m_available -= length;
  return true;
}


bool MockUdpSocket::RecvFrom(uint8_t *buffer, ssize_t *data_read) const {
  ssize_t length = std::min(m_available, *data_read);
  memcpy(buffer, m_buffer, length);
  *data_read = length;

  buffer += length;
  m_available -= length;
  return true;
}


bool MockUdpSocket::RecvFrom(uint8_t *buffer,
                             ssize_t *data_read,
                             ola::network::IPV4Address &source) const {
  struct sockaddr_in src_sockaddr;
  socklen_t src_size = sizeof(src_sockaddr);
  bool ok = RecvFrom(buffer, data_read, src_sockaddr, src_size);
  if (ok)
    source = IPV4Address(src_sockaddr.sin_addr);
  return ok;
}


bool MockUdpSocket::EnableBroadcast() {
  m_broadcast_set = true;
  return true;
}


bool MockUdpSocket::SetMulticastInterface(const IPV4Address &interface) {
  CPPUNIT_ASSERT_EQUAL(m_interface, interface);
  return true;
}


bool MockUdpSocket::JoinMulticast(const IPV4Address &interface,
                                  const IPV4Address &group,
                                  bool loop) {
  CPPUNIT_ASSERT_EQUAL(m_interface, interface);
  (void) group;
  (void) loop;
  return true;
}


bool MockUdpSocket::LeaveMulticast(const IPV4Address &interface,
                                   const IPV4Address &group) {
  CPPUNIT_ASSERT_EQUAL(m_interface, interface);
  (void) group;
  return true;
}


bool MockUdpSocket::SetTos(uint8_t tos) {
  m_tos = tos;
  return true;
}


void MockUdpSocket::NewData(uint8_t *buffer,
                            ssize_t *data_read,
                            struct sockaddr_in &source) {
  m_buffer = buffer;
  m_available = *data_read;
  m_source = source;
  OnData()->Run();
}

void MockUdpSocket::AddExpectedData(const uint8_t *data,
                                    unsigned int size,
                                    const struct sockaddr_in &destination) {
  expected_call call = {data, size, destination};
  m_expected_calls.push(call);
}


void MockUdpSocket::Verify() {
  CPPUNIT_ASSERT(m_expected_calls.empty());
}


bool MockUdpSocket::CheckNetworkParamsMatch(bool init_called,
                                            bool bound_to_port,
                                            uint16_t port,
                                            bool broadcast_set) {
  return (init_called == m_init_called &&
          bound_to_port == m_bound_to_port &&
          port == m_port &&
          broadcast_set == m_broadcast_set);
}


void MockUdpSocket::SetInterface(const IPV4Address &interface) {
  m_interface = interface;
}
