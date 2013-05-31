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
 * StringUtils.cpp
 * Random String functions.
 * Copyright (C) 2005-2008 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "ola/StringUtils.h"

namespace ola {

using std::endl;
using std::string;
using std::stringstream;
using std::vector;

/*
 * Split a string on delimiters. If two delimiters appear next to each other an
 * empty string is added to the output list.
 * @param input the string to split
 * @param tokens the vector to store the result in
 * @param delimiters what to split on
 */
void StringSplit(const string &input,
                 vector<string> &tokens,
                 const string &delimiters) {
  string::size_type start_offset = 0;
  string::size_type end_offset = 0;

  while (1) {
    end_offset = input.find_first_of(delimiters, start_offset);
    if (end_offset == string::npos) {
      tokens.push_back(input.substr(start_offset, input.size() - start_offset));
      return;
    }
    tokens.push_back(input.substr(start_offset, end_offset - start_offset));
    start_offset = end_offset + 1 > input.size() ? string::npos :
                   end_offset + 1;
  }
}


/*
 * Trim leading and trailing whitespace from a string.
 * @param the string to trim
 */
void StringTrim(std::string *input) {
  string characters_to_trim = " \n\r\t";
  string::size_type start = input->find_first_not_of(characters_to_trim);
  string::size_type end = input->find_last_not_of(characters_to_trim);

  if (start == string::npos)
    input->clear();
  else
    *input = input->substr(start, end - start + 1);
}


/**
 * Take care of any NULLs embedded in the string
 * @param the string to shorten
 */
void ShortenString(string *input) {
  size_t index = input->find(static_cast<char>(0));
  if (index != string::npos)
    input->erase(index);
}


/**
 * Check if one string ends with another
 */
bool StringEndsWith(const string &s, const string &ending) {
  if (s.length() >= ending.length()) {
    return
      0 == s.compare(s.length() - ending.length(), ending.length(), ending);
  } else {
    return false;
  }
}


/*
 * Convert an int to a string.
 * @param i the int to convert
 * @return the string representation of the int
 */
string IntToString(int i) {
  stringstream str;
  str << i;
  return str.str();
}


/*
 * Convert an int to a string.
 * @param i the int to convert
 * @return the string representation of the int
 */
string IntToString(unsigned int i) {
  stringstream str;
  str << i;
  return str.str();
}


/*
 * Convert a string to a unsigned int.
 * @returns true if sucessfull, false otherwise
 */
bool StringToInt(const string &value, unsigned int *output, bool strict) {
  if (value.empty())
    return false;
  char *end_ptr;
  errno = 0;
  long long l = strtoll(value.data(), &end_ptr, 10);  // NOLINT(runtime/int)
  if (l < 0 || (l == 0 && errno != 0))
    return false;
  if (value == end_ptr)
    return false;
  if (strict && *end_ptr != 0)
    return false;
  if (l > static_cast<long long>(UINT32_MAX))  // NOLINT(runtime/int)
    return false;
  *output = static_cast<unsigned int>(l);
  return true;
}


/*
 * Convert a string to a uint16_t.
 * @returns true if sucessfull, false otherwise
 */
bool StringToInt(const string &value, uint16_t *output, bool strict) {
  unsigned int v;
  if (!StringToInt(value, &v, strict))
    return false;
  if (v > 0xffff)
    return false;
  *output = static_cast<uint16_t>(v);
  return true;
}


/*
 * Convert a string to a uint8_t.
 * @returns true if sucessfull, false otherwise
 */
bool StringToInt(const string &value, uint8_t *output, bool strict) {
  unsigned int v;
  if (!StringToInt(value, &v, strict))
    return false;
  if (v > 0xff)
    return false;
  *output = static_cast<uint8_t>(v);
  return true;
}


/*
 * Convert a string to a int.
 * @returns true if sucessfull, false otherwise
 */
bool StringToInt(const string &value, int *output, bool strict) {
  if (value.empty())
    return false;
  char *end_ptr;
  errno = 0;
  long long l = strtoll(value.data(), &end_ptr, 10);  // NOLINT(runtime/int)
  if (l == 0 && errno != 0)
    return false;
  if (value == end_ptr)
    return false;
  if (strict && *end_ptr != 0)
    return false;
  if (l < INT32_MIN || l > INT32_MAX)
    return false;
  *output = static_cast<unsigned int>(l);
  return true;
}


/*
 * Convert a string to a int16_t.
 * @returns true if sucessfull, false otherwise
 */
bool StringToInt(const string &value, int16_t *output, bool strict) {
  int v;
  if (!StringToInt(value, &v, strict))
    return false;
  if (v < INT16_MIN || v > INT16_MAX)
    return false;
  *output = static_cast<int16_t>(v);
  return true;
}


/*
 * Convert a string to a int16_t.
 * @returns true if sucessfull, false otherwise
 */
bool StringToInt(const string &value, int8_t *output, bool strict) {
  int v;
  if (!StringToInt(value, &v, strict))
    return false;
  if (v < INT8_MIN || v > INT8_MAX)
    return false;
  *output = static_cast<int8_t>(v);
  return true;
}


/*
 * Escape \
 */
void Escape(string *original) {
  for (string::iterator iter = original->begin(); iter != original->end();
      ++iter) {
    switch (*iter) {
      case '"':
        iter = original->insert(iter, '\\');
        iter++;
        break;
      case '\\':
        iter = original->insert(iter, '\\');
        iter++;
        break;
      case '/':
        iter = original->insert(iter, '\\');
        iter++;
        break;
      case '\b':
        *iter = 'b';
        iter = original->insert(iter, '\\');
        iter++;
        break;
      case '\f':
        *iter = 'f';
        iter = original->insert(iter, '\\');
        iter++;
        break;
      case '\n':
        *iter = 'n';
        iter = original->insert(iter, '\\');
        iter++;
        break;
      case '\r':
        *iter = 'r';
        iter = original->insert(iter, '\\');
        iter++;
        break;
      case '\t':
        *iter = 't';
        iter = original->insert(iter, '\\');
        iter++;
        break;
      default:
        break;
    }
  }
}


/**
 * Escape a string, returning a copy
 */
string EscapeString(const string &original) {
  string result = original;
  Escape(&result);
  return result;
}


/*
 * Convert a hex string to a uint8_t
 */
bool HexStringToInt(const string &value, uint8_t *output) {
  uint32_t temp;
  if (!HexStringToInt(value, &temp))
    return false;
  if (temp > 0xff)
    return false;
  *output = static_cast<uint8_t>(temp);
  return true;
}


/*
 * Convert a hex string to a uint16_t
 */
bool HexStringToInt(const string &value, uint16_t *output) {
  uint32_t temp;
  if (!HexStringToInt(value, &temp))
    return false;
  if (temp > UINT16_MAX)
    return false;
  *output = static_cast<uint16_t>(temp);
  return true;
}


/*
 * Convert a hex string to a uint32_t
 */
bool HexStringToInt(const string &value, uint32_t *output) {
  if (value.empty())
    return false;

  size_t found = value.find_first_not_of("ABCDEFabcdef0123456789");
  if (found != string::npos)
    return false;
  *output = strtoul(value.data(), NULL, 16);
  return true;
}


/*
 * Convert a hex string to a int8_t
 */
bool HexStringToInt(const string &value, int8_t *output) {
  int32_t temp;
  if (!HexStringToInt(value, &temp))
    return false;
  if (temp < 0 || temp > static_cast<int32_t>(UINT8_MAX))
    return false;
  *output = static_cast<int8_t>(temp);
  return true;
}


/*
 * Convert a hex string to a int16_t
 */
bool HexStringToInt(const string &value, int16_t *output) {
  int32_t temp;
  if (!HexStringToInt(value, &temp))
    return false;
  if (temp < 0 || temp > static_cast<int32_t>(UINT16_MAX))
    return false;
  *output = static_cast<int16_t>(temp);
  return true;
}


/*
 * Convert a hex string to a int32_t
 */
bool HexStringToInt(const string &value, int32_t *output) {
  if (value.empty())
    return false;

  size_t found = value.find_first_not_of("ABCDEFabcdef0123456789");
  if (found != string::npos)
    return false;
  *output = strtoll(value.data(), NULL, 16);
  return true;
}


/*
 * Return a lower case version of this string
 */
void ToLower(string *s) {
  std::transform(s->begin(), s->end(), s->begin(),
      std::ptr_fun<int, int>(std::tolower));
}


/*
 * Return an upper case version of this string
 */
void ToUpper(string *s) {
  std::transform(s->begin(), s->end(), s->begin(),
      std::ptr_fun<int, int>(std::toupper));
}


/**
 * Given a label in the form ([a-zA-Z0-9][-_])?[a-zA-Z0-9], return the string
 * with the transform s/-_/ / and words capitalized
 * @param s a string to transform.
 */
void CapitalizeLabel(string *s) {
  bool capitalize = true;
  for (string::iterator iter = s->begin(); iter != s->end(); ++iter) {
    switch (*iter) {
      case '-':
      case '_':
        *iter = ' ';
      case ' ':
        capitalize = true;
        break;
      default:
        if (capitalize && islower(*iter))
          *iter = toupper(*iter);
        capitalize = false;
    }
  }
}


/**
 * Given a label in the form ([a-zA-Z0-9][-_])?[a-zA-Z0-9], return the string
 * with the transform s/-_/ / and words capitalized.
 * This also capitalizes ancronyns like DMX
 * @param s a string to transform.
 */
void CustomCapitalizeLabel(string *s) {
  static const char* const transforms[] = {
    "dmx",
    "ip",
    NULL
  };
  const size_t size = s->size();
  const char* const *transform = transforms;
  while (*transform) {
    size_t last_match = 0;
    const string ancronym(*transform);
    const size_t ancronym_size = ancronym.size();

    while (true) {
      size_t match_position = s->find(ancronym, last_match);
      if (match_position == string::npos)
        break;
      last_match = match_position + 1;
      size_t end_position = match_position + ancronym_size;

      if ((match_position == 0 || ispunct(s->at(match_position - 1))) &&
          (end_position == size || ispunct(s->at(end_position)))) {
        while (match_position < end_position) {
          s->at(match_position) = toupper(s->at(match_position));
          match_position++;
        }
      }
    }
    transform++;
  }

  CapitalizeLabel(s);
}


/**
 * Write formatted output data to the ostream
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
                unsigned int indent,
                unsigned int byte_per_line) {
  stringstream raw, ascii;
  raw << std::setw(2) << std::hex;
  for (unsigned int i = 0; i != length; i++) {
    raw << std::setfill('0') << std::setw(2) <<
        static_cast<unsigned int>(data[i]) << " ";
    if (data[i] >= ' ' && data[i] <= '~')
      ascii << data[i];
    else
      ascii << ".";

    if (i % byte_per_line == byte_per_line - 1) {
      *out << string(indent, ' ') << raw.str() << " " << ascii.str() << endl;
      raw.str("");
      ascii.str("");
    }
  }
  if (length % byte_per_line != 0) {
    // pad if needed
    raw << string(3 * (byte_per_line - (length % byte_per_line)), ' ');
    *out << string(indent, ' ') << raw.str() << " " << ascii.str() << endl;
  }
}
}  // namespace ola
