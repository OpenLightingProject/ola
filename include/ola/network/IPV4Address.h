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
 * IPV4Address.h
 * Represents a IPv4 Address
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_
#define INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <stdint.h>
#include <string.h>
#include <sstream>
#include <string>

namespace ola {
namespace network {

using std::ostream;

/*
 * Represents a IPv4 Address.
 * All methods use network byte order unless otherwise mentioned.
 */
class IPV4Address {
  public:
    enum { LENGTH = 4 };

    IPV4Address() {
      m_address.s_addr = 0;
    }

    explicit IPV4Address(const struct in_addr &address)
        : m_address(address) {
    }

    explicit IPV4Address(unsigned int address) {
      m_address.s_addr = address;
    }

    IPV4Address(const IPV4Address &other)
        : m_address(other.m_address) {
    }

    IPV4Address& operator=(const IPV4Address &other) {
      if (this != &other) {
        m_address = other.m_address;
      }
      return *this;
    }

    bool operator==(const IPV4Address &other) const {
      return m_address.s_addr == other.m_address.s_addr;
    }

    bool operator!=(const IPV4Address &other) const {
      return !(*this == other);
    }

    /**
     * @brief Order addresses. Note that this won't order how humans expect
     * because s_addr is in network byte order.
     */
    bool operator<(const IPV4Address &other) const {
      return m_address.s_addr < other.m_address.s_addr;
    }

    bool operator>(const IPV4Address &other) const {
      return m_address.s_addr > other.m_address.s_addr;
    }

    const struct in_addr Address() const {
      return m_address;
    }

    uint32_t AsInt() const { return m_address.s_addr; }

    bool IsWildcard() const {
      return m_address.s_addr == INADDR_ANY;
    }

    // copy the address in network byte order to a location. The location
    // should be at least LENGTH bytes.
    void Get(uint8_t ptr[LENGTH]) {
      memcpy(ptr,
             reinterpret_cast<uint8_t*>(&m_address.s_addr),
             LENGTH);
    }

    std::string ToString() const;

    friend ostream& operator<< (ostream &out, const IPV4Address &address) {
      return out << address.ToString();
    }

    static IPV4Address* FromString(const std::string &address);
    static bool FromString(const std::string &address, IPV4Address *target);
    // useful for testing
    static IPV4Address FromStringOrDie(const std::string &address);

    /*
     * @brief Convert a subnet mask to its CIDR format value
     * @param address the subnet mask as an IPV4Address object
     * @param mask the mask variable to populate
     * @return true if we managed to convert the address to a CIDR value, false
         otherwise
     */
    static bool ToCIDRMask(IPV4Address address, uint8_t *mask);

    static IPV4Address WildCard() {
      return IPV4Address(INADDR_ANY);
    }

    static IPV4Address Broadcast() {
      return IPV4Address(INADDR_NONE);
    }

    static IPV4Address Loopback();

  private:
    struct in_addr m_address;
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_
