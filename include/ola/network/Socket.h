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
 * The Socket interfaces
 * Copyright (C) 2005-2009 Simon Newton
 *
 *  - UdpSocket, allows sending and receiving UDP datagrams
 *  - TcpSocket, this represents a TCP connection to a remote endpoint
 *
 * AcceptingSocket is the interface that defines sockets which can spawn new
 * ConnectedDescriptors. TcpAcceptingSocket is the only subclass and provides
 * the accept() functionality.
 */

#ifndef INCLUDE_OLA_NETWORK_SOCKET_H_
#define INCLUDE_OLA_NETWORK_SOCKET_H_

#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <ola/Callback.h>
#include <ola/io/Descriptor.h>
#include <ola/network/IPV4Address.h>
#include <string>


namespace ola {
namespace network {


/*
 * A TcpSocket
 */
class TcpSocket: public ola::io::ConnectedDescriptor {
  public:
    explicit TcpSocket(int sd)
        : m_sd(sd) {
      SetNoSigPipe(sd);
    }

    ~TcpSocket() { Close(); }

    int ReadDescriptor() const { return m_sd; }
    int WriteDescriptor() const { return m_sd; }
    bool Close();

    bool GetPeer(IPV4Address *address, uint16_t *port);

    static TcpSocket* Connect(const IPV4Address &ip_address,
                              unsigned short port);
    static TcpSocket* Connect(const std::string &ip_address,
                              unsigned short port);

  protected:
    bool IsSocket() const { return true; }

  private:
    int m_sd;

    TcpSocket(const TcpSocket &other);
    TcpSocket& operator=(const TcpSocket &other);
};


/*
 * The UdpSocketInterface.
 * This is done as an Interface so we can mock it out for testing.
 */
class UdpSocketInterface: public ola::io::BidirectionalFileDescriptor {
  public:
    UdpSocketInterface(): ola::io::BidirectionalFileDescriptor() {}
    ~UdpSocketInterface() {}
    virtual bool Init() = 0;
    virtual bool Bind(const IPV4Address &ip,
                      unsigned short port) = 0;
    virtual bool Bind(unsigned short port = 0) = 0;
    virtual bool Close() = 0;
    virtual int ReadDescriptor() const = 0;
    virtual int WriteDescriptor() const = 0;
    virtual ssize_t SendTo(const uint8_t *buffer,
                           unsigned int size,
                           const IPV4Address &ip,
                           unsigned short port) const = 0;
    virtual bool RecvFrom(uint8_t *buffer, ssize_t *data_read) const = 0;
    virtual bool RecvFrom(uint8_t *buffer,
                          ssize_t *data_read,
                          IPV4Address &source) const = 0;
    virtual bool RecvFrom(uint8_t *buffer,
                          ssize_t *data_read,
                          IPV4Address &source,
                          uint16_t &port) const = 0;

    virtual bool EnableBroadcast() = 0;
    virtual bool SetMulticastInterface(const IPV4Address &iface) = 0;
    virtual bool JoinMulticast(const IPV4Address &iface,
                               const IPV4Address &group,
                               bool loop = false) = 0;
    virtual bool LeaveMulticast(const IPV4Address &iface,
                                const IPV4Address &group) = 0;
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
                 m_fd(ola::io::INVALID_DESCRIPTOR),
                 m_bound_to_port(false) {}
    ~UdpSocket() { Close(); }
    bool Init();
    bool Bind(const IPV4Address &ip,
              unsigned short port);
    bool Bind(unsigned short port = 0);
    bool Close();
    int ReadDescriptor() const { return m_fd; }
    int WriteDescriptor() const { return m_fd; }
    ssize_t SendTo(const uint8_t *buffer,
                   unsigned int size,
                   const IPV4Address &ip,
                   unsigned short port) const;
    bool RecvFrom(uint8_t *buffer, ssize_t *data_read) const;
    bool RecvFrom(uint8_t *buffer,
                  ssize_t *data_read,
                  IPV4Address &source) const;
    bool RecvFrom(uint8_t *buffer,
                  ssize_t *data_read,
                  IPV4Address &source,
                  uint16_t &port) const;
    bool EnableBroadcast();
    bool SetMulticastInterface(const IPV4Address &iface);
    bool JoinMulticast(const IPV4Address &iface,
                       const IPV4Address &group,
                       bool loop = false);
    bool LeaveMulticast(const IPV4Address &iface,
                        const IPV4Address &group);

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
 * A TCP accepting socket
 */
class TcpAcceptingSocket: public ola::io::ReadFileDescriptor {
  public:
    explicit TcpAcceptingSocket(class TCPSocketFactoryInterface *factory);
    ~TcpAcceptingSocket();
    bool Listen(const std::string &address,
                unsigned short port,
                int backlog = 10);
    bool Listen(const IPV4Address &address,
                unsigned short port,
                int backlog = 10);
    int ReadDescriptor() const { return m_sd; }
    bool Close();
    void PerformRead();

    void SetFactory(class TCPSocketFactoryInterface *factory) {
      m_factory = factory;
    }

  private:
    int m_sd;
    class TCPSocketFactoryInterface *m_factory;

    TcpAcceptingSocket(const TcpAcceptingSocket &other);
    TcpAcceptingSocket& operator=(const TcpAcceptingSocket &other);
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_SOCKET_H_
