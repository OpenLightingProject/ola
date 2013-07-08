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
 * StringUtils..h
 * Random String functions.
 * Copyright (C) 2005-2008 Simon Newton
 */

/**
 * @file StringUtils.h
 * @brief Various string utility functions.
 */

#ifndef INCLUDE_OLA_STRINGUTILS_H_
#define INCLUDE_OLA_STRINGUTILS_H_

#include <stdint.h>
#include <sstream>
#include <string>
#include <vector>

namespace ola {

using std::string;
using std::vector;

/**
 * @brief Split a string into pieces.
 *
 * If two delimiters appear next to each other an empty string is added to the
 *   output vector.
 * @param[in] input the string to split
 * @param[out] tokens the parts of the string
 * @param delimiters the delimiiter to use for splitting. Defaults to ' '
 */
void StringSplit(const string &input,
                 vector<string> &tokens,
                 const string &delimiters=" ");

/**
 * @brief Trim leading and trailing whitespace from a string
 * @param input the string to trim.
 */
void StringTrim(string *input);

/**
 * @brief Truncate the string based on the presence of \0 characters.
 * @param input the string to shorten
 */
void ShortenString(string *input);

/**
 * @brief Check if one string is a suffix of another.
 * @param s the string to check
 * @param suffix the suffix to check for
 * @returns true if s ends with suffix, false otherwise.
 */
bool StringEndsWith(const string &s, const string &suffix);

/**
 * @brief Convert an int to a string.
 * @param i the int to convert
 * @return the string representation of the int
 */
string IntToString(int i);

/**
 * Convert an unsigned int to a string.
 * @param i the unsigned int to convert
 * @return The string representation of the unsigned int
 */
string IntToString(unsigned int i);

/**
 * @brief Escape a string with \\.
 *
 * The string is modified in place.
 * The following characters are escaped:
 *  - \\
 *  - "
 *  - /
 *  - \\b
 *  - \\f
 *  - \\n
 *  - \\r
 *  - \\t
 * @param original the string to escape
 */
void Escape(string *original);

/**
 * @brief Escape a string, returning a copy.
 * @param original the string to escape.
 * @returns The escaped string.
 * @sa Escape()
 */
string EscapeString(const string &original);

/**
 * @brief Convert a string to a unsigned int.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 */
bool StringToInt(const string &value,
                 unsigned int *output,
                 bool strict = false);

/**
 * @brief Convert a string to a uint16_t.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const string &value, uint16_t *output, bool strict = false);

/**
 * @brief Convert a string to a uint8_t.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const string &value, uint8_t *output, bool strict = false);

/**
 * @brief Convert a string to a int.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const string &value, int *output, bool strict = false);

/**
 * @brief Convert a string to a int16_t.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const string &value, int16_t *output, bool strict = false);

/**
 * @brief Convert a string to a int8_t.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const string &value, int8_t *output, bool strict = false);

/**
 * @brief Convert a hex string to a uint8_t.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 *
 * The string can contain upper or lower case hex characters.
 */
bool HexStringToInt(const string &value, uint8_t *output);

/**
 * @brief Convert a hex string to a uint16_t.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 *
 * The string can contain upper or lower case hex characters.
 */
bool HexStringToInt(const string &value, uint16_t *output);

/**
 * @brief Convert a hex string to a uint32_t.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 *
 * The string can contain upper or lower case hex characters.
 */
bool HexStringToInt(const string &value, uint32_t *output);

/**
 * @brief Convert a hex string to a int8_t.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 *
 * The string can contain upper or lower case hex characters.
 */
bool HexStringToInt(const string &value, int8_t *output);

/**
 * @brief Convert a hex string to a int16_t.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 *
 * The string can contain upper or lower case hex characters.
 */
bool HexStringToInt(const string &value, int16_t *output);

/**
 * @brief Convert a hex string to a int32_t.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 *
 * The string can contain upper or lower case hex characters.
 */
bool HexStringToInt(const string &value, int32_t *output);

/**
 * @brief Convert a string to lower case.
 * @param s the string to convert to lower case.
 */
void ToLower(string *s);

/**
 * @brief Convert a string to upper case.
 * @param s the string to convert to upper case.
 */
void ToUpper(string *s);

/**
 * @brief Transform a string to a pretty-printed form.
 *
 * Given a label in the form ([a-zA-Z0-9][-_])?[a-zA-Z0-9], this replaces [-_]
 * with [ ] and capitalizes each word. e.g.
 * "my_label" becomes "My Label"
 * @param s a string to transform.
 */
void CapitalizeLabel(string *s);

/**
 * @brief Similar to CapitalizeLabel() but this also capitalized known
 * acronyms.
 *
 * @param s a string to transform.
 * The following are capitalized:
 *   - ip
 *   - dmx
 */
void CustomCapitalizeLabel(string *s);

/**
 * @brief Write binary data to an ostream in a human readable form.
 *
 * @param out the ostream to write to
 * @param data pointer to the data
 * @param length length of the data
 * @param indent the number of spaces to prefix each line with
 * @param byte_per_line the number of bytes to display per line
 *
 * The data is printed in two columns, hex on the left, ascii on the right.
 * Non ascii values are printed as .
 */
void FormatData(std::ostream *out,
                const uint8_t *data,
                unsigned int length,
                unsigned int indent = 0,
                unsigned int byte_per_line = 8);

/**
 * Convert a hex string, prefixed with 0x to an int type.
 */
template <typename int_type>
bool PrefixedHexStringToInt(const string &input, int_type *output) {
  if (input.find("0x") != 0)
    return false;
  string modified_input = input.substr(2);
  return HexStringToInt(modified_input, output);
}

/**
 * @brief Join a vector of a type.
 * @param T can be any type for which the << operator is defined
 */
template<typename T>
string StringJoin(const string &delim, const T &input) {
  std::ostringstream str;
  typename T::const_iterator iter = input.begin();
  while (iter != input.end()) {
    str << *iter++;
    if (iter != input.end())
      str << delim;
  }
  return str.str();
}
}  // namespace ola
#endif  // INCLUDE_OLA_STRINGUTILS_H_
