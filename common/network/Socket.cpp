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
 * Socket.cpp
 * Implementation of the Socket classes
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
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

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/Socket.h"
#include "ola/network/TCPSocketFactory.h"

namespace ola {
namespace network {


// TcpSocket
// ------------------------------------------------


/**
 * Get the remote IPAddress and port for this socket
 */
bool TcpSocket::GetPeer(IPV4Address *address, uint16_t *port) {
  struct sockaddr_in remote_address;
  socklen_t length = sizeof(remote_address);
  int r = getpeername(m_sd,
                      (struct sockaddr*) &remote_address,
                      &length);
  if (r) {
    OLA_WARN << "Failed to get peer information for fd: " << m_sd << ", " <<
      strerror(errno);
    return false;
  }

  *address = IPV4Address(remote_address.sin_addr);
  *port = remote_address.sin_port;
  return true;
}


/*
 * Close this TcpSocket
 */
bool TcpSocket::Close() {
  if (m_sd != ola::io::INVALID_DESCRIPTOR) {
    close(m_sd);
    m_sd = ola::io::INVALID_DESCRIPTOR;
  }
  return true;
}


/*
 * Connect
 * @param ip_address the IP to connect to
 * @param port the port to connect to
 * @param blocking whether to block on connect or not
 */
TcpSocket* TcpSocket::Connect(const IPV4Address &ip_address,
                              unsigned short port) {
  struct sockaddr_in server_address;
  socklen_t length = sizeof(server_address);

  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    OLA_WARN << "socket() failed, " << strerror(errno);
    return NULL;
  }

  // setup
  memset(&server_address, 0x00, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = HostToNetwork(port);
  server_address.sin_addr.s_addr = ip_address.AsInt();

  int r = connect(sd, (struct sockaddr*) &server_address, length);

  if (r) {
    OLA_WARN << "connect to " << ip_address << ":" << port << " failed, "
      << strerror(errno);
    return NULL;
  }
  TcpSocket *socket = new TcpSocket(sd);
  socket->SetReadNonBlocking();
  return socket;
}

/*
 * Connect
 * @param ip_address the IP to connect to
 * @param port the port to connect to
 * @param blocking whether to block on connect or not
 */
TcpSocket* TcpSocket::Connect(const std::string &ip_address,
                              unsigned short port) {
  IPV4Address address;
  if (!IPV4Address::FromString(ip_address, &address))
    return NULL;

  return TcpSocket::Connect(address, port);
}


// UdpSocket
// ------------------------------------------------

/*
 * Start listening
 * @return true if it succeeded, false otherwise
 */
bool UdpSocket::Init() {
  if (m_fd != ola::io::INVALID_DESCRIPTOR)
    return false;

  int sd = socket(PF_INET, SOCK_DGRAM, 0);

  if (sd < 0) {
    OLA_WARN << "Could not create socket " << strerror(errno);
    return false;
  }

  m_fd = sd;
  return true;
}


/*
 * Bind this socket to an external address/port
 */
bool UdpSocket::Bind(const IPV4Address &ip,
                     unsigned short port) {
  if (m_fd == ola::io::INVALID_DESCRIPTOR)
    return false;

  #if HAVE_DECL_SO_REUSEADDR
  int reuse_addr_flag = 1;
  int addr_ok = setsockopt(m_fd,
                           SOL_SOCKET,
                           SO_REUSEADDR,
                           reinterpret_cast<char*>(&reuse_addr_flag),
                           sizeof(reuse_addr_flag));
  if (addr_ok < 0) {
    OLA_WARN << "can't set SO_REUSEADDR for " << m_fd << ", " <<
      strerror(errno);
    return false;
  }
  #endif

  #if HAVE_DECL_SO_REUSEPORT
  // turn on REUSEPORT if we can
  int reuse_port_flag = 1;
  int ok = setsockopt(m_fd,
                      SOL_SOCKET,
                      SO_REUSEPORT,
                      reinterpret_cast<char*>(&reuse_port_flag),
                      sizeof(reuse_port_flag));
  if (ok < 0) {
    OLA_WARN << "can't set SO_REUSEPORT for " << m_fd << ", " <<
      strerror(errno);
    return false;
  }
  #endif

  struct sockaddr_in servAddr;
  memset(&servAddr, 0x00, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = HostToNetwork(port);
  servAddr.sin_addr.s_addr = ip.AsInt();

  OLA_DEBUG << "Binding to " << AddressToString(servAddr.sin_addr) << ":" <<
    port;

  if (bind(m_fd, (struct sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
    OLA_INFO << "Failed to bind socket " << strerror(errno);
    return false;
  }
  m_bound_to_port = true;
  return true;
}


/*
 * Bind this socket to an address/port using the any address
 */
bool UdpSocket::Bind(unsigned short port) {
  return Bind(IPV4Address::WildCard(), port);
}


/*
 * Close this socket
 */
bool UdpSocket::Close() {
  if (m_fd == ola::io::INVALID_DESCRIPTOR)
    return false;

  int fd = m_fd;
  m_fd = ola::io::INVALID_DESCRIPTOR;
  m_bound_to_port = false;
#ifdef WIN32
  if (closesocket(fd)) {
      WSACleanup();
#else
  if (close(fd)) {
#endif
    OLA_WARN << "close() failed, " << strerror(errno);
    return false;
  }
  return true;
}


/*
 * Send data
 * @param buffer the data to send
 * @param size the length of the data
 * @param ip_address the IP to send to
 * @param port the port to send to in HOST byte order.
 * @return the number of bytes sent
 */
ssize_t UdpSocket::SendTo(const uint8_t *buffer,
                          unsigned int size,
                          const IPV4Address &ip,
                          unsigned short port) const {
  struct sockaddr_in destination;
  memset(&destination, 0, sizeof(destination));
  destination.sin_family = AF_INET;
  destination.sin_port = HostToNetwork(port);
  destination.sin_addr = ip.Address();
  ssize_t bytes_sent = sendto(
    m_fd,
    reinterpret_cast<const char*>(buffer),
    size,
    0,
    reinterpret_cast<const struct sockaddr*>(&destination),
    sizeof(struct sockaddr));
  if (bytes_sent < 0 || static_cast<unsigned int>(bytes_sent) != size)
    OLA_INFO << "Failed to send, " << strerror(errno);
  return bytes_sent;
}


/*
 * Receive data
 * @param buffer the buffer to store the data
 * @param data_read the size of the buffer, updated with the number of bytes
 * read
 * @return true or false
 */
bool UdpSocket::RecvFrom(uint8_t *buffer, ssize_t *data_read) const {
  socklen_t length = 0;
  return _RecvFrom(buffer, data_read, NULL, &length);
}


/*
 * Receive data
 * @param buffer the buffer to store the data
 * @param data_read the size of the buffer, updated with the number of bytes
 * read
 * @param source the src ip of the packet
 * @return true or false
 */
bool UdpSocket::RecvFrom(uint8_t *buffer,
                         ssize_t *data_read,
                         IPV4Address &source) const {
  struct sockaddr_in src_sockaddr;
  socklen_t src_size = sizeof(src_sockaddr);
  bool ok = _RecvFrom(buffer, data_read, &src_sockaddr, &src_size);
  if (ok)
    source = IPV4Address(src_sockaddr.sin_addr);
  return ok;
}


/*
 * Receive data and record the src address & port
 * @param buffer the buffer to store the data
 * @param data_read the size of the buffer, updated with the number of bytes
 * read
 * @param source the src ip of the packet
 * @param port the src port of the packet in host byte order
 * @return true or false
 */
bool UdpSocket::RecvFrom(uint8_t *buffer,
                         ssize_t *data_read,
                         IPV4Address &source,
                         uint16_t &port) const {
  struct sockaddr_in src_sockaddr;
  socklen_t src_size = sizeof(src_sockaddr);
  bool ok = _RecvFrom(buffer, data_read, &src_sockaddr, &src_size);
  if (ok) {
    source = IPV4Address(src_sockaddr.sin_addr);
    port = NetworkToHost(src_sockaddr.sin_port);
  }
  return ok;
}


/*
 * Enable broadcasting for this socket.
 * @return true if it worked, false otherwise
 */
bool UdpSocket::EnableBroadcast() {
  if (m_fd == ola::io::INVALID_DESCRIPTOR)
    return false;

  int broadcast_flag = 1;
  int ok = setsockopt(m_fd,
                      SOL_SOCKET,
                      SO_BROADCAST,
                      reinterpret_cast<char*>(&broadcast_flag),
                      sizeof(broadcast_flag));
  if (ok == -1) {
    OLA_WARN << "Failed to enable broadcasting: " << strerror(errno);
    return false;
  }
  return true;
}


/**
 * Set the outgoing interface to be used for multicast transmission
 */
bool UdpSocket::SetMulticastInterface(const IPV4Address &iface) {
  struct in_addr addr = iface.Address();
  int ok = setsockopt(m_fd,
                      IPPROTO_IP,
                      IP_MULTICAST_IF,
                      reinterpret_cast<const char*>(&addr),
                      sizeof(addr));
  if (ok < 0) {
    OLA_WARN << "Failed to set outgoing multicast interface to " <<
      iface << ": " << strerror(errno);
    return false;
  }
  return true;
}


/*
 * Join a multicast group
 * @param group the address of the group to join
 * @return true if it worked, false otherwise
 */
bool UdpSocket::JoinMulticast(const IPV4Address &iface,
                              const IPV4Address &group,
                              bool multicast_loop) {
  char loop = multicast_loop;
  struct ip_mreq mreq;
  mreq.imr_interface = iface.Address();
  mreq.imr_multiaddr = group.Address();

  int ok = setsockopt(m_fd,
                      IPPROTO_IP,
                      IP_ADD_MEMBERSHIP,
                      reinterpret_cast<char*>(&mreq),
                      sizeof(mreq));
  if (ok < 0) {
    OLA_WARN << "Failed to join multicast group " << group <<
    ": " << strerror(errno);
    return false;
  }

  if (!multicast_loop) {
    ok = setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    if (ok < 0) {
      OLA_WARN << "Failed to disable looping for " << m_fd << ":" <<
        strerror(errno);
      return false;
    }
  }
  return true;
}


/*
 * Leave a multicast group
 * @param group the address of the group to join
 * @return true if it worked, false otherwise
 */
bool UdpSocket::LeaveMulticast(const IPV4Address &iface,
                               const IPV4Address &group) {
  struct ip_mreq mreq;
  mreq.imr_interface = iface.Address();
  mreq.imr_multiaddr = group.Address();

  int ok = setsockopt(m_fd,
                      IPPROTO_IP,
                      IP_DROP_MEMBERSHIP,
                      reinterpret_cast<char*>(&mreq),
                      sizeof(mreq));
  if (ok < 0) {
    OLA_WARN << "Failed to leave multicast group " << group <<
    ": " << strerror(errno);
    return false;
  }
  return true;
}


bool UdpSocket::_RecvFrom(uint8_t *buffer,
                          ssize_t *data_read,
                          struct sockaddr_in *source,
                          socklen_t *src_size) const {
  *data_read = recvfrom(
    m_fd,
    reinterpret_cast<char*>(buffer),
    *data_read,
    0,
    reinterpret_cast<struct sockaddr*>(source),
    src_size);
  if (*data_read < 0) {
    OLA_WARN << "recvfrom failed: " << strerror(errno);
    return false;
  }
  return true;
}


/*
 * Set the tos field for a socket
 * @param tos the tos field
 */
bool UdpSocket::SetTos(uint8_t tos) {
  unsigned int value = tos & 0xFC;  // zero the ECN fields
  int ok = setsockopt(m_fd,
                      IPPROTO_IP,
                      IP_TOS,
                      reinterpret_cast<char*>(&value),
                      sizeof(value));
  if (ok < 0) {
    OLA_WARN << "Failed to set tos for " << m_fd << ", " << strerror(errno);
    return false;
  }
  return true;
}


// TcpAcceptingSocket
// ------------------------------------------------

/*
 * Create a new TcpListeningSocket
 */
TcpAcceptingSocket::TcpAcceptingSocket(TCPSocketFactoryInterface *factory)
    : ReadFileDescriptor(),
      m_sd(ola::io::INVALID_DESCRIPTOR),
      m_factory(factory) {
}


/**
 * Clean up
 */
TcpAcceptingSocket::~TcpAcceptingSocket() {
  Close();
}


/*
 * Start listening
 * @param address the address to listen on
 * @param port the port to listen on
 * @param backlog the backlog
 * @return true if it succeeded, false otherwise
 */
bool TcpAcceptingSocket::Listen(const std::string &address,
                                unsigned short port,
                                int backlog) {
  IPV4Address ip_address;
  if (!IPV4Address::FromString(address, &ip_address))
    return false;

  return Listen(ip_address, port, backlog);
}


/*
 * Start listening
 * @param address the address to listen on
 * @param port the port to listen on
 * @param backlog the backlog
 * @return true if it succeeded, false otherwise
 */
bool TcpAcceptingSocket::Listen(const IPV4Address &address,
                                unsigned short port,
                                int backlog) {
  struct sockaddr_in server_address;
  int reuse_flag = 1;

  if (m_sd != ola::io::INVALID_DESCRIPTOR)
    return false;

  // setup
  memset(&server_address, 0x00, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = HostToNetwork(port);

  int sd = socket(AF_INET, SOCK_STREAM, 0);
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

  if (bind(sd, (struct sockaddr *) &server_address,
           sizeof(server_address)) == -1) {
    OLA_WARN << "bind to " << address << ":" << port << " failed, "
      << strerror(errno);
    close(sd);
    return false;
  }

  if (listen(sd, backlog)) {
    OLA_WARN << "listen on " << address << ":" << port << " failed, "
      << strerror(errno);
    return false;
  }
  m_sd = sd;
  return true;
}


/*
 * Stop listening & close this socket
 * @return true if close succeeded, false otherwise
 */
bool TcpAcceptingSocket::Close() {
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
void TcpAcceptingSocket::PerformRead() {
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
}  // network
}  // ola
