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
 * Strerror_r.cpp
 * Definition of strerror_r that is XSI-compliant.
 * Copyright (C) 2018 Shenghao Yang
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

// Required for string.h to declare the standards-compliant version of
// strerror_r(), when compiling under glibc. Must come before the inclusion
// of string.h.
// These are conditional to avoid errors when not compiling with glibc, or
// when compiling with a compiler that does not define _POSIX_C_SOURCE.
// See strerror(3) as to why the macros are defined this way.
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200112L

#include <string.h>

#include "ola/base/Strerror_r.h"

namespace ola {

int Strerror_r(int errnum, char* buf, size_t buflen) {
  return strerror_r(errnum, buf, buflen);
}

}  // namespace ola
