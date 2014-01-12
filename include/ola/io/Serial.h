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
 * Serial.h
 * Serial IO functions.
 * Copyright (C) 2014 Peter Newman
 */

#ifndef INCLUDE_OLA_IO_SERIAL_H_
#define INCLUDE_OLA_IO_SERIAL_H_

#include <stdint.h>
#include <termios.h>

namespace ola {
namespace io {

typedef enum {
  BAUD_RATE_9600 = 9600,
  BAUD_RATE_19200 = 19200,
  BAUD_RATE_38400 = 38400,
  BAUD_RATE_57600 = 57600,
  BAUD_RATE_115200 = 115200,
  BAUD_RATE_230400 = 230400,
} baud_rate;

/**
 * @brief Convert an integer baud rate to the termios struct speed_t
 * @param[in] value the baudrate value to convert
 * @param[out] output a pointer where the value will be stored
 * @returns true if the value was converted, false if the baud rate wasn't
 * supported by the method.
 */
bool UIntToSpeedT(uint32_t value, speed_t *output);
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_SERIAL_H_
