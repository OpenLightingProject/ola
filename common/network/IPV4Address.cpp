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
 * IPV4Address.cpp
 * A IPV4 address
 * Copyright (C) 2011-2014 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#ifndef in_addr_t
#define in_addr_t uint32_t
#endif
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>  // Required by FreeBSD
#endif

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string>

#include "common/network/NetworkUtilsInternal.h"
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"

namespace ola {
namespace network {

using std::string;

bool IPV4StringToAddress(const string &address, struct in_addr *addr) {
  bool ok;
// TODO(Peter): This currently allows some rather quirky values as per
// inet_aton, we may want to restrict that in future to match IPV4Validator

#ifdef HAVE_INET_ATON
  ok = (1 == inet_aton(address.data(), addr));
#else
  in_addr_t ip_addr4 = inet_addr(address.c_str());
  ok = (INADDR_NONE != ip_addr4 || address == "255.255.255.255");
  addr->s_addr = ip_addr4;
#endif

  if (!ok) {
    OLA_WARN << "Could not convert address " << address;
  }
  return ok;
}

bool IPV4Address::IsWildcard() const {
  return m_address == INADDR_ANY;
}

std::string IPV4Address::ToString() const {
  struct in_addr addr;
  addr.s_addr = m_address;
  return inet_ntoa(addr);
}

IPV4Address* IPV4Address::FromString(const std::string &address) {
  struct in_addr addr;
  if (!IPV4StringToAddress(address, &addr))
    return NULL;

  return new IPV4Address(addr.s_addr);
}

bool IPV4Address::FromString(const std::string &address, IPV4Address *target) {
  struct in_addr addr;
  if (!IPV4StringToAddress(address, &addr))
    return false;
  *target = IPV4Address(addr.s_addr);
  return true;
}

IPV4Address IPV4Address::FromStringOrDie(const std::string &address) {
  struct in_addr addr;
  assert(IPV4StringToAddress(address, &addr));
  return IPV4Address(addr.s_addr);
}

bool IPV4Address::ToCIDRMask(IPV4Address address, uint8_t *mask) {
  uint32_t netmask = NetworkToHost(address.AsInt());
  uint8_t bits = 0;
  bool seen_one = false;
  for (uint8_t i = 0; i < 32; i++) {
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
