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
#include "ola/network/Socket.h"
#include "ola/network/NetworkUtils.h"

namespace ola {
namespace network {


/**
 * Helper function to create a annonymous pipe
 * @param fd_pair a 2 element array which is updated with the fds
 * @return true if successfull, false otherwise.
 */
bool CreatePipe(int fd_pair[2]) {
#ifdef WIN32
  HANDLE read_handle = NULL;
  HANDLE write_handle = NULL;

  SECURITY_ATTRIBUTES security_attributes;
  // Set the bInheritHandle flag so pipe handles are inherited.
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.bInheritHandle = TRUE;
  security_attributes.lpSecurityDescriptor = NULL;

  if (!CreatePipe(&read_handle, &write_handle, &security_attributes, 0)) {
    OLA_WARN << "CreatePipe() failed, " << strerror(errno);
    return false;
  }
  fd_pair[0] = SetStdHandle(STD_INPUT_HANDLE, read_handle);
  fd_pair[1] = SetStdHandle(STD_OUTPUT_HANDLE, write_handle);
#else
  if (pipe(fd_pair) < 0) {
    OLA_WARN << "pipe() failed, " << strerror(errno);
    return false;
  }
#endif
  return true;
}



// BidirectionalFileDescriptor
// ------------------------------------------------
void BidirectionalFileDescriptor::PerformRead() {
  if (m_on_read)
    m_on_read->Run();
  else
    OLA_FATAL << "FileDescriptor " << ReadDescriptor() <<
      " is ready but no handler attached, this is bad!";
}


void BidirectionalFileDescriptor::PerformWrite() {
  if (m_on_write)
    m_on_write->Run();
  else
    OLA_FATAL << "FileDescriptor " << WriteDescriptor() <<
      " is ready but no write handler attached, this is bad!";
}


// ConnectedDescriptor
// ------------------------------------------------

/*
 * Turn on non-blocking reads.
 * @param fd the descriptor to enable non-blocking on
 * @return true if it worked, false otherwise
 */
bool ConnectedDescriptor::SetNonBlocking(int fd) {
  if (fd == INVALID_DESCRIPTOR)
    return false;

#ifdef WIN32
  u_long mode = 1;
  bool success = ioctlsocket(fd, FIONBIO, &mode) != SOCKET_ERROR;
#else
  int val = fcntl(fd, F_GETFL, 0);
  bool success =  fcntl(fd, F_SETFL, val | O_NONBLOCK) == 0;
#endif
  if (!success) {
    OLA_WARN << "failed to set " << fd << " non-blocking: " << strerror(errno);
    return false;
  }
  return true;
}


/*
 * Turn off the SIGPIPE for this socket
 */
bool ConnectedDescriptor::SetNoSigPipe(int fd) {
  if (!IsSocket())
    return true;

  #if HAVE_DECL_SO_NOSIGPIPE
  int sig_pipe_flag = 1;
  int ok = setsockopt(fd,
                      SOL_SOCKET,
                      SO_NOSIGPIPE,
                      &sig_pipe_flag,
                      sizeof(sig_pipe_flag));
  if (ok == -1) {
    OLA_INFO << "Failed to disable SIGPIPE on " << fd << ": " <<
      strerror(errno);
    return false;
  }
  #else
  (void) fd;
  #endif
  return true;
}


/*
 * Find out how much data is left to read
 * @return the amount of unread data for the socket
 */
int ConnectedDescriptor::DataRemaining() const {
  if (!ValidReadDescriptor())
    return 0;

#ifdef WIN32
  u_long unread;
  bool failed = ioctlsocket(ReadDescriptor(), FIONREAD, &unread) < 0;
#else
  int unread;
  bool failed = ioctl(ReadDescriptor(), FIONREAD, &unread) < 0;
#endif
  if (failed) {
    OLA_WARN << "ioctl error for " << ReadDescriptor() << ", "
      << strerror(errno);
    return 0;
  }
  return unread;
}


/*
 * Write data to this descriptor.
 * @param buffer the data to write
 * @param size the length of the data
 * @return the number of bytes sent
 */
ssize_t ConnectedDescriptor::Send(const uint8_t *buffer,
                                  unsigned int size) const {
  if (!ValidWriteDescriptor())
    return 0;

ssize_t bytes_sent;
#if HAVE_DECL_MSG_NOSIGNAL
  if (IsSocket())
    bytes_sent = send(WriteDescriptor(), buffer, size, MSG_NOSIGNAL);
  else
#endif
    bytes_sent = write(WriteDescriptor(), buffer, size);

  if (bytes_sent < 0 || static_cast<unsigned int>(bytes_sent) != size)
    OLA_INFO << "Failed to send on " << WriteDescriptor() << ": " <<
      strerror(errno);
  return bytes_sent;
}


/*
 * Read data from this descriptor.
 * @param buffer a pointer to the buffer to store new data in
 * @param size the size of the buffer
 * @param data_read a value result argument which returns the amount of data
 * copied into the buffer
 * @returns -1 on error, 0 on success.
 */
int ConnectedDescriptor::Receive(uint8_t *buffer,
                                 unsigned int size,
                                 unsigned int &data_read) {
  int ret;
  uint8_t *data = buffer;
  data_read = 0;
  if (!ValidReadDescriptor())
    return -1;

  while (data_read < size) {
    if ((ret = read(ReadDescriptor(), data, size - data_read)) < 0) {
      if (errno == EAGAIN)
        return 0;
      if (errno != EINTR) {
        OLA_WARN << "read failed, " << strerror(errno);
        return -1;
      }
    } else if (ret == 0) {
      return 0;
    }
    data_read += ret;
    data += data_read;
  }
  return 0;
}


/*
 * Check if the remote end has closed the connection.
 * @return true if the socket is closed, false otherwise
 */
bool ConnectedDescriptor::IsClosed() const {
  return DataRemaining() == 0;
}


// LoopbackDescriptor
// ------------------------------------------------


/*
 * Setup this loopback socket
 */
bool LoopbackDescriptor::Init() {
  if (m_fd_pair[0] != INVALID_DESCRIPTOR ||
      m_fd_pair[1] != INVALID_DESCRIPTOR)
    return false;

  if (!CreatePipe(m_fd_pair))
    return false;

  SetReadNonBlocking();
  SetNoSigPipe(WriteDescriptor());
  return true;
}


/*
 * Close the loopback socket
 * @return true if close succeeded, false otherwise
 */
bool LoopbackDescriptor::Close() {
  if (m_fd_pair[0] != INVALID_DESCRIPTOR)
    close(m_fd_pair[0]);

  if (m_fd_pair[1] != INVALID_DESCRIPTOR)
    close(m_fd_pair[1]);

  m_fd_pair[0] = INVALID_DESCRIPTOR;
  m_fd_pair[1] = INVALID_DESCRIPTOR;
  return true;
}


/*
 * Close the write portion of the loopback socket
 * @return true if close succeeded, false otherwise
 */
bool LoopbackDescriptor::CloseClient() {
  if (m_fd_pair[1] != INVALID_DESCRIPTOR)
    close(m_fd_pair[1]);

  m_fd_pair[1] = INVALID_DESCRIPTOR;
  return true;
}



// PipeDescriptor
// ------------------------------------------------

/*
 * Create a new pipe socket
 */
bool PipeDescriptor::Init() {
  if (m_in_pair[0] != INVALID_DESCRIPTOR ||
      m_out_pair[1] != INVALID_DESCRIPTOR)
    return false;

  if (!CreatePipe(m_in_pair))
    return false;

  if (!CreatePipe(m_out_pair)) {
    close(m_in_pair[0]);
    close(m_in_pair[1]);
    m_in_pair[0] = m_in_pair[1] = INVALID_DESCRIPTOR;
    return false;
  }

  SetReadNonBlocking();
  SetNoSigPipe(WriteDescriptor());
  return true;
}


/*
 * Fetch the other end of the pipe socket. The caller now owns the new
 * PipeDescriptor.
 * @returns NULL if the socket wasn't initialized correctly.
 */
PipeDescriptor *PipeDescriptor::OppositeEnd() {
  if (m_in_pair[0] == INVALID_DESCRIPTOR ||
      m_out_pair[1] == INVALID_DESCRIPTOR)
    return NULL;

  if (!m_other_end) {
    m_other_end = new PipeDescriptor(m_out_pair, m_in_pair, this);
    m_other_end->SetReadNonBlocking();
  }
  return m_other_end;
}


/*
 * Close this PipeDescriptor
 */
bool PipeDescriptor::Close() {
  if (m_in_pair[0] != INVALID_DESCRIPTOR)
    close(m_in_pair[0]);

  if (m_out_pair[1] != INVALID_DESCRIPTOR)
    close(m_out_pair[1]);

  m_in_pair[0] = INVALID_DESCRIPTOR;
  m_out_pair[1] = INVALID_DESCRIPTOR;
  return true;
}


/*
 * Close the write portion of this PipeDescriptor
 */
bool PipeDescriptor::CloseClient() {
  if (m_out_pair[1] != INVALID_DESCRIPTOR)
    close(m_out_pair[1]);

  m_out_pair[1] = INVALID_DESCRIPTOR;
  return true;
}


// UnixSocket
// ------------------------------------------------

/*
 * Create a new unix socket
 */
bool UnixSocket::Init() {
  int pair[2];
  if (m_fd != INVALID_DESCRIPTOR || m_other_end)
    return false;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) {
    OLA_WARN << "socketpair() failed, " << strerror(errno);
    return false;
  }

  m_fd = pair[0];
  SetReadNonBlocking();
  SetNoSigPipe(WriteDescriptor());
  m_other_end = new UnixSocket(pair[1], this);
  m_other_end->SetReadNonBlocking();
  return true;
}


/*
 * Fetch the other end of the unix socket. The caller now owns the new
 * UnixSocket.
 * @returns NULL if the socket wasn't initialized correctly.
 */
UnixSocket *UnixSocket::OppositeEnd() {
  return m_other_end;
}


/*
 * Close this UnixSocket
 */
bool UnixSocket::Close() {
  if (m_fd != INVALID_DESCRIPTOR)
    close(m_fd);

  m_fd = INVALID_DESCRIPTOR;
  return true;
}


/*
 * Close the write portion of this UnixSocket
 */
bool UnixSocket::CloseClient() {
  if (m_fd != INVALID_DESCRIPTOR)
    shutdown(m_fd, SHUT_WR);

  m_fd = INVALID_DESCRIPTOR;
  return true;
}


// TcpSocket
// ------------------------------------------------

/*
 * Connect
 * @param ip_address the IP to connect to
 * @param port the port to connect to
 */
TcpSocket* TcpSocket::Connect(const std::string &ip_address,
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

  if (!StringToAddress(ip_address, server_address.sin_addr)) {
    close(sd);
    return NULL;
  }

  if (connect(sd, (struct sockaddr*) &server_address, length)) {
    OLA_WARN << "connect to " << ip_address << ":" << port << " failed, "
      << strerror(errno);
    return NULL;
  }
  TcpSocket *socket = new TcpSocket(sd);
  socket->SetReadNonBlocking();
  return socket;
}


/*
 * Close this TcpSocket
 */
bool TcpSocket::Close() {
  if (m_sd != INVALID_DESCRIPTOR) {
    close(m_sd);
    m_sd = INVALID_DESCRIPTOR;
  }
  return true;
}


// DeviceDescriptor
// ------------------------------------------------

bool DeviceDescriptor::Close() {
  if (m_fd == INVALID_DESCRIPTOR)
    return true;

#ifdef WIN32
  int ret = closesocket(m_fd);
  WSACleanup();
#else
  int ret = close(m_fd);
#endif
  m_fd = INVALID_DESCRIPTOR;
  return ret == 0;
}


// UdpSocket
// ------------------------------------------------

/*
 * Start listening
 * @return true if it succeeded, false otherwise
 */
bool UdpSocket::Init() {
  if (m_fd != INVALID_DESCRIPTOR)
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
  if (m_fd == INVALID_DESCRIPTOR)
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
  if (m_fd == INVALID_DESCRIPTOR)
    return false;

  int fd = m_fd;
  m_fd = INVALID_DESCRIPTOR;
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
  if (m_fd == INVALID_DESCRIPTOR)
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
TcpAcceptingSocket::TcpAcceptingSocket()
    : AcceptingSocket(),
      m_sd(INVALID_DESCRIPTOR),
      m_on_accept(NULL) {
}


/**
 * Clean up
 */
TcpAcceptingSocket::~TcpAcceptingSocket() {
  Close();
  if (m_on_accept)
    delete m_on_accept;
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

  if (m_sd != INVALID_DESCRIPTOR)
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
  if (m_sd != INVALID_DESCRIPTOR)
    if (close(m_sd)) {
      OLA_WARN << "close() failed " << strerror(errno);
      ret = false;
    }
  m_sd = INVALID_DESCRIPTOR;
  return ret;
}


/*
 * Accept new connections
 * @return a new connected socket
 */
void TcpAcceptingSocket::PerformRead() {
  struct sockaddr_in cli_address;
  socklen_t length = sizeof(cli_address);

  if (m_sd == INVALID_DESCRIPTOR)
    return;

  int sd = accept(m_sd, (struct sockaddr*) &cli_address, &length);
  if (sd < 0) {
    OLA_WARN << "accept() failed, " << strerror(errno);
    return;
  }

  if (m_on_accept) {
    TcpSocket *socket = new TcpSocket(sd);
    socket->SetReadNonBlocking();
    m_on_accept->Run(socket);
  } else {
    OLA_WARN <<
      "Accepted new TCP Connection but no OnAccept handler registered";
    close(sd);
  }
}
}  // network
}  // ola
