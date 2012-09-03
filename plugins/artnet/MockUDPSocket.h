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
 * MockUDPsocket.h
 * Header file for the Mock UDP Socket class
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_ARTNET_MOCKUDPSOCKET_H_
#define PLUGINS_ARTNET_MOCKUDPSOCKET_H_

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <queue>

#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"
#include "ola/network/SocketAddress.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;

/*
 * A MockUDPSocket
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
    ssize_t SendTo(ola::io::IOQueue *ioqueue,
                   const ola::network::IPV4Address &ip,
                   unsigned short port) const;
    ssize_t SendTo(ola::io::IOQueue *ioqueue,
                   const IPV4SocketAddress &dest) const {
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

    // this can be fetched by calling PerformRead() on the socket
    void ReceiveData(const uint8_t *data,
                     unsigned int size,
                     const IPV4Address &ip,
                     uint16_t port);

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
#endif  // PLUGINS_ARTNET_MOCKUDPSOCKET_H_
