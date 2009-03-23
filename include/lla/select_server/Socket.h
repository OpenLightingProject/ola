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
#include <string>
#include <lla/select_server/FdListener.h>

namespace lla {
namespace select_server {

/*
 * A SocketListener, implement this if you want to respond to events when the
 * socket has data.
 */
class SocketListener {
  public:
    virtual ~SocketListener() {}
    virtual int SocketReady(class ConnectedSocket *socket) = 0;
};


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
 * A AcceptSocketListener, implement this if you want to respond to a
 * new connection arriving..
 *
 * NewConnect is passed a new ConncetedSocket which it then ownes and is
 * responseible for registering with the select server.
 */
class AcceptSocketListener {
  public:
    virtual ~AcceptSocketListener() {}
    virtual int NewConnection(class ConnectedSocket *socket) = 0;
};


/*
 * The socket interface
 */
class Socket {
  public :
    Socket() {}
    virtual ~Socket() {};
    virtual int ReadDescriptor() const = 0; // returns the read socket descriptor
    virtual int SocketReady() = 0; // called when there is data to read
    virtual bool IsClosed() const = 0;
    virtual bool Close() = 0;
};


/*
 * A connected socket can be read from / written to.
 */
class ConnectedSocket: public Socket {
  public:
    ConnectedSocket(int read_fd=-1, int write_fd=-1):
                    m_read_fd(read_fd),
                    m_write_fd(write_fd),
                    m_listener(NULL) {}
    virtual ~ConnectedSocket() { Close(); }

    virtual int ReadDescriptor() const { return m_read_fd; }
    virtual int WriteDescriptor() const { return m_write_fd; }
    virtual ssize_t Send(const uint8_t *buffer, unsigned int size);
    virtual int Receive(uint8_t *buffer,
                        unsigned int size,
                        unsigned int &data_read);
    virtual int SocketReady();
    virtual void SetListener(SocketListener *listener) { m_listener = listener; }
    virtual bool SetReadNonBlocking() { return SetNonBlocking(m_read_fd); }
    virtual bool Close();
    virtual bool IsClosed() const;
    virtual int UnreadData() const;

  protected:
    int m_read_fd, m_write_fd;
    bool SetNonBlocking(int sd);
  private:
    SocketListener *m_listener;
};


/*
 * A loopback socket.
 * Everything written is available for reading.
 */
class LoopbackSocket: public ConnectedSocket {
  public:
    LoopbackSocket(): ConnectedSocket() {}
    bool Init();
    bool Close();
};


/*
 * A pipe socket. You can get the 'other end' of the Socket with OppositeEnd()
 */
class PipeSocket: public ConnectedSocket {
  public:
    PipeSocket(): ConnectedSocket() {}
    bool Init();
    PipeSocket *OppositeEnd();
  private:
    int m_in_pair[2];
    int m_out_pair[2];
    PipeSocket(int read_fd, int write_fd): ConnectedSocket(read_fd, write_fd) {}
};


/*
 * A TcpSocket
 */
class TcpSocket: public ConnectedSocket {
  public:
    TcpSocket(): ConnectedSocket() {}
    TcpSocket(int sd): ConnectedSocket(sd, sd) {}
    bool Connect(std::string ip_address, unsigned short port);
};


/*
 * A listening socket creates new Sockets when clients connect
 */
class ListeningSocket: public Socket {
  public:
    ListeningSocket(): m_listener(NULL) {}
    virtual bool Listen() { return 0; }
    virtual bool Close() = 0;
    virtual int SocketReady() = 0;
    virtual void SetListener(AcceptSocketListener *listener) { m_listener = listener; }
  protected:
    AcceptSocketListener *m_listener;
};


/*
 * A TCP listening socket
 */
class TcpListeningSocket: public ListeningSocket {
  public:
    TcpListeningSocket(std::string address, unsigned short port, int backlog=10);
    ~TcpListeningSocket() { Close(); }
    bool Listen();
    int SocketReady();
    int ReadDescriptor() const { return m_sd; }
    bool Close();
    bool IsClosed() const;
  private:
    std::string m_address;
    unsigned short m_port;
    int m_sd, m_backlog;
};

} //select_server
} //lla
#endif

