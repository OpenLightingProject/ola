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
 * Format.h
 * Formatting functions for basic types.
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @file Format.h
 * @brief Formatting functions for basic types.
 */

#ifndef INCLUDE_OLA_STRINGS_FORMAT_H_
#define INCLUDE_OLA_STRINGS_FORMAT_H_

#include <stdint.h>
#include <ola/strings/FormatPrivate.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>

namespace ola {
namespace strings {

/**
 * @brief Convert an int to a string.
 * @param i the int to convert
 * @return the string representation of the int
 */
std::string IntToString(int64_t i);

/**
 * @brief Convert an unsigned int to a string.
 * @param i the unsigned int to convert
 * @return The string representation of the unsigned int
 */
std::string IntToString(uint64_t i);

inline std::string IntToString(int i) {
  return ola::strings::IntToString(static_cast<int64_t>(i));
}

inline std::string IntToString(unsigned int i) {
  return ola::strings::IntToString(static_cast<uint64_t>(i));
}

/**
 * @brief Convert a value to a hex string.
 *
 * Automatic constructor for _ToHex that deals with widths
 * @tparam T the type of value to convert
 * @param v the value to convert
 * @param prefix show the 0x prefix
 * @return A _ToHex struct representing the value, output it to an ostream to
 *     use it.
 * @note We only currently support unsigned ints due to a lack of requirement
 * for anything else
 */
template<typename T>
_ToHex<T> ToHex(T v, bool prefix = true) {
  return _ToHex<T>(v, (std::numeric_limits<T>::digits / HEX_BIT_WIDTH), prefix);
}

/**
 * @brief Output the _ToHex type to an ostream
 */
template <typename T>
std::ostream& operator<<(std::ostream &out, const ola::strings::_ToHex<T> &i) {
  std::ios::fmtflags flags(out.flags());  // Store the current format flags
  // In C++, you only get the 0x on non-zero values, so we have to explicitly
  // add it for all values if we want it
  if (i.prefix) {
    out << "0x";
  }
  out << std::setw(i.width) << std::hex << std::setfill('0')
      << ola::strings::_HexCast(i.value);
  out.flags(flags);  // Put the format flags back to normal
  return out;
}

/**
 * @brief Write binary data to an ostream in a human readable form.
 *
 * @param out the ostream to write to
 * @param data pointer to the data
 * @param length length of the data
 * @param indent the number of spaces to prefix each line with
 * @param byte_per_line the number of bytes to display per line
 *
 * @note The data is printed in two columns, hex on the left, ascii on the
 * right. Non ascii values are printed as `.'
 */
void FormatData(std::ostream *out,
                const uint8_t *data,
                unsigned int length,
                unsigned int indent = 0,
                unsigned int byte_per_line = 8);
}  // namespace strings
}  // namespace ola
#endif  // INCLUDE_OLA_STRINGS_FORMAT_H_
