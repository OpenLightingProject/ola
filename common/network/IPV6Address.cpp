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
 * IPV6Address.cpp
 * An IPV6 address
 * Copyright (C) 2023 Peter Newman
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>  // Required by FreeBSD
#endif  // HAVE_SYS_SOCKET_H
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif  // HAVE_ARPA_INET_H
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>  // Required by FreeBSD
#endif  // HAVE_NETINET_IN_H

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <limits>
#include <string>

#include "common/network/NetworkUtilsInternal.h"
#include "ola/Logging.h"
#include "ola/network/IPV6Address.h"
#include "ola/network/NetworkUtils.h"

namespace ola {
namespace network {

using std::string;

IPV6Address::IPV6Address(const uint8_t *address) {
  // TODO(Peter): Deal with any network byte order conversion?
  memcpy(&m_address.s6_addr[0], address, sizeof (struct in6_addr));
}

bool IPV6Address::operator<(const IPV6Address &other) const {
  // TODO(Peter): Deal with any network byte order conversion?
  return (memcmp(&m_address.s6_addr[0],
          &other.m_address.s6_addr[0],
          sizeof (struct in6_addr)) < 0);
}

bool IPV6Address::operator>(const IPV6Address &other) const {
  // TODO(Peter): Deal with any network byte order conversion?
  return (memcmp(&m_address.s6_addr[0],
          &other.m_address.s6_addr[0],
          sizeof (struct in6_addr)) > 0);
}

bool IPV6StringToAddress(const string &address, struct in6_addr *addr) {
  bool ok;
//// TODO(Peter): This currently allows some rather quirky values as per
//// inet_pton, we may want to restrict that in future to match IPV6Validator
//// if that deviates

  if (address.empty()) {
    // Don't bother trying to extract an address if we weren't given one
    return false;
  }

#ifdef HAVE_INET_PTON
  ok = (1 == inet_pton(AF_INET6, address.data(), addr));
#else
  OLA_FATAL << "Failed to convert string to address, inet_pton unavailable";
  return false;
#endif  // HAVE_INET_PTON

  if (!ok) {
    OLA_WARN << "Could not convert address " << address;
  }
  return ok;
}

bool IPV6Address::IsWildcard() const {
  return IN6_IS_ADDR_UNSPECIFIED(&m_address);
}

string IPV6Address::ToString() const {
  struct in6_addr addr;
  addr = m_address;
#ifdef HAVE_INET_NTOP
  char str[INET6_ADDRSTRLEN];
  if (inet_ntop(AF_INET6, &addr, str, INET6_ADDRSTRLEN) == NULL) {
    OLA_FATAL << "Failed to convert address to string using inet_ntop";
    return NULL;
  }
  return str;
#else
    OLA_FATAL << "Failed to convert address to string, inet_ntop unavailable";
    return NULL;
#endif  // HAVE_INET_NTOP
}

IPV6Address* IPV6Address::FromString(const string &address) {
  struct in6_addr addr;
  if (!IPV6StringToAddress(address, &addr)) {
    return NULL;
  }

  return new IPV6Address(addr);
}

bool IPV6Address::FromString(const string &address, IPV6Address *target) {
  struct in6_addr addr;
  if (!IPV6StringToAddress(address, &addr)) {
    return false;
  }
  *target = IPV6Address(addr);
  return true;
}

IPV6Address IPV6Address::FromStringOrDie(const string &address) {
  struct in6_addr addr;
  assert(IPV6StringToAddress(address, &addr));
  return IPV6Address(addr);
}

/*bool IPV6Address::ToCIDRMask(IPV6Address address, uint8_t *mask) {
  uint32_t netmask = NetworkToHost(address.AsInt());
  uint8_t bits = 0;
  bool seen_one = false;
  for (uint8_t i = 0; i < std::numeric_limits<uint32_t>::digits; i++) {
    if (netmask & 1) {
      bits++;
      seen_one = true;
    } else {
      if (seen_one) {
        return false;
      }
    }
    netmask = netmask >> 1;
  }
  *mask = bits;
  return true;
}*/

IPV6Address IPV6Address::WildCard() {
  in6_addr wildCard = IN6ADDR_ANY_INIT;
  // TODO(Peter): Deal with any host to network conversion...
  return IPV6Address(wildCard);
}

IPV6Address IPV6Address::Loopback() {
  in6_addr loopback = IN6ADDR_LOOPBACK_INIT;
  // TODO(Peter): Deal with any host to network conversion...
  // return IPV6Address(HostToNetwork(IN6ADDR_LOOPBACK_INIT));
  return IPV6Address(loopback);
}
}  // namespace network
}  // namespace ola
