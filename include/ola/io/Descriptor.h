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
 */

#ifndef INCLUDE_OLA_IO_DESCRIPTOR_H_
#define INCLUDE_OLA_IO_DESCRIPTOR_H_

#include <stdint.h>
#include <unistd.h>
#include <ola/Callback.h>
#include <ola/base/Macro.h>
#include <ola/io/IOQueue.h>
#include <string>

namespace ola {
namespace io {

/**
 * @defgroup io I/O & Event Management
 * @brief The core event management system.
 * @details
 * This defines all the different types of file descriptors that can be used by
 * the SelectServer. At the top level, the ReadFileDescriptor /
 * WriteFileDescriptor interfaces provide the minimum functionality needed to
 * register a socket with the SelectServer to handle read / write events.
 * The BidirectionalFileDescriptor extends this interface to handle
 * both reading and writing.
 *
 * The UnmanagedFileDescriptor allows socket descriptors created by other
 * libraries to be used with the SelectServer.
 *
 * ConnectedDescriptor is a socket with tighter integration with the
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

/**
 * @addtogroup io
 * @{
 * @file ola/io/Descriptor.h
 * @}
 */

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
  void* m_event;
  // Pointer to read result of an async I/O call
  uint8_t* m_async_data;
  // Pointer to size of read result data
  uint32_t* m_async_data_size;

  DescriptorHandle();
  ~DescriptorHandle();

  bool AllocAsyncBuffer();
  void FreeAsyncBuffer();

  bool IsValid() const;
};

void* ToHandle(const DescriptorHandle &handle);

static DescriptorHandle INVALID_DESCRIPTOR;
static const uint32_t ASYNC_DATA_BUFFER_SIZE = 1024;
bool operator!=(const DescriptorHandle &lhs, const DescriptorHandle &rhs);
bool operator==(const DescriptorHandle &lhs, const DescriptorHandle &rhs);
bool operator<(const DescriptorHandle &lhs, const DescriptorHandle &rhs);
std::ostream& operator<<(std::ostream &stream, const DescriptorHandle &data);
#else
typedef int DescriptorHandle;
static DescriptorHandle INVALID_DESCRIPTOR = -1;
#endif  // _WIN32

/**
 * @addtogroup io
 * @{
 */
/**
 * Helper function to convert a DescriptorHandle to a file descriptor.
 * @param handle The descriptor handle
 * @return -1 on error, file descriptor otherwise
 */
int ToFD(const DescriptorHandle& handle);

/*
 * A FileDescriptor which can be read from.
 */

/**
 * @brief Represents a file descriptor that supports reading data.
 */
class ReadFileDescriptor {
 public:
  virtual ~ReadFileDescriptor() {}

  /**
   * @brief Returns the read descriptor for this socket.
   * @returns the DescriptorHandle for this descriptor.
   */
  virtual DescriptorHandle ReadDescriptor() const = 0;

  /**
   * @brief Check if this file descriptor is valid.
   * @return true if the read descriptor is valid.
   */
  bool ValidReadDescriptor() const {
    return ReadDescriptor() != INVALID_DESCRIPTOR;
  }

  /**
   * @brief Called when there is data available on the descriptor.
   *
   * This is usually called by the SelectServer.
   */
  virtual void PerformRead() = 0;
};


/**
 * @brief Represents a file descriptor that supports writing data.
 */
class WriteFileDescriptor {
 public:
  virtual ~WriteFileDescriptor() {}

  /**
   * @brief Returns the write descriptor for this socket.
   * @returns the DescriptorHandle for this descriptor.
   */
  virtual DescriptorHandle WriteDescriptor() const = 0;

  /**
   * @brief Check if this file descriptor is valid.
   * @return true if the write descriptor is valid.
   */
  bool ValidWriteDescriptor() const {
    return WriteDescriptor() != INVALID_DESCRIPTOR;
  }

  /**
   * @brief Called when the descriptor can be written to.
   *
   * This is usually called by the SelectServer.
   */
  virtual void PerformWrite() = 0;
};


/**
 * @brief A file descriptor that supports both read & write.
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

  /**
   * @brief Set the callback to be run when data is available for reading.
   * @param on_read the callback to run, ownership of the callback is
   *   transferred.
   */
  void SetOnData(ola::Callback0<void> *on_read) {
    if (m_on_read)
      delete m_on_read;
    m_on_read = on_read;
  }

  /**
   * @brief Set the callback to be run when the descriptor can be written to.
   * @param on_write the callback to run, ownership of the callback is
   *   transferred.
   */
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


/**
 * @brief Allows a FD created by a library to be used with the SelectServer.
 */
class UnmanagedFileDescriptor: public BidirectionalFileDescriptor {
 public :
  /**
   * @brief Create a new UnmanagedFileDescriptor.
   * @param fd the file descriptor to use
   */
  explicit UnmanagedFileDescriptor(int fd);
  ~UnmanagedFileDescriptor() {}
  DescriptorHandle ReadDescriptor() const { return m_handle; }
  DescriptorHandle WriteDescriptor() const { return m_handle; }

 protected:
  // This is only protected because WIN32-specific subclasses need access.
  DescriptorHandle m_handle;

 private:
  DISALLOW_COPY_AND_ASSIGN(UnmanagedFileDescriptor);
};


/**
 * @brief Comparison operator for UnmanagedFileDescriptor.
 */
struct UnmanagedFileDescriptor_lt {
  bool operator()(const ola::io::UnmanagedFileDescriptor *d1,
                  const ola::io::UnmanagedFileDescriptor *d2) const {
    return d1->ReadDescriptor() < d2->ReadDescriptor();
  }
};


/**
 * @brief A BidirectionalFileDescriptor that also generates notifications when
 * closed.
 */
class ConnectedDescriptor: public BidirectionalFileDescriptor {
 public:
  typedef ola::SingleUseCallback0<void> OnCloseCallback;

  ConnectedDescriptor(): BidirectionalFileDescriptor(), m_on_close(NULL) {}

  virtual ~ConnectedDescriptor() {
    if (m_on_close)
      delete m_on_close;
  }

  /**
   * @brief Write a buffer to the descriptor.
   * @param buffer a pointer to the buffer to write
   * @param size the number of bytes in the buffer to write
   * @return the number of bytes written
   */
  virtual ssize_t Send(const uint8_t *buffer, unsigned int size);

  /**
   * @brief Write data from an IOQueue to a descriptor.
   * @param data the IOQueue containing the data to write. Data written to the
   * descriptor will be removed from the IOQueue.
   * @return the number of bytes written.
   *
   * This attempts to send as much of the IOQueue data as possible. The IOQueue
   * may be non-empty when this completes if the descriptor buffer is full.
   * @returns the number of bytes sent.
   */
  virtual ssize_t Send(IOQueue *data);


  /**
   * @brief Read data from this descriptor.
   * @param buffer a pointer to the buffer to store new data in
   * @param size the size of the buffer
   * @param data_read a value result argument which returns the size of the data
   * copied into the buffer.
   * @returns -1 on error, 0 on success.
   */
  virtual int Receive(uint8_t *buffer,
                      unsigned int size,
                      unsigned int &data_read);  // NOLINT(runtime/references)

  /**
   * @brief Enable on non-blocking reads..
   * @return true if it worked, false otherwise.
   *
   * On Windows, this is only supported for sockets.
   */
  virtual bool SetReadNonBlocking() {
    return SetNonBlocking(ReadDescriptor());
  }

  virtual bool Close() = 0;

  /**
   * @brief Find out how much data is left to read
   * @return the amount of unread data for the descriptor.
   */
  int DataRemaining() const;

  /**
   * @brief Check if the descriptor is closed.
   */
  bool IsClosed() const;

  /**
   * @brief Set the callback to be run when the descriptor is closed.
   * @param on_close the callback to run, ownership of the callback is
   *   transferred.
   */
  void SetOnClose(OnCloseCallback *on_close) {
    if (m_on_close)
      delete m_on_close;
    m_on_close = on_close;
  }

  /**
   * @brief Take ownership of the on_close callback.
   *
   * This method transfers ownership of the on_close callback from the socket
   * to the caller. Often an on_close callback ends up deleting the socket that
   * its bound to. This can cause problems because we run the destructor from
   * within the Close() method of the same object. To avoid this when we want
   * to call the on close handler we transfer ownership away from the socket
   */
  OnCloseCallback *TransferOnClose() {
    OnCloseCallback *on_close = m_on_close;
    m_on_close = NULL;
    return on_close;
  }

  /**
   * @brief Set a DescriptorHandle to non-blocking mode.
   */
  static bool SetNonBlocking(DescriptorHandle fd);

 protected:
  virtual bool IsSocket() const = 0;

  /**
   * @brief Disable SIGPIPE for this descriptor.
   */
  bool SetNoSigPipe(DescriptorHandle fd);

 private:
  OnCloseCallback *m_on_close;
};


/**
 * @brief A loopback descriptor.
 *
 * Everything written is available for reading.
 */
class LoopbackDescriptor: public ConnectedDescriptor {
 public:
  LoopbackDescriptor();
  ~LoopbackDescriptor() { Close(); }

  /**
   * @brief Setup this loopback descriptor.
   * @return true if initialization succeeded, false if it failed.
   */
  bool Init();
  DescriptorHandle ReadDescriptor() const { return m_handle_pair[0]; }
  DescriptorHandle WriteDescriptor() const { return m_handle_pair[1]; }

  /**
   * @brief Close the loopback descriptor.
   * @return true if close succeeded, false otherwise
   */
  bool Close();

  /**
   * @brief Close the write portion of the loopback descriptor.
   * @return true if close succeeded, false otherwise
   */
  bool CloseClient();

 protected:
  bool IsSocket() const { return false; }

 private:
  DescriptorHandle m_handle_pair[2];


  DISALLOW_COPY_AND_ASSIGN(LoopbackDescriptor);
};


/**
 * @brief A descriptor that uses unix pipes.
 *
 * You can get the 'other end' of the PipeDescriptor by calling OppositeEnd().
 */
class PipeDescriptor: public ConnectedDescriptor {
 public:
  PipeDescriptor();
  ~PipeDescriptor() { Close(); }

  /**
   * @brief Initialize the PipeDescriptor.
   */
  bool Init();

  /**
   * @brief Fetch the other end of the PipeDescriptor.
   *
   * @returns A new PipeDescriptor or NULL if the descriptor wasn't initialized
   * correctly. Its an error to call this more than once. Ownership of the
   * returned PipeDescriptor is transferred to the caller.
   */
  PipeDescriptor *OppositeEnd();
  DescriptorHandle ReadDescriptor() const { return m_in_pair[0]; }
  DescriptorHandle WriteDescriptor() const { return m_out_pair[1]; }

  /**
   * @brief Close this PipeDescriptor
   */
  bool Close();

  /**
   * @brief Close the write portion of this PipeDescriptor
   */
  bool CloseClient();

 protected:
  bool IsSocket() const { return false; }

 private:
  DescriptorHandle m_in_pair[2];
  DescriptorHandle m_out_pair[2];
  PipeDescriptor *m_other_end;

  PipeDescriptor(DescriptorHandle in_pair[2],
                 DescriptorHandle out_pair[2],
                 PipeDescriptor *other_end);


  DISALLOW_COPY_AND_ASSIGN(PipeDescriptor);
};

/**
 * @brief A unix domain socket pair.
 */
class UnixSocket: public ConnectedDescriptor {
 public:
  UnixSocket():
    m_other_end(NULL) {
    m_handle = INVALID_DESCRIPTOR;
  }
  ~UnixSocket() { Close(); }

  /**
   * @brief Initialize the UnixSocket.
   */
  bool Init();

  /**
   * @brief Fetch the other end of the unix socket.
   *
   * @returns A new UnixSocket or NULL if the socket wasn't initialized
   * correctly. Its an error to call this more than once. Ownership of the
   * returned UnixSocket is transferred to the caller.
   */
  UnixSocket *OppositeEnd();
  DescriptorHandle ReadDescriptor() const { return m_handle; }
  DescriptorHandle WriteDescriptor() const { return m_handle; }

  /**
   * @brief Close this UnixSocket
   */
  bool Close();


  /**
   * @brief Close the write portion of this UnixSocket.
   */
  bool CloseClient();

 protected:
  bool IsSocket() const { return true; }

 private:
  DescriptorHandle m_handle;
  UnixSocket *m_other_end;
  UnixSocket(int socket, UnixSocket *other_end);

  DISALLOW_COPY_AND_ASSIGN(UnixSocket);
};

/**
 * @brief A descriptor which represents a connection to a device.
 */
class DeviceDescriptor: public ConnectedDescriptor {
 public:
  /**
   * @brief Create a new DeviceDescriptor.
   * @param fd the file descriptor to use
   */
  explicit DeviceDescriptor(int fd);
  ~DeviceDescriptor() { Close(); }

  DescriptorHandle ReadDescriptor() const { return m_handle; }
  DescriptorHandle WriteDescriptor() const { return m_handle; }

  /**
   * @brief Close this DeviceDescriptor
   */
  bool Close();

 protected:
  bool IsSocket() const { return false; }

 private:
  DescriptorHandle m_handle;

  DISALLOW_COPY_AND_ASSIGN(DeviceDescriptor);
};

/**@}*/
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_DESCRIPTOR_H_
