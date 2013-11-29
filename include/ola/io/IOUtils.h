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

namespace ola {
namespace io {


/**
 * @brief Convert an integer baud rate to the termios struct speed_t
 * @param[in] baudrate value to convert
 * @param[out] output a pointer where the value will be stored
 * @returns true if the value was converted, false if the baud rate wasn't supported
 * by the method.
 */
bool UIntToSpeedT(uint32_t value, speed_t *output) {
  switch (value) {
    case 9600:
      *output = B9600;
      return true;

    case 19200:
      *output = B19200;
      return true;

    case 38400:
      *output = B38400;
      return true;

    case 57600:
      *output = B57600;
      return true;

    case 115200:
      *output = B115200;
      return true;

    case 230400:
      *output = B230400;
      return true;
  }

  return false;
}
}  // namespace  io
}  // namespace  ola
#endif  // INCLUDE_OLA_IO_IOUTILS_H_
