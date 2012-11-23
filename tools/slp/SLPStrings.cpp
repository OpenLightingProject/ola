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
#include <algorithm>
#include <set>
#include <string>
#include <sstream>
#include <vector>
#include "tools/slp/ServerCommon.h"
#include "tools/slp/SLPStrings.h"

namespace ola {
namespace slp {

using std::ostringstream;
using std::string;

/**
 * Escape a String to use in SLP packets
 */
void SLPStringEscape(string *str) {
  ostringstream converted;
  string::size_type i = 0;
  while (true) {
    i = str->find_first_of("(),\\!<=>~;*+", i);
    if (i == string::npos)
      break;

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
 * Compare two strings by converting to lower case and folding whitespace.
 */
bool SLPStringCanonicalizeAndCompare(const string &s1, const string s2) {
  string str1 = s1;
  string str2 = s2;
  SLPCanonicalizeString(&str1);
  SLPCanonicalizeString(&str2);
  return str1 == str2;
}


/**
 * Return true if any of the elements in set one exist in set two. This assumes
 * the strings are in canonical form.
 */
bool SLPSetIntersect(const set<string> &one, const set<string> &two) {
  set<string>::iterator iter1 = one.begin();
  set<string>::iterator iter2 = two.begin();
  while (iter1 != one.end() && iter2 != two.end()) {
    if (*iter1 == *iter2)
      return true;
    else if (*iter1 < *iter2)
      iter1++;
    else
      iter2++;
  }
  return false;
}


/**
 * Return true if any of the non-canonicalized scopes in the vector match any
 * of those in the canonicalized set.
 */
bool SLPScopesMatch(const vector<string> &scopes_v,
                    const set<string> &scopes_s) {
  set<string> canonicalized_scopes;
  SLPReduceList(scopes_v, &canonicalized_scopes);
  return SLPSetIntersect(canonicalized_scopes, scopes_s);
}


/**
 * Remove the service:// from the start of a string
 */
void SLPStripService(string *str) {
  static const size_t PREFIX_LENGTH = strlen(SLP_SERVICE_PREFIX);
  if (0 == str->compare(0, PREFIX_LENGTH, SLP_SERVICE_PREFIX))
    *str = str->substr(PREFIX_LENGTH);
}


/**
 * Extract the service name from a URL.
 * Note we should really use a full BNF parser here.
 */
string SLPServiceFromURL(const string &url) {
  string service = url;
  SLPStripService(&service);

  size_t pos = service.find("://");
  if (pos != string::npos)
    service = service.substr(0, pos);
  SLPCanonicalizeString(&service);
  return service;
}


/**
 * Give a comma-separated list of scopes, return the set of canonical scopes
 * this represents.
 */
void SLPExtractScopes(const string &scopes, set<string> *output) {
  string::size_type start_offset = 0;
  string::size_type end_offset = 0;

  while (end_offset != string::npos) {
    end_offset = scopes.find_first_of(",", start_offset);
    string scope;
    if (end_offset == string::npos)
      scope = scopes.substr(start_offset, scopes.size() - start_offset);
    else
      scope = scopes.substr(start_offset, end_offset - start_offset);

    start_offset = end_offset + 1 > scopes.size() ? string::npos :
                   end_offset + 1;

    if (scope.empty())
      continue;

    SLPStringUnescape(&scope);
    SLPCanonicalizeString(&scope);
    output->insert(scope);
  }
}
}  // slp
}  // ola
