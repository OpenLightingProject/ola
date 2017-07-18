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
 * OscAddressTemplate.cpp
 * Expand a template, substituting values.
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include "ola/StringUtils.h"
#include "plugins/osc/OscAddressTemplate.h"

namespace ola {
namespace plugin {
namespace osc {

using std::string;

/**
 * If the string contains %d, replace it with the given value. This only
 * replaces the first instance of %d.
 * @param str the template string to use
 * @param value the value to use as the replacement.
 * @returns str with %d replaced by value.
 */
string ExpandTemplate(const string &str, unsigned int value) {
  string output = str;
  // find the first instance of "%d" in the string.
  size_t pos = output.find("%d");
  if (pos != string::npos) {
    // %d was found, perform the replacement.
    output.replace(pos, 2, IntToString(value));
  }
  return output;
}
}  // namespace osc
}  // namespace plugin
}  // namespace ola
