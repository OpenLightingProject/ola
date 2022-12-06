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
 * SocketAddress.h
 * Represents a sockaddr structure.
 * Copyright (C) 2012 Simon Newton
 */

/**
 * @addtogroup network
 * @{
 * @file SocketAddress.h
 * @brief Represents Socket Addresses.
 * @}
 */

#ifndef INCLUDE_OLA_NETWORK_SOCKETADDRESS_H_
#define INCLUDE_OLA_NETWORK_SOCKETADDRESS_H_

#include <ola/network/IPV4Address.h>
#include <ola/base/Macro.h>
#include <stdint.h>
#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <ola/win/CleanWinSock2.h>
#else
#include <sys/socket.h>
#endif  // _WIN32
#include <sstream>
#include <string>

namespace ola {
namespace network {

/**
 * @addtogroup network
 * @{
 */

/**
 * @brief The base SocketAddress.
 *
 * One day if we support V6 there will be another derived class.
 **/
class SocketAddress {
 public:
    virtual ~SocketAddress() {}

    virtual uint16_t Family() const = 0;
    virtual bool ToSockAddr(struct sockaddr *addr, unsigned int size) const = 0;
    virtual std::string ToString() const = 0;

    friend std::ostream& operator<<(std::ostream &out,
                                    const SocketAddress &address) {
      return out << address.ToString();
    }
};


/**
 * @brief An IPv4 SocketAddress.
 *
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

    /**
     * @brief Less than operator for partial ordering.
     *
     * Sorts by host, then port.
     */
    bool operator<(const IPV4SocketAddress &other) const {
      if (m_host == other.m_host)
        return m_port < other.m_port;
      else
        return m_host < other.m_host;
    }

    /**
     * @brief Greater than operator.
     *
     * Sorts by host, then port.
     */
    bool operator>(const IPV4SocketAddress &other) const {
      if (m_host == other.m_host)
        return m_port > other.m_port;
      else
        return m_host > other.m_host;
    }

    uint16_t Family() const { return AF_INET; }
    const IPV4Address& Host() const { return m_host; }
    void Host(const IPV4Address &host) { m_host = host; }
    uint16_t Port() const { return m_port; }
    void Port(uint16_t port) { m_port = port; }

    std::string ToString() const;

    static bool FromString(const std::string &str,
                           IPV4SocketAddress *socket_address);

    // useful for testing
    static IPV4SocketAddress FromStringOrDie(const std::string &address);

    bool ToSockAddr(struct sockaddr *addr, unsigned int size) const;

 private:
    IPV4Address m_host;
    uint16_t m_port;
};


/**
 * @brief a Generic Socket Address
 *
 * Wraps a struct sockaddr.
 */
class GenericSocketAddress: public SocketAddress {
 public:
    explicit GenericSocketAddress(const struct sockaddr &addr)
      : m_addr(addr) {
    }

    GenericSocketAddress() {
      memset(reinterpret_cast<uint8_t*>(&m_addr), 0, sizeof(m_addr));
    }

    GenericSocketAddress(const GenericSocketAddress& other) {
      memcpy(&m_addr, &(other.m_addr), sizeof(m_addr));
    }

    bool IsValid() const;

    uint16_t Family() const {
      return m_addr.sa_family;
    }

    GenericSocketAddress& operator=(const GenericSocketAddress &other) {
      if (this != &other) {
        memcpy(&m_addr, &(other.m_addr), sizeof(m_addr));
      }
      return *this;
    }

    bool ToSockAddr(struct sockaddr *addr,
                    OLA_UNUSED unsigned int size) const {
      *addr = m_addr;
      return true;
    }

    std::string ToString() const;

    // Return a IPV4SocketAddress object, only valid if Family() is AF_INET
    IPV4SocketAddress V4Addr() const;
    // Add V6 here as well

 private:
    struct sockaddr m_addr;
};
/**
 * @}
 */
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_SOCKETADDRESS_H_
