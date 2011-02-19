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
 * Socket.h
 * The Socket interface
 * Copyright (C) 2005-2009 Simon Newton
 *
 * This defines all the different types of sockets that can be used by the
 * SelectServer. At the top level, the Socket interface provides the minimum
 * functionality needed to register a socket with the SelectServer to handle
 * read events. The BidirectionalSocket extends this interface to handle
 * ready-to-write events as well.
 *
 * The UnmanagedSocket allows socket descriptors controller by other libraries
 * to be used with the SelectServer.
 *
 * ConnectedSocket is a socket with tighter intergration with the SelectServer.
 * This allows the SelectServer to detect when the socket is closed and call
 * the OnClose() handler. It also provides methods to disable SIGPIPE, control
 * the blocking attributes and check how much data remains to be read.
 * ConnectedSocket has the following sub-classes:
 *
 *  - LoopbackSocket this socket is just a pipe(). Data written to the socket
 *  is available to be read
 *  - PipeSocket allows a pair of sockets to be created. Data written to socket
 *  A is available at Socket B and visa versa.
 *  - TcpSocket, this represents a TCP connection to a remote endpoint
 *  - DeviceSocket, this is a generic ConnectedSocket. It can be used with file
 *    descriptors to handle local devices.
 *
 * AcceptingSocket is the interface that defines sockets which can spawn new
 * Socket. TcpAcceptingSocket is the only subclass and provides the accept()
 * functionality.
 */

#ifndef INCLUDE_OLA_NETWORK_SOCKET_H_
#define INCLUDE_OLA_NETWORK_SOCKET_H_

#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <string>
#include <ola/Callback.h>  // NOLINT


namespace ola {
namespace network {


/*
 * The base Socket Interface with the functionality required by the select
 * server. This provides just enough functionality to be registered with the
 * SelectServer, and a callback to be invoked when data is available.
 * All other sockets inherit from this one.
 */
class Socket {
  public :
    Socket(): m_on_read(NULL) {}
    virtual ~Socket() {
      if (m_on_read)
        delete m_on_read;
    }

    /*
     * Returns the read descriptor for this socket
     */
    virtual int ReadDescriptor() const = 0;

    /*
     * Close this socket
     */
    virtual bool Close() = 0;

    /*
     * Set the OnData closure
     */
    void SetOnData(ola::Callback0<void> *on_read) {
      if (m_on_read)
        delete m_on_read;
      m_on_read = on_read;
    }

    ola::Callback0<void> *OnData() const { return m_on_read; }

    static const int INVALID_SOCKET = -1;

  private:
    ola::Callback0<void> *m_on_read;
};


/*
 * A bi-directional socket. This can be registered with the SelectServer for
 * both Read and Write events.
 */
class BidirectionalSocket: public Socket {
  public :
    BidirectionalSocket(): Socket(), m_on_write(NULL) {}
    virtual ~BidirectionalSocket() {
      if (m_on_write)
        delete m_on_write;
    }

    virtual int WriteDescriptor() const = 0;

    /*
     * Set the OnWrite closure
     */
    void SetOnWritable(ola::Callback0<void> *on_write) {
      if (m_on_write)
        delete m_on_write;
      m_on_write = on_write;
    }

    /*
     * This is called when the socket is ready to be written to
     */
    ola::Callback0<void> *PerformWrite() const { return m_on_write; }

  private:
    ola::Callback0<void> *m_on_write;
};


/*
 * An unmanaged socket is used to glue sockets from other software into the
 * SelectServer. This class doesn't define any read/write methods, it simply
 * allows a third-party sd to be registered with a callback.
 */
class UnmanagedSocket: public BidirectionalSocket {
  public :
    explicit UnmanagedSocket(int sd): BidirectionalSocket(), m_sd(sd) {}
    ~UnmanagedSocket() {}
    int ReadDescriptor() const { return m_sd; }
    int WriteDescriptor() const { return m_sd; }
    // Closing is left to something else
    bool Close() { return true; }
  private:
    int m_sd;
    UnmanagedSocket(const UnmanagedSocket &other);
    UnmanagedSocket& operator=(const UnmanagedSocket &other);
};



/*
 * A connected socket can be read from / written to.
 */
class ConnectedSocket: public BidirectionalSocket {
  public:
    ConnectedSocket(): BidirectionalSocket(), m_on_close(NULL) {}
    virtual ~ConnectedSocket() {
      if (m_on_close)
        delete m_on_close;
    }

    virtual ssize_t Send(const uint8_t *buffer, unsigned int size) const {
      return FDSend(WriteDescriptor(), buffer, size);
    }

    virtual int Receive(uint8_t *buffer,
                        unsigned int size,
                        unsigned int &data_read) {
      return FDReceive(ReadDescriptor(), buffer, size, data_read);
    }

    virtual bool SetReadNonBlocking() {
      return SetNonBlocking(ReadDescriptor());
    }

    virtual bool Close() = 0;

    int DataRemaining() const;

    bool CheckIfInvalid() {
      if (ReadDescriptor() == INVALID_SOCKET) {
        if (m_on_close) {
          m_on_close->Run();
          m_on_close = NULL;
        }
        return true;
      }
      return false;
    }

    /*
     * Used to check if the socket has been closed
     */
    bool CheckIfActive() {
      if (IsClosed()) {
        if (m_on_close) {
          m_on_close->Run();
          m_on_close = NULL;
        }
        return true;
      }
      return false;
    }

    /*
     * Set the OnClose closure
     */
    void SetOnClose(ola::SingleUseCallback0<void> *on_close) {
      if (m_on_close)
        delete m_on_close;
      m_on_close = on_close;
    }

  protected:
    virtual bool IsClosed() const;
    bool SetNonBlocking(int fd);
    bool SetNoSigPipe(int fd);
    ssize_t FDSend(int fd, const uint8_t *buffer, unsigned int size) const;
    int FDReceive(int fd,
                  uint8_t *buffer,
                  unsigned int size,
                  unsigned int &data_read);

    ConnectedSocket(const ConnectedSocket &other);
    ConnectedSocket& operator=(const ConnectedSocket &other);
  private:
    ola::SingleUseCallback0<void> *m_on_close;
};


/*
 * A loopback socket.
 * Everything written is available for reading.
 */
class LoopbackSocket: public ConnectedSocket {
  public:
    LoopbackSocket() {
      m_fd_pair[0] = INVALID_SOCKET;
      m_fd_pair[1] = INVALID_SOCKET;
    }
    ~LoopbackSocket() { Close(); }
    bool Init();
    int ReadDescriptor() const { return m_fd_pair[0]; }
    int WriteDescriptor() const { return m_fd_pair[1]; }
    bool Close();
    bool CloseClient();

  private:
    int m_fd_pair[2];
    LoopbackSocket(const LoopbackSocket &other);
    LoopbackSocket& operator=(const LoopbackSocket &other);
};


/*
 * A pipe socket. You can get the 'other end' of the Socket with OppositeEnd()
 */
class PipeSocket: public ConnectedSocket {
  public:
    PipeSocket():
      m_other_end(NULL) {
      m_in_pair[0] = m_in_pair[1] = INVALID_SOCKET;
      m_out_pair[0] = m_out_pair[1] = INVALID_SOCKET;
    }
    ~PipeSocket() { Close(); }

    bool Init();
    PipeSocket *OppositeEnd();
    int ReadDescriptor() const { return m_in_pair[0]; }
    int WriteDescriptor() const { return m_out_pair[1]; }
    bool Close();
    bool CloseClient();

  private:
    int m_in_pair[2];
    int m_out_pair[2];
    PipeSocket *m_other_end;
    PipeSocket(int in_pair[2], int out_pair[2], PipeSocket *other_end) {
      m_in_pair[0] = in_pair[0];
      m_in_pair[1] = in_pair[1];
      m_out_pair[0] = out_pair[0];
      m_out_pair[1] = out_pair[1];
      m_other_end = other_end;
    }
    PipeSocket(const PipeSocket &other);
    PipeSocket& operator=(const PipeSocket &other);
};


/*
 * A TcpSocket
 */
class TcpSocket: public ConnectedSocket {
  public:
    explicit TcpSocket(int sd): m_sd(sd) {
      SetNoSigPipe(sd);
    }
    ~TcpSocket() { Close(); }

    int ReadDescriptor() const { return m_sd; }
    int WriteDescriptor() const { return m_sd; }
    bool Close();

    static TcpSocket* Connect(const std::string &ip_address,
                              unsigned short port);
  private:
    int m_sd;
    TcpSocket(const TcpSocket &other);
    TcpSocket& operator=(const TcpSocket &other);
};


/*
 * A socket which represents a connection to a device
 */
class DeviceSocket: public ConnectedSocket {
  public:
    explicit DeviceSocket(int fd): m_fd(fd) {}
    ~DeviceSocket() { Close(); }

    int ReadDescriptor() const { return m_fd; }
    int WriteDescriptor() const { return m_fd; }
    bool Close();
  private:
    int m_fd;
    DeviceSocket(const DeviceSocket &other);
    DeviceSocket& operator=(const DeviceSocket &other);
};


/*
 * The UdpSocketInterface.
 * This is done as an Interface so we can mock it out for testing.
 */
class UdpSocketInterface: public BidirectionalSocket {
  public:
    UdpSocketInterface(): BidirectionalSocket() {}
    ~UdpSocketInterface() {}
    virtual bool Init() = 0;
    virtual bool Bind(unsigned short port = INADDR_ANY) = 0;
    virtual bool Close() = 0;
    virtual int ReadDescriptor() const = 0;
    virtual int WriteDescriptor() const = 0;
    virtual ssize_t SendTo(const uint8_t *buffer,
                           unsigned int size,
                           const struct sockaddr_in &destination) const = 0;
    virtual ssize_t SendTo(const uint8_t *buffer,
                           unsigned int size,
                           const std::string &ip,
                           unsigned short port) const = 0;
    virtual bool RecvFrom(uint8_t *buffer,
                          ssize_t *data_read,
                          struct sockaddr_in &source,
                          socklen_t &src_size) const = 0;
    virtual bool RecvFrom(uint8_t *buffer, ssize_t *data_read) const = 0;
    virtual bool EnableBroadcast() = 0;
    virtual bool SetMulticastInterface(const struct in_addr &interface) = 0;
    virtual bool JoinMulticast(const struct in_addr &interface,
                               const struct in_addr &group,
                               bool loop = false) = 0;
    virtual bool JoinMulticast(const struct in_addr &interface,
                               const std::string &address,
                               bool loop = false) = 0;
    virtual bool LeaveMulticast(const struct in_addr &interface,
                                const struct in_addr &group) = 0;
    virtual bool LeaveMulticast(const struct in_addr &interface,
                                const std::string &address) = 0;

    virtual bool SetTos(uint8_t tos) = 0;

  private:
    UdpSocketInterface(const UdpSocketInterface &other);
    UdpSocketInterface& operator=(const UdpSocketInterface &other);
};


/*
 * A UdpSocket (non connected)
 */
class UdpSocket: public UdpSocketInterface {
  public:
    UdpSocket(): UdpSocketInterface(),
                 m_fd(INVALID_SOCKET),
                 m_bound_to_port(false) {}
    ~UdpSocket() { Close(); }
    bool Init();
    bool Bind(unsigned short port = INADDR_ANY);
    bool Close();
    int ReadDescriptor() const { return m_fd; }
    int WriteDescriptor() const { return m_fd; }
    ssize_t SendTo(const uint8_t *buffer,
                   unsigned int size,
                   const struct sockaddr_in &destination) const;
    ssize_t SendTo(const uint8_t *buffer,
                   unsigned int size,
                   const std::string &ip,
                   unsigned short port) const;
    bool RecvFrom(uint8_t *buffer,
                  ssize_t *data_read,
                  struct sockaddr_in &source,
                  socklen_t &src_size) const;
    bool RecvFrom(uint8_t *buffer, ssize_t *data_read) const;
    bool EnableBroadcast();
    bool SetMulticastInterface(const struct in_addr &interface);
    bool JoinMulticast(const struct in_addr &interface,
                       const struct in_addr &group,
                       bool loop = false);
    bool JoinMulticast(const struct in_addr &interface,
                       const std::string &address,
                       bool loop = false);
    bool LeaveMulticast(const struct in_addr &interface,
                        const struct in_addr &group);
    bool LeaveMulticast(const struct in_addr &interface,
                        const std::string &address);

    bool SetTos(uint8_t tos);

  private:
    int m_fd;
    bool m_bound_to_port;
    UdpSocket(const UdpSocket &other);
    UdpSocket& operator=(const UdpSocket &other);
    bool _RecvFrom(uint8_t *buffer,
                   ssize_t *data_read,
                   struct sockaddr_in *source,
                   socklen_t *src_size) const;
};


/*
 * A Socket creates new Sockets when clients connect.
 */
class AcceptingSocket: public Socket {
  public:
    AcceptingSocket() {}
    virtual bool Listen() = 0;
    virtual bool Close() = 0;
    virtual ConnectedSocket* Accept() = 0;
};


/*
 * A TCP accepting socket
 */
class TcpAcceptingSocket: public AcceptingSocket {
  public:
    TcpAcceptingSocket(const std::string &address, unsigned short port,
                       int backlog = 10);
    ~TcpAcceptingSocket() { Close(); }
    bool Listen();
    int ReadDescriptor() const { return m_sd; }
    bool Close();
    ConnectedSocket *Accept();
  private:
    std::string m_address;
    uint16_t m_port;
    int m_sd, m_backlog;
    TcpAcceptingSocket(const TcpAcceptingSocket &other);
    TcpAcceptingSocket& operator=(const TcpAcceptingSocket &other);
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_SOCKET_H_

