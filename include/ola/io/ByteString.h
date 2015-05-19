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
 * ByteString.h
 * A string of uint8_t data.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef INCLUDE_OLA_IO_BYTESTRING_H_
#define INCLUDE_OLA_IO_BYTESTRING_H_

#include <stdint.h>
#include <string>

namespace ola {
namespace io {

/**
 * @brief A contiguous block of uint8_t data.
 *
 * The constraints of the .data() and .c_str() methods from the C++ standard
 * mean that string stores it's data in a contiguous block.
 *
 * In some implementations, basic_string may be reference counted, but with the
 * additional contraints added by C++11 this will not be the case, so think
 * twice about copying ByteStrings.
 */
typedef std::basic_string<uint8_t> ByteString;

}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_BYTESTRING_H_
