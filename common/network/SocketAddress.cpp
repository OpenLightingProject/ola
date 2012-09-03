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

#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include <ola/network/SocketAddress.h>
#include <string.h>

namespace ola {
namespace network {


/**
 * Copy this IPV4SocketAddress into a sockaddr.
 */
bool IPV4SocketAddress::ToSockAddr(struct sockaddr *addr, unsigned int size) {
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
}  // network
}  // ola
