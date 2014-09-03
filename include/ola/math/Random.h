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
 * Random.h
 * A simple random number generator.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_MATH_RANDOM_H_
#define INCLUDE_OLA_MATH_RANDOM_H_

namespace ola {
namespace math {

/**
 * @brief Seed the random number generator
 */
void InitRandom();

/**
 * @brief Return a random number between lower and upper, inclusive. i.e.
 * [lower, upper]
 * @param lower the lower bound of the range the random number should be within
 * @param upper the upper bound of the range the random number should be within
 * @return the random number
 */
int Random(int lower, int upper);
}  // namespace math
}  // namespace ola
#endif  // INCLUDE_OLA_MATH_RANDOM_H_
