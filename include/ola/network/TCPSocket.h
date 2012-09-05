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
 * Socket.h
 * The Socket interfaces
 * Copyright (C) 2005-2009 Simon Newton
 *
 *  - UDPSocket, allows sending and receiving UDP datagrams
 *  - TCPSocket, this represents a TCP connection to a remote endpoint
 *
 * AcceptingSocket is the interface that defines sockets which can spawn new
 * ConnectedDescriptors. TCPAcceptingSocket is the only subclass and provides
 * the accept() functionality.
 */

#ifndef INCLUDE_OLA_NETWORK_TCPSOCKET_H_
#define INCLUDE_OLA_NETWORK_TCPSOCKET_H_

#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <ola/io/Descriptor.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/SocketAddress.h>


namespace ola {
namespace network {


/*
 * A TCPSocket
 */
class TCPSocket: public ola::io::ConnectedDescriptor {
  public:
    explicit TCPSocket(int sd)
        : m_sd(sd) {
      SetNoSigPipe(sd);
    }

    ~TCPSocket() { Close(); }

    int ReadDescriptor() const { return m_sd; }
    int WriteDescriptor() const { return m_sd; }
    bool Close();

    bool GetPeer(IPV4Address *address, uint16_t *port);

    static TCPSocket* Connect(const SocketAddress &endpoint);

  protected:
    bool IsSocket() const { return true; }

  private:
    int m_sd;

    TCPSocket(const TCPSocket &other);
    TCPSocket& operator=(const TCPSocket &other);
};


/*
 * A TCP accepting socket
 */
class TCPAcceptingSocket: public ola::io::ReadFileDescriptor {
  public:
    explicit TCPAcceptingSocket(class TCPSocketFactoryInterface *factory);
    ~TCPAcceptingSocket();
    bool Listen(const SocketAddress &endpoint, int backlog = 10);
    int ReadDescriptor() const { return m_sd; }
    bool Close();
    void PerformRead();

    void SetFactory(class TCPSocketFactoryInterface *factory) {
      m_factory = factory;
    }

  private:
    int m_sd;
    class TCPSocketFactoryInterface *m_factory;

    TCPAcceptingSocket(const TCPAcceptingSocket &other);
    TCPAcceptingSocket& operator=(const TCPAcceptingSocket &other);
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_TCPSOCKET_H_
