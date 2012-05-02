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
 * Descriptor.cpp
 * Implementation of the Descriptor classes
 * Copyright (C) 2005-2012 Simon Newton
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
#include <sys/socket.h>
#endif

#include <string>

#include "ola/Logging.h"
#include "ola/io/Descriptor.h"

namespace ola {
namespace io {


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
                                  unsigned int size) {
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


/**
 * Send an iovec.
 * @returns the number of bytes sent.
 */
ssize_t ConnectedDescriptor::SendV(const struct iovec *iov, int iocnt) {
  if (!ValidWriteDescriptor())
    return 0;

  ssize_t bytes_sent;
#if HAVE_DECL_MSG_NOSIGNAL
  if (IsSocket()) {
    struct msghdr message;
    message.msg_name = NULL;
    message.msg_namelen = 0;
    message.msg_iov = iov;
    message.msg_iovlen = iocnt;
    bytes_sent = sendmsg(WriteDescriptor(), message, MSG_NOSIGNAL);
  } else {
#else
  {
#endif
    bytes_sent = writev(WriteDescriptor(), iov, iocnt);
  }

  if (bytes_sent < 0)
    OLA_INFO << "Failed to send on " << WriteDescriptor() << ": " <<
      strerror(errno);
  return bytes_sent;
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
}  // io
}  // ola
