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
 * Array.h
 * A macro for determining array size.
 */

/**
 * @file Array.h
 * @brief Helper macros / methods for static arrays.
 */

#ifndef INCLUDE_OLA_BASE_ARRAY_H_
#define INCLUDE_OLA_BASE_ARRAY_H_

namespace ola {

/**
 * @brief A helper to determine the size of a statically allocated array.
 * @private
 * @tparam T is the type of your object.
 * @tparam N is the size of your type T.
 *
 * @note
 * Please see
 * http://src.chromium.org/svn/trunk/src/third_party/cld/base/macros.h
 * for all the gory details
 */
template <typename T, size_t N>
  char (&ArraySizeHelper(T (&array)[N]))[N];

/**
 * @brief Part of a helper to determine the size of a statically allocated
 * const array
 * @private
 *
 * @tparam T is your class or variable
 * @tparam N is the size of your type T
 */
template <typename T, size_t N>
  char (&ArraySizeHelper(const T (&array)[N]))[N];


/**
 * @def arraysize(array)
 * @brief Computes the size of the statically allocated array.
 * @param array the array to get the size of.
 * @return the number of elements in the array.
 */
#define arraysize(array) (sizeof(ola::ArraySizeHelper(array)))

}  // namespace ola
#endif  // INCLUDE_OLA_BASE_ARRAY_H_
