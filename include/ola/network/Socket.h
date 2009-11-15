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
 */

#ifndef INCLUDE_OLA_NETWORK_SOCKET_H_
#define INCLUDE_OLA_NETWORK_SOCKET_H_

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <ola/Closure.h>  // NOLINT

namespace ola {
namespace network {


/*
 * The base Socket class with the functionality required by the select server.
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
    void SetOnData(ola::Closure *on_read) {
      if (m_on_read)
        delete m_on_read;
      m_on_read = on_read;
    }

    ola::Closure *OnData() const { return m_on_read; }

    static const int INVALID_SOCKET = -1;

  private:
    ola::Closure *m_on_read;
};


/*
 * A connected socket can be read from / written to.
 */
class ConnectedSocket: public Socket {
  public:
    ConnectedSocket(): m_on_close(NULL) {}
    virtual ~ConnectedSocket() {
      if (m_on_close)
        delete m_on_close;
    }

    virtual int ReadDescriptor() const = 0;
    virtual int WriteDescriptor() const = 0;

    virtual ssize_t Send(const uint8_t *buffer, unsigned int size) {
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

    /*
     * Used to check if the socket has been closed
     */
    bool CheckIfClosed() {
      if (IsClosed()) {
        if (m_on_close) {
          m_on_close->Run();
          m_on_close = NULL;
        }
        return true;
      }
      return false;
    }

    void SetOnClose(ola::SingleUseClosure *on_close) {
      if (m_on_close)
        delete m_on_close;
      m_on_close = on_close;
    }

  protected:
    virtual bool IsClosed() const;
    bool SetNonBlocking(int fd);
    ssize_t FDSend(int fd, const uint8_t *buffer, unsigned int size);
    int FDReceive(int fd,
                  uint8_t *buffer,
                  unsigned int size,
                  unsigned int &data_read);

    ConnectedSocket(const ConnectedSocket &other);
    ConnectedSocket& operator=(const ConnectedSocket &other);
  private:
    ola::SingleUseClosure *m_on_close;
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
    explicit TcpSocket(int sd): m_sd(sd) {}
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
 * An unmanaged socket is used to glue sockets from other software into the
 * SelectServer.
 */
class UnmanagedSocket: public Socket {
  public :
    explicit UnmanagedSocket(int sd): m_sd(sd) {}
    ~UnmanagedSocket() {}
    int ReadDescriptor() const { return m_sd; }
    // Closing is left to something else
    bool Close() { return true; }
  private:
    int m_sd;
    UnmanagedSocket(const UnmanagedSocket &other);
    UnmanagedSocket& operator=(const UnmanagedSocket &other);
};


/*
 * A UdpSocket (non connected)
 */
class UdpSocket: public Socket {
  public:
    UdpSocket(): m_fd(INVALID_SOCKET),
                 m_bound_to_port(false) {}
    ~UdpSocket() { Close(); }
    bool Init();
    bool Bind(unsigned short port = INADDR_ANY);
    bool Close();
    int ReadDescriptor() const { return m_fd; }
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
 * A server socket creates new Sockets when clients connect
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

