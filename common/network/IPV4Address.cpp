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
 * IPV4Address.cpp
 * A IPV4 address
 * Copyright (C) 2011 Simon Newton
 */

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


IPV4Address IPV4Address::Loopback() {
  return IPV4Address(HostToNetwork(0x7f000001));
}
}  // network
}  // ola
