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
 * NetworkUtils.cpp
 * Abstract various network functions.
 * Copyright (C) 2005-2009 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef WIN32
typedef uint32_t in_addr_t;
#else
#include <resolv.h>
#endif

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/network/Interface.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"


namespace ola {
namespace network {


bool StringToAddress(const string &address, struct in_addr &addr) {
  bool ok;

#ifdef HAVE_INET_ATON
  ok = (1 == inet_aton(address.data(), &addr));
#else
  in_addr_t ip_addr4 = inet_addr(address.c_str());
  ok = (INADDR_NONE != ip_addr4 || address == "255.255.255.255");
  addr.s_addr = ip_addr4;
#endif

  if (!ok) {
    OLA_WARN << "Could not convert address " << address;
  }
  return ok;
}


string AddressToString(const struct in_addr &addr) {
  return inet_ntoa(addr);
}


bool IsBigEndian() {
#ifdef HAVE_ENDIAN_H
  return BYTE_ORDER == __BIG_ENDIAN;
#else
  return BYTE_ORDER == BIG_ENDIAN;
#endif
}


uint8_t NetworkToHost(uint8_t value) {
  return value;
}


uint16_t NetworkToHost(uint16_t value) {
  return ntohs(value);
}


uint32_t NetworkToHost(uint32_t value) {
  return ntohl(value);
}


int8_t NetworkToHost(int8_t value) {
  return value;
}


int16_t NetworkToHost(int16_t value) {
  return ntohs(value);
}


int32_t NetworkToHost(int32_t value) {
  return ntohl(value);
}


uint8_t HostToNetwork(uint8_t value) {
  return value;
}


int8_t HostToNetwork(int8_t value) {
  return value;
}


uint16_t HostToNetwork(uint16_t value) {
  return htons(value);
}


int16_t HostToNetwork(int16_t value) {
  return htons(value);
}


uint32_t HostToNetwork(uint32_t value) {
  return htonl(value);
}


int32_t HostToNetwork(int32_t value) {
  return htonl(value);
}


uint8_t HostToLittleEndian(uint8_t value) {
  return value;
}


int8_t HostToLittleEndian(int8_t value) {
  return value;
}


uint16_t HostToLittleEndian(uint16_t value) {
  if (IsBigEndian())
    return ((value & 0xff) << 8) | (value >> 8);
  else
    return value;
}


int16_t HostToLittleEndian(int16_t value) {
  if (IsBigEndian())
    return ((value & 0xff) << 8) | (value >> 8);
  else
    return value;
}


uint32_t _ByteSwap(uint32_t value) {
  return ((value & 0x000000ff) << 24) |
         ((value & 0x0000ff00) << 8) |
         ((value & 0x00ff0000) >> 8) |
         ((value & 0xff000000) >> 24);
}

uint32_t HostToLittleEndian(uint32_t value) {
  if (IsBigEndian())
    return _ByteSwap(value);
  else
    return value;
}


int32_t HostToLittleEndian(int32_t value) {
  if (IsBigEndian())
    return _ByteSwap(value);
  else
    return value;
}


uint8_t LittleEndianToHost(uint8_t value) {
  return value;
}


int8_t LittleEndianToHost(int8_t value) {
  return value;
}


uint16_t LittleEndianToHost(uint16_t value) {
  if (IsBigEndian())
    return ((value & 0xff) << 8) | (value >> 8);
  else
    return value;
}


int16_t LittleEndianToHost(int16_t value) {
  if (IsBigEndian())
    return ((value & 0xff) << 8) | (value >> 8);
  else
    return value;
}


uint32_t LittleEndianToHost(uint32_t value) {
  if (IsBigEndian())
    return _ByteSwap(value);
  else
    return value;
}


int32_t LittleEndianToHost(int32_t value) {
  if (IsBigEndian())
    return _ByteSwap(value);
  else
    return value;
}


string HostnameFromFQDN(const string &fqdn) {
  string::size_type first_dot = string::npos;
  first_dot = fqdn.find_first_of(".");
  if (first_dot == string::npos)
    return fqdn;  // TODO(Peter): Is this safe with a pass by ref?
  return fqdn.substr(0, (first_dot));  // Don't return the dot itself
}


string DomainNameFromFQDN(const string &fqdn) {
  string::size_type first_dot = string::npos;
  first_dot = fqdn.find_first_of(".");
  if (first_dot == string::npos)
    return "";
  return fqdn.substr(first_dot + 1);  // Don't return the dot itself
}


string DomainName() {
  return DomainNameFromFQDN(FQDN());
}


string FQDN() {
#ifdef _POSIX_HOST_NAME_MAX
  char hostname[_POSIX_HOST_NAME_MAX];
#else
  char hostname[256];
#endif
  int ret = gethostname(hostname, sizeof(hostname));

  if (ret) {
    OLA_WARN << "gethostname failed: " << strerror(errno);
    return "";
  }
  return hostname;
}


string FullHostname() {
  return FQDN();
};


string Hostname() {
  return HostnameFromFQDN(FQDN());
}


bool NameServers(vector<IPV4Address> *name_servers) {
  // TODO(Peter): Do something on Windows
  OLA_DEBUG << "Getting nameservers";

  if (res_init() != 0) {
    OLA_WARN << "Error getting nameservers";
    return false;
  }

  for (int32_t i = 0; i < _res.nscount; i++) {
    IPV4Address addr = IPV4Address(_res.nsaddr_list[i].sin_addr);
    OLA_DEBUG << "Found Nameserver " << i << ": " << addr;
    name_servers->push_back(addr);
  }

  return true;
}
}  // namespace network
}  // namespace ola
