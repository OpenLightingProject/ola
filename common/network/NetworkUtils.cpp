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

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"


namespace ola {
namespace network {


/*
 * Convert a string to a struct in_addr
 */
bool StringToAddress(const string &address, struct in_addr &addr) {
  if (inet_aton(address.data(), &addr) == 0) {
    OLA_WARN << "Could not convert multicast address " << address;
    return false;
  }
  return true;
}


string AddressToString(struct in_addr &addr) {
  return inet_ntoa(addr);
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
