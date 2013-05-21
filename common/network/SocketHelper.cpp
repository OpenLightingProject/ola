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
 * SocketHelper.cpp
 * Various functions to operate on sockets.
 * Copyright (C) 2013 Simon Newton
 */

#include <errno.h>
#include <string.h>

#include <ola/Logging.h>
#include <ola/network/SocketAddress.h>

namespace ola {
namespace network {

/**
 * Wrapper around getsockname().
 * The caller should check IsValid() on the GenericSocketAddress before using.
 */
GenericSocketAddress GetLocalAddress(int sd) {
  struct sockaddr remote_address;
  socklen_t length = sizeof(remote_address);
  int r = getsockname(sd, &remote_address, &length);
  if (r) {
    OLA_WARN << "Failed to get peer information for fd: " << sd << ", "
             << strerror(errno);
    return GenericSocketAddress();
  }
  return GenericSocketAddress(remote_address);
}

/**
 * Wrapper around getpeername().
 * The caller should check IsValid() on the GenericSocketAddress before using.
 */
GenericSocketAddress GetPeerAddress(int sd) {
  struct sockaddr remote_address;
  socklen_t length = sizeof(remote_address);
  int r = getpeername(sd, &remote_address, &length);
  if (r) {
    OLA_WARN << "Failed to get peer information for fd: " << sd << ", "
             << strerror(errno);
    return GenericSocketAddress();
  }
  return GenericSocketAddress(remote_address);
}
}  // namespace network
}  // namespace ola
