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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Descriptor.h
 * The Descriptor classes
 * Copyright (C) 2005-2012 Simon Newton
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

static const int INVALID_DESCRIPTOR = -1;

/*
 * A FileDescriptor which can be read from.
 */
class ReadFileDescriptor {
  public:
    virtual ~ReadFileDescriptor() {}

    // Returns the read descriptor for this socket
    virtual int ReadDescriptor() const = 0;

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
    virtual int WriteDescriptor() const = 0;

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
 * simply allows a third-party sd to be registered with a callback.
 */
class UnmanagedFileDescriptor: public BidirectionalFileDescriptor {
  public :
    explicit UnmanagedFileDescriptor(int fd)
        : BidirectionalFileDescriptor(),
          m_fd(fd) {}
    ~UnmanagedFileDescriptor() {}
    int ReadDescriptor() const { return m_fd; }
    int WriteDescriptor() const { return m_fd; }
    // Closing is left to something else
    bool Close() { return true; }
  private:
    int m_fd;
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
                        unsigned int &data_read);

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

    static bool SetNonBlocking(int fd);

  protected:
    virtual bool IsSocket() const = 0;
    bool SetNoSigPipe(int fd);

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
      m_fd_pair[0] = INVALID_DESCRIPTOR;
      m_fd_pair[1] = INVALID_DESCRIPTOR;
    }
    ~LoopbackDescriptor() { Close(); }
    bool Init();
    int ReadDescriptor() const { return m_fd_pair[0]; }
    int WriteDescriptor() const { return m_fd_pair[1]; }
    bool Close();
    bool CloseClient();

  protected:
    bool IsSocket() const { return false; }

  private:
    int m_fd_pair[2];
    LoopbackDescriptor(const LoopbackDescriptor &other);
    LoopbackDescriptor& operator=(const LoopbackDescriptor &other);
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
    }
    ~PipeDescriptor() { Close(); }

    bool Init();
    PipeDescriptor *OppositeEnd();
    int ReadDescriptor() const { return m_in_pair[0]; }
    int WriteDescriptor() const { return m_out_pair[1]; }
    bool Close();
    bool CloseClient();

  protected:
    bool IsSocket() const { return false; }

  private:
    int m_in_pair[2];
    int m_out_pair[2];
    PipeDescriptor *m_other_end;
    PipeDescriptor(int in_pair[2], int out_pair[2], PipeDescriptor *other_end) {
      m_in_pair[0] = in_pair[0];
      m_in_pair[1] = in_pair[1];
      m_out_pair[0] = out_pair[0];
      m_out_pair[1] = out_pair[1];
      m_other_end = other_end;
    }
    PipeDescriptor(const PipeDescriptor &other);
    PipeDescriptor& operator=(const PipeDescriptor &other);
};


/*
 * A unix domain socket pair.
 */
class UnixSocket: public ConnectedDescriptor {
  public:
    UnixSocket():
      m_other_end(NULL) {
      m_fd = INVALID_DESCRIPTOR;
    }
    ~UnixSocket() { Close(); }

    bool Init();
    UnixSocket *OppositeEnd();
    int ReadDescriptor() const { return m_fd; }
    int WriteDescriptor() const { return m_fd; }
    bool Close();
    bool CloseClient();

  protected:
    bool IsSocket() const { return true; }

  private:
    int m_fd;
    UnixSocket *m_other_end;
    UnixSocket(int socket, UnixSocket *other_end) {
      m_fd = socket;
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
    explicit DeviceDescriptor(int fd): m_fd(fd) {}
    ~DeviceDescriptor() { Close(); }

    int ReadDescriptor() const { return m_fd; }
    int WriteDescriptor() const { return m_fd; }
    bool Close();

  protected:
    bool IsSocket() const { return false; }

  private:
    int m_fd;
    DeviceDescriptor(const DeviceDescriptor &other);
    DeviceDescriptor& operator=(const DeviceDescriptor &other);
};
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_DESCRIPTOR_H_
