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
 * IOUtils.cpp
 * IO Util functions.
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef INCLUDE_OLA_IO_IOUTILS_H_
#define INCLUDE_OLA_IO_IOUTILS_H_

#include <termios.h>

namespace ola {
namespace io {


/**
 * Convert an integer baud rate to the termios struct speed_t
 */
speed_t IntegerToSpeedT(unsigned int baudrate) {
  switch (baudrate) {
    case 9600:
      return B9600;

    case 19200:
      return B19200;

    case 38400:
      return B38400;

    case 57600:
      return B57600;

    case 115200:
      return B115200;
  }
  
  return B0;
}
}  // namespace  io
}  // namespace  ola
#endif  // INCLUDE_OLA_IO_IOUTILS_H_
