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
 * MockUDPsocket.h
 * Header file for the Mock UDP Socket class
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_TESTING_MOCKUDPSOCKET_H_
#define INCLUDE_OLA_TESTING_MOCKUDPSOCKET_H_

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <queue>

#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"
#include "ola/network/SocketAddress.h"

namespace ola {
namespace testing {

using ola::io::IOQueue;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;

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
    MockUDPSocket(): ola::network::UDPSocketInterface(),
                     m_init_called(false),
                     m_bound_to_port(false),
                     m_broadcast_set(false),
                     m_port(0),
                     m_discard_mode(false) {}
    ~MockUDPSocket() { Close(); }

    // These are the socket methods
    bool Init();
    bool Bind(const ola::network::IPV4SocketAddress &endpoint);
    bool GetSocketAddress(IPV4SocketAddress *address) const;
    bool Close();
    int ReadDescriptor() const;
    int WriteDescriptor() const;
    ssize_t SendTo(const uint8_t *buffer,
                   unsigned int size,
                   const ola::network::IPV4Address &ip,
                   unsigned short port) const;
    ssize_t SendTo(const uint8_t *buffer,
                   unsigned int size,
                   const IPV4SocketAddress &dest) const {
      return SendTo(buffer, size, dest.Host(), dest.Port());
    }
    ssize_t SendTo(IOQueue *ioqueue,
                   const ola::network::IPV4Address &ip,
                   unsigned short port) const;
    ssize_t SendTo(IOQueue *ioqueue, const IPV4SocketAddress &dest) const {
      return SendTo(ioqueue, dest.Host(), dest.Port());
    }

    bool RecvFrom(uint8_t *buffer, ssize_t *data_read) const;
    bool RecvFrom(uint8_t *buffer,
                  ssize_t *data_read,
                  ola::network::IPV4Address &source) const;
    bool RecvFrom(uint8_t *buffer,
                  ssize_t *data_read,
                  ola::network::IPV4Address &source,
                  uint16_t &port) const;
    bool EnableBroadcast();
    bool SetMulticastInterface(const IPV4Address &interface);
    bool JoinMulticast(const IPV4Address &interface,
                       const IPV4Address &group,
                       bool loop = false);
    bool LeaveMulticast(const IPV4Address &interface,
                        const IPV4Address &group);

    bool SetTos(uint8_t tos);

    void SetDiscardMode(bool discard_mode) { m_discard_mode = discard_mode; }

    // these are methods used for verification
    void AddExpectedData(const uint8_t *data,
                         unsigned int size,
                         const IPV4Address &ip,
                         uint16_t port);
    void AddExpectedData(IOQueue *queue, const IPV4SocketAddress &dest);

    // this can be fetched by calling PerformRead() on the socket
    void InjectData(const uint8_t *data,
                    unsigned int size,
                    const IPV4Address &ip,
                    uint16_t port);
    void InjectData(const uint8_t *data,
                    unsigned int size,
                    const IPV4SocketAddress &source);
    void InjectData(IOQueue *ioqueue, const IPV4SocketAddress &source);

    void Verify();

    bool CheckNetworkParamsMatch(bool init_called,
                                 bool bound_to_port,
                                 uint16_t port,
                                 bool broadcast_set);

    void SetInterface(const IPV4Address &interface);

  private:
    typedef struct {
      const uint8_t *data;
      unsigned int size;
      IPV4Address address;
      uint16_t port;
      bool free_data;
    } expected_call;

    typedef expected_call received_data;

    bool m_init_called;
    bool m_bound_to_port;
    bool m_broadcast_set;
    uint16_t m_port;
    uint8_t m_tos;
    mutable std::queue<expected_call> m_expected_calls;
    mutable std::queue<received_data> m_received_data;
    IPV4Address m_interface;
    bool m_discard_mode;

    uint8_t* IOQueueToBuffer(IOQueue *ioqueue, unsigned int *size) const;
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
};
}  // testing
}  // ola
#endif  // INCLUDE_OLA_TESTING_MOCKUDPSOCKET_H_
