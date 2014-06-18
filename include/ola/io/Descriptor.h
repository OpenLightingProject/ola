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
 * Descriptor.h
 * The Descriptor classes
 * Copyright (C) 2005 Simon Newton
 *
 * This defines all the different types of file descriptors that can be used by
 * the SelectServer. At the top level, the ReadFileDescriptor /
 * WriteFileDescriptor interfaces provide the minimum functionality needed to
 * register a socket with the SelectServer to handle read / write events.
 * The BidirectionalFileDescriptor extends this interface to handle
 * both reading and writing.
 *
 * The UnmanagedFileDescriptor allows socket descriptors controller by other
 * libraries to be used with the SelectServer.
 *
 * ConnectedDescriptor is a socket with tighter intergration with the
 * SelectServer. This allows the SelectServer to detect when the socket is
 * closed and call the OnClose() handler. It also provides methods to disable
 * SIGPIPE, control the blocking attributes and check how much data remains to
 * be read.  ConnectedDescriptor has the following sub-classes:
 *
 *  - LoopbackDescriptor this socket is just a pipe(). Data written to the
 *    socket is available to be read.
 *  - PipeDescriptor allows a pair of sockets to be created. Data written to
 *    socket A is available at FileDescriptor B and visa versa.
 *  - UnixSocket, similar to PipeDescriptor but uses unix domain sockets.
 *  - DeviceDescriptor, this is a generic ConnectedDescriptor. It can be used
 *    with file descriptors to handle local devices.
 */

#ifndef INCLUDE_OLA_IO_DESCRIPTOR_H_
#define INCLUDE_OLA_IO_DESCRIPTOR_H_

#include <stdint.h>
#include <unistd.h>
#include <ola/Callback.h>
#include <ola/io/IOQueue.h>
#include <string>

namespace ola {
namespace io {

// The following section of code defines the various types and helper functions
// needed for the Descriptor infrastructure.
// On *nix, there's the "everything is a file" philosophy, so we can just use
// ints as handles.
// On Windows, the situation is more complicated, so we need to treat sockets,
// files, devices, pipes, etc. in special ways.
#ifdef _WIN32
// Internal use only. Semantic type of the descriptor.
enum DescriptorType {
  GENERIC_DESCRIPTOR = 0,  // Catch-all type without special handling
  SOCKET_DESCRIPTOR,  // WinSock socket
  PIPE_DESCRIPTOR  // Named Pipe handle
};

// Consider this to be an opaque type.
struct DescriptorHandle {
  // The actual OS handle
  union {
    int m_fd;
    void* m_handle;
  } m_handle;
  // Type of this descriptor's handle
  DescriptorType m_type;
  // Handler to an event for async I/O
  void* m_event_handle;
  // Pointer to read result of an async I/O call
  uint8_t* m_read_data;
  // Pointer to size of read result data
  uint32_t* m_read_data_size;

  DescriptorHandle()
      : m_type(GENERIC_DESCRIPTOR),
      m_event_handle(0),
      m_read_data(NULL),
      m_read_data_size(NULL)
    {
    m_handle.m_fd = -1;
  }
};

static DescriptorHandle INVALID_DESCRIPTOR;
static const size_t READ_DATA_BUFFER_SIZE = 1024;
bool operator!=(const DescriptorHandle &lhs, const DescriptorHandle &rhs);
bool operator==(const DescriptorHandle &lhs, const DescriptorHandle &rhs);
bool operator<(const DescriptorHandle &lhs, const DescriptorHandle &rhs);
std::ostream& operator<<(std::ostream &stream, const DescriptorHandle &data);
#else
typedef int DescriptorHandle;
static DescriptorHandle INVALID_DESCRIPTOR = -1;
#endif

/*
 * A FileDescriptor which can be read from.
 */
class ReadFileDescriptor {
 public:
  virtual ~ReadFileDescriptor() {}

  // Returns the read descriptor for this socket
  virtual DescriptorHandle ReadDescriptor() const = 0;

  // True if the read descriptor is valid
  bool ValidReadDescriptor() const {
    return ReadDescriptor() != INVALID_DESCRIPTOR;
  }

  // Called when there is data to be read from this fd
  virtual void PerformRead() = 0;
};


/*
 * A FileDescriptor which can be written to.
 */
class WriteFileDescriptor {
 public:
  virtual ~WriteFileDescriptor() {}
  virtual DescriptorHandle WriteDescriptor() const = 0;

  // True if the write descriptor is valid
  bool ValidWriteDescriptor() const {
    return WriteDescriptor() != INVALID_DESCRIPTOR;
  }

  // This is called when the socket is ready to be written to
  virtual void PerformWrite() = 0;
};


/*
 * A bi-directional file descriptor. This can be registered with the
 * SelectServer for both Read and Write events.
 */
class BidirectionalFileDescriptor: public ReadFileDescriptor,
                                   public WriteFileDescriptor {
 public :
  BidirectionalFileDescriptor(): m_on_read(NULL), m_on_write(NULL) {}
  virtual ~BidirectionalFileDescriptor() {
    if (m_on_read)
      delete m_on_read;

    if (m_on_write)
      delete m_on_write;
  }

  // Set the OnData closure
  void SetOnData(ola::Callback0<void> *on_read) {
    if (m_on_read)
      delete m_on_read;
    m_on_read = on_read;
  }

  // Set the OnWrite closure
  void SetOnWritable(ola::Callback0<void> *on_write) {
    if (m_on_write)
      delete m_on_write;
    m_on_write = on_write;
  }

  void PerformRead();
  void PerformWrite();

 private:
  ola::Callback0<void> *m_on_read;
  ola::Callback0<void> *m_on_write;
};


/*
 * An UnmanagedFileDescriptor allows file descriptors from other software to
 * use the SelectServer. This class doesn't define any read/write methods, it
 * simply allows a third-party fd to be registered with a callback.
 */
class UnmanagedFileDescriptor: public BidirectionalFileDescriptor {
 public :
  explicit UnmanagedFileDescriptor(int fd);
  ~UnmanagedFileDescriptor() {}
  DescriptorHandle ReadDescriptor() const { return m_handle; }
  DescriptorHandle WriteDescriptor() const { return m_handle; }
  // Closing is left to something else
  bool Close() { return true; }

 private:
  DescriptorHandle m_handle;
  UnmanagedFileDescriptor(const UnmanagedFileDescriptor &other);
  UnmanagedFileDescriptor& operator=(const UnmanagedFileDescriptor &other);
};


// Comparison operation.
struct UnmanagedFileDescriptor_lt {
  bool operator()(const ola::io::UnmanagedFileDescriptor *d1,
                  const ola::io::UnmanagedFileDescriptor *d2) const {
    return d1->ReadDescriptor() < d2->ReadDescriptor();
  }
};


/*
 * A ConnectedDescriptor is a BidirectionalFileDescriptor that also generates
 * notifications when it's closed.
 */
class ConnectedDescriptor: public BidirectionalFileDescriptor {
 public:
  ConnectedDescriptor(): BidirectionalFileDescriptor(), m_on_close(NULL) {}
  virtual ~ConnectedDescriptor() {
    if (m_on_close)
      delete m_on_close;
  }

  virtual ssize_t Send(const uint8_t *buffer, unsigned int size);
  virtual ssize_t Send(IOQueue *data);

  virtual int Receive(uint8_t *buffer,
                      unsigned int size,
                      unsigned int &data_read);  // NOLINT

  virtual bool SetReadNonBlocking() {
    return SetNonBlocking(ReadDescriptor());
  }

  virtual bool Close() = 0;
  int DataRemaining() const;
  bool IsClosed() const;

  typedef ola::SingleUseCallback0<void> OnCloseCallback;

  // Set the OnClose closure
  void SetOnClose(OnCloseCallback *on_close) {
    if (m_on_close)
      delete m_on_close;
    m_on_close = on_close;
  }

  /**
   * This is a special method which transfers ownership of the on close
   * handler away from the socket. Often when an on_close callback runs we
   * want to delete the socket that it's bound to. This causes problems
   * because we can't tell the difference between a normal deletion and a
   * deletion triggered by a close, and the latter causes the callback to be
   * deleted while it's running. To avoid this we we want to call the on
   * close handler we transfer ownership away from the socket so doesn't need
   * to delete the running handler.
   */
  OnCloseCallback *TransferOnClose() {
    OnCloseCallback *on_close = m_on_close;
    m_on_close = NULL;
    return on_close;
  }

  static bool SetNonBlocking(DescriptorHandle fd);

 protected:
  virtual bool IsSocket() const = 0;
  bool SetNoSigPipe(DescriptorHandle fd);

  ConnectedDescriptor(const ConnectedDescriptor &other);
  ConnectedDescriptor& operator=(const ConnectedDescriptor &other);

 private:
  OnCloseCallback *m_on_close;
};


/*
 * A loopback socket.
 * Everything written is available for reading.
 */
class LoopbackDescriptor: public ConnectedDescriptor {
 public:
  LoopbackDescriptor() {
    m_handle_pair[0] = INVALID_DESCRIPTOR;
    m_handle_pair[1] = INVALID_DESCRIPTOR;
#ifdef _WIN32
    memset(m_read_data, 0, READ_DATA_BUFFER_SIZE);
    m_read_data_size = 0;
#endif
  }
  ~LoopbackDescriptor() { Close(); }
  bool Init();
  DescriptorHandle ReadDescriptor() const { return m_handle_pair[0]; }
  DescriptorHandle WriteDescriptor() const { return m_handle_pair[1]; }
  bool Close();
  bool CloseClient();

 protected:
  bool IsSocket() const { return false; }

 private:
  DescriptorHandle m_handle_pair[2];
  LoopbackDescriptor(const LoopbackDescriptor &other);
  LoopbackDescriptor& operator=(const LoopbackDescriptor &other);
#ifdef _WIN32
  uint8_t m_read_data[READ_DATA_BUFFER_SIZE];
  uint32_t m_read_data_size;
#endif
};


/*
 * A descriptor that uses unix pipes. You can get the 'other end' of the
 * PipeDescriptor by calling OppositeEnd().
 */
class PipeDescriptor: public ConnectedDescriptor {
 public:
  PipeDescriptor():
    m_other_end(NULL) {
    m_in_pair[0] = m_in_pair[1] = INVALID_DESCRIPTOR;
    m_out_pair[0] = m_out_pair[1] = INVALID_DESCRIPTOR;
#ifdef _WIN32
    memset(m_read_data, 0, READ_DATA_BUFFER_SIZE);
    m_read_data_size = 0;
#endif
  }
  ~PipeDescriptor() { Close(); }

  bool Init();
  PipeDescriptor *OppositeEnd();
  DescriptorHandle ReadDescriptor() const { return m_in_pair[0]; }
  DescriptorHandle WriteDescriptor() const { return m_out_pair[1]; }
  bool Close();
  bool CloseClient();

 protected:
  bool IsSocket() const { return false; }

 private:
  DescriptorHandle m_in_pair[2];
  DescriptorHandle m_out_pair[2];
  PipeDescriptor *m_other_end;
  PipeDescriptor(DescriptorHandle in_pair[2],
                 DescriptorHandle out_pair[2],
                 PipeDescriptor *other_end) {
    m_in_pair[0] = in_pair[0];
    m_in_pair[1] = in_pair[1];
    m_out_pair[0] = out_pair[0];
    m_out_pair[1] = out_pair[1];
    m_other_end = other_end;
#ifdef _WIN32
    m_in_pair[0].m_read_data = m_read_data;
    m_in_pair[0].m_read_data_size = &m_read_data_size;
#endif
  }
  PipeDescriptor(const PipeDescriptor &other);
  PipeDescriptor& operator=(const PipeDescriptor &other);
#ifdef _WIN32
  uint8_t m_read_data[READ_DATA_BUFFER_SIZE];
  uint32_t m_read_data_size;
#endif
};

/*
 * A unix domain socket pair.
 */
class UnixSocket: public ConnectedDescriptor {
 public:
  UnixSocket():
    m_other_end(NULL) {
    m_handle = INVALID_DESCRIPTOR;
  }
  ~UnixSocket() { Close(); }

  bool Init();
  UnixSocket *OppositeEnd();
  DescriptorHandle ReadDescriptor() const { return m_handle; }
  DescriptorHandle WriteDescriptor() const { return m_handle; }
  bool Close();
  bool CloseClient();

 protected:
  bool IsSocket() const { return true; }

 private:
  DescriptorHandle m_handle;
  UnixSocket *m_other_end;
  UnixSocket(int socket, UnixSocket *other_end) {
#ifdef _WIN32
    m_handle.m_handle.m_fd = socket;
#else
    m_handle = socket;
#endif
    m_other_end = other_end;
  }
  UnixSocket(const UnixSocket &other);
  UnixSocket& operator=(const UnixSocket &other);
};

/*
 * A descriptor which represents a connection to a device
 */
class DeviceDescriptor: public ConnectedDescriptor {
 public:
  explicit DeviceDescriptor(int fd);
  ~DeviceDescriptor() { Close(); }

  DescriptorHandle ReadDescriptor() const { return m_handle; }
  DescriptorHandle WriteDescriptor() const { return m_handle; }
  bool Close();

 protected:
  bool IsSocket() const { return false; }

 private:
  DescriptorHandle m_handle;
  DeviceDescriptor(const DeviceDescriptor &other);
  DeviceDescriptor& operator=(const DeviceDescriptor &other);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_DESCRIPTOR_H_
