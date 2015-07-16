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
 * TCPSocket.h
 * The TCP Socket interfaces
 * Copyright (C) 2005 Simon Newton
 *
 * TCPSocket, this represents a TCP connection to a remote endpoint
 *
 * AcceptingSocket is the interface that defines sockets which can spawn new
 * ConnectedDescriptors. TCPAcceptingSocket is the only subclass and provides
 * the accept() functionality.
 */

#ifndef INCLUDE_OLA_NETWORK_TCPSOCKET_H_
#define INCLUDE_OLA_NETWORK_TCPSOCKET_H_

#include <stdint.h>

#include <ola/base/Macro.h>
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
  explicit TCPSocket(int sd);

  ~TCPSocket() { Close(); }

  ola::io::DescriptorHandle ReadDescriptor() const { return m_handle; }
  ola::io::DescriptorHandle WriteDescriptor() const { return m_handle; }
  bool Close();

  GenericSocketAddress GetLocalAddress() const;
  GenericSocketAddress GetPeerAddress() const;

  static TCPSocket* Connect(const SocketAddress &endpoint);

  bool SetNoDelay();

 protected:
  bool IsSocket() const { return true; }

 private:
  ola::io::DescriptorHandle m_handle;

  DISALLOW_COPY_AND_ASSIGN(TCPSocket);
};


/*
 * A TCP accepting socket
 */
class TCPAcceptingSocket: public ola::io::ReadFileDescriptor {
 public:
  explicit TCPAcceptingSocket(class TCPSocketFactoryInterface *factory);
  ~TCPAcceptingSocket();
  bool Listen(const SocketAddress &endpoint, int backlog = 10);
  ola::io::DescriptorHandle ReadDescriptor() const { return m_handle; }
  bool Close();
  void PerformRead();

  void SetFactory(class TCPSocketFactoryInterface *factory) {
    m_factory = factory;
  }

  GenericSocketAddress GetLocalAddress() const;

 private:
  ola::io::DescriptorHandle m_handle;
  class TCPSocketFactoryInterface *m_factory;

  DISALLOW_COPY_AND_ASSIGN(TCPAcceptingSocket);
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_TCPSOCKET_H_
