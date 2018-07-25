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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Systemd.cpp
 * Provides wrapped access to systemd interfaces.
 * Copyright (C) 2018 Shenghao Yang
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#if HAVE_LIBSYSTEMD
#include <systemd/sd-daemon.h>
#include <errno.h>
#include <string.h>
#endif  // HAVE_LIBSYSTEMD

#include "ola/Logging.h"

#include "olad/Systemd.h"

namespace ola {

int SystemdNotify(int unset_environment, const char *state) {
  int rtn = sd_notify(unset_environment, state);
  if (rtn < 0) {
    char buf[1024];
    // olad is compiled under GNU extensions: the return value of
    // strerror_r() is not an error code but a pointer to a character array
    // containing the error description. The array returned is always NUL
    // terminated, even in case of errors, so blindly using it is not a problem.
    OLA_WARN << "Error sending notification to systemd: "
             << strerror_r(-rtn, buf, sizeof(buf));
  }
  return rtn;
}

bool SystemdNotifyAvailable() {
  return (sd_notify(0, "") != 0);
}

}  // namespace ola
