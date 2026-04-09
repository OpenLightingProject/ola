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
 * IPV4Address.cpp
 * An IPV4 address
 * Copyright (C) 2011 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef HAVE_WINSOCK2_H
#include <ola/win/CleanWinSock2.h>
#ifndef in_addr_t
#define in_addr_t uint32_t
#endif  // !in_addr_t
#endif  // HAVE_WINSOCK2_H

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
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"

namespace ola {
namespace network {

using std::string;

bool IPV4Address::operator<(const IPV4Address &other) const {
  // Stored in network byte order, so convert to sort appropriately
  return NetworkToHost(m_address) < NetworkToHost(other.m_address);
}

bool IPV4Address::operator>(const IPV4Address &other) const {
  // Stored in network byte order, so convert to sort appropriately
  return NetworkToHost(m_address) > NetworkToHost(other.m_address);
}

bool IPV4StringToAddress(const string &address, struct in_addr *addr) {
  bool ok;
// TODO(Peter): This currently allows some rather quirky values as per
// inet_aton, we may want to restrict that in future to match IPV4Validator

  if (address.empty()) {
    // Don't bother trying to extract an address if we weren't given one
    return false;
  }

#ifdef HAVE_INET_PTON
  ok = (1 == inet_pton(AF_INET, address.data(), addr));
#elif HAVE_INET_ATON
  ok = (1 == inet_aton(address.data(), addr));
#else
  in_addr_t ip_addr4 = inet_addr(address.c_str());
  ok = (INADDR_NONE != ip_addr4 || address == "255.255.255.255");
  addr->s_addr = ip_addr4;
#endif  // HAVE_INET_PTON

  if (!ok) {
    OLA_WARN << "Could not convert address " << address;
  }
  return ok;
}

bool IPV4Address::IsWildcard() const {
  return m_address == INADDR_ANY;
}

string IPV4Address::ToString() const {
  struct in_addr addr;
  addr.s_addr = m_address;
#ifdef HAVE_INET_NTOP
  char str[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &addr, str, INET_ADDRSTRLEN) == NULL) {
    OLA_WARN << "Failed to convert address to string using inet_ntop, failing "
             << "back to inet_ntoa";
    return inet_ntoa(addr);
  }
  return str;
#else
  return inet_ntoa(addr);
#endif  // HAVE_INET_NTOP
}

IPV4Address* IPV4Address::FromString(const string &address) {
  struct in_addr addr;
  if (!IPV4StringToAddress(address, &addr)) {
    return NULL;
  }

  return new IPV4Address(addr.s_addr);
}

bool IPV4Address::FromString(const string &address, IPV4Address *target) {
  struct in_addr addr;
  if (!IPV4StringToAddress(address, &addr)) {
    return false;
  }
  *target = IPV4Address(addr.s_addr);
  return true;
}

IPV4Address IPV4Address::FromStringOrDie(const string &address) {
  struct in_addr addr;
  assert(IPV4StringToAddress(address, &addr));
  return IPV4Address(addr.s_addr);
}

bool IPV4Address::ToCIDRMask(IPV4Address address, uint8_t *mask) {
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
}

IPV4Address IPV4Address::WildCard() {
  return IPV4Address(INADDR_ANY);
}

IPV4Address IPV4Address::Broadcast() {
  return IPV4Address(INADDR_NONE);
}

IPV4Address IPV4Address::Loopback() {
  return IPV4Address(HostToNetwork(0x7f000001));
}
}  // namespace network
}  // namespace ola
