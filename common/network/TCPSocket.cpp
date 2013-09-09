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
 * Socket.cpp
 * Implementation of the Socket classes
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#include <winioctl.h>
#else
#include <sys/ioctl.h>
#endif

#include <string>

#include "common/network/SocketHelper.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/Socket.h"
#include "ola/network/TCPSocketFactory.h"

namespace ola {
namespace network {


// TCPSocket
// ------------------------------------------------


/**
 * Get the remote IPAddress and port for this socket
 */
GenericSocketAddress TCPSocket::GetPeerAddress() const {
  return ola::network::GetPeerAddress(m_sd);
}

GenericSocketAddress TCPSocket::GetLocalAddress() const {
  return ola::network::GetLocalAddress(m_sd);
}

/*
 * Close this TCPSocket
 */
bool TCPSocket::Close() {
  if (m_sd != ola::io::INVALID_DESCRIPTOR) {
    close(m_sd);
    m_sd = ola::io::INVALID_DESCRIPTOR;
  }
  return true;
}

/*
 * Set the TCP_NODELAY option
 */
bool TCPSocket::SetNoDelay() {
  int flag = 1;
  int result = setsockopt(m_sd, IPPROTO_TCP, TCP_NODELAY,
                          reinterpret_cast<char*>(&flag),
                          sizeof(flag));
  if (result < 0) {
    OLA_WARN << "Can't set TCP_NODELAY for " << m_sd << ", "
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

  int r = connect(sd, &server_address, sizeof(server_address));

  if (r) {
    OLA_WARN << "connect to " << endpoint << " failed, " << strerror(errno);
    return NULL;
  }
  TCPSocket *socket = new TCPSocket(sd);
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
      m_sd(ola::io::INVALID_DESCRIPTOR),
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

  if (m_sd != ola::io::INVALID_DESCRIPTOR)
    return false;

  if (!endpoint.ToSockAddr(&server_address, sizeof(server_address)))
    return false;

  int sd = socket(endpoint.Family(), SOCK_STREAM, 0);
  if (sd < 0) {
    OLA_WARN << "socket() failed: " << strerror(errno);
    return false;
  }

  int ok = setsockopt(sd,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      reinterpret_cast<char*>(&reuse_flag),
                      sizeof(reuse_flag));
  if (ok < 0) {
    OLA_WARN << "can't set reuse for " << sd << ", " << strerror(errno);
    close(sd);
    return false;
  }

  if (bind(sd, &server_address, sizeof(server_address)) == -1) {
    OLA_WARN << "bind to " << endpoint << " failed, " << strerror(errno);
    close(sd);
    return false;
  }

  if (listen(sd, backlog)) {
    OLA_WARN << "listen on " << endpoint << " failed, " << strerror(errno);
    return false;
  }
  m_sd = sd;
  return true;
}


/*
 * Stop listening & close this socket
 * @return true if close succeeded, false otherwise
 */
bool TCPAcceptingSocket::Close() {
  bool ret = true;
  if (m_sd != ola::io::INVALID_DESCRIPTOR)
    if (close(m_sd)) {
      OLA_WARN << "close() failed " << strerror(errno);
      ret = false;
    }
  m_sd = ola::io::INVALID_DESCRIPTOR;
  return ret;
}


/*
 * Accept new connections
 * @return a new connected socket
 */
void TCPAcceptingSocket::PerformRead() {
  struct sockaddr_in cli_address;
  socklen_t length = sizeof(cli_address);

  if (m_sd == ola::io::INVALID_DESCRIPTOR)
    return;

  int sd = accept(m_sd, (struct sockaddr*) &cli_address, &length);
  if (sd < 0) {
    OLA_WARN << "accept() failed, " << strerror(errno);
    return;
  }

  if (m_factory) {
    m_factory->NewTCPSocket(sd);
  } else {
    OLA_WARN << "Accepted new TCP Connection but no factory registered";
    close(sd);
  }
}

/**
 * Get the local IPAddress and port for this socket
 */
GenericSocketAddress TCPAcceptingSocket::GetLocalAddress() const {
  return ola::network::GetLocalAddress(m_sd);
}
}  // namespace network
}  // namespace ola
