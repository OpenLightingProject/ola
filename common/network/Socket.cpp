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
 * Socket.cpp
 * Implementation of the UDP Socket classes
 * Copyright (C) 2005 Simon Newton
 */

#include "ola/network/Socket.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#include <Ws2tcpip.h>
#include <winioctl.h>
#include <Mswsock.h>
#else
#include <sys/ioctl.h>
#endif  // _WIN32

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif  // HAVE_SYS_SOCKET_H

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif  // HAVE_NETINET_IN_H

#include <string>

#include "common/network/SocketHelper.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/TCPSocketFactory.h"

namespace ola {
namespace network {

namespace {

bool ReceiveFrom(int fd, uint8_t *buffer, ssize_t *data_read,
                 struct sockaddr_in *source, socklen_t *src_size) {
  *data_read = recvfrom(
    fd, reinterpret_cast<char*>(buffer), *data_read,
    0, reinterpret_cast<struct sockaddr*>(source), source ? src_size : NULL);
  if (*data_read < 0) {
#ifdef _WIN32
    OLA_WARN << "recvfrom fd: " << fd << " failed: " << WSAGetLastError();
#else
    OLA_WARN << "recvfrom fd: " << fd << " failed: " << strerror(errno);
#endif  // _WIN32
    return false;
  }
  return true;
}

}  // namespace

// UDPSocket
// ------------------------------------------------

bool UDPSocket::Init() {
  if (m_handle != ola::io::INVALID_DESCRIPTOR)
    return false;

  int sd = socket(PF_INET, SOCK_DGRAM, 0);

  if (sd < 0) {
    OLA_WARN << "Could not create socket " << strerror(errno);
    return false;
  }

#ifdef _WIN32
  m_handle.m_handle.m_fd = sd;
  m_handle.m_type = ola::io::SOCKET_DESCRIPTOR;
  // Set socket to nonblocking to enable WSAEventSelect
    u_long mode = 1;
    ioctlsocket(sd, FIONBIO, &mode);
#else
  m_handle = sd;
#endif  // _WIN32
  return true;
}

bool UDPSocket::Bind(const IPV4SocketAddress &endpoint) {
  if (m_handle == ola::io::INVALID_DESCRIPTOR)
    return false;

  struct sockaddr server_address;
  if (!endpoint.ToSockAddr(&server_address, sizeof(server_address)))
    return false;

  #if HAVE_DECL_SO_REUSEADDR
  int reuse_addr_flag = 1;
  int addr_ok = setsockopt(m_handle,
                           SOL_SOCKET,
                           SO_REUSEADDR,
                           reinterpret_cast<char*>(&reuse_addr_flag),
                           sizeof(reuse_addr_flag));
  if (addr_ok < 0) {
    OLA_WARN << "can't set SO_REUSEADDR for " << m_handle << ", " <<
      strerror(errno);
    return false;
  }
  #endif  // HAVE_DECL_SO_REUSEADDR

  #if HAVE_DECL_SO_REUSEPORT
  // turn on REUSEPORT if we can
  int reuse_port_flag = 1;
  int ok = setsockopt(m_handle,
                      SOL_SOCKET,
                      SO_REUSEPORT,
                      reinterpret_cast<char*>(&reuse_port_flag),
                      sizeof(reuse_port_flag));
  if (ok < 0) {
    OLA_WARN << "can't set SO_REUSEPORT for " << m_handle << ", " <<
      strerror(errno);
    // This is non fatal, since Linux introduced this option in the 3.9 series.
  }
  #endif  // HAVE_DECL_SO_REUSEPORT

  OLA_DEBUG << "Binding to " << endpoint;
#ifdef _WIN32
  if (bind(m_handle.m_handle.m_fd, &server_address, sizeof(server_address))
      == -1) {
#else
  if (bind(m_handle, &server_address, sizeof(server_address)) == -1) {
#endif  // _WIN32
    OLA_WARN << "bind(" << endpoint << "): " << strerror(errno);
    return false;
  }
  m_bound_to_port = true;
  return true;
}

bool UDPSocket::GetSocketAddress(IPV4SocketAddress *address) const {
#ifdef _WIN32
  GenericSocketAddress addr = GetLocalAddress(m_handle.m_handle.m_fd);
#else
  GenericSocketAddress addr = GetLocalAddress(m_handle);
#endif  // _WIN32
  if (!addr.IsValid()) {
    return false;
  }
  *address = addr.V4Addr();
  return true;
}

bool UDPSocket::Close() {
  if (m_handle == ola::io::INVALID_DESCRIPTOR)
    return false;

#ifdef _WIN32
  int fd = m_handle.m_handle.m_fd;
#else
  int fd = m_handle;
#endif  // _WIN32
  m_handle = ola::io::INVALID_DESCRIPTOR;
  m_bound_to_port = false;
#ifdef _WIN32
  if (closesocket(fd)) {
#else
  if (close(fd)) {
#endif  // _WIN32
    OLA_WARN << "close() failed, " << strerror(errno);
    return false;
  }
  return true;
}

ssize_t UDPSocket::SendTo(const uint8_t *buffer,
                          unsigned int size,
                          const IPV4Address &ip,
                          unsigned short port) const {
  return SendTo(buffer, size, IPV4SocketAddress(ip, port));
}

ssize_t UDPSocket::SendTo(const uint8_t *buffer,
                          unsigned int size,
                          const IPV4SocketAddress &dest) const {
  if (!ValidWriteDescriptor())
    return 0;

  struct sockaddr_in destination;
  if (!dest.ToSockAddr(reinterpret_cast<sockaddr*>(&destination),
                       sizeof(destination))) {
    return 0;
  }

  ssize_t bytes_sent = sendto(
#ifdef _WIN32
    m_handle.m_handle.m_fd,
#else
    m_handle,
#endif  // _WIN32
    reinterpret_cast<const char*>(buffer),
    size,
    0,
    reinterpret_cast<const struct sockaddr*>(&destination),
    sizeof(struct sockaddr));
  if (bytes_sent < 0 || static_cast<unsigned int>(bytes_sent) != size)
    OLA_INFO << "sendto failed: " << dest << " : " << strerror(errno);
  return bytes_sent;
}

ssize_t UDPSocket::SendTo(ola::io::IOVecInterface *data,
                          const IPV4Address &ip,
                          unsigned short port) const {
  return SendTo(data, IPV4SocketAddress(ip, port));
}

ssize_t UDPSocket::SendTo(ola::io::IOVecInterface *data,
                          const IPV4SocketAddress &dest) const {
  if (!ValidWriteDescriptor())
    return 0;

  struct sockaddr_in destination;
  if (!dest.ToSockAddr(reinterpret_cast<sockaddr*>(&destination),
                       sizeof(destination))) {
    return 0;
  }

  int io_len;
  const struct ola::io::IOVec *iov = data->AsIOVec(&io_len);

  if (iov == NULL)
    return 0;

#ifdef _WIN32
  ssize_t bytes_sent = 0;

  for (int buffer = 0; buffer < io_len; ++buffer) {
    bytes_sent += SendTo(reinterpret_cast<uint8_t*>(iov[buffer].iov_base),
        iov[buffer].iov_len, dest);
  }

#else
  struct msghdr message;
  message.msg_name = &destination;
  message.msg_namelen = sizeof(destination);
  message.msg_iov = reinterpret_cast<iovec*>(const_cast<io::IOVec*>(iov));
  message.msg_iovlen = io_len;
  message.msg_control = NULL;
  message.msg_controllen = 0;
  message.msg_flags = 0;

  ssize_t bytes_sent = sendmsg(WriteDescriptor(), &message, 0);
#endif  // _WIN32
  data->FreeIOVec(iov);

  if (bytes_sent < 0) {
    OLA_INFO << "Failed to send on " << WriteDescriptor() << ": to "
             << dest << " : " <<  strerror(errno);
  } else {
    data->Pop(bytes_sent);
  }
  return bytes_sent;
}

bool UDPSocket::RecvFrom(uint8_t *buffer, ssize_t *data_read) const {
  socklen_t length = 0;
#ifdef _WIN32
  return ReceiveFrom(m_handle.m_handle.m_fd, buffer, data_read, NULL, &length);
#else
  return ReceiveFrom(m_handle, buffer, data_read, NULL, &length);
#endif  // _WIN32
}

bool UDPSocket::RecvFrom(
    uint8_t *buffer,
    ssize_t *data_read,
    IPV4Address &source) const {  // NOLINT(runtime/references)
  struct sockaddr_in src_sockaddr;
  socklen_t src_size = sizeof(src_sockaddr);
#ifdef _WIN32
  bool ok = ReceiveFrom(m_handle.m_handle.m_fd, buffer, data_read,
                        &src_sockaddr, &src_size);
#else
  bool ok = ReceiveFrom(m_handle, buffer, data_read, &src_sockaddr, &src_size);
#endif  // _WIN32
  if (ok)
    source = IPV4Address(src_sockaddr.sin_addr.s_addr);
  return ok;
}

bool UDPSocket::RecvFrom(uint8_t *buffer,
                         ssize_t *data_read,
                         IPV4Address &source,  // NOLINT(runtime/references)
                         uint16_t &port) const {  // NOLINT(runtime/references)
  struct sockaddr_in src_sockaddr;
  socklen_t src_size = sizeof(src_sockaddr);
#ifdef _WIN32
  bool ok = ReceiveFrom(m_handle.m_handle.m_fd, buffer, data_read,
                        &src_sockaddr, &src_size);
#else
  bool ok = ReceiveFrom(m_handle, buffer, data_read, &src_sockaddr, &src_size);
#endif  // _WIN32
  if (ok) {
    source = IPV4Address(src_sockaddr.sin_addr.s_addr);
    port = NetworkToHost(src_sockaddr.sin_port);
  }
  return ok;
}

bool UDPSocket::RecvFrom(uint8_t *buffer,
                         ssize_t *data_read,
                         IPV4SocketAddress *source) {
  struct sockaddr_in src_sockaddr;
  socklen_t src_size = sizeof(src_sockaddr);
#ifdef _WIN32
  bool ok = ReceiveFrom(m_handle.m_handle.m_fd, buffer, data_read,
                        &src_sockaddr, &src_size);
#else
  bool ok = ReceiveFrom(m_handle, buffer, data_read, &src_sockaddr, &src_size);
#endif  // _WIN32
  if (ok) {
    *source = IPV4SocketAddress(IPV4Address(src_sockaddr.sin_addr.s_addr),
                                NetworkToHost(src_sockaddr.sin_port));
  }
  return ok;
}

bool UDPSocket::EnableBroadcast() {
  if (m_handle == ola::io::INVALID_DESCRIPTOR)
    return false;

  int broadcast_flag = 1;
#ifdef _WIN32
  int ok = setsockopt(m_handle.m_handle.m_fd,
#else
  int ok = setsockopt(m_handle,
#endif  // _WIN32
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


bool UDPSocket::SetMulticastInterface(const IPV4Address &iface) {
  struct in_addr addr;
  addr.s_addr = iface.AsInt();
#ifdef _WIN32
  int ok = setsockopt(m_handle.m_handle.m_fd,
#else
  int ok = setsockopt(m_handle,
#endif  // _WIN32
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

bool UDPSocket::JoinMulticast(const IPV4Address &iface,
                              const IPV4Address &group,
                              bool multicast_loop) {
  char loop = multicast_loop;
  struct ip_mreq mreq;
  mreq.imr_interface.s_addr = iface.AsInt();
  mreq.imr_multiaddr.s_addr = group.AsInt();

#ifdef _WIN32
  int ok = setsockopt(m_handle.m_handle.m_fd,
#else
  int ok = setsockopt(m_handle,
#endif  // _WIN32
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
#ifdef _WIN32
    ok = setsockopt(m_handle.m_handle.m_fd, IPPROTO_IP, IP_MULTICAST_LOOP,
                    &loop, sizeof(loop));
#else
    ok = setsockopt(m_handle, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,
                    sizeof(loop));
#endif  // _WIN32
    if (ok < 0) {
      OLA_WARN << "Failed to disable looping for " << m_handle << ": "
               << strerror(errno);
      return false;
    }
  }
  return true;
}

bool UDPSocket::LeaveMulticast(const IPV4Address &iface,
                               const IPV4Address &group) {
  struct ip_mreq mreq;
  mreq.imr_interface.s_addr = iface.AsInt();
  mreq.imr_multiaddr.s_addr = group.AsInt();

#ifdef _WIN32
  int ok = setsockopt(m_handle.m_handle.m_fd,
#else
  int ok = setsockopt(m_handle,
#endif  // _WIN32
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

bool UDPSocket::SetTos(uint8_t tos) {
  unsigned int value = tos & 0xFC;  // zero the ECN fields
#ifdef _WIN32
  int ok = setsockopt(m_handle.m_handle.m_fd,
#else
  int ok = setsockopt(m_handle,
#endif  // _WIN32
                      IPPROTO_IP,
                      IP_TOS,
                      reinterpret_cast<char*>(&value),
                      sizeof(value));
  if (ok < 0) {
    OLA_WARN << "Failed to set tos for " << m_handle << ", " << strerror(errno);
    return false;
  }
  return true;
}
}  // namespace network
}  // namespace ola
