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
 * SocketAddress.cpp
 * Represents a sockaddr structure.
 * Copyright (C) 2012 Simon Newton
 */

#include <assert.h>
#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/SocketAddress.h>
#include <string.h>
#include <string>

namespace ola {
namespace network {

using std::string;

/**
 * Copy this IPV4SocketAddress into a sockaddr.
 */
bool IPV4SocketAddress::ToSockAddr(struct sockaddr *addr,
                                   unsigned int size) const {
  if (size < sizeof(struct sockaddr_in)) {
    OLA_FATAL << "Length passed to ToSockAddr is too small.";
    return false;
  }
  struct sockaddr_in *v4_addr = reinterpret_cast<struct sockaddr_in*>(addr);

  memset(v4_addr, 0, size);
  v4_addr->sin_family = AF_INET;
  v4_addr->sin_port = HostToNetwork(m_port);
  v4_addr->sin_addr = m_host.Address();
  return true;
}


/**
 * Convert the sockaddr to a sockaddr_in.
 * The caller should check that Family() is AF_INET before calling this.
 */
IPV4SocketAddress GenericSocketAddress::V4Addr() const {
  if (Family() == AF_INET) {
    const struct sockaddr_in *v4_addr =
      reinterpret_cast<const struct sockaddr_in*>(&m_addr);
    return IPV4SocketAddress(IPV4Address(v4_addr->sin_addr),
                             NetworkToHost(v4_addr->sin_port));
  } else {
    OLA_FATAL << "Invalid conversion of socket family " << Family();
    return IPV4SocketAddress(IPV4Address(), 0);
  }
}


/**
 * Extract a IPV4SocketAddress from a string.
 */
bool IPV4SocketAddress::FromString(const string &input,
                                   IPV4SocketAddress *socket_address) {
  size_t pos = input.find_first_of(":");
  if (pos == string::npos)
    return false;

  IPV4Address address;
  if (!IPV4Address::FromString(input.substr(0, pos), &address))
    return false;
  uint16_t port;
  if (!StringToInt(input.substr(pos + 1), &port))
    return false;
  *socket_address = IPV4SocketAddress(address, port);
  return true;
}


IPV4SocketAddress IPV4SocketAddress::FromStringOrDie(
    const std::string &address) {
  IPV4SocketAddress socket_address;
  assert(FromString(address, &socket_address));
  return socket_address;
}
}  // network
}  // ola
