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
#include <ola/StringUtils.h>
#include <iomanip>
#include <string>
#include <vector>

namespace ola {
namespace network {

/*
 * Convert a mac address to a human readable string
 * @return a string
 */
std::string MACAddress::ToString() const {
  /**
   * ether_ntoa_r doesn't exist on Mac, so can't use it; ether_ntoa isn't
   * thread safe
   */
  std::stringstream str;
  for (unsigned int i = 0 ; i < MACAddress::LENGTH; i++) {
    if (i != 0)
      str << ":";
    str << std::hex << std::setfill('0') << std::setw(2) <<
      static_cast<int>(m_address.ether_addr_octet[i]);
  }
  return str.str();
}


/**
 * Convert a string to a ether_addr struct
 * @param mac_address a string in the form 'nn:nn:nn:nn:nn:nn' or
 * 'nn.nn.nn.nn.nn.nn'
 * @param target a pointer to a ether_addr struct
 * @return true if it worked, false otherwise
 */
bool MACAddress::StringToEther(const std::string &address, ether_addr &target) {
  /**
   * ether_aton_r doesn't exist on Mac, so can't use it (it also might not
   * handle dots as well as colons as seperators
   */
  struct ether_addr addr;
  vector<string> tokens;
  ola::StringSplit(address, tokens, ":.");
  if (tokens.size() != MACAddress::LENGTH)
    return false;

  for (unsigned int i = 0; i < MACAddress::LENGTH; i++) {
    if (!ola::HexStringToInt(tokens[i], addr.ether_addr_octet + i))
      return false;
  }

  target = addr;
  return true;
}


MACAddress* MACAddress::FromString(const std::string &address) {
  struct ether_addr addr;
  if (!MACAddress::StringToEther(address, addr))
    return NULL;

  return new MACAddress(addr);
}


/**
 * Convert a string to a MACAddress object
 * @param mac_address a string in the form 'nn:nn:nn:nn:nn:nn' or
 * 'nn.nn.nn.nn.nn.nn'
 * @param target a pointer to a MACAddress object
 * @return true if it worked, false otherwise
 */
bool MACAddress::FromString(const std::string &address, MACAddress *target) {
  struct ether_addr addr;

  if (!MACAddress::StringToEther(address, addr))
    return false;

  *target = MACAddress(addr);
  return true;
}


MACAddress MACAddress::FromStringOrDie(const std::string &address) {
  struct ether_addr addr;
  assert(MACAddress::StringToEther(address, addr));
  return MACAddress(addr);
}
}  // namespace network
}  // namespace ola
