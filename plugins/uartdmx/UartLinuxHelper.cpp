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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// this provides ioctl() definition without conflicting with asm/termios.h
#include <stropts.h>
#include <asm/termios.h>  // use this not standard termios for custom baud rates

#include "plugins/uartdmx/UartLinuxHelper.h"

namespace ola {
namespace plugin {
namespace uartdmx {

bool LinuxHelper::SetDmxBaud(const int fd) {
struct termios2 tio;  // linux-specific terminal stuff
const int rate = 250000;

/* Set up custom speed for UART */
if (ioctl(fd, TCGETS2, &tio) < 0)  // get current uart state
  return false;
tio.c_cflag &= ~CBAUD;
tio.c_cflag |= BOTHER;
tio.c_ispeed = rate;
tio.c_ospeed = rate;  // set custom speed directly
if (ioctl(fd, TCSETS2, &tio) < 0)  // push uart state
  return false;
#if 0
if (verbose > 1) {
    // if verbose, read and print
    // get current uart state
    if (ioctl(fh, TCGETS2, &tio) < 0) {
         fprintf(stderr, "Error getting altered settings from port\n");
    } else {
         fprintf(stderr, "Port speeds are %i in and %i out\n",
           tio.c_ispeed, tio.c_ospeed);
    }
}
#endif
return true;
}

}  // namespace uartdmx
}  // namespace plugin
}  // namespace ola
