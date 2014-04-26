/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UartLinuxHelper.h
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#ifndef PLUGINS_UARTDMX_UARTLINUXHELPER_H_
#define PLUGINS_UARTDMX_UARTLINUXHELPER_H_

namespace ola {
namespace plugin {
namespace uartdmx {

/** @brief A static class containing platform-specific helper code
 * for Linux
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
  static bool SetDmxBaud(const int fd);
};
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_UARTDMX_UARTLINUXHELPER_H_
