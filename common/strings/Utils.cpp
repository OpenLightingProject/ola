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
 * Utils.cpp
 * Miscellaneous string functions.
 * Copyright (C) 2014 Simon Newton
 */

#include "ola/strings/Utils.h"

#include <string.h>
#include <string>

namespace ola {
namespace strings {

using std::string;

void CopyToFixedLengthBuffer(const std::string &input,
                             char *buffer,
                             unsigned int size) {
  // buffer may not be NULL terminated.
  // coverity[BUFFER_SIZE_WARNING]
  strncpy(buffer, input.c_str(), size);
}
}  // namespace strings
}  // namespace ola
