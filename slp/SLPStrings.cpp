/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * SLPStrings.h
 * Utility functions for dealing with strings in SLP
 * Copyright (C) 2012 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <string>
#include <sstream>
#include "slp/SLPStrings.h"

namespace ola {
namespace slp {

using std::string;

/**
 * Escape a String to use in SLP packets
 */
void SLPStringEscape(string *str) {
  std::ostringstream converted;
  string::size_type i = 0;
  while (true) {
    i = str->find_first_of("(),\\!<=>~;*+", i);
    if (i == string::npos)
      return;

    converted.str("");
    converted << "\\" << std::hex << static_cast<int>(str->at(i));
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
  while (true) {
    i = str->find_first_of('\\', i);
    if (i == string::npos)
      break;

    if (i + ESCAPED_SIZE >= str->size()) {
      OLA_WARN << "Insufficient characters remaining to un-escape in: " <<
        *str;
      str->erase(i);
      break;
    }
    uint8_t value;
    if (!HexStringToInt(str->substr(i+1, ESCAPED_SIZE), &value)) {
      OLA_WARN << "Invalid hex string while trying to escape in:" << *str;
    } else if (value > 0x7f) {
      OLA_WARN << "Escaped value greater than 0x7f in:" << *str;
    } else {
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
 * Convert str to its canonical form.
 */
void SLPCanonicalizeString(string *str) {
  ToLower(str);
  FoldWhitespace(str);
}


/**
 * Same as above but returns a new string
 */
string SLPGetCanonicalString(const string &str) {
  string canonical_str = str;
  SLPCanonicalizeString(&canonical_str);
  return canonical_str;
}


/**
 * Extract the service name from a URL.
 * Note we should really use a full BNF parser here.
 */
string SLPServiceFromURL(const string &url) {
  string service = url;
  size_t pos = service.find("://");
  if (pos != string::npos)
    service = service.substr(0, pos);
  SLPCanonicalizeString(&service);
  return service;
}


/**
 * Strip the service type from the url. This returns everything after the ://
 * Note we should really use a full BNF parser here.
 */
string SLPStripServiceFromURL(const string &url) {
  string remainder;
  size_t pos = url.find("://");
  if (pos != string::npos)
    remainder = url.substr(pos + 3);
  return remainder;
}
}  // namespace slp
}  // namespace ola
