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

#include <arpa/inet.h>
#include <errno.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <lla/Logging.h>
#include <lla/network/Socket.h>

namespace lla {
namespace network {

// ReceivingSocket
// ------------------------------------------------

/*
 * Read data from this socket.
 * @param buffer a pointer to the buffer to store new data in
 * @param size the size of the buffer
 * @param data_read a value result argument which returns the amount of data
 * copied into the buffer
 * @returns -1 on error, 0 on success.
 */
int ReceivingSocket::Receive(uint8_t *buffer,
                             unsigned int size,
                             unsigned int &data_read) {
  int ret;
  uint8_t *data = buffer;
  data_read = 0;
  if (m_read_fd == INVALID_SOCKET)
    return -1;

  while (data_read < size) {
    if ((ret = read(m_read_fd, data, size - data_read)) < 0) {
      if (errno == EAGAIN)
        return 0;
      if (errno != EINTR) {
        LLA_WARN << "read failed, " << strerror(errno);
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
 * Called by the select server when there is data to be read.
 * @returns true if everything works, false if there was an error
 */
bool ReceivingSocket::SocketReady() {
  if (m_listener)
    return m_listener->SocketReady(this);
  return true;
}


/*
 * Close this socket
 * @return true if close succeeded, false otherwise
 */
bool ReceivingSocket::Close() {
  bool ret = true;
  if (m_read_fd != INVALID_SOCKET) {
    if (close(m_read_fd)) {
      LLA_WARN << "close() failed, " << strerror(errno);
      ret = false;
    }
    m_read_fd = INVALID_SOCKET;
  }
  return ret;
}


/*
 * Check if this socket has been closed (this is a bit of a lie)
 * @return true if the socket is closed, false otherwise
 */
bool ReceivingSocket::IsClosed() const {
  if (m_read_fd == INVALID_SOCKET)
    return true;

  return UnreadData() == 0;
}


/*
 * Find out how much data is left to read
 * @return the amount of unread data for the socket
 */
int ReceivingSocket::UnreadData() const {
  int unread;
  if (m_read_fd == INVALID_SOCKET)
    return 0;

  if (ioctl(m_read_fd, FIONREAD, &unread) < 0) {
    LLA_WARN << "ioctl error for " << m_read_fd << ", " << strerror(errno);
    return 0;
  }
  return unread;
}


/*
 * Turn on non-blocking reads.
 * @param fd the fd to enable non-blocking for
 */
bool ReceivingSocket::SetNonBlocking(int fd) {
  if (fd == INVALID_SOCKET)
    return false;

  int val = fcntl(fd, F_GETFL, 0);
  int ret = fcntl(fd, F_SETFL, val | O_NONBLOCK);
  if (ret) {
    LLA_WARN << "failed to set " << fd << " non-blocking: " << strerror(errno);
    return false;
  }
  return true;
}



// ConnectedSocket
// ------------------------------------------------

/*
 * Write data to this socket.
 */
ssize_t ConnectedSocket::Send(const uint8_t *buffer, unsigned int size) {
  if (m_write_fd == INVALID_SOCKET)
    return false;

  ssize_t bytes_sent = write(m_write_fd, buffer, size);
  if (bytes_sent != size)
    LLA_WARN << "Failed to send, " << strerror(errno);
  return bytes_sent;
}


/*
 * Close this connected socket.
 * @return true if close succeeded, false otherwise
 */
bool ConnectedSocket::Close() {
  if (m_read_fd != INVALID_SOCKET)
    close(m_read_fd);

  if (m_read_fd != m_write_fd && m_write_fd != INVALID_SOCKET) {
    close(m_write_fd);
    m_write_fd = INVALID_SOCKET;
  }
  m_read_fd = INVALID_SOCKET;
  return true;
}


// LoopbackSocket
// ------------------------------------------------
bool LoopbackSocket::Init() {
  if (m_read_fd >= 0 || m_write_fd >= 0)
    return false;

  int fd_pair[2];
  if (pipe(fd_pair) < 0) {
    LLA_WARN << "pipe() failed, " << strerror(errno);
    return false;
  }
  m_read_fd = fd_pair[0];
  m_write_fd = fd_pair[1];

  SetReadNonBlocking();
  return true;
}

bool LoopbackSocket::Close() {
  if (m_read_fd > 0)
    close(m_write_fd);
  m_write_fd = -1;
}


// PipeSocket
// ------------------------------------------------

/*
 * Create a new pipe socket
 */
bool PipeSocket::Init() {
  if (m_read_fd >= 0 || m_write_fd >= 0)
    return false;

  if (pipe(m_in_pair) < 0) {
    LLA_WARN << "pipe() failed, " << strerror(errno);
    return false;
  }

  if (pipe(m_out_pair) < 0) {
    LLA_WARN << "pipe() failed, " << strerror(errno);
    close(m_in_pair[0]);
    close(m_in_pair[1]);
    return false;
  }

  m_read_fd = m_in_pair[0];
  m_write_fd = m_out_pair[1];
  SetReadNonBlocking();
  return true;
}


/*
 * Fetch the other end of the pipe socket. The caller now owns the new
 * PipeSocket.
 */
PipeSocket *PipeSocket::OppositeEnd() {
  if (!m_other_end) {
    m_other_end = new PipeSocket(m_out_pair[0], m_in_pair[1]);
    m_other_end->SetReadNonBlocking();
  }
  return m_other_end;
}


// TcpSocket
// ------------------------------------------------

/*
 * Connect
 * @param ip_address the IP to connect to
 * @param port the port to connect to
 */
bool TcpSocket::Connect() {
  struct sockaddr_in server_address;
  socklen_t length;

  if (m_read_fd > 0 || m_write_fd > 0)
    return false;

  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    LLA_WARN << "socket() failed, " << strerror(errno);
    return false;
  }

  // setup
  memset(&server_address, 0x00, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(m_port);
  inet_aton(m_ip_address.c_str(), &server_address.sin_addr);

  length = sizeof(server_address);
  if (connect(sd, (struct sockaddr*) &server_address, length)) {
    LLA_WARN << "connect to " << m_ip_address << ":" << m_port << " failed, "
      << strerror(errno);
    return false;
  }
  m_read_fd = sd;
  m_write_fd = sd;
  SetReadNonBlocking();
  return true;
}


// UdpSocket
// ------------------------------------------------

/*
 * Connect
 * @param ip_address the IP to connect to
 * @param port the port to connect to
 */
bool UdpSocket::Connect() {
  struct sockaddr_in server_address;
  socklen_t length;

  if (m_read_fd > 0 || m_write_fd > 0)
    return false;

  int sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0) {
    LLA_WARN << "socket() failed, " << strerror(errno);
    return false;
  }

  // setup
  memset(&server_address, 0x00, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(m_port);
  inet_aton(m_ip_address.c_str(), &server_address.sin_addr);

  length = sizeof(server_address);
  if (connect(sd, (struct sockaddr*) &server_address, length)) {
    LLA_WARN << "connect to " << m_ip_address << ":" << m_port << " failed, "
      << strerror(errno);
    return false;
  }
  m_read_fd = sd;
  m_write_fd = sd;
  SetReadNonBlocking();
  return true;
}


// UdpServerSocket
// ------------------------------------------------

/*
 * Start listening
 * @return true if it succeeded, false otherwise
 */
bool UdpServerSocket::Listen() {
  int sd = socket(PF_INET, SOCK_DGRAM, 0);

  if (sd < 0) {
    LLA_WARN << "Could not create socket " << strerror(errno);
    return false;
  }

  struct sockaddr_in servAddr;
  memset(&servAddr, 0x00, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(m_port);
  servAddr.sin_addr.s_addr =  htonl(INADDR_ANY);

  LLA_DEBUG << "Binding to " << inet_ntoa(servAddr.sin_addr);

  if (bind(sd, (struct sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
    LLA_INFO << "Failed to bind to socket " << strerror(errno);
    close(sd);
    return false;
  }

  m_read_fd = sd;
  return true;
}


/*
 * Enable broadcasting for this socket.
 * @return true if it worked, false otherwise
 */
bool UdpServerSocket::EnableBroadcast() {
  if (m_read_fd == INVALID_SOCKET)
    return false;

  int broadcast_flag = 1;
  int ret = setsockopt(m_read_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_flag,
                       sizeof(int));

  if (ret == -1) {
    LLA_WARN << "Failed to bind to socket " << strerror(errno);
    return false;
  }
}


// TcpAcceptingSocket
// ------------------------------------------------

/*
 * Create a new TcpListeningSocket
 * @param address the address to listen on
 * @param port the port to listen on
 * @param backlog the backlog
 */
TcpAcceptingSocket::TcpAcceptingSocket(std::string address,
                                       unsigned short port,
                                       int backlog):
    AcceptingSocket(),
    m_address(address),
    m_port(port),
    m_sd(INVALID_SOCKET),
    m_backlog(backlog) {
}


/*
 * Start listening
 * @return true if it succeeded, false otherwise
 */
bool TcpAcceptingSocket::Listen() {
  struct sockaddr_in server_address;
  int reuse_flag = 1;

  if (m_sd > 0)
    return false;

  m_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_sd < 0) {
    LLA_WARN << "socket() failed: " << strerror(errno);
    return false;
  }

  if (setsockopt(m_sd, SOL_SOCKET, SO_REUSEADDR, &reuse_flag,
                 sizeof(reuse_flag))) {
    LLA_WARN << "can't set reuse for " << m_sd << ", " << strerror(errno);
    close(m_sd);
    return false;
  }

  // setup
  memset(&server_address, 0x00, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(m_port);
  inet_aton(m_address.c_str(), &server_address.sin_addr);

  if (bind(m_sd, (struct sockaddr *) &server_address,
           sizeof(server_address)) == -1) {
    LLA_WARN << "bind to " << m_address << ":" << m_port << " failed, "
      << strerror(errno);
    close(m_sd);
    return false;
  }

  if (listen(m_sd, m_backlog)) {
    LLA_WARN << "listen on " << m_address << ":" << m_port << " failed, "
      << strerror(errno);
    return false;
  }
  return true;
}


/*
 * Stop listening & close this socket
 * @return true if close succeeded, false otherwise
 */
bool TcpAcceptingSocket::Close() {
  bool ret = true;
  if (m_sd != INVALID_SOCKET)
    if (close(m_sd)) {
      LLA_WARN << "close() failed " << strerror(errno);
      ret = false;
    }
  m_sd = INVALID_SOCKET;
  return ret;
}


/*
 * Check if this socket has been closed (this is a bit of a lie)
 * @return true if the socket is closed, false otherwise
 */
bool TcpAcceptingSocket::IsClosed() const {
  return m_sd == INVALID_SOCKET;
}


/*
 * Accept new connections
 */
bool TcpAcceptingSocket::SocketReady() {
  struct sockaddr_in cli_address;
  socklen_t length = sizeof(cli_address);

  if (m_sd == INVALID_SOCKET)
    return 0;

  int sd = accept(m_sd, (struct sockaddr*) &cli_address, &length);
  if (sd < 0) {
    LLA_WARN << "accept() failed, " << strerror(errno);
    return 0;
  }

  ConnectedSocket *socket = new ConnectedSocket(sd);
  socket->SetReadNonBlocking();

  if (m_listener)
    m_listener->NewConnection(socket);
}

} // network
} //lla
