/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * MockUDPSocket.cpp
 * This implements the UDPSocketInterface in a way that we can use it for
 * testing.
 * Copyright (C) 2010 Simon Newton
 */


#include <cppunit/extensions/HelperMacros.h>
#include <errno.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <queue>
#include <string>

#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/MockUDPSocket.h"
#include "ola/testing/TestUtils.h"

namespace ola {
namespace testing {

using ola::io::IOQueue;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;

MockUDPSocket::MockUDPSocket()
    : ola::network::UDPSocketInterface(),
      m_init_called(false),
      m_dummy_sd(ola::io::INVALID_DESCRIPTOR),
      m_bound_to_port(false),
      m_broadcast_set(false),
      m_port(0),
      m_discard_mode(false) {
}


bool MockUDPSocket::Init() {
  if (m_dummy_sd == ola::io::INVALID_DESCRIPTOR) {
    m_dummy_sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (m_dummy_sd < 0) {
      OLA_WARN << "Could not create socket " << strerror(errno);
      return false;
    }
  }
  m_init_called = true;
  return true;
}


bool MockUDPSocket::Bind(const ola::network::IPV4SocketAddress &endpoint) {
  m_bound_to_port = true;
  m_port = endpoint.Port();
  return true;
}


bool MockUDPSocket::GetSocketAddress(IPV4SocketAddress *address) const {
  // Return a dummy address
  *address = IPV4SocketAddress(IPV4Address::Loopback(), 0);
  return true;
}

bool MockUDPSocket::Close() {
  m_bound_to_port = false;
  if (m_dummy_sd != ola::io::INVALID_DESCRIPTOR) {
    close(m_dummy_sd);
  }
  return true;
}

ssize_t MockUDPSocket::SendTo(const uint8_t *buffer,
                              unsigned int size,
                              const ola::network::IPV4Address &ip_address,
                              unsigned short port) const {
  if (m_discard_mode)
    return size;

  OLA_ASSERT_FALSE(m_expected_calls.empty());
  expected_call call = m_expected_calls.front();

  ola::testing::ASSERT_DATA_EQUALS(__LINE__, call.data, call.size, buffer,
                                   size);
  OLA_ASSERT_EQ(call.address, ip_address);
  OLA_ASSERT_EQ(call.port, port);
  if (call.free_data)
    delete[] call.data;

  m_expected_calls.pop();
  return size;
}


ssize_t MockUDPSocket::SendTo(IOVecInterface *data,
                              const ola::network::IPV4Address &ip_address,
                              unsigned short port) const {
  // This incurs a copy but it's only testing code.

  int io_len;
  const struct iovec *iov = data->AsIOVec(&io_len);
  if (iov == NULL)
    return 0;

  unsigned int data_size = 0;
  for (int i = 0; i < io_len; i++) {
    data_size += iov[i].iov_len;
  }

  uint8_t *raw_data = new uint8_t[data_size];
  unsigned int offset = 0;
  for (int i = 0; i < io_len; i++) {
    memcpy(raw_data + offset, iov[i].iov_base, iov[i].iov_len);
    offset += iov[i].iov_len;
  }

  data->Pop(data_size);
  data->FreeIOVec(iov);
  ssize_t data_sent = SendTo(raw_data, data_size, ip_address, port);
  delete[] raw_data;
  return data_sent;
}

bool MockUDPSocket::RecvFrom(uint8_t *buffer, ssize_t *data_read) const {
  IPV4Address address;
  uint16_t port;
  return RecvFrom(buffer, data_read, address, port);
}


bool MockUDPSocket::RecvFrom(uint8_t *buffer,
                             ssize_t *data_read,
                             ola::network::IPV4Address &source) const {
  uint16_t port;
  return RecvFrom(buffer, data_read, source, port);
}


bool MockUDPSocket::RecvFrom(uint8_t *buffer,
                             ssize_t *data_read,
                             ola::network::IPV4Address &source,
                             uint16_t &port) const {
  OLA_ASSERT_FALSE(m_received_data.empty());
  const received_data &new_data = m_received_data.front();

  OLA_ASSERT_TRUE(static_cast<unsigned int>(*data_read) >= new_data.size);
  unsigned int size = std::min(new_data.size,
                               static_cast<unsigned int>(*data_read));
  memcpy(buffer, new_data.data, size);
  *data_read = new_data.size;
  source = new_data.address;
  port = new_data.port;

  if (new_data.free_data)
    delete[] new_data.data;
  m_received_data.pop();
  return true;
}


bool MockUDPSocket::EnableBroadcast() {
  m_broadcast_set = true;
  return true;
}


bool MockUDPSocket::SetMulticastInterface(const IPV4Address &interface) {
  OLA_ASSERT_EQ(m_interface, interface);
  return true;
}


bool MockUDPSocket::JoinMulticast(const IPV4Address &interface,
                                  const IPV4Address &group,
                                  bool loop) {
  OLA_ASSERT_EQ(m_interface, interface);
  (void) group;
  (void) loop;
  return true;
}


bool MockUDPSocket::LeaveMulticast(const IPV4Address &interface,
                                   const IPV4Address &group) {
  OLA_ASSERT_EQ(m_interface, interface);
  (void) group;
  return true;
}


bool MockUDPSocket::SetTos(uint8_t tos) {
  m_tos = tos;
  return true;
}


void MockUDPSocket::AddExpectedData(const uint8_t *data,
                                    unsigned int size,
                                    const IPV4Address &ip,
                                    uint16_t port) {
  expected_call call = {data, size, ip, port, false};
  m_expected_calls.push(call);
}


void MockUDPSocket::AddExpectedData(IOQueue *ioqueue,
                                    const IPV4SocketAddress &dest) {
  unsigned int size;
  uint8_t *data = IOQueueToBuffer(ioqueue, &size);
  expected_call call = {data, size, dest.Host(), dest.Port(), true};
  m_expected_calls.push(call);
}


/**
 * Ownership of the data is not transferred.
 */
void MockUDPSocket::InjectData(const uint8_t *data,
                               unsigned int size,
                               const IPV4Address &ip,
                               uint16_t port) {
  expected_call call = {data, size, ip, port, false};
  m_received_data.push(call);
  PerformRead();
}


/**
 * Ownership of the data is not transferred.
 */
void MockUDPSocket::InjectData(const uint8_t *data,
                               unsigned int size,
                               const IPV4SocketAddress &source) {
  InjectData(data, size, source.Host(), source.Port());
}


/**
 * Inject the data in an IOQueue into the socket. This acts as if the data was
 * received on the UDP socket.
 * @param ioqueue the data to inject
 * @param source the socket address where this fake data came from
 */
void MockUDPSocket::InjectData(IOQueue *ioqueue,
                               const IPV4SocketAddress &source) {
  unsigned int data_size;
  // This incurs a copy, but this is just testing code so it doesn't matter.
  uint8_t *data = IOQueueToBuffer(ioqueue, &data_size);
  expected_call call = {data, data_size, source.Host(), source.Port(), true};
  m_received_data.push(call);
  PerformRead();
}

void MockUDPSocket::Verify() {
  // if an exception is outstanding, don't both to check if we have consumed
  // all calls. This avoids raising a second exception which calls terminate.
  if (!std::uncaught_exception()) {
    std::ostringstream msg;
    msg << m_expected_calls.size() << " packets remain on the MockUDPSocket";
    OLA_ASSERT_TRUE_MSG(m_expected_calls.empty(), msg.str());

  }
}


bool MockUDPSocket::CheckNetworkParamsMatch(bool init_called,
                                            bool bound_to_port,
                                            uint16_t port,
                                            bool broadcast_set) {
  return (init_called == m_init_called &&
          bound_to_port == m_bound_to_port &&
          port == m_port &&
          broadcast_set == m_broadcast_set);
}


void MockUDPSocket::SetInterface(const IPV4Address &interface) {
  m_interface = interface;
}


uint8_t* MockUDPSocket::IOQueueToBuffer(IOQueue *ioqueue,
                                        unsigned int *size) const {
  *size = ioqueue->Size();
  uint8_t *data = new uint8_t[*size];
  *size = ioqueue->Read(data, *size);
  return data;
}
}  // testing
}  // ola
