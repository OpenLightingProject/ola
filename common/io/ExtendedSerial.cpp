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
 * ExtendedSerial.cpp
 * The DMX through a UART plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#include "ola/io/ExtendedSerial.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STROPTS_H
// this provides ioctl() definition without conflicting with asm/termios.h
#include <stropts.h>
#else
// otherwise use the sys/ioctl.h version for newer Linux which has dropped the
// stropts.h version
// N.B. This will pull in the kernel definition of struct termios, which may
// conflict with the libc version, so we wouldn't be able to use both in the
// same file
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif  // HAVE_SYS_IOCTL_H
#ifdef HAVE_ASM_TERMBITS_H
#include <asm/termbits.h>
#endif  // HAVE_ASM_TERMBITS_H
#endif  // HAVE_STROPTS_H

#if defined(HAVE_STROPTS_H) && defined(HAVE_ASM_TERMIOS_H)
// use this non-standard termios for custom baud rates
//
// On newer Linux, this duplicates winsize and termio as they're also defined
// in bits/ioctl-types.h, so only include this header if we also have the
// stropts.h sourced version of ioctl()
//
// On mips architectures, <asm/termios.h> sets some cpp macros which cause
// <cerrno> (included by <ostream>, used by <ola/Logging.h>) to not define
// ERANGE, EDOM, or EILSEQ, causing a spectacular compile failure there.
//
// Explicitly include <cerrno> now to avoid the issue.
#include <errno.h>
#include <asm/termios.h>
#endif  // HAVE_ASM_TERMIOS_H

#include <ola/Logging.h>

namespace ola {
namespace io {

bool LinuxHelper::SetDmxBaud(int fd) {
#if (defined(HAVE_STROPTS_H) || \
     (defined(HAVE_SYS_IOCTL_H) && defined(HAVE_ASM_TERMBITS_H))) && \
    defined(HAVE_TERMIOS2)
  static const int rate = 250000;

  struct termios2 tio;  // linux-specific terminal stuff

  if (ioctl(fd, TCGETS2, &tio) < 0) {
    OLA_INFO << "Failed to get current serial port settings";
    return false;
  }

  tio.c_cflag &= ~CBAUD;
  tio.c_cflag |= BOTHER;
  tio.c_ispeed = rate;
  tio.c_ospeed = rate;  // set custom speed directly
  if (ioctl(fd, TCSETS2, &tio) < 0) {
    OLA_INFO << "Failed to update serial port settings";
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
#else
  OLA_INFO << "Failed to set baud rate, due to missing "
#if !defined(HAVE_STROPTS_H)
           << "stropts.h or "
#endif
#if !(defined(HAVE_SYS_IOCTL_H) && defined(HAVE_ASM_TERMBITS_H))
           << "sys/ioctl.h or asm/termbits.h or "
#endif
           << "termios2";
  return false;
  (void) fd;
#endif  // (defined(HAVE_STROPTS_H) ||
//  (defined(HAVE_SYS_IOCTL_H) && defined(HAVE_ASM_TERMBITS_H))) &&
// defined(HAVE_TERMIOS2)
}
}  // namespace io
}  // namespace ola
