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
 * SocketCloser.h
 * Close a socket when the object goes out of scope.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup network
 * @{
 * @file SocketCloser.h
 * @brief Automatically close a socket when it goes out of scope.
 * @}
 */

#ifndef INCLUDE_OLA_NETWORK_SOCKETCLOSER_H_
#define INCLUDE_OLA_NETWORK_SOCKETCLOSER_H_

#include <unistd.h>
#include <ola/base/Macro.h>

namespace ola {
namespace network {

/**
 * @addtogroup network
 * @{
 */

/**
 * @brief Automatically close a socket when it goes out of scope.
 *
 * This class is useful if you need to temporarily open a socket and want to
 * make sure it's cleaned up. Think of it as an unique_ptr for file descriptors.
 */
class SocketCloser {
 public:
  /**
   * @brief Create a new SocketCloser.
   * @param fd the file descriptor to close.
   */
  explicit SocketCloser(int fd)
    : m_fd(fd) {
  }

  /**
   * @brief Destructor.
   */
  ~SocketCloser();

  /**
   * @brief Release the file descriptor.
   *
   * Calling Release prevents the file descriptor from being closed when this
   * object goes out of scope.
   * @returns the original file descriptor or -1 if the descriptor was already
   * released.
   */
  int Release() {
    int fd = m_fd;
    m_fd = -1;
    return fd;
  }

 private:
  int m_fd;

  DISALLOW_COPY_AND_ASSIGN(SocketCloser);
};
/**
 * @}
 */
}  // namespace network
}  // namespace ola
#endif  // INCLUDE_OLA_NETWORK_SOCKETCLOSER_H_
