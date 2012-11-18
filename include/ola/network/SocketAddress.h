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
 * SocketAddress.h
 * Represents a sockaddr structure.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_SOCKETADDRESS_H_
#define INCLUDE_OLA_NETWORK_SOCKETADDRESS_H_

#include <ola/network/IPV4Address.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sstream>
#include <string>

namespace ola {
namespace network {

using std::string;

/**
 * The base SocketAddress. One day if we support V6 there will be another
 * derived class.
 **/
class SocketAddress {
  public:
    virtual ~SocketAddress() {}

    virtual uint16_t Family() const = 0;
    virtual bool ToSockAddr(struct sockaddr *addr, unsigned int size) const = 0;
    virtual string ToString() const = 0;

    friend ostream& operator<< (ostream &out, const SocketAddress &address) {
      return out << address.ToString();
    }
};


/**
 * Wraps a sockaddr_in.
 */
class IPV4SocketAddress: public SocketAddress {
  public:
    IPV4SocketAddress()
        : SocketAddress(),
          m_host(),
          m_port(0) {
    }

    IPV4SocketAddress(const IPV4Address &host, uint16_t port)
        : SocketAddress(),
          m_host(host),
          m_port(port) {
    }
    IPV4SocketAddress(const IPV4SocketAddress &other)
        : SocketAddress(),
          m_host(other.m_host),
          m_port(other.m_port) {
    }

    ~IPV4SocketAddress() {}

    IPV4SocketAddress& operator=(const IPV4SocketAddress &other) {
      if (this != &other) {
        m_host = other.m_host;
        m_port = other.m_port;
      }
      return *this;
    }

    bool operator==(const IPV4SocketAddress &other) const {
      return m_host == other.m_host && m_port == other.m_port;
    }

    bool operator!=(const IPV4SocketAddress &other) const {
      return !(*this == other);
    }

    bool operator<(const IPV4SocketAddress &other) const {
      if (m_host == other.m_host)
        return m_port < other.m_port;
      else
        return m_host < other.m_host;
    }

    uint16_t Family() const { return AF_INET; }
    const IPV4Address& Host() const { return m_host; }
    void Host(const IPV4Address &host) { m_host = host; }
    uint16_t Port() const { return m_port; }
    void Port(uint16_t port) { m_port = port; }

    string ToString() const {
      std::ostringstream str;
      str << Host() << ":" << Port();
      return str.str();
    }

    static bool FromString(const string &str,
                           IPV4SocketAddress *socket_address);

    bool ToSockAddr(struct sockaddr *addr, unsigned int size) const;

  private:
    IPV4Address m_host;
    uint16_t m_port;
};


/**
 * Wraps a struct sockaddr.
 */
class GenericSocketAddress: public SocketAddress {
  public:
    explicit GenericSocketAddress(const struct sockaddr &addr)
      : m_addr(addr) {
    }

    uint16_t Family() const {
      return m_addr.sa_family;
    }

    GenericSocketAddress& operator=(const GenericSocketAddress &other) {
      if (this != &other) {
        memcpy(&m_addr, &(other.m_addr), sizeof(m_addr));
      }
      return *this;
    }

    bool ToSockAddr(struct sockaddr *addr, unsigned int size) const {
      *addr = m_addr;
      return true;
      (void) size;
    }

    string ToString() const {
      std::ostringstream str;
      str << "Generic sockaddr of type: " << m_addr.sa_family;
      return str.str();
    }

    // Return a IPV4SocketAddress object, only valid if Family() is AF_INET
    IPV4SocketAddress V4Addr() const;
    // Add V6 here as well

  private:
    struct sockaddr m_addr;
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_SOCKETADDRESS_H_
