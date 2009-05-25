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
 * Copyright (C) 2005-2008 Simon Newton
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
 * Implement this to be notifed when the remote end closes the connection or
 * SocketReady fails.
 */
class SocketManager {
  public:
    virtual ~SocketManager() {}
    virtual void SocketClosed(class Socket *socket) = 0;
};


/*
 * The base Socket class with the functionality required by the select server.
 * All other sockets inherit from this one.
 */
class Socket {
  public :
    Socket() {}
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
 * A socket on which we can receive data (but not necessarily send).
 */
class ReceivingSocket: public Socket {
  public:
    ReceivingSocket(int read_fd=INVALID_SOCKET):
      m_read_fd(read_fd) {}
    virtual ~ReceivingSocket() { Close(); }

    virtual int ReadDescriptor() const { return m_read_fd; }
    virtual int Receive(uint8_t *buffer,
                        unsigned int size,
                        unsigned int &data_read);
    virtual bool SetReadNonBlocking() { return SetNonBlocking(m_read_fd); }
    virtual bool Close();
    virtual bool IsClosed() const;
    virtual int UnreadData() const;

  protected:
    int m_read_fd;
    bool SetNonBlocking(int sd);
};


/*
 * A connected socket can be read from / written to.
 */
class ConnectedSocket: public ReceivingSocket {
  public:
    ConnectedSocket(int read_fd=INVALID_SOCKET, int write_fd=INVALID_SOCKET):
                    ReceivingSocket(read_fd),
                    m_write_fd(write_fd) {}
    virtual ~ConnectedSocket() { Close(); }

    virtual int WriteDescriptor() const { return m_write_fd; }
    virtual ssize_t Send(const uint8_t *buffer, unsigned int size);
    virtual bool Close();

  protected:
    int m_write_fd;
};


/*
 * A loopback socket.
 * Everything written is available for reading.
 */
class LoopbackSocket: public ConnectedSocket {
  public:
    LoopbackSocket(): ConnectedSocket() {}
    bool Init();
};


/*
 * A pipe socket. You can get the 'other end' of the Socket with OppositeEnd()
 */
class PipeSocket: public ConnectedSocket {
  public:
    PipeSocket(): ConnectedSocket(),
      m_other_end(NULL) {
      m_in_pair[0] = m_in_pair[1] = INVALID_SOCKET;
      m_out_pair[0] = m_out_pair[1] = INVALID_SOCKET;
    }
    bool Init();
    PipeSocket *OppositeEnd();
  private:
    int m_in_pair[2];
    int m_out_pair[2];
    PipeSocket *m_other_end;
    PipeSocket(int read_fd, int write_fd):
      ConnectedSocket(read_fd, write_fd) {}
};


/*
 * A TcpSocket
 */
class TcpSocket: public ConnectedSocket {
  public:
    TcpSocket(const std::string &ip_address, unsigned short port):
      ConnectedSocket(),
      m_ip_address(ip_address),
      m_port(port) {}
    bool Connect();
  private:
    std::string m_ip_address;
    unsigned short m_port;
};


/*
 * A UdpSocket.
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
};

} //network
} //lla
#endif

