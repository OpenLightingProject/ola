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
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <lla/select_server/Socket.h>

namespace lla {
namespace select_server {

// ConnectedSocket
// ------------------------------------------------

/*
 * Write data to this socket.
 */
ssize_t ConnectedSocket::Send(const uint8_t *buffer, unsigned int size) {
  return write(m_write_fd, buffer, size);
}


/*
 * Read data from this socket.
 *
 * @returns -1 on error, 0 on success.
 */
int ConnectedSocket::Receive(uint8_t *buffer,
                             unsigned int size,
                             unsigned int &data_read) {
  int ret;
  uint8_t *data = buffer;
  data_read = 0;
  while (data_read < size) {
    if ((ret = read(m_read_fd, data, size - data_read)) < 0) {
      if (errno == EAGAIN)
        return 0;
      if (errno != EINTR)
        return -1;
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
 */
int ConnectedSocket::SocketReady() {
  if (m_listener)
    return m_listener->SocketReady(this);
  return 0;
}


/*
 * Turn on non-blocking reads.
 */
bool ConnectedSocket::SetNonBlocking(int fd) {
  int val = fcntl(fd, F_GETFL, 0);
  int ret = fcntl(fd, F_SETFL, val | O_NONBLOCK);
  if (ret) {
    printf("failed to set non-blocking\n");
    return false;
  }
  return true;
}


bool ConnectedSocket::Close() {
  if (m_read_fd > 0)
    close(m_read_fd);

  if (m_read_fd != m_write_fd && m_write_fd > 0) {
    close(m_write_fd);
    m_write_fd = -1;
  }
  m_read_fd = -1;
}


/*
 * This is a bit of a lie
 */
bool ConnectedSocket::IsClosed() const {
  if (m_read_fd == -1)
    return true;

  return UnreadData() == 0;
}


/*
 * Find out how much data is left to read
 */
int ConnectedSocket::UnreadData() const {
  int unread;
  if (ioctl(m_read_fd, FIONREAD, &unread) < 0) {
    printf("ioctl error for %d\n", m_read_fd);
    return 0;
  }

  return unread;
}


// LoopbackSocket
// ------------------------------------------------
bool LoopbackSocket::Init() {
  if (m_read_fd >= 0 || m_write_fd >= 0)
    return false;

  int fd_pair[2];
  if (pipe(fd_pair) < 0) {
    printf("pipe failed\n");
    return false;
  }
  m_read_fd = fd_pair[0];
  m_write_fd = fd_pair[1];

  SetReadNonBlocking();
  return true;
}


// PipeSocket
// ------------------------------------------------
bool PipeSocket::Init() {
  if (m_read_fd >= 0 || m_write_fd >= 0)
    return false;

  if (pipe(m_in_pair) < 0) {
    printf("pipe failed\n");
    return false;
  }

  if (pipe(m_out_pair) < 0) {
    printf("pipe failed\n");
    close(m_in_pair[0]);
    close(m_in_pair[1]);
    return false;
  }

  m_read_fd = m_in_pair[0];
  m_write_fd = m_out_pair[1];
  SetReadNonBlocking();
  return true;
}


PipeSocket *PipeSocket::OppositeEnd() {
  PipeSocket *pipe_socket = new PipeSocket(m_out_pair[0], m_in_pair[1]);
  pipe_socket->SetReadNonBlocking();
  return pipe_socket;
}


// TcpSocket
// ------------------------------------------------

/*
 * Connect
 *
 * @param ip_address the IP to connect to
 * @param port the port to connect to
 */
bool TcpSocket::Connect(std::string ip_address, unsigned short port) {
  struct sockaddr_in server_address;
  socklen_t length;

  if (m_read_fd > 0 || m_write_fd > 0)
    return false;

  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    printf("socket call failed: %s\n", strerror(errno));
    return false;
  }

  // setup
  memset(&server_address, 0x00, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  inet_aton(ip_address.c_str(), &server_address.sin_addr);

  length = sizeof(server_address);
  if(connect(sd, (struct sockaddr*) &server_address, length)) {
    printf("connect failed: %s\n", strerror(errno));
    return false;
  }
  m_read_fd = sd;
  m_write_fd = sd;
  SetReadNonBlocking();
  return true;
}


// TcpListeningSocket
// ------------------------------------------------

TcpListeningSocket::TcpListeningSocket(std::string address, unsigned short port, int backlog):
    ListeningSocket(),
    m_address(address),
    m_port(port),
    m_sd(-1),
    m_backlog(backlog) {
}


/*
 * Start listening
 */
bool TcpListeningSocket::Listen() {
  struct sockaddr_in server_address;
  int reuse_flag = 1;

  if (m_sd > 0)
    return false;

  m_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_sd < 0) {
    printf("socket call failed: %s\n", strerror(errno));
    return false;
  }

  if (setsockopt(m_sd, SOL_SOCKET, SO_REUSEADDR, &reuse_flag, sizeof(reuse_flag))) {
    printf("can't set reuse\n");
    close(m_sd);
    return false;
  }

  // setup
  memset(&server_address, 0x00, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(m_port);
  inet_aton(m_address.c_str(), &server_address.sin_addr);

  if (bind(m_sd, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
    printf("bind failed: %s\n", strerror(errno));
    close(m_sd);
    return false;
  }

  if (listen(m_sd, m_backlog)) {
    printf("listen failed: %s\n", strerror(errno));
    return false;
  }
  return true;
}


/*
 * Stop listening
 */
bool TcpListeningSocket::Close() {
  if (m_sd != -1)
    close(m_sd);
  m_sd = -1;
}


bool TcpListeningSocket::IsClosed() const {
  return m_sd == -1;
}


/*
 * accept new connections
 */
int TcpListeningSocket::SocketReady() {
  struct sockaddr_in cli_address;
  socklen_t length = sizeof(cli_address);

  int sd = accept(m_sd, (struct sockaddr*) &cli_address, &length);
  if (!sd) {
    printf("accept failed\n");
    return 0;
  }

  TcpSocket *socket = new TcpSocket(sd);
  socket->SetReadNonBlocking();

  if (m_listener)
    m_listener->NewConnection(socket);
}

} // select_server
} //lla
