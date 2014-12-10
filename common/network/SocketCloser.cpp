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
 * SocketCloser.cpp
 * Close a socket when the object goes out of scope.
 * Copyright (C) 2013 Simon Newton
 */

#include "ola/network/SocketCloser.h"

#include <errno.h>
#include <string.h>

#ifdef _WIN32
#include <Ws2tcpip.h>
#endif

namespace ola {
namespace network {

SocketCloser::~SocketCloser() {
  if (m_fd >= 0) {
#ifdef _WIN32
    closesocket(m_fd);
#else
    close(m_fd);
#endif
  }
}

}  // namespace network
}  // namespace ola
