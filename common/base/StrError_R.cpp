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
#include <sstream>

#include "ola/base/StrError_R.h"
#include "ola/strings/Format.h"

namespace ola {

const int StrError_R_BufSize = 1024;

std::string StrError_R(int errnum) {
  // Pre-allocate to prevent re-allocation.
  std::string out(StrError_R_BufSize, '\0');
  if (StrError_R_XSI(errnum, &(out[0]), out.size())) {
    out.assign(std::string("errno = ") + ola::strings::IntToString(errnum));
  } else {
    out.resize(strlen(&(out[0])));
  }
  return out;
}

}  // namespace ola
