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
 * TCPSocket.cpp
 * Implementation of the TCP Socket classes
 * Copyright (C) 2005 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <errno.h>
#include <fcntl.h>
/* FreeBSD needs types.h before tcp.h */
#include <sys/types.h>
#ifndef _WIN32
#include <netinet/tcp.h>
#endif  // !_WIN32
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#include <Ws2tcpip.h>
#include <winioctl.h>
#else
#include <sys/ioctl.h>
#endif  // _WIN32

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif  // HAVE_ARPA_INET_H
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>  // Required by FreeBSD
#endif  // HAVE_NETINET_IN_H

#include <string>

#include "common/network/SocketHelper.h"
#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/Socket.h"
#include "ola/network/SocketCloser.h"
#include "ola/network/TCPSocketFactory.h"

namespace ola {
namespace network {


// TCPSocket
// ------------------------------------------------


/**
 * Get the remote IPAddress and port for this socket
 */
GenericSocketAddress TCPSocket::GetPeerAddress() const {
#ifdef _WIN32
  return ola::network::GetPeerAddress(m_handle.m_handle.m_fd);
#else
  return ola::network::GetPeerAddress(m_handle);
#endif  // _WIN32
}

GenericSocketAddress TCPSocket::GetLocalAddress() const {
#ifdef _WIN32
  return ola::network::GetLocalAddress(m_handle.m_handle.m_fd);
#else
  return ola::network::GetLocalAddress(m_handle);
#endif  // _WIN32
}

TCPSocket::TCPSocket(int sd) {
#ifdef _WIN32
  m_handle.m_handle.m_fd = sd;
  m_handle.m_type = ola::io::SOCKET_DESCRIPTOR;
#else
  m_handle = sd;
#endif  // _WIN32
  SetNoSigPipe(m_handle);
}

/*
 * Close this TCPSocket
 */
bool TCPSocket::Close() {
  if (m_handle != ola::io::INVALID_DESCRIPTOR) {
#ifdef _WIN32
    closesocket(m_handle.m_handle.m_fd);
#else
    if (close(m_handle)) {
      OLA_WARN << "close: " << strerror(errno);
    }
#endif  // _WIN32
    m_handle = ola::io::INVALID_DESCRIPTOR;
  }
  return true;
}

/*
 * Set the TCP_NODELAY option
 */
bool TCPSocket::SetNoDelay() {
  int flag = 1;
#ifdef _WIN32
  int sd = m_handle.m_handle.m_fd;
#else
  int sd = m_handle;
#endif  // _WIN32
  int result = setsockopt(sd, IPPROTO_TCP, TCP_NODELAY,
                          reinterpret_cast<char*>(&flag),
                          sizeof(flag));
  if (result < 0) {
    OLA_WARN << "Can't set TCP_NODELAY for " << sd << ", "
             << strerror(errno);
    return false;
  }
  return true;
}


/*
 * Connect
 * @param ip_address the IP to connect to
 * @param port the port to connect to
 * @param blocking whether to block on connect or not
 */
TCPSocket* TCPSocket::Connect(const SocketAddress &endpoint) {
  struct sockaddr server_address;

  if (!endpoint.ToSockAddr(&server_address, sizeof(server_address)))
    return NULL;

  int sd = socket(endpoint.Family(), SOCK_STREAM, 0);
  if (sd < 0) {
    OLA_WARN << "socket() failed, " << strerror(errno);
    return NULL;
  }

  SocketCloser closer(sd);

  int r = connect(sd, &server_address, sizeof(server_address));

  if (r) {
    OLA_WARN << "connect(" << endpoint << "): " << strerror(errno);
    return NULL;
  }
  TCPSocket *socket = new TCPSocket(closer.Release());
  socket->SetReadNonBlocking();
  return socket;
}


// TCPAcceptingSocket
// ------------------------------------------------

/*
 * Create a new TCPListeningSocket
 */
TCPAcceptingSocket::TCPAcceptingSocket(TCPSocketFactoryInterface *factory)
    : ReadFileDescriptor(),
      m_handle(ola::io::INVALID_DESCRIPTOR),
      m_factory(factory) {
}


/**
 * Clean up
 */
TCPAcceptingSocket::~TCPAcceptingSocket() {
  Close();
}


/*
 * Start listening
 * @param endpoint the SocketAddress to listen on
 * @param backlog the backlog
 * @return true if it succeeded, false otherwise
 */
bool TCPAcceptingSocket::Listen(const SocketAddress &endpoint, int backlog) {
  struct sockaddr server_address;
  int reuse_flag = 1;

  if (m_handle != ola::io::INVALID_DESCRIPTOR)
    return false;

  if (!endpoint.ToSockAddr(&server_address, sizeof(server_address)))
    return false;

  int sd = socket(endpoint.Family(), SOCK_STREAM, 0);
  if (sd < 0) {
    OLA_WARN << "socket() failed: " << strerror(errno);
    return false;
  }

  SocketCloser closer(sd);

#ifdef _WIN32
  ola::io::DescriptorHandle temp_handle;
  temp_handle.m_handle.m_fd = sd;
  temp_handle.m_type = ola::io::SOCKET_DESCRIPTOR;
  if (!ola::io::ConnectedDescriptor::SetNonBlocking(temp_handle)) {
#else
  if (!ola::io::ConnectedDescriptor::SetNonBlocking(sd)) {
#endif  // _WIN32
    OLA_WARN << "Failed to mark TCP accept socket as non-blocking";
    return false;
  }

  int ok = setsockopt(sd,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      reinterpret_cast<char*>(&reuse_flag),
                      sizeof(reuse_flag));
  if (ok < 0) {
    OLA_WARN << "can't set reuse for " << sd << ", " << strerror(errno);
    return false;
  }

  if (bind(sd, &server_address, sizeof(server_address)) == -1) {
    OLA_WARN << "bind to " << endpoint << " failed, " << strerror(errno);
    return false;
  }

  if (listen(sd, backlog)) {
    OLA_WARN << "listen on " << endpoint << " failed, " << strerror(errno);
    return false;
  }
#ifdef _WIN32
  m_handle.m_handle.m_fd = closer.Release();
  m_handle.m_type = ola::io::SOCKET_DESCRIPTOR;
#else
  m_handle = closer.Release();
#endif  // _WIN32
  return true;
}


/*
 * Stop listening & close this socket
 * @return true if close succeeded, false otherwise
 */
bool TCPAcceptingSocket::Close() {
  bool ret = true;
  if (m_handle != ola::io::INVALID_DESCRIPTOR) {
#ifdef _WIN32
    if (closesocket(m_handle.m_handle.m_fd)) {
#else
    if (close(m_handle)) {
#endif  // _WIN32
      OLA_WARN << "close() failed " << strerror(errno);
      ret = false;
    }
  }
  m_handle = ola::io::INVALID_DESCRIPTOR;
  return ret;
}


/*
 * Accept new connections
 * @return a new connected socket
 */
void TCPAcceptingSocket::PerformRead() {
  if (m_handle == ola::io::INVALID_DESCRIPTOR)
    return;

  while (1) {
    struct sockaddr_in cli_address;
    socklen_t length = sizeof(cli_address);

#ifdef _WIN32
    int sd = accept(m_handle.m_handle.m_fd, (struct sockaddr*) &cli_address,
                    &length);
#else
    int sd = accept(m_handle, (struct sockaddr*) &cli_address, &length);
#endif  // _WIN32
    if (sd < 0) {
#ifdef _WIN32
      if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
      if (errno == EWOULDBLOCK) {
#endif  // _WIN32
        return;
      }

      OLA_WARN << "accept() failed, " << strerror(errno);
      return;
    }

    if (m_factory) {
      // The callback takes ownership of the new socket descriptor
      // coverity[RESOURCE_LEAK]
      m_factory->NewTCPSocket(sd);
    } else {
      OLA_WARN << "Accepted new TCP Connection but no factory registered";
#ifdef _WIN32
      closesocket(sd);
#else
      close(sd);
#endif  // _WIN32
    }
  }
}

/**
 * Get the local IPAddress and port for this socket
 */
GenericSocketAddress TCPAcceptingSocket::GetLocalAddress() const {
#ifdef _WIN32
  return ola::network::GetLocalAddress(m_handle.m_handle.m_fd);
#else
  return ola::network::GetLocalAddress(m_handle);
#endif  // _WIN32
}
}  // namespace network
}  // namespace ola
