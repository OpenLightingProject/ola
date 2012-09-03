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

#include <stdint.h>
#include <ola/network/IPV4Address.h>

namespace ola {
namespace network {


/**
 * The base SocketAddress. One day if we support V6 there will be another
 * derived class.
 **/
class SocketAddress {
  public:
    virtual ~SocketAddress() {}

    virtual bool ToSockAddr(struct sockaddr *addr, unsigned int size) = 0;
};


/**
 * Wraps a sockaddr_in.
 */
class IPV4SocketAddress: public SocketAddress {
  public:
    IPV4SocketAddress(const IPV4Address &host, uint16_t port)
        : m_host(host),
          m_port(port) {
    }
    ~IPV4SocketAddress() {}

    const IPV4Address& Host() { return m_host; }
    void Host(const IPV4Address &host) { m_host = host; }
    uint16_t Port() const { return m_port; }
    void Port(uint16_t port) { m_port = port; }

    bool ToSockAddr(struct sockaddr *addr, unsigned int size);

  private:
    IPV4Address m_host;
    uint16_t m_port;
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_SOCKETADDRESS_H_
