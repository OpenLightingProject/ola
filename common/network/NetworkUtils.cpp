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
 * NetworkUtils.h
 * Abstract various network functions.
 * Copyright (C) 2005-2009 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef WIN32
#include <winsock2.h>
typedef unsigned long in_addr_t;
#else
#include <arpa/inet.h>
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


namespace ola {
namespace network {


/*
 * Convert a string to a struct in_addr
 */
bool StringToAddress(const string &address, struct in_addr &addr) {
#ifdef HAVE_INET_ATON
  if (!inet_aton(address.data(), &addr)) {
#else
  in_addr_t *ip_addr4 = static_cast<in_addr_t*>(&addr);
  if ((*ip_addr4 = inet_addr(address.data())) == INADDR_NONE) {
#endif
    OLA_WARN << "Could not convert address " << address;
    return false;
  }
  return true;
}


string AddressToString(const struct in_addr &addr) {
  return inet_ntoa(addr);
}


/*
 * Convert a mac address to a human readable string
 * @param hw_address the mac address.
 * @return a string
 */
std::string HardwareAddressToString(uint8_t hw_address[MAC_LENGTH]) {
  std::stringstream str;
  for (unsigned int i = 0 ; i < ola::network::MAC_LENGTH; i++) {
    if (i != 0)
      str << ":";
    str << std::hex << std::setfill('0') << std::setw(2) <<
      static_cast<int>(hw_address[i]);
  }
  return str.str();
}


uint8_t NetworkToHost(uint8_t value) {
  return value;
}


/*
 * Convert a uint16_t from network to host byte order
 */
uint16_t NetworkToHost(uint16_t value) {
  return ntohs(value);
}


/*
 * Convert a uint32_t from network to host byte order
 */
uint32_t NetworkToHost(uint32_t value) {
  return ntohl(value);
}


int8_t NetworkToHost(int8_t value) {
  return value;
}


/*
 * Convert a int16_t from network to host byte order
 */
int16_t NetworkToHost(int16_t value) {
  return ntohs(value);
}


/*
 * Convert a int32_t from network to host byte order
 */
int32_t NetworkToHost(int32_t value) {
  return ntohl(value);
}

uint8_t HostToNetwork(uint8_t value) {
  return value;
}


/*
 * Convert a uint16_t from network to host byte order
 */
uint16_t HostToNetwork(uint16_t value) {
  return htons(value);
}


/*
 * Convert a uint32_t from host to network byte order
 */
uint32_t HostToNetwork(uint32_t value) {
  return htonl(value);
}


uint8_t HostToLittleEndian(uint8_t value) {
  return value;
}


uint16_t HostToLittleEndian(uint16_t value) {
#ifdef HAVE_ENDIAN_H
#  if BYTE_ORDER == __BIG_ENDIAN
  return ((value & 0xff) << 8) | (value >> 8);
#  else
  return value;
#  endif
#else
#  if BYTE_ORDER == BIG_ENDIAN
  return ((value & 0xff) << 8) | (value >> 8);
#  else
  return value;
#  endif
#endif
}


uint32_t HostToLittleEndian(uint32_t value) {
#ifdef HAVE_ENDIAN_H
#  if BYTE_ORDER == __BIG_ENDIAN
  return ((value & 0xff) << 24) | (value & 0xff00 << 16) |
          (0xff00 & (value >> 16) | (value >> 24));
#  else
  return value;
#  endif
#else
#  if BYTE_ORDER == BIG_ENDIAN
  return ((value & 0xff) << 24) | (value & 0xff00 << 16) |
          (0xff00 & (value >> 16) | (value >> 24));
#  else
  return value;
#  endif
#endif
}

uint8_t LittleEndianToHost(uint8_t value) {
  return value;
}


uint16_t LittleEndianToHost(uint16_t value) {
#ifdef HAVE_ENDIAN_H
#  if BYTE_ORDER == __BIG_ENDIAN
  return ((value & 0xff) << 8) | (value >> 8);
#  else
  return value;
#  endif
#else
#  if BYTE_ORDER == BIG_ENDIAN
  return ((value & 0xff) << 8) | (value >> 8);
#  else
  return value;
#  endif
#endif
}


uint32_t LittleEndianToHost(uint32_t value) {
#ifdef HAVE_ENDIAN_H
#  if BYTE_ORDER == __BIG_ENDIAN
  return ((value & 0xff) << 24) | (value & 0xff00 << 16) |
          (0xff00 & (value >> 16) | (value >> 24));
#  else
  return value;
#  endif
#else
#  if BYTE_ORDER == BIG_ENDIAN
  return ((value & 0xff) << 24) | (value & 0xff00 << 16) |
          (0xff00 & (value >> 16) | (value >> 24));
#  else
  return value;
#  endif
#endif
}

/*
 * Return the full hostname
 */
string FullHostname() {
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


/*
 * Return the hostname as a string.
 */
string Hostname() {
  string hostname = FullHostname();
  std::vector<string> tokens;
  StringSplit(hostname, tokens, ".");
  return string(tokens[0]);
}
}  // network
}  // ola
