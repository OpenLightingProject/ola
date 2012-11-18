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


// UDPSocket
// ------------------------------------------------

/*
 * Start listening
 * @return true if it succeeded, false otherwise
 */
bool UDPSocket::Init() {
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
 * Bind this socket to an external address:port
 */
bool UDPSocket::Bind(const IPV4SocketAddress &endpoint) {
  if (m_fd == ola::io::INVALID_DESCRIPTOR)
    return false;

  struct sockaddr server_address;
  if (!endpoint.ToSockAddr(&server_address, sizeof(server_address)))
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

  OLA_DEBUG << "Binding to " << endpoint;
  if (bind(m_fd, &server_address, sizeof(server_address)) == -1) {
    OLA_WARN << "Failed to bind " << endpoint << ", " << strerror(errno);
    return false;
  }
  m_bound_to_port = true;
  return true;
}


/**
 * Returns the local address for this socket
 */
bool UDPSocket::GetSocketAddress(IPV4SocketAddress *address) const {
  struct sockaddr_in remote_address;
  socklen_t length = sizeof(remote_address);
  int r = getsockname(m_fd, (struct sockaddr*) &remote_address, &length);
  if (r) {
    OLA_WARN << "Failed to get peer information for fd: " << m_fd << ", " <<
      strerror(errno);
    return false;
  }
  *address = IPV4SocketAddress(IPV4Address(remote_address.sin_addr),
                               NetworkToHost(remote_address.sin_port));
  return true;
}

/*
 * Close this socket
 */
bool UDPSocket::Close() {
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
ssize_t UDPSocket::SendTo(const uint8_t *buffer,
                          unsigned int size,
                          const IPV4Address &ip,
                          unsigned short port) const {
  if (!ValidWriteDescriptor())
    return 0;

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
 * Send data from an IOQueue. This will try to send as much data as possible.
 * If the data exceeds the MTU the UDP packet will probably get fragmented at
 * the IP layer (depends on OS really). Try to avoid this.
 * @param ioqueue the IOQueue to send.
 * @param ip_address the IP to send to
 * @param port the port to send to in HOST byte order.
 * @return the number of bytes sent
 */
ssize_t UDPSocket::SendTo(ola::io::IOQueue *ioqueue,
                          const IPV4Address &ip,
                          unsigned short port) const {
  if (!ValidWriteDescriptor())
    return 0;

  struct sockaddr_in destination;
  memset(&destination, 0, sizeof(destination));
  destination.sin_family = AF_INET;
  destination.sin_port = HostToNetwork(port);
  destination.sin_addr = ip.Address();

  int io_len;
  const struct iovec *iov = ioqueue->AsIOVec(&io_len);

  struct msghdr message;
  message.msg_name = &destination;
  message.msg_namelen = sizeof(destination);
  message.msg_iov = const_cast<struct iovec*>(iov);
  message.msg_iovlen = io_len;
  message.msg_control = NULL;
  message.msg_controllen = 0;
  message.msg_flags = 0;
  ssize_t bytes_sent = sendmsg(WriteDescriptor(), &message, 0);

  if (bytes_sent < 0) {
    OLA_INFO << "Failed to send on " << WriteDescriptor() << ": " <<
      strerror(errno);
  } else {
    ioqueue->FreeIOVec(iov);
    ioqueue->Pop(bytes_sent);
  }
  return bytes_sent;
}


/*
 * Receive data
 * @param buffer the buffer to store the data
 * @param data_read the size of the buffer, updated with the number of bytes
 * read
 * @return true or false
 */
bool UDPSocket::RecvFrom(uint8_t *buffer, ssize_t *data_read) const {
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
bool UDPSocket::RecvFrom(uint8_t *buffer,
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
bool UDPSocket::RecvFrom(uint8_t *buffer,
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
bool UDPSocket::EnableBroadcast() {
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
bool UDPSocket::SetMulticastInterface(const IPV4Address &iface) {
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
bool UDPSocket::JoinMulticast(const IPV4Address &iface,
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
bool UDPSocket::LeaveMulticast(const IPV4Address &iface,
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


bool UDPSocket::_RecvFrom(uint8_t *buffer,
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
bool UDPSocket::SetTos(uint8_t tos) {
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
}  // network
}  // ola
