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
 * UartLinuxHelper.cpp
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include "plugins/uartdmx/UartLinuxHelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// this provides ioctl() definition without conflicting with asm/termios.h
#include <stropts.h>
#include <asm/termios.h>  // use this not standard termios for custom baud rates
#include <ola/Logging.h>

namespace ola {
namespace plugin {
namespace uartdmx {

bool LinuxHelper::SetDmxBaud(int fd) {
  static const int rate = 250000;

  struct termios2 tio;  // linux-specific terminal stuff

  if (ioctl(fd, TCGETS2, &tio) < 0) {
    return false;
  }

  tio.c_cflag &= ~CBAUD;
  tio.c_cflag |= BOTHER;
  tio.c_ispeed = rate;
  tio.c_ospeed = rate;  // set custom speed directly
  if (ioctl(fd, TCSETS2, &tio) < 0) {
    return false;
  }

  if (LogLevel() >= OLA_LOG_INFO) {
    if (ioctl(fd, TCGETS2, &tio) < 0) {
       OLA_INFO << "Error getting altered settings from port";
    } else {
       OLA_INFO << "Port speeds for " << fd << " are " << tio.c_ispeed
                << " in and " << tio.c_ospeed << " out";
    }
  }
  return true;
}
}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
