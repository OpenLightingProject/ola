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
 * Limits.cpp
 * Functions that deal with system limits (rlimits)
 * Copyright (C) 2014 Simon Newton
 */


#ifndef _WIN32
#include "ola/system/Limits.h"

#include <errno.h>
#include <string.h>
#include <sys/resource.h>

#include "ola/Logging.h"

namespace ola {
namespace system {

bool GetRLimit(int resource, struct rlimit *lim) {
  int r = getrlimit(resource, lim);
  if (r) {
    OLA_WARN << "getrlimit(" << resource << "): " << strerror(errno);
    return false;
  }
  return true;
}

bool SetRLimit(int resource, const struct rlimit &lim) {
  int r = setrlimit(resource, &lim);
  if (r) {
    OLA_WARN << "setrlimit(" << resource << "): " << strerror(errno);
    return false;
  }
  return true;
}
}  // namespace system
}  // namespace ola
#endif  // _WIN32
