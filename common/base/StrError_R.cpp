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
 * StrError_R.cpp
 * Helper function for StrError_R_XSI().
 * Copyright (C) 2018 Shenghao Yang
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <stdio.h>
#include <string.h>

#include <limits>
#include <string>

#include "ola/base/StrError_R.h"

namespace ola {

const int StrError_R_BufSize(1024);

std::string StrError_R(int errnum) {
  // Pre-allocate to prevent re-allocation.
  std::string out(StrError_R_BufSize, '\0');
  if (StrError_R_XSI(errnum, &(out[0]), out.size())) {
    out.assign("errno = ");
    // Buffer size here is digits10 + 3 to account for:
    // - Systems in which the int type cannot represent the full range
    //   of digits in the position of the most significant base 10 digit.
    //   (i.e. 0-2 representable in an 8-bit unsigned integer at the
    //   hundredth's place instead of 0-9 because an u8 is clamped to
    //   [0, 255])
    // - Terminating NUL byte added by snprintf.
    // - 1 additional unit of storage just in case a negative errnum is
    //   passed, and somehow StrError_R_XSI() failed on that.
    char errs[std::numeric_limits<int>::digits10 + 3];
    int r(snprintf(errs, sizeof(errs), "%d", errnum));
    if (r < 0) {
      out.append("<error>");
    // Ugly cast required to avoid compiler warning when comparing
    // numbers of different signedness.
    } else if (static_cast<unsigned int>(r) >= sizeof(errs)) {
      out.append("<truncated>");
    } else {
      out.append(errs);
    }
  } else {
    out.resize(strlen(&(out[0])));
  }
  return out;
}

}  // namespace ola
