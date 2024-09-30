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
 * Utils.h
 * Basic util helper functions.
 * Copyright (C) 2013 Peter Newman
 */

#ifndef INCLUDE_OLA_UTIL_UTILS_H_
#define INCLUDE_OLA_UTIL_UTILS_H_

#include <stdint.h>

#include <limits>

namespace ola {
namespace utils {

/**
 * @brief Convert a uint16_t to two uint8_t's
 * @param[in] input the uint16_t to split
 * @param[out] high the high byte
 * @param[out] low the low byte
 */
inline void SplitUInt16(uint16_t input, uint8_t *high, uint8_t *low) {
  *high = static_cast<uint8_t>((input >> std::numeric_limits<uint8_t>::digits) &
      std::numeric_limits<uint8_t>::max());
  *low = static_cast<uint8_t>(input & std::numeric_limits<uint8_t>::max());
}


/**
 * @brief Convert two uint8_t's to a uint16_t
 * @param high the high byte
 * @param low the low byte
 * @return the combined uint16_t
 */
inline uint16_t JoinUInt8(uint8_t high, uint8_t low)  {
  return static_cast<uint16_t>(
      (static_cast<uint16_t>(high) << std::numeric_limits<uint8_t>::digits)
      | low);
}


/**
 * @brief Convert four uint8_t's to a uint32_t
 * @param byte0 the highest byte
 * @param byte1 the high middle byte
 * @param byte2 the low middle byte
 * @param byte3 the lowest byte
 * @return the combined uint32_t
 */
inline uint32_t JoinUInt8(uint8_t byte0, uint8_t byte1, uint8_t byte2,
                          uint8_t byte3)  {
  return ((static_cast<uint32_t>(byte0) <<
           (std::numeric_limits<uint8_t>::digits * 3))
          | (static_cast<uint32_t>(byte1) <<
             (std::numeric_limits<uint8_t>::digits * 2))
          | (static_cast<uint32_t>(byte2) <<
             (std::numeric_limits<uint8_t>::digits))
          | byte3);
}


/**
 * @brief Truncate a uint16_t keeping the high byte
 * @param[in] input the uint16_t to truncate
 * @return the high byte
 */
inline uint8_t TruncateUInt16High(uint16_t input) {
  return static_cast<uint8_t>((input >> std::numeric_limits<uint8_t>::digits) &
      std::numeric_limits<uint8_t>::max());
}


/**
 * @brief Truncate a uint16_t keeping the low byte
 * @param[in] input the uint16_t to truncate
 * @return the low byte
 */
inline uint8_t TruncateUInt16Low(uint16_t input) {
  return static_cast<uint8_t>(input & std::numeric_limits<uint8_t>::max());
}
}  // namespace utils
}  // namespace ola
#endif  // INCLUDE_OLA_UTIL_UTILS_H_
