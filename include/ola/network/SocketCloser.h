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
 * SocketCloser.h
 * Close a socket when the object goes out of scope.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_SOCKETCLOSER_H_
#define INCLUDE_OLA_NETWORK_SOCKETCLOSER_H_

#include <unistd.h>

namespace ola {
namespace network {

class SocketCloser {
  public:
    explicit SocketCloser(int fd)
      : m_fd(fd) {
    }
    ~SocketCloser() {
      if (m_fd >= 0)
        close(m_fd);
    }

    int Release() {
      int fd = m_fd;
      m_fd = -1;
      return fd;
    }
  private:
    int m_fd;
};
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_SOCKETCLOSER_H_
