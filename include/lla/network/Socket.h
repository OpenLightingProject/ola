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

#ifndef LLA_SOCKET_H
#define LLA_SOCKET_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

namespace lla {
namespace network {


/*
 * The base Socket class with the functionality required by the select server.
 * All other sockets inherit from this one.
 */
class Socket {
  public :
    virtual ~Socket() {};
    /*
     * Returns the read descriptor for this socket
     */
    virtual int ReadDescriptor() const = 0;
    /*
     * Used to check if the socket has been closed
     */
    virtual bool IsClosed() const = 0;
    /*
     * Close this socket
     */
    virtual bool Close() = 0;
    static const int INVALID_SOCKET = -1;
};


/*
 * A connected socket can be read from / written to.
 */
class ConnectedSocket: public Socket {
  public:
    ConnectedSocket() {}
    virtual ~ConnectedSocket() {}

    virtual int ReadDescriptor() const = 0;
    virtual int WriteDescriptor() const = 0;
    virtual bool Close() = 0;
    virtual bool IsClosed() const {
      if (ReadDescriptor() == INVALID_SOCKET)
        return true;

      return UnreadData() == 0;
    }
    virtual int UnreadData() const { return DataRemaining(ReadDescriptor()); }

    virtual ssize_t Send(const uint8_t *buffer, unsigned int size) {
      return FDSend(WriteDescriptor(), buffer, size);
    }
    virtual int Receive(uint8_t *buffer,
                        unsigned int size,
                        unsigned int &data_read) {
      return FDReceive(ReadDescriptor(), buffer, size, data_read);
    }
    virtual bool SetReadNonBlocking() { return SetNonBlocking(ReadDescriptor()); }

  protected:
    bool SetNonBlocking(int fd);
    int DataRemaining(int fd) const;
    ssize_t FDSend(int fd, const uint8_t *buffer, unsigned int size);
    int FDReceive(int fd,
                  uint8_t *buffer,
                  unsigned int size,
                  unsigned int &data_read);

    ConnectedSocket(const ConnectedSocket &other);
    ConnectedSocket& operator=(const ConnectedSocket &other);
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
    TcpSocket(int sd): ConnectedSocket(),
                       m_sd(sd) {}
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
    DeviceSocket(int fd):
      ConnectedSocket(),
      m_fd(fd) {}
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
 * A UdpSocket (non connected)
 */
class UdpSocket: public Socket {
  public:
    UdpSocket():
      Socket(),
      m_fd(INVALID_SOCKET),
      m_bound_to_port(false) {}
    ~UdpSocket() { Close(); }
    bool Init();
    bool Bind(unsigned short port=INADDR_ANY);
    bool Close();
    bool IsClosed() const { return m_fd == INVALID_SOCKET; }
    int ReadDescriptor() const { return m_fd; }
    ssize_t SendTo(const uint8_t *buffer,
                   unsigned int size,
                   const struct sockaddr_in &destination) const;
    ssize_t SendTo(const uint8_t *buffer,
                   unsigned int size,
                   const std::string &ip,
                   unsigned short port) const;
    bool RecvFrom(uint8_t *buffer,
                  ssize_t &data_read,
                  struct sockaddr_in &source,
                  socklen_t &src_size) const;
    bool EnableBroadcast();
    // JoinMulticast
  private:
    int m_fd;
    bool m_bound_to_port;
    UdpSocket(const UdpSocket &other);
    UdpSocket& operator=(const UdpSocket &other);
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
                       int backlog=10);
    ~TcpAcceptingSocket() { Close(); }
    bool Listen();
    int ReadDescriptor() const { return m_sd; }
    bool Close();
    bool IsClosed() const;
    ConnectedSocket *Accept();
  private:
    std::string m_address;
    unsigned short m_port;
    int m_sd, m_backlog;
    TcpAcceptingSocket(const TcpAcceptingSocket &other);
    TcpAcceptingSocket& operator=(const TcpAcceptingSocket &other);
};

} //network
} //lla
#endif

