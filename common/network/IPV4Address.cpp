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
 * Copyright (C) 2011 Simon Newton
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/NetworkUtils.h>
#include <string>

namespace ola {
namespace network {

std::string IPV4Address::ToString() const {
  return AddressToString(m_address);
}

IPV4Address* IPV4Address::FromString(const std::string &address) {
  struct in_addr addr;
  if (!StringToAddress(address, addr))
    return NULL;

  return new IPV4Address(addr);
}

bool IPV4Address::FromString(const std::string &address, IPV4Address *target) {
  struct in_addr addr;
  if (!StringToAddress(address, addr))
    return false;
  *target = IPV4Address(addr);
  return true;
}


IPV4Address IPV4Address::FromStringOrDie(const std::string &address) {
  struct in_addr addr;
  assert(StringToAddress(address, addr));
  return IPV4Address(addr);
}


bool IPV4Address::ToCIDRMask(IPV4Address address, uint8_t *mask) {
  // TODO(Peter): Is this endian safe?
  uint32_t netmask = NetworkToHost(address.AsInt());
  uint8_t bits = 0;
  bool seen_one = false;
  for (uint8_t i = 0; i < 32; i++) {
    if ( netmask & 1 ) {
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


IPV4Address IPV4Address::Loopback() {
  return IPV4Address(HostToNetwork(0x7f000001));
}
}  // namespace network
}  // namespace ola
