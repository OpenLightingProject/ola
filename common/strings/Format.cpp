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
 * Format.cpp
 * Formatting functions for basic types.
 * Copyright (C) 2005 Simon Newton
 */

#include "ola/strings/Format.h"

#include <iomanip>
#include <sstream>
#include <string>

namespace ola {
namespace strings {

using std::endl;
using std::ostringstream;
using std::string;

string IntToString(int64_t i) {
  ostringstream str;
  str << i;
  return str.str();
}

string IntToString(uint64_t i) {
  ostringstream str;
  str << i;
  return str.str();
}

void FormatData(std::ostream *out,
                const uint8_t *data,
                unsigned int length,
                unsigned int indent,
                unsigned int byte_per_line) {
  ostringstream raw, ascii;
  raw << std::hex;
  for (unsigned int i = 0; i != length; i++) {
    raw << std::setfill('0') << std::setw(2)
        << static_cast<unsigned int>(data[i]) << " ";
    if (isprint(data[i])) {
      ascii << data[i];
    } else {
      ascii << ".";
    }

    if (i % byte_per_line == byte_per_line - 1) {
      *out << string(indent, ' ') << raw.str() << " " << ascii.str() << endl;
      raw.str("");
      ascii.str("");
    }
  }
  if (length % byte_per_line != 0) {
    // pad if needed
    raw << string(3 * (byte_per_line - (length % byte_per_line)), ' ');
    *out << string(indent, ' ') << raw.str() << " " << ascii.str() << endl;
  }
}
}  // namespace strings
}  // namespace ola
