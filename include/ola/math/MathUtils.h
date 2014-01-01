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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * MathUtils.h
 * Basic math helper functions.
 * Copyright (C) 2013 Peter Newman
 */

#ifndef INCLUDE_OLA_MATH_MATHUTILS_H_
#define INCLUDE_OLA_MATH_MATHUTILS_H_

#include <stdint.h>

#include <limits>

namespace ola {
namespace math {

/**
 * @brief Convert a uint16_t to two uint8_t's
 * @param[in] input the uint16_t to split
 * @param[out] high the high byte
 * @param[out] low the low byte
 */
void UInt16ToTwoUInt8(const uint16_t &input, uint8_t *high, uint8_t *low) {
  *high = (input >> std::numeric_limits<uint8_t>::digits) &
      std::numeric_limits<uint8_t>::max();
  *low = input & std::numeric_limits<uint8_t>::max();
}

/**
 * @brief Convert two uint8_t's to a uint16_t
 * @param[in] high the high byte
 * @param[in] low the low byte
 * @param[out] output the combined uint16_t
 */
void TwoUInt8ToUInt16(const uint8_t &high,
                      const uint8_t &low,
                      uint16_t *output) {
  *output = ((uint16_t)high << std::numeric_limits<uint8_t>::digits) | low;
}
}  // namespace math
}  // namespace ola
#endif  // INCLUDE_OLA_MATH_MATHUTILS_H_
