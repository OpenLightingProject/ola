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
 * IPV4Address.h
 * Represents a network interface.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_
#define INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <ola/network/NetworkUtils.h>
#include <sstream>
#include <string>

namespace ola {
namespace network {

using std::ostream;

/*
 * Represents a IPv4 Address.
 */
class IPV4Address {
  public:
    IPV4Address() {
      m_address.s_addr = 0;
    }

    explicit IPV4Address(const struct in_addr &address)
        : m_address(address) {
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

    const struct in_addr Address() const {
      return m_address;
    }

    std::string ToString() const {
      return AddressToString(m_address);
    }

    friend ostream& operator<< (ostream &out, const IPV4Address &address) {
      return out << address.ToString();
    }

  private:
    struct in_addr m_address;
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_IPV4ADDRESS_H_

