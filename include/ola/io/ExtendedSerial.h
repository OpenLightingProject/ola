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
 * ExtendedSerial.h
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#ifndef INCLUDE_OLA_IO_EXTENDEDSERIAL_H_
#define INCLUDE_OLA_IO_EXTENDEDSERIAL_H_

namespace ola {
namespace io {

/**
 * @brief A static class containing platform-specific helper code
 * for Linux.
 *
 * This code can't live in the UartWidget() class because it's
 * platform specfic-includes clash with the POSIX ones there
 */
class LinuxHelper {
 public:
  /**
   * Set the baud rate of the serial port to 250k using the non-standard
   * speed selection mechanism from the Linux kernel.
   */
  static bool SetDmxBaud(int fd);
};
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_EXTENDEDSERIAL_H_
