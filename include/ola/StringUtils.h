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
 * StringUtils.h
 * Random String functions.
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @file StringUtils.h
 * @brief Various string utility functions.
 */

#ifndef INCLUDE_OLA_STRINGUTILS_H_
#define INCLUDE_OLA_STRINGUTILS_H_

#include <ola/strings/Format.h>
#include <stdint.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace ola {

/**
 * @brief Split a string into pieces.
 *
 * If two delimiters appear next to each other an empty string is added to the
 *   output vector.
 * @param[in] input the string to split
 * @param[out] tokens pointer to a vector with the parts of the string
 * @param delimiters the delimiter to use for splitting. Defaults to ' '
 */
void StringSplit(const std::string &input,
                 std::vector<std::string> *tokens,
                 const std::string &delimiters = " ");

/**
 * @brief Split a string into pieces.
 *
 * If two delimiters appear next to each other an empty string is added to the
 *   output vector.
 * @param[in] input the string to split
 * @param[out] tokens the parts of the string
 * @param delimiters the delimiter to use for splitting. Defaults to ' '
 * @deprecated Use the version with a vector pointer instead (3 Jan 2015).
 */
inline void StringSplit(
    const std::string &input,
    std::vector<std::string> &tokens,  // NOLINT(runtime/references)
    const std::string &delimiters = " ") {
  StringSplit(input, &tokens, delimiters);
}

/**
 * @brief Trim leading and trailing whitespace from a string
 * @param input the string to trim.
 */
void StringTrim(std::string *input);

/**
 * @brief Truncate the string based on the presence of \0 characters.
 * @param input the string to shorten
 */
void ShortenString(std::string *input);

/**
 * @brief Check if one string is a prefix of another.
 * @param s the string to check
 * @param prefix the prefix to check for
 * @returns true if s begins with prefix, false otherwise.
 */
bool StringBeginsWith(const std::string &s, const std::string &prefix);

/**
 * @brief Check if one string is a suffix of another.
 * @param s the string to check
 * @param suffix the suffix to check for
 * @returns true if s ends with suffix, false otherwise.
 */
bool StringEndsWith(const std::string &s, const std::string &suffix);

/**
 * @brief Strips a prefix from a string.
 * @param s the string to strip
 * @param prefix the prefix to remove
 * @returns true if we stripped prefix from s, false otherwise.
 */
bool StripPrefix(std::string *s, const std::string &prefix);

/**
 * @brief Strips a suffix from a string.
 * @param s the string to strip
 * @param suffix the suffix to remove
 * @returns true if we stripped suffix from s, false otherwise.
 */
bool StripSuffix(std::string *s, const std::string &suffix);

/**
 * @brief Convert an int to a string.
 * @param i the int to convert
 * @return the string representation of the int
 * @deprecated Use ola::strings::IntToString instead (30 Dec 2014).
 */
inline std::string IntToString(int i) {
  return ola::strings::IntToString(i);
}

/**
 * Convert an unsigned int to a string.
 * @param i the unsigned int to convert
 * @return The string representation of the unsigned int
 * @deprecated Use ola::strings::IntToString instead (30 Dec 2014).
 */
inline std::string IntToString(unsigned int i) {
  return ola::strings::IntToString(i);
}

/**
 * Convert an unsigned int to a hex string.
 * @param i the unsigned int to convert
 * @param width the width to zero pad to
 * @return The hex string representation of the unsigned int
 * @note We don't currently support signed ints due to a lack of requirement
 * for it and issues with negative handling and hex in C++
 * @deprecated ola::ToHex() instead, unless you really want a string rather
 *   than use in an ostream (30 Dec 2014)
 */
std::string IntToHexString(unsigned int i, unsigned int width);

/**
 * Convert a uint8_t to a hex string.
 * @param i the number to convert
 * @return The string representation of the number
 * @deprecated ola::ToHex() instead, unless you really want a string rather
 *   than use in an ostream (30 Dec 2014)
 */
inline std::string IntToHexString(uint8_t i) {
  std::ostringstream str;
  str << ola::strings::ToHex(i);
  return str.str();
}

/**
 * Convert a uint16_t to a hex string.
 * @param i the number to convert
 * @return The string representation of the number
 * @deprecated ola::ToHex() instead, unless you really want a string rather
 *   than use in an ostream (30 Dec 2014)
 */
inline std::string IntToHexString(uint16_t i) {
  std::ostringstream str;
  str << ola::strings::ToHex(i);
  return str.str();
}

/**
 * Convert a uint32_t to a hex string.
 * @param i the number to convert
 * @return The string representation of the number
 * @deprecated ola::ToHex() instead, unless you really want a string rather
 *   than use in an ostream (30 Dec 2014)
 */
inline std::string IntToHexString(uint32_t i) {
  std::ostringstream str;
  str << ola::strings::ToHex(i);
  return str.str();
}

/**
 * @brief Escape a string with \\ .
 *
 * The string is modified in place according to the grammar from json.org
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
void Escape(std::string *original);

/**
 * @brief Escape a string, returning a copy.
 * @param original the string to escape.
 * @returns The escaped string.
 * @sa Escape()
 */
std::string EscapeString(const std::string &original);

/**
 * @brief Replace all instances of the find string with the replace string.
 * @param original the string to operate on.
 * @param find the string to find
 * @param replace what to replace it with
 */
void ReplaceAll(std::string *original,
                const std::string &find,
                const std::string &replace);

/**
 * @brief Encode any unprintable characters in a string as hex, returning a
 * copy.
 *
 * "foo\\nbar" becomes "foo\\\\x0abar"
 * "foo\\x01test" becomes "foo\\\\x01test"
 * @param original the string to encode.
 * @returns The encoded string.
 */
std::string EncodeString(const std::string &original);

/**
 * @brief Convert a string to a bool.
 *
 * The string can be 'true' or 'false', 't' or 'f', '1' or '0' or case
 * insensitive variations of any of the above.
 *
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @returns true if the value was converted, false if the string was not a
 * bool.
 */
bool StringToBool(const std::string &value, bool *output);

/**
 * @brief Convert a string to a bool in a tolerant way.
 *
 * The string can be 'true' or 'false', 't' or 'f', '1' or '0', 'on' or 'off',
 * 'enable' or 'disable', 'enabled' or 'disabled' or case insensitive
 * variations of any of the above.
 *
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @returns true if the value was converted, false if the string was not a
 * bool.
 * @sa StringToBool
 */
bool StringToBoolTolerant(const std::string &value, bool *output);

/**
 * @brief Convert a string to a unsigned int.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 */
bool StringToInt(const std::string &value,
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
bool StringToInt(const std::string &value,
                 uint16_t *output,
                 bool strict = false);

/**
 * @brief Convert a string to a uint8_t.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const std::string &value,
                 uint8_t *output,
                 bool strict = false);

/**
 * @brief Convert a string to a int.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const std::string &value, int *output, bool strict = false);

/**
 * @brief Convert a string to a int16_t.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const std::string &value,
                 int16_t *output,
                 bool strict = false);

/**
 * @brief Convert a string to a int8_t.
 * @param[in] value the string to convert
 * @param[out] output a pointer where the value will be stored.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 * @sa StringToInt.
 */
bool StringToInt(const std::string &value, int8_t *output, bool strict = false);

/**
 * @brief Convert a string to an int type or return a default if it failed.
 * @tparam int_type the type to convert to
 * @param value the string to convert
 * @param alternative the default value to return if conversion failed.
 * @param[in] strict this controls if trailing characters produce an error.
 * @returns the value if it converted successfully or the default if the string
 * was not an int or the value was too large / small for the type.
 */
template <typename int_type>
int_type StringToIntOrDefault(const std::string &value,
                              int_type alternative,
                              bool strict = false) {
  int_type output;
  return (StringToInt(value, &output, strict)) ? output : alternative;
}

/**
 * @brief Convert a hex string to a uint8_t.
 *
 * The string can contain upper or lower case hex characters.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 */
bool HexStringToInt(const std::string &value, uint8_t *output);

/**
 * @brief Convert a hex string to a uint16_t.
 *
 * The string can contain upper or lower case hex characters.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 */
bool HexStringToInt(const std::string &value, uint16_t *output);

/**
 * @brief Convert a hex string to a uint32_t.
 *
 * The string can contain upper or lower case hex characters.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 */
bool HexStringToInt(const std::string &value, uint32_t *output);

/**
 * @brief Convert a hex string to a int8_t.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 *
 * The string can contain upper or lower case hex characters.
 */
bool HexStringToInt(const std::string &value, int8_t *output);

/**
 * @brief Convert a hex string to a int16_t.
 *
 * The string can contain upper or lower case hex characters.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 */
bool HexStringToInt(const std::string &value, int16_t *output);

/**
 * @brief Convert a hex string to a int32_t.
 *
 * The string can contain upper or lower case hex characters.
 * @param[in] value the string to convert.
 * @param[out] output a pointer to the store the converted value in.
 * @returns true if the value was converted, false if the string was not an int
 * or the value was too large / small for the type.
 */
bool HexStringToInt(const std::string &value, int32_t *output);

/**
 * @brief Convert a string to lower case.
 * @param s the string to convert to lower case.
 */
void ToLower(std::string *s);

/**
 * @brief Convert a string to upper case.
 * @param s the string to convert to upper case.
 */
void ToUpper(std::string *s);

/**
 * @brief Transform a string to a pretty-printed form.
 *
 * Given a label in the form ([a-zA-Z0-9][-_])?[a-zA-Z0-9], this replaces [-_]
 * with [ ] and capitalizes each word. e.g.
 * "my_label" becomes "My Label"
 * @param s a string to transform.
 */
void CapitalizeLabel(std::string *s);

/**
 * @brief Similar to CapitalizeLabel() but this also capitalized known
 * acronyms.
 *
 * @param s a string to transform.
 * The following are capitalized:
 *   - dhcp
 *   - dmx
 *   - dns
 *   - ip
 *   - ipv4
 *   - ipv6
 *   - led
 *   - mdmx
 *   - rdm
 *   - uid
 */
void CustomCapitalizeLabel(std::string *s);

/**
 * @brief Transform a string by capitalizing the first character.
 * @param s a string to transform.
 */
void CapitalizeFirst(std::string *s);

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
 * @deprecated Use ola::strings::FormatData instead (30 Dec 2014).
 */
inline void FormatData(std::ostream *out,
                       const uint8_t *data,
                       unsigned int length,
                       unsigned int indent = 0,
                       unsigned int byte_per_line = 8) {
  return ola::strings::FormatData(out, data, length, indent, byte_per_line);
}

/**
 * @brief Convert a hex string, prefixed with 0x or 0X to an int type.
 */
template <typename int_type>
bool PrefixedHexStringToInt(const std::string &input, int_type *output) {
  if ((input.find("0x") != 0) && (input.find("0X") != 0))
    return false;
  std::string modified_input = input.substr(2);
  return HexStringToInt(modified_input, output);
}

/**
 * @brief Join a vector of a type.
 * @param delim the delimiter to use between items in the vector
 * @param input T can be any type for which the << operator is defined
 */
template<typename T>
std::string StringJoin(const std::string &delim, const T &input) {
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
