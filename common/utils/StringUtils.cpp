/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "ola/StringUtils.h"

namespace ola {

using std::string;
using std::stringstream;
using std::vector;

/*
 * Split a string on delimtiers. If two delimiters appear next to each other an
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
bool StringToInt(const string &value, unsigned int *output) {
  if (value.empty())
    return false;
  char *end_ptr;
  errno = 0;
  long l = strtol(value.data(), &end_ptr, 10);
  if (l < 0 || (l == 0 && errno != 0))
    return false;
  if (value == end_ptr)
    return false;
  if (l > static_cast<long>(UINT32_MAX))
    return false;
  *output = static_cast<unsigned int>(l);
  return true;
}


/*
 * Convert a string to a uint16_t.
 * @returns true if sucessfull, false otherwise
 */
bool StringToInt(const string &value, uint16_t *output) {
  unsigned int v;
  if (!StringToInt(value, &v))
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
bool StringToInt(const string &value, uint8_t *output) {
  unsigned int v;
  if (!StringToInt(value, &v))
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
bool StringToInt(const string &value, int *output) {
  if (value.empty())
    return false;
  char *end_ptr;
  errno = 0;
  long l = strtol(value.data(), &end_ptr, 10);
  if (l == 0 && errno != 0)
    return false;
  if (value == end_ptr)
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
bool StringToInt(const string &value, int16_t *output) {
  int v;
  if (!StringToInt(value, &v))
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
bool StringToInt(const string &value, int8_t *output) {
  int v;
  if (!StringToInt(value, &v))
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
 * Convert a hex string to a unsigned int
 */
bool HexStringToUInt(const string &value, unsigned int *output) {
  if (value.empty())
    return false;

  size_t found = value.find_first_not_of("ABCDEFabcdef0123456789");
  if (found != string::npos)
    return false;
  *output = strtoul(value.data(), NULL, 16);
  return true;
}


/*
 * Convert a hex string to a uint16_t
 */
bool HexStringToUInt16(const string &value, uint16_t *output) {
  unsigned int v;
  if (!HexStringToUInt(value, &v))
    return false;
  if (v > 0xffff)
    return false;
  *output = static_cast<uint16_t>(v);
  return true;
}


/*
 * Return a lower case version of this string
 */
void ToLower(string *s) {
  std::transform(s->begin(), s->end(), s->begin(),
      std::ptr_fun<int, int>(std::tolower));
}
}  // ola
