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
 * SLPString.h
 * Utility functions for dealing with strings in SLP
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <string>
#include <sstream>
#include "tools/slp/SLPStrings.h"

namespace ola {
namespace slp {

using std::ostringstream;
using std::string;

/**
 * Escape a String to use in SLP packets
 */
void SLPStringEscape(string *str) {
  static const char RESERVED_CHARACTERS[] = {
    '(', ')', ',', '\\', '!', '<', '=', '>', '~', ';', '*', '+'};

  ostringstream converted;
  string::size_type i = 0;
  while (i < str->size()) {
    bool match = false;
    char c = str->at(i);
    for (unsigned int j = 0; j < sizeof(RESERVED_CHARACTERS); ++j) {
      if (RESERVED_CHARACTERS[j] == c) {
        match = true;
        break;
      }
    }
    if (!match) {
      i++;
      continue;
    }
    converted.str("");
    converted << "\\" << std::hex << static_cast<int>(c);
    str->erase(i, 1);
    str->insert(i, converted.str().c_str());
    i += converted.str().size();
  }
}


/**
 * Unescape a string
 */
void SLPStringUnescape(string *str) {
  static const int ESCAPED_SIZE = 2;
  string::size_type i = 0;
  while (i < str->size()) {
    if (str->at(i) == '\\') {
      if (i + ESCAPED_SIZE >= str->size()) {
        OLA_WARN << "Insufficient characters remaining to un-escape in: " <<
          *str;
        str->erase(i);
        break;
      }
      uint8_t value;
      if (!HexStringToInt(str->substr(i+1, ESCAPED_SIZE), &value)) {
        OLA_WARN << "Invalid hex string while trying to escape in:" << *str;
        i++;
        continue;
      }
      if (value > 0x7f) {
        OLA_WARN << "Escaped value greater than 0x7f in:" << *str;
        i++;
        continue;
      }
      str->erase(i, ESCAPED_SIZE + 1);
      str->insert(i, 1, static_cast<char>(value));
    }
    i++;
  }
}


/**
 * Reduce whitespace to a single space character. Remove whitespace from the
 * start and end of the string.
 */
void FoldWhitespace(string *str) {
  static const char WHITESPACE[] = " \t\r\n";
  string::size_type start = 0;

  while (1) {
    start = str->find_first_of(WHITESPACE, start);
    if (start == string::npos)
      break;
    string::size_type end = str->find_first_not_of(WHITESPACE, start);

    if (end == string::npos) {
      str->erase(start, end);
      break;
    }
    if (start == 0) {
      str->erase(start, end);
      continue;
    }
    str->erase(start, end - start);
    str->insert(start, " ");
    start += 1;
  }
}


/**
 * Compare two strings by converting to lower case and folding whitespace.
 */
bool SLPStringCompare(const string &s1, const string s2) {
  string str1 = s1;
  string str2 = s2;
  ToLower(&str1);
  ToLower(&str2);
  FoldWhitespace(&str1);
  FoldWhitespace(&str2);
  return str1 == str2;
}
}  // slp
}  // ola
