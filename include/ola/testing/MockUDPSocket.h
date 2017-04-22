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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * MockUDPSocket.h
 * Header file for the Mock UDP Socket class
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_TESTING_MOCKUDPSOCKET_H_
#define INCLUDE_OLA_TESTING_MOCKUDPSOCKET_H_

#include <cppunit/extensions/HelperMacros.h>

#include <ola/base/Macro.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/Socket.h>
#include <ola/network/SocketAddress.h>

#include <string>
#include <queue>

namespace ola {
namespace testing {

/*
 * The MockUDPSocket allows one to stub out a UDP Socket for testing. The
 * code-under-test can use this object as it would a UDP socket, and the code
 * performing the test can verify that the data written matches what it
 * expects. It does this by calling AddExpectedData(), e.g.
 *
 *  // add the data we expect, you can call this more than once
 *  udp_socket.AddExpectedData(....);
 *  udp_socket.AddExpectedData(....);
 *  // this calls one of the SendTo methods
 *  MethodToTest(udp_socket);
 *  // verify all the expected data has been consumed
 *  udp_socket.Verify()
 *
 * You can also inject packets into the socket by calling InjectData(), this
 * will trigger the on_read callback that was attached to the socket.
 */
class MockUDPSocket: public ola::network::UDPSocketInterface {
 public:
  MockUDPSocket();
  ~MockUDPSocket() { Close(); }

  // These are the socket methods
  bool Init();
  bool Bind(const ola::network::IPV4SocketAddress &endpoint);
  bool GetSocketAddress(ola::network::IPV4SocketAddress *address) const;
  bool Close();
  ola::io::DescriptorHandle ReadDescriptor() const { return m_dummy_handle; }
  ola::io::DescriptorHandle WriteDescriptor() const { return m_dummy_handle; }
  ssize_t SendTo(const uint8_t *buffer,
                 unsigned int size,
                 const ola::network::IPV4Address &ip,
                 unsigned short port) const;
  ssize_t SendTo(const uint8_t *buffer,
                 unsigned int size,
                 const ola::network::IPV4SocketAddress &dest) const {
    return SendTo(buffer, size, dest.Host(), dest.Port());
  }
  ssize_t SendTo(ola::io::IOVecInterface *data,
                 const ola::network::IPV4Address &ip,
                 unsigned short port) const;
  ssize_t SendTo(ola::io::IOVecInterface *data,
                 const ola::network::IPV4SocketAddress &dest) const {
    return SendTo(data, dest.Host(), dest.Port());
  }

  bool RecvFrom(uint8_t *buffer, ssize_t *data_read) const;
  bool RecvFrom(
      uint8_t *buffer,
      ssize_t *data_read,
      ola::network::IPV4Address &source) const;  // NOLINT(runtime/references)
  bool RecvFrom(
      uint8_t *buffer,
      ssize_t *data_read,
      ola::network::IPV4Address &source,  // NOLINT(runtime/references)
      uint16_t &port) const;  // NOLINT(runtime/references)
  bool RecvFrom(uint8_t *buffer,
                ssize_t *data_read,
                ola::network::IPV4SocketAddress *source);
  bool EnableBroadcast();
  bool SetMulticastInterface(const ola::network::IPV4Address &iface);
  bool JoinMulticast(const ola::network::IPV4Address &iface,
                     const ola::network::IPV4Address &group,
                     bool multicast_loop = false);
  bool LeaveMulticast(const ola::network::IPV4Address &iface,
                      const ola::network::IPV4Address &group);

  bool SetTos(uint8_t tos);

  void SetDiscardMode(bool discard_mode) { m_discard_mode = discard_mode; }

  // these are methods used for verification
  void AddExpectedData(const uint8_t *data,
                       unsigned int size,
                       const ola::network::IPV4Address &ip,
                       uint16_t port);
  void AddExpectedData(ola::io::IOQueue *queue,
                       const ola::network::IPV4SocketAddress &dest);

  // this can be fetched by calling PerformRead() on the socket
  void InjectData(const uint8_t *data,
                  unsigned int size,
                  const ola::network::IPV4Address &ip,
                  uint16_t port);
  void InjectData(const uint8_t *data,
                  unsigned int size,
                  const ola::network::IPV4SocketAddress &source);
  void InjectData(ola::io::IOQueue *ioqueue,
                  const ola::network::IPV4SocketAddress &source);

  void Verify();

  bool CheckNetworkParamsMatch(bool init_called,
                               bool bound_to_port,
                               uint16_t port,
                               bool broadcast_set);

  void SetInterface(const ola::network::IPV4Address &iface);

 private:
  typedef struct {
    const uint8_t *data;
    unsigned int size;
    ola::network::IPV4Address address;
    uint16_t port;
    bool free_data;
  } expected_call;

  typedef expected_call received_data;

  bool m_init_called;
  // We need a sd so that calls to select continue to work. This isn't used
  // for anything else.
  ola::io::DescriptorHandle m_dummy_handle;
  bool m_bound_to_port;
  bool m_broadcast_set;
  uint16_t m_port;
  uint8_t m_tos;
  mutable std::queue<expected_call> m_expected_calls;
  mutable std::queue<received_data> m_received_data;
  ola::network::IPV4Address m_interface;
  bool m_discard_mode;

  uint8_t* IOQueueToBuffer(ola::io::IOQueue *ioqueue,
                           unsigned int *size) const;

  DISALLOW_COPY_AND_ASSIGN(MockUDPSocket);
};


/**
 * This can be used to break the tests into sections.
 */
class SocketVerifier {
 public:
  explicit SocketVerifier(MockUDPSocket *socket)
      : m_socket(socket) {
  }
  ~SocketVerifier() {
    m_socket->Verify();
  }

 private:
  MockUDPSocket *m_socket;

  DISALLOW_COPY_AND_ASSIGN(SocketVerifier);
};
}  // namespace testing
}  // namespace ola
#endif  // INCLUDE_OLA_TESTING_MOCKUDPSOCKET_H_
