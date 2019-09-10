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
 * MACAddress.cpp
 * A MAC address
 * Copyright (C) 2013 Peter Newman
 */

#include "ola/network/MACAddress.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
struct ether_addr {
  unsigned char octet[ola::network::MACAddress::LENGTH];
};
#define ether_addr_octet octet
#else
#include <sys/types.h>  // required for FreeBSD uchar - doesn't hurt others
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif  // HAVE_NET_ETHERNET_H
// NetBSD and OpenBSD don't have net/ethernet.h
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif  // HAVE_SYS_SOCKET_H
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif  // HAVE_NET_IF_H
#ifdef HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif  // HAVE_NET_IF_ETHER_H
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif  // HAVE_NETINET_IN_H
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif  // HAVE_NET_IF_ARP_H
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif  // HAVE_NETINET_IF_ETHER_H
#endif  // _WIN32

#if defined(__FreeBSD__) || defined(__DragonFly__)
// In the FreeBSD struct ether_addr, the single field is named octet, instead
// of ether_addr_octet.
// OS X does this too, but avoids it by adding the following line to its
// header, for compatibility with linux and others:
// http://www.opensource.apple.com/source/xnu/xnu-1456.1.26/bsd/net/ethernet.h
#define ether_addr_octet octet
#endif  // defined(__FreeBSD__) || defined(__DragonFly__)

#include <assert.h>
#include <string.h>
#include <iomanip>
#include <string>
#include <vector>

#include "ola/network/NetworkUtils.h"
#include "ola/StringUtils.h"

namespace ola {
namespace network {

using std::string;
using std::vector;

MACAddress::MACAddress() {
  memset(m_address, 0, LENGTH);
}

MACAddress::MACAddress(const uint8_t *address) {
  memcpy(m_address, address, LENGTH);
}

MACAddress::MACAddress(const MACAddress &other) {
  memcpy(m_address, other.m_address, LENGTH);
}

MACAddress& MACAddress::operator=(const MACAddress &other) {
  if (this != &other) {
    memcpy(m_address, other.m_address, LENGTH);
  }
  return *this;
}

bool MACAddress::operator==(const MACAddress &other) const {
  return (memcmp(m_address, other.m_address, LENGTH) == 0);
}

bool MACAddress::operator<(const MACAddress &other) const {
  return (memcmp(m_address, other.m_address, LENGTH) < 0);
}

bool MACAddress::operator>(const MACAddress &other) const {
  return (memcmp(m_address, other.m_address, LENGTH) > 0);
}

void MACAddress::Get(uint8_t ptr[LENGTH]) const {
  memcpy(ptr, m_address, LENGTH);
}

string MACAddress::ToString() const {
  /**
   * ether_ntoa_r doesn't exist on Mac, so can't use it; ether_ntoa isn't
   * thread safe
   */
  std::ostringstream str;
  for (unsigned int i = 0 ; i < MACAddress::LENGTH; i++) {
    if (i != 0)
      str << ":";
    str << std::hex << std::setfill('0') << std::setw(2) <<
      static_cast<int>(m_address[i]);
  }
  return str.str();
}


/**
 * Convert a string to a ether_addr struct
 * @param address a string in the form 'nn:nn:nn:nn:nn:nn' or
 * 'nn.nn.nn.nn.nn.nn'
 * @param target a pointer to a ether_addr struct
 * @return true if it worked, false otherwise
 */
bool StringToEther(const string &address, ether_addr *target) {
  /**
   * ether_aton_r doesn't exist on Mac, so can't use it (also it might not
   * handle dots as well as colons as separators)
   */
  vector<string> tokens;
  ola::StringSplit(address, &tokens, ":.");
  if (tokens.size() != MACAddress::LENGTH) {
    return false;
  }

  for (unsigned int i = 0; i < MACAddress::LENGTH; i++) {
    if (!ola::HexStringToInt(tokens[i], target->ether_addr_octet + i)) {
      return false;
    }
  }
  return true;
}


MACAddress* MACAddress::FromString(const string &address) {
  struct ether_addr addr;
  if (!StringToEther(address, &addr)) {
    return NULL;
  }

  return new MACAddress(addr.ether_addr_octet);
}

bool MACAddress::FromString(const string &address, MACAddress *target) {
  struct ether_addr addr;

  if (!StringToEther(address, &addr)) {
    return false;
  }

  *target = MACAddress(addr.ether_addr_octet);
  return true;
}

MACAddress MACAddress::FromStringOrDie(const string &address) {
  struct ether_addr addr;
  assert(StringToEther(address, &addr));
  return MACAddress(addr.ether_addr_octet);
}
}  // namespace network
}  // namespace ola
