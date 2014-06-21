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
 * TCPSocketFactory.h
 * Factories for creating TCP Sockets
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_TCPSOCKETFACTORY_H_
#define INCLUDE_OLA_NETWORK_TCPSOCKETFACTORY_H_

#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/network/TCPSocket.h>

namespace ola {
namespace network {

/**
 * The Factory Interface
 */
class TCPSocketFactoryInterface {
 public:
  virtual ~TCPSocketFactoryInterface() {}

  virtual void NewTCPSocket(int fd) = 0;
};


/**
 * A factory that creates TCPSockets.
 */
template<class SocketType>
class GenericTCPSocketFactory: public TCPSocketFactoryInterface {
 public:
  typedef ola::Callback1<void, SocketType*> NewSocketCallback;

  explicit GenericTCPSocketFactory(NewSocketCallback *on_accept)
      : m_new_socket(on_accept) {
  }

  ~GenericTCPSocketFactory() {
    if (m_new_socket)
      delete m_new_socket;
  }

  void NewTCPSocket(int fd) {
    SocketType *socket = new SocketType(fd);
    socket->SetReadNonBlocking();
    m_new_socket->Run(socket);
  }

 private:
  NewSocketCallback *m_new_socket;

  DISALLOW_COPY_AND_ASSIGN(GenericTCPSocketFactory);
};

typedef GenericTCPSocketFactory<TCPSocket> TCPSocketFactory;
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_TCPSOCKETFACTORY_H_
