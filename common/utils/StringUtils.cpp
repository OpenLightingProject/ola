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
 * StringUtils.cpp
 * Random String functions.
 * Copyright (C) 2005 Simon Newton
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
#include "ola/base/Macro.h"

namespace ola {

using std::endl;
using std::ostringstream;
using std::string;
using std::vector;

void StringSplit(const string &input,
                 vector<string> *tokens,
                 const string &delimiters) {
  string::size_type start_offset = 0;
  string::size_type end_offset = 0;

  while (1) {
    end_offset = input.find_first_of(delimiters, start_offset);
    if (end_offset == string::npos) {
      tokens->push_back(
          input.substr(start_offset, input.size() - start_offset));
      return;
    }
    tokens->push_back(input.substr(start_offset, end_offset - start_offset));
    start_offset = (end_offset + 1 > input.size()) ?
                   string::npos : (end_offset + 1);
  }
}

void StringTrim(string *input) {
  string characters_to_trim = " \n\r\t";
  string::size_type start = input->find_first_not_of(characters_to_trim);
  string::size_type end = input->find_last_not_of(characters_to_trim);

  if (start == string::npos) {
    input->clear();
  } else {
    *input = input->substr(start, end - start + 1);
  }
}

void ShortenString(string *input) {
  size_t index = input->find(static_cast<char>(0));
  if (index != string::npos) {
    input->erase(index);
  }
}

bool StringBeginsWith(const string &s, const string &prefix) {
  if (s.length() >= prefix.length()) {
    return (0 == s.compare(0, prefix.length(), prefix));
  } else {
    return false;
  }
}

bool StringEndsWith(const string &s, const string &suffix) {
  if (s.length() >= suffix.length()) {
    return
      0 == s.compare(s.length() - suffix.length(), suffix.length(), suffix);
  } else {
    return false;
  }
}

bool StripPrefix(string *s, const string &prefix) {
  if (StringBeginsWith(*s, prefix)) {
    *s = s->substr(prefix.length());
    return true;
  } else {
    return false;
  }
}

bool StripSuffix(string *s, const string &suffix) {
  if (StringEndsWith(*s, suffix)) {
    *s = s->substr(0, s->length() - suffix.length());
    return true;
  } else {
    return false;
  }
}

string IntToHexString(unsigned int i, unsigned int width) {
  strings::_ToHex<unsigned int> v = strings::_ToHex<unsigned int>(
      i, static_cast<int>(width), true);
  ostringstream str;
  str << v;
  return str.str();
}

bool StringToBool(const string &value, bool *output) {
  string lc_value(value);
  ToLower(&lc_value);
  if ((lc_value == "true") || (lc_value == "t") || (lc_value == "1")) {
    *output = true;
    return true;
  } else if ((lc_value == "false") || (lc_value == "f") || (lc_value == "0")) {
    *output = false;
    return true;
  }
  return false;
}

bool StringToBoolTolerant(const string &value, bool *output) {
  if (StringToBool(value, output)) {
    return true;
  } else {
    string lc_value(value);
    ToLower(&lc_value);
    if ((lc_value == "on") || (lc_value == "enable") ||
        (lc_value == "enabled")) {
      *output = true;
      return true;
    } else if ((lc_value == "off") || (lc_value == "disable") ||
               (lc_value == "disabled")) {
      *output = false;
      return true;
    }
  }
  return false;
}

bool StringToInt(const string &value, unsigned int *output, bool strict) {
  if (value.empty()) {
    return false;
  }
  char *end_ptr;
  errno = 0;
  long long l = strtoll(value.data(), &end_ptr, 10);  // NOLINT(runtime/int)
  if (l < 0 || (l == 0 && errno != 0)) {
    return false;
  }
  if (value == end_ptr) {
    return false;
  }
  if (strict && *end_ptr != 0) {
    return false;
  }
  if (l > static_cast<long long>(UINT32_MAX)) {  // NOLINT(runtime/int)
    return false;
  }
  *output = static_cast<unsigned int>(l);
  return true;
}

bool StringToInt(const string &value, uint16_t *output, bool strict) {
  unsigned int v;
  if (!StringToInt(value, &v, strict)) {
    return false;
  }
  if (v > UINT16_MAX) {
    return false;
  }
  *output = static_cast<uint16_t>(v);
  return true;
}

bool StringToInt(const string &value, uint8_t *output, bool strict) {
  unsigned int v;
  if (!StringToInt(value, &v, strict)) {
    return false;
  }
  if (v > UINT8_MAX) {
    return false;
  }
  *output = static_cast<uint8_t>(v);
  return true;
}

bool StringToInt(const string &value, int *output, bool strict) {
  if (value.empty()) {
    return false;
  }
  char *end_ptr;
  errno = 0;
  long long l = strtoll(value.data(), &end_ptr, 10);  // NOLINT(runtime/int)
  if (l == 0 && errno != 0) {
    return false;
  }
  if (value == end_ptr) {
    return false;
  }
  if (strict && *end_ptr != 0) {
    return false;
  }
  if (l < INT32_MIN || l > INT32_MAX) {
    return false;
  }
  *output = static_cast<unsigned int>(l);
  return true;
}

bool StringToInt(const string &value, int16_t *output, bool strict) {
  int v;
  if (!StringToInt(value, &v, strict)) {
    return false;
  }
  if (v < INT16_MIN || v > INT16_MAX) {
    return false;
  }
  *output = static_cast<int16_t>(v);
  return true;
}

bool StringToInt(const string &value, int8_t *output, bool strict) {
  int v;
  if (!StringToInt(value, &v, strict)) {
    return false;
  }
  if (v < INT8_MIN || v > INT8_MAX) {
    return false;
  }
  *output = static_cast<int8_t>(v);
  return true;
}

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

string EscapeString(const string &original) {
  string result = original;
  Escape(&result);
  return result;
}

string EncodeString(const string &original) {
  ostringstream encoded;
  for (string::const_iterator iter = original.begin();
       iter != original.end();
       ++iter) {
    if (isprint(*iter)) {
      encoded << *iter;
    } else {
      encoded << "\\x"
              << ola::strings::ToHex(static_cast<uint8_t>(*iter), false);
    }
  }
  return encoded.str();
}

void ReplaceAll(string *original, const string &find, const string &replace) {
  if (original->empty() || find.empty()) {
    return;  // No text or nothing to find, so nothing to do
  }

  size_t start = 0;
  while ((start = original->find(find, start)) != string::npos) {
    original->replace(start, find.length(), replace);
    // Move to the end of the replaced section
    start += ((replace.length() > find.length()) ? replace.length() : 0);
  }
}

bool HexStringToInt(const string &value, uint8_t *output) {
  uint32_t temp;
  if (!HexStringToInt(value, &temp)) {
    return false;
  }
  if (temp > UINT8_MAX) {
    return false;
  }
  *output = static_cast<uint8_t>(temp);
  return true;
}

bool HexStringToInt(const string &value, uint16_t *output) {
  uint32_t temp;
  if (!HexStringToInt(value, &temp)) {
    return false;
  }
  if (temp > UINT16_MAX) {
    return false;
  }
  *output = static_cast<uint16_t>(temp);
  return true;
}

bool HexStringToInt(const string &value, uint32_t *output) {
  if (value.empty()) {
    return false;
  }

  size_t found = value.find_first_not_of("ABCDEFabcdef0123456789");
  if (found != string::npos) {
    return false;
  }
  *output = strtoul(value.data(), NULL, 16);
  return true;
}

bool HexStringToInt(const string &value, int8_t *output) {
  int32_t temp;
  if (!HexStringToInt(value, &temp)) {
    return false;
  }
  if (temp < 0 || temp > static_cast<int32_t>(UINT8_MAX)) {
    return false;
  }
  *output = static_cast<int8_t>(temp);
  return true;
}

bool HexStringToInt(const string &value, int16_t *output) {
  int32_t temp;
  if (!HexStringToInt(value, &temp)) {
    return false;
  }
  if (temp < 0 || temp > static_cast<int32_t>(UINT16_MAX)) {
    return false;
  }
  *output = static_cast<int16_t>(temp);
  return true;
}

bool HexStringToInt(const string &value, int32_t *output) {
  if (value.empty()) {
    return false;
  }

  size_t found = value.find_first_not_of("ABCDEFabcdef0123456789");
  if (found != string::npos) {
    return false;
  }
  *output = strtoll(value.data(), NULL, 16);
  return true;
}

void ToLower(string *s) {
  std::transform(s->begin(), s->end(), s->begin(),
                 std::ptr_fun<int, int>(std::tolower));
}

void ToUpper(string *s) {
  std::transform(s->begin(), s->end(), s->begin(),
                 std::ptr_fun<int, int>(std::toupper));
}

void CapitalizeLabel(string *s) {
  bool capitalize = true;
  for (string::iterator iter = s->begin(); iter != s->end(); ++iter) {
    switch (*iter) {
      case '-':
        // fall through, also convert to space then capitalize next character
        OLA_FALLTHROUGH
      case '_':
        *iter = ' ';
        // fall through, also convert to space then capitalize next character
        OLA_FALLTHROUGH
      case ' ':
        capitalize = true;
        break;
      default:
        if (capitalize && islower(*iter)) {
          *iter = toupper(*iter);
        }
        capitalize = false;
    }
  }
}

void CustomCapitalizeLabel(string *s) {
  static const char* const transforms[] = {
    "dhcp",
    "dmx",
    "dns",
    "ip",
    "ipv4",  // Should really be IPv4 probably, but better than nothing
    "ipv6",  // Should really be IPv6 probably, but better than nothing
    "led",
    "pdl",
    "pid",
    "rdm",
    "uid",
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
      if (match_position == string::npos) {
        break;
      }
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

void CapitalizeFirst(string *s) {
  string::iterator iter = s->begin();
  if (islower(*iter)) {
    *iter = toupper(*iter);
  }
}
}  // namespace ola
