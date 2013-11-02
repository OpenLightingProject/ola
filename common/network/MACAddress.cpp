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
 * MACAddress.cpp
 * A MAC address
 * Copyright (C) 2013 Peter Newman
 */

#include <assert.h>
#include <ola/network/MACAddress.h>
#include <ola/network/NetworkUtils.h>
#include <string>

namespace ola {
namespace network {

std::string MACAddress::ToString() const {
  return MACAddressToString(m_address);
}

MACAddress* MACAddress::FromString(const std::string &address) {
  struct ether_addr addr;
  if (!StringToMACAddress(address, addr))
    return NULL;

  return new MACAddress(addr);
}

bool MACAddress::FromString(const std::string &address, MACAddress *target) {
  struct ether_addr addr;
  if (!StringToMACAddress(address, addr))
    return false;
  *target = MACAddress(addr);
  return true;
}


MACAddress MACAddress::FromStringOrDie(const std::string &address) {
  struct ether_addr addr;
  assert(StringToMACAddress(address, addr));
  return MACAddress(addr);
}
}  // namespace network
}  // namespace ola
