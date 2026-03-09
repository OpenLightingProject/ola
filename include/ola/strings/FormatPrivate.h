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
 * FormatPrivate.h
 * Private implementations of the formatting functions from Format.h
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @file FormatPrivate.h
 * @brief Private implementations of the formatting functions from Format.h
 */

#ifndef INCLUDE_OLA_STRINGS_FORMATPRIVATE_H_
#define INCLUDE_OLA_STRINGS_FORMATPRIVATE_H_

#include <stdint.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>

namespace ola {
namespace strings {

/* @brief the width of a hex character in bits */
enum { HEX_BIT_WIDTH = 4 };

/*
 * Internal type used by ToHex().
 */
template<typename T>
struct _ToHex {
 public:
  _ToHex(T v, int _width, bool _prefix)
      : width(_width),
        value(v),
        prefix(_prefix) {
  }

  int width;  // setw takes an int
  T value;
  bool prefix;
};

inline uint32_t _HexCast(uint8_t v) { return v; }
inline uint32_t _HexCast(int8_t v) { return static_cast<uint8_t>(v); }
inline uint16_t _HexCast(uint16_t v) { return v; }
inline uint16_t _HexCast(int16_t v) { return static_cast<uint16_t>(v); }
inline uint32_t _HexCast(uint32_t v) { return v; }
inline uint32_t _HexCast(int32_t v) { return static_cast<uint32_t>(v); }
inline uint64_t _HexCast(uint64_t v) { return v; }
inline uint64_t _HexCast(int64_t v) { return static_cast<uint64_t>(v); }

}  // namespace strings
}  // namespace ola
#endif  // INCLUDE_OLA_STRINGS_FORMATPRIVATE_H_
