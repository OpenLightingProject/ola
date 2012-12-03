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
 * ScopeSet.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include <set>
#include <sstream>
#include <vector>
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ScopeSet.h"

namespace ola {
namespace slp {

using std::ostringstream;
using std::set;
using std::string;
using std::vector;

ScopeSet::ScopeSet(const string &scopes) {
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
    m_scopes.insert(scope);
  }
}


// Check for intersection between two ScopeSets.
bool ScopeSet::Intersects(const ScopeSet &other) const {
  set<string>::iterator iter1 = m_scopes.begin();
  set<string>::iterator iter2 = other.m_scopes.begin();
  while (iter1 != m_scopes.end() && iter2 != other.m_scopes.end()) {
    if (*iter1 == *iter2)
      return true;
    else if (*iter1 < *iter2)
      iter1++;
    else
      iter2++;
  }
  return false;
}

// Return the number of scopes that appear in both lists.
unsigned int ScopeSet::IntersectionCount(const ScopeSet &other) const {
  set<string>::iterator iter1 = m_scopes.begin();
  set<string>::iterator iter2 = other.m_scopes.begin();
  unsigned int i = 0;
  while (iter1 != m_scopes.end() && iter2 != other.m_scopes.end()) {
    if (*iter1 == *iter2) {
      i++;
      iter1++;
      iter2++;
    } else if (*iter1 < *iter2) {
      iter1++;
    } else {
      iter2++;
    }
  }
  return i;
}

// Get the intersection
ScopeSet ScopeSet::Intersection(const ScopeSet &other) const {
  set<string> intersection;
  set_intersection(m_scopes.begin(), m_scopes.end(), other.m_scopes.begin(),
                   other.m_scopes.end(),
                   inserter(intersection, intersection.end()));
  return ScopeSet(intersection);
}

// Get the difference
ScopeSet ScopeSet::Difference(const ScopeSet &other) const {
  set<string> difference;
  set_difference(m_scopes.begin(), m_scopes.end(), other.m_scopes.begin(),
                 other.m_scopes.end(),
                 inserter(difference, difference.end()));
  return ScopeSet(difference);
}

// Remove the difference between this set and other from this set.
// The removed elements are returned.
ScopeSet ScopeSet::DifferenceUpdate(const ScopeSet &other) {
  set<string> difference;
  set<string>::iterator iter1 = m_scopes.begin();
  set<string>::iterator iter2 = other.m_scopes.begin();
  while (iter1 != m_scopes.end() && iter2 != other.m_scopes.end()) {
    if (*iter1 == *iter2) {
      difference.insert(*iter1);
      m_scopes.erase(iter1++);
      iter2++;
    } else if (*iter1 < *iter2) {
      iter1++;
    } else {
      iter2++;
    }
  }
  return ScopeSet(difference);
}


string ScopeSet::AsEscapedString() const {
  string joined_scopes;
  std::ostringstream str;
  set<string>::const_iterator iter = m_scopes.begin();
  while (iter != m_scopes.end()) {
    string val = *iter;
    SLPStringEscape(&val);
    joined_scopes.append(val);
    iter++;
    if (iter != m_scopes.end())
      joined_scopes.append(",");
  }
  return joined_scopes;
}
}  // slp
}  // ola
