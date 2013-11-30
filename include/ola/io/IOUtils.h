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
 * IOUtils.h
 * IO Util functions.
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef INCLUDE_OLA_IO_IOUTILS_H_
#define INCLUDE_OLA_IO_IOUTILS_H_

#include <stdint.h>
#include <termios.h>

#include <string>

namespace ola {
namespace io {

/**
 * @brief Wrapper around open().
 * This logs a message is the open fails.
 * @param path the path to open
 * @param oflag flags passed to open
 * @param[out] fd a pointer to the fd which is returned.
 * @returns true if the open succeeded, false otherwise.
 */
bool Open(const std::string &path, int oflag, int *fd);

/**
 * @brief Convert an integer baud rate to the termios struct speed_t
 * @param[in] baudrate value to convert
 * @param[out] output a pointer where the value will be stored
 * @returns true if the value was converted, false if the baud rate wasn't
 * supported by the method.
 */
bool UIntToSpeedT(uint32_t value, speed_t *output);
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_IOUTILS_H_
