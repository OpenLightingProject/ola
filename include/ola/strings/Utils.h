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
 * Miscellaneous string functions.
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @file ola/strings/Utils.h
 * @brief Miscellaneous string functions
 */

#ifndef INCLUDE_OLA_STRINGS_UTILS_H_
#define INCLUDE_OLA_STRINGS_UTILS_H_

#include <string.h>
#include <string>

namespace ola {
namespace strings {

/**
 * @brief Copy a string to a fixed length buffer without NULL terminating.
 * @param input The string to copy to the buffer.
 * @param buffer The memory location to copy the contents of the string to.
 * @param size The size of the memory buffer.
 * @note The buffer may not be NULL terminated, it's not safe to use
 *   functions like strlen(), printf() etc. on the result. This is typically
 *   used in RDM code.
 */
void CopyToFixedLengthBuffer(const std::string &input,
                             char *buffer,
                             unsigned int size);

/**
 * @brief A safe version of strncpy that null-terminates the output string.
 * @param[out] output The output array
 * @param[in] input The input string.
 */
template <size_t size>
inline void StrNCopy(char (&output)[size], const char* input) {
  strncpy(output, input, size);
  output[size - 1] = 0;
}

/**
 * @brief A safe version of strlen that handles input strings without NULL
 *   termination.
 * @param input The input string.
 * @param max_length The max length to search, unless a NULL is found
 *   before this.
 * @returns The string length, total, or to the first NULL; at most max_length
 */
inline size_t StrNLength(const char* input, size_t max_length) {
  char test[max_length + 1];
  strncpy(test, input, max_length);
  test[max_length] = 0;
  return strlen(test);
}

/**
 * @brief A safe version of strlen that handles input strings without NULL
 *   termination.
 * @param input The input string.
 * @param max_length The max length to search, unless a NULL is found
 *   before this.
 * @returns The string length, total, or to the first NULL; at most max_length
 */
inline size_t StrNLength(const std::string input, size_t max_length) {
  return StrNLength(input.c_str(), max_length);
}
}  // namespace strings
}  // namespace ola
#endif  // INCLUDE_OLA_STRINGS_UTILS_H_
