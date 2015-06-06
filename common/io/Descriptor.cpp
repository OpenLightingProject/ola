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
 * Descriptor.cpp
 * Implementation of the Descriptor classes
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#include <Winioctl.h>
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif

#include <algorithm>
#include <string>

#include "ola/Logging.h"
#include "ola/base/Macro.h"
#include "ola/io/Descriptor.h"

namespace ola {
namespace io {

#ifndef _WIN32
// Check binary compatibility between IOVec and iovec
STATIC_ASSERT(sizeof(struct iovec) == sizeof(struct IOVec));
#endif


#ifdef _WIN32
// DescriptorHandle
// ------------------------------------------------
DescriptorHandle::DescriptorHandle():
    m_type(GENERIC_DESCRIPTOR),
    m_event(NULL),
    m_async_data(NULL),
    m_async_data_size(NULL) {
  m_handle.m_fd = -1;
}

DescriptorHandle::~DescriptorHandle() {
}

void* ToHandle(const DescriptorHandle &handle) {
  return handle.m_handle.m_handle;
}

bool DescriptorHandle::AllocAsyncBuffer() {
  if (m_async_data || m_async_data_size) {
    OLA_WARN << "Async data already allocated";
    return false;
  }
  try {
    m_async_data = new uint8_t[ASYNC_DATA_BUFFER_SIZE];
    m_async_data_size = new uint32_t;
    *m_async_data_size = 0;
  } catch (std::exception& ex) {
    OLA_WARN << ex.what();
  }

  return (m_async_data && m_async_data_size);
}

void DescriptorHandle::FreeAsyncBuffer() {
  if (m_async_data) {
    delete[] m_async_data;
    m_async_data = NULL;
  }
  if (m_async_data_size) {
    delete m_async_data_size;
    m_async_data_size = NULL;
  }
}

bool DescriptorHandle::IsValid() const {
  return (m_handle.m_fd != -1);
}

bool operator!=(const DescriptorHandle &lhs, const DescriptorHandle &rhs) {
  return !(lhs == rhs);
}

bool operator==(const DescriptorHandle &lhs, const DescriptorHandle &rhs) {
  return ((lhs.m_handle.m_fd == rhs.m_handle.m_fd) &&
          (lhs.m_type == rhs.m_type));
}

bool operator<(const DescriptorHandle &lhs, const DescriptorHandle &rhs) {
  return (lhs.m_handle.m_fd < rhs.m_handle.m_fd);
}

std::ostream& operator<<(std::ostream &stream, const DescriptorHandle &data) {
  stream << data.m_handle.m_fd;
  return stream;
}
#endif

int ToFD(const DescriptorHandle &handle) {
#ifdef _WIN32
  switch (handle.m_type) {
    case SOCKET_DESCRIPTOR:
      return handle.m_handle.m_fd;
    default:
      return -1;
  }
#else
  return handle;
#endif
}

/**
 * Helper function to create a anonymous pipe
 * @param handle_pair a 2 element array which is updated with the handles
 * @return true if successful, false otherwise.
 */
bool CreatePipe(DescriptorHandle handle_pair[2]) {
#ifdef _WIN32
  HANDLE read_handle = NULL;
  HANDLE write_handle = NULL;

  static unsigned int pipe_name_counter = 0;

  std::ostringstream pipe_name;
  pipe_name << "\\\\.\\Pipe\\OpenLightingArchitecture.";
  pipe_name.setf(std::ios::hex, std::ios::basefield);
  pipe_name.setf(std::ios::showbase);
  pipe_name.width(8);
  pipe_name << GetCurrentProcessId() << ".";
  pipe_name.width(8);
  pipe_name << pipe_name_counter++;

  SECURITY_ATTRIBUTES security_attributes;
  // Set the bInheritHandle flag so pipe handles are inherited.
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.bInheritHandle = TRUE;
  security_attributes.lpSecurityDescriptor = NULL;

  read_handle = CreateNamedPipeA(
      pipe_name.str().c_str(),
      PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_WAIT,
      1,
      4096,
      4096,
      0,
      &security_attributes);
  if (read_handle == INVALID_HANDLE_VALUE) {
    OLA_WARN << "Could not create read end of pipe: %d" << GetLastError();
    return false;
  }

  write_handle = CreateFileA(
      pipe_name.str().c_str(),
      GENERIC_WRITE,
      0,
      &security_attributes,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
      NULL);
  if (write_handle == INVALID_HANDLE_VALUE) {
    OLA_WARN << "Could not create write end of pipe: %d" << GetLastError();
    CloseHandle(read_handle);
    return false;
  }

  handle_pair[0].m_handle.m_handle = read_handle;
  handle_pair[0].m_type = PIPE_DESCRIPTOR;
  handle_pair[1].m_handle.m_handle = write_handle;
  handle_pair[1].m_type = PIPE_DESCRIPTOR;

  if (!handle_pair[0].AllocAsyncBuffer() || !handle_pair[1].AllocAsyncBuffer())
    return false;
#else
  if (pipe(handle_pair) < 0) {
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


// UnmanagedFileDescriptor
// ------------------------------------------------
UnmanagedFileDescriptor::UnmanagedFileDescriptor(int fd)
  : BidirectionalFileDescriptor() {
#ifdef _WIN32
  m_handle.m_handle.m_fd = fd;
  m_handle.m_type = GENERIC_DESCRIPTOR;
#else
  m_handle = fd;
#endif
}


// ConnectedDescriptor
// ------------------------------------------------

bool ConnectedDescriptor::SetNonBlocking(DescriptorHandle fd) {
  if (fd == INVALID_DESCRIPTOR)
    return false;

#ifdef _WIN32
  bool success = true;
  if (fd.m_type == SOCKET_DESCRIPTOR) {
    u_long mode = 1;
    success = (ioctlsocket(ToFD(fd), FIONBIO, &mode) != SOCKET_ERROR);
  }
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

bool ConnectedDescriptor::SetNoSigPipe(DescriptorHandle fd) {
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

int ConnectedDescriptor::DataRemaining() const {
  if (!ValidReadDescriptor())
    return 0;

  int unread = 0;
#ifdef _WIN32
  bool failed = false;
  if (ReadDescriptor().m_type == PIPE_DESCRIPTOR) {
    return ReadDescriptor().m_async_data_size ?
        *ReadDescriptor().m_async_data_size : 0;
  } else if (ReadDescriptor().m_type == SOCKET_DESCRIPTOR) {
    u_long unrd;
    failed = ioctlsocket(ToFD(ReadDescriptor()), FIONREAD, &unrd) < 0;
    unread = unrd;
  } else {
    OLA_WARN << "DataRemaining() called on unsupported descriptor type";
    failed = true;
  }
#else
  bool failed = ioctl(ReadDescriptor(), FIONREAD, &unread) < 0;
#endif
  if (failed) {
    OLA_WARN << "ioctl error for " << ReadDescriptor() << ", "
      << strerror(errno);
    return 0;
  }
  return unread;
}

ssize_t ConnectedDescriptor::Send(const uint8_t *buffer,
                                  unsigned int size) {
  if (!ValidWriteDescriptor())
    return 0;

  ssize_t bytes_sent;
#ifdef _WIN32
  if (WriteDescriptor().m_type == PIPE_DESCRIPTOR) {
    DWORD bytes_written = 0;
    if (!WriteFile(ToHandle(WriteDescriptor()),
                   buffer,
                   size,
                   &bytes_written,
                   NULL)) {
      OLA_WARN << "WriteFile() failed with " << GetLastError();
      bytes_sent = -1;
    } else {
      bytes_sent = bytes_written;
    }
  } else if (WriteDescriptor().m_type == SOCKET_DESCRIPTOR) {
    bytes_sent = send(ToFD(WriteDescriptor()),
                      reinterpret_cast<const char*>(buffer),
                      size,
                      0);
  } else {
    OLA_WARN << "Send() called on unsupported descriptor type";
    return 0;
  }
#else
  // BSD Sockets
#if HAVE_DECL_MSG_NOSIGNAL
  if (IsSocket()) {
    bytes_sent = send(WriteDescriptor(), buffer, size, MSG_NOSIGNAL);
  } else {
#endif
    bytes_sent = write(WriteDescriptor(), buffer, size);
#if HAVE_DECL_MSG_NOSIGNAL
  }
#endif

#endif

  if (bytes_sent < 0 || static_cast<unsigned int>(bytes_sent) != size) {
    OLA_INFO << "Failed to send on " << WriteDescriptor() << ": " <<
      strerror(errno);
  }
  return bytes_sent;
}

ssize_t ConnectedDescriptor::Send(IOQueue *ioqueue) {
  if (!ValidWriteDescriptor())
    return 0;

  int iocnt;
  const struct IOVec *iov = ioqueue->AsIOVec(&iocnt);

  ssize_t bytes_sent = 0;

#ifdef _WIN32
  /* There is no scatter/gather functionality for generic descriptors on
   * Windows, so this is implemented as a write loop. Derived classes should
   * re-implement Send() using scatter/gather I/O where available.
   */
  int bytes_written = 0;
  for (int io = 0; io < iocnt; ++io) {
    bytes_written = Send(reinterpret_cast<const uint8_t*>(iov[io].iov_base),
                         iov[io].iov_len);
    if (bytes_written == 0) {
      OLA_INFO << "Failed to send on " << WriteDescriptor() << ": " <<
        strerror(errno);
      bytes_sent = -1;
      break;
    }
    bytes_sent += bytes_written;
  }
#else
#if HAVE_DECL_MSG_NOSIGNAL
  if (IsSocket()) {
    struct msghdr message;
    memset(&message, 0, sizeof(message));
    message.msg_name = NULL;
    message.msg_namelen = 0;
    message.msg_iov = reinterpret_cast<iovec*>(const_cast<IOVec*>(iov));
    message.msg_iovlen = iocnt;
    bytes_sent = sendmsg(WriteDescriptor(), &message, MSG_NOSIGNAL);
  } else {
#else
  {
#endif
    bytes_sent = writev(WriteDescriptor(),
                        reinterpret_cast<const struct iovec*>(iov), iocnt);
  }
#endif

  ioqueue->FreeIOVec(iov);
  if (bytes_sent < 0) {
    OLA_INFO << "Failed to send on " << WriteDescriptor() << ": " <<
      strerror(errno);
  } else {
    ioqueue->Pop(bytes_sent);
  }
  return bytes_sent;
}

int ConnectedDescriptor::Receive(
    uint8_t *buffer,
    unsigned int size,
    unsigned int &data_read) {  // NOLINT(runtime/references)
  int ret;
  uint8_t *data = buffer;
  data_read = 0;
  if (!ValidReadDescriptor())
    return -1;

  while (data_read < size) {
#ifdef _WIN32
    if (ReadDescriptor().m_type == PIPE_DESCRIPTOR) {
      if (!ReadDescriptor().m_async_data_size) {
        OLA_WARN << "No async data buffer for descriptor " << ReadDescriptor();
        return -1;
      }
      // Check if data was read by the async ReadFile() call
      DWORD async_data_size = *ReadDescriptor().m_async_data_size;
      if (async_data_size > 0) {
        DWORD size_to_copy = std::min(static_cast<DWORD>(size),
            async_data_size);
        memcpy(buffer, ReadDescriptor().m_async_data, size_to_copy);
        data_read = size_to_copy;
        if (async_data_size > size) {
          memmove(ReadDescriptor().m_async_data,
                  &(ReadDescriptor().m_async_data[size_to_copy]),
                  async_data_size - size_to_copy);
        }
        *ReadDescriptor().m_async_data_size -= size_to_copy;
      }
      return 0;
    } else if (ReadDescriptor().m_type == SOCKET_DESCRIPTOR) {
      ret = recv(ToFD(ReadDescriptor()), reinterpret_cast<char*>(data),
                 size - data_read, 0);
      if (ret < 0) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
          return 0;
        } else if (WSAGetLastError() != WSAEINTR) {
          OLA_WARN << "read failed, " << WSAGetLastError();
          return -1;
        }
      } else if (ret == 0) {
        return 0;
      }
      data_read += ret;
      data += data_read;
    } else {
      OLA_WARN << "Descriptor type not implemented for reading: "
               << ReadDescriptor().m_type;
      return -1;
    }
  }
#else
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
#endif
  return 0;
}

bool ConnectedDescriptor::IsClosed() const {
  return DataRemaining() == 0;
}

// LoopbackDescriptor
// ------------------------------------------------

LoopbackDescriptor::LoopbackDescriptor() {
  m_handle_pair[0] = INVALID_DESCRIPTOR;
  m_handle_pair[1] = INVALID_DESCRIPTOR;
}

bool LoopbackDescriptor::Init() {
  if (m_handle_pair[0] != INVALID_DESCRIPTOR ||
      m_handle_pair[1] != INVALID_DESCRIPTOR)
    return false;

  if (!CreatePipe(m_handle_pair))
    return false;

  SetReadNonBlocking();
  SetNoSigPipe(WriteDescriptor());
  return true;
}

bool LoopbackDescriptor::Close() {
  if (m_handle_pair[0] != INVALID_DESCRIPTOR) {
#ifdef _WIN32
    CloseHandle(ToHandle(m_handle_pair[0]));
#else
    close(m_handle_pair[0]);
#endif
  }

  if (m_handle_pair[1] != INVALID_DESCRIPTOR) {
#ifdef _WIN32
    CloseHandle(ToHandle(m_handle_pair[1]));
#else
    close(m_handle_pair[1]);
#endif
  }

  m_handle_pair[0] = INVALID_DESCRIPTOR;
  m_handle_pair[1] = INVALID_DESCRIPTOR;
  return true;
}

bool LoopbackDescriptor::CloseClient() {
  if (m_handle_pair[1] != INVALID_DESCRIPTOR) {
#ifdef _WIN32
    CloseHandle(ToHandle(m_handle_pair[1]));
#else
    close(m_handle_pair[1]);
#endif
  }

  m_handle_pair[1] = INVALID_DESCRIPTOR;
  return true;
}

// PipeDescriptor
// ------------------------------------------------

PipeDescriptor::PipeDescriptor():
  m_other_end(NULL) {
  m_in_pair[0] = m_in_pair[1] = INVALID_DESCRIPTOR;
  m_out_pair[0] = m_out_pair[1] = INVALID_DESCRIPTOR;
}

bool PipeDescriptor::Init() {
  if (m_in_pair[0] != INVALID_DESCRIPTOR ||
      m_out_pair[1] != INVALID_DESCRIPTOR)
    return false;

  if (!CreatePipe(m_in_pair)) {
    return false;
  }

  if (!CreatePipe(m_out_pair)) {
#ifdef _WIN32
    CloseHandle(ToHandle(m_in_pair[0]));
    CloseHandle(ToHandle(m_in_pair[1]));
#else
    close(m_in_pair[0]);
    close(m_in_pair[1]);
#endif
    m_in_pair[0] = m_in_pair[1] = INVALID_DESCRIPTOR;
    return false;
  }

  SetReadNonBlocking();
  SetNoSigPipe(WriteDescriptor());
  return true;
}

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

bool PipeDescriptor::Close() {
  if (m_in_pair[0] != INVALID_DESCRIPTOR) {
#ifdef _WIN32
    CloseHandle(ToHandle(m_in_pair[0]));
#else
    close(m_in_pair[0]);
#endif
  }

  if (m_out_pair[1] != INVALID_DESCRIPTOR) {
#ifdef _WIN32
    CloseHandle(ToHandle(m_out_pair[1]));
#else
    close(m_out_pair[1]);
#endif
  }

  m_in_pair[0] = INVALID_DESCRIPTOR;
  m_out_pair[1] = INVALID_DESCRIPTOR;
  return true;
}

bool PipeDescriptor::CloseClient() {
  if (m_out_pair[1] != INVALID_DESCRIPTOR) {
#ifdef _WIN32
    CloseHandle(ToHandle(m_out_pair[1]));
#else
    close(m_out_pair[1]);
#endif
  }

  m_out_pair[1] = INVALID_DESCRIPTOR;
  return true;
}

PipeDescriptor::PipeDescriptor(DescriptorHandle in_pair[2],
                               DescriptorHandle out_pair[2],
                               PipeDescriptor *other_end) {
  m_in_pair[0] = in_pair[0];
  m_in_pair[1] = in_pair[1];
  m_out_pair[0] = out_pair[0];
  m_out_pair[1] = out_pair[1];
  m_other_end = other_end;
}


// UnixSocket
// ------------------------------------------------

bool UnixSocket::Init() {
#ifdef _WIN32
  return false;
#else
  int pair[2];
  if ((m_handle != INVALID_DESCRIPTOR) || m_other_end)
    return false;

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) {
    OLA_WARN << "socketpair() failed, " << strerror(errno);
    return false;
  }

  m_handle = pair[0];
  SetReadNonBlocking();
  SetNoSigPipe(WriteDescriptor());
  m_other_end = new UnixSocket(pair[1], this);
  m_other_end->SetReadNonBlocking();
  return true;
#endif
}

UnixSocket *UnixSocket::OppositeEnd() {
  return m_other_end;
}

/*
 * Close this UnixSocket
 */
bool UnixSocket::Close() {
#ifdef _WIN32
  return true;
#else
  if (m_handle != INVALID_DESCRIPTOR) {
    close(m_handle);
  }

  m_handle = INVALID_DESCRIPTOR;
  return true;
#endif
}

bool UnixSocket::CloseClient() {
#ifndef _WIN32
  if (m_handle != INVALID_DESCRIPTOR)
    shutdown(m_handle, SHUT_WR);
#endif

  m_handle = INVALID_DESCRIPTOR;
  return true;
}

UnixSocket::UnixSocket(int socket, UnixSocket *other_end) {
#ifdef _WIN32
  m_handle.m_handle.m_fd = socket;
#else
  m_handle = socket;
#endif
  m_other_end = other_end;
}

// DeviceDescriptor
// ------------------------------------------------
DeviceDescriptor::DeviceDescriptor(int fd) {
#ifdef _WIN32
  m_handle.m_handle.m_fd = fd;
  m_handle.m_type = GENERIC_DESCRIPTOR;
#else
  m_handle = fd;
#endif
}

bool DeviceDescriptor::Close() {
  if (m_handle == INVALID_DESCRIPTOR)
    return true;

#ifdef _WIN32
  int ret = close(m_handle.m_handle.m_fd);
#else
  int ret = close(m_handle);
#endif
  m_handle = INVALID_DESCRIPTOR;
  return ret == 0;
}
}  // namespace io
}  // namespace ola
