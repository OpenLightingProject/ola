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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ScopeSet.h
 * Copyright (C) 2012 Simon Newton
 */

#ifndef SLP_SCOPESET_H_
#define SLP_SCOPESET_H_

#include <ola/StringUtils.h>
#include <ostream>
#include <set>
#include <string>
#include <vector>
#include "slp/SLPStrings.h"

namespace ola {
namespace slp {

using std::ostream;
using std::set;
using std::string;
using std::vector;

/**
 * ScopeSet, a set of (canonical) scopes.
 *
 * TODO(simon): If the scopes we care about are static, we could reduce this to
 * a bit-vector which will speed up calls like Intersects().
 */
class ScopeSet {
 public:
    ScopeSet() {}

    ScopeSet(const ScopeSet &other)
      : m_scopes(other.m_scopes) {
    }

    // Construct from a set of strings
    explicit ScopeSet(const set<string> &scopes) {
      for (set<string>::const_iterator iter = scopes.begin();
           iter != scopes.end(); ++iter)
        m_scopes.insert(SLPGetCanonicalString(*iter));
    }

    // Construct from a comma separated string
    explicit ScopeSet(const string &scopes);

    unsigned int empty() const { return m_scopes.empty(); }
    unsigned int size() const { return m_scopes.size(); }

    // Check for membership
    bool Contains(const string &scope) {
      return m_scopes.find(SLPGetCanonicalString(scope)) != m_scopes.end();
    }

    typedef set<string>::const_iterator Iterator;

    // iterate
    Iterator begin() const { return m_scopes.begin(); }
    Iterator end() const { return m_scopes.end(); }

    // Is this ScopeSet a superset of the other one
    // This doesn't check for proper / strict supersets. That is if the two
    // sets are equal, IsSuperSet will return true.
    bool IsSuperSet(const ScopeSet &other) const;

    // Check for intersection between two ScopeSets.
    bool Intersects(const ScopeSet &other) const;

    // Return the number of scopes that appear in both lists.
    unsigned int IntersectionCount(const ScopeSet &other) const;

    // Get the intersection
    ScopeSet Intersection(const ScopeSet &other) const;

    // Get the difference
    ScopeSet Difference(const ScopeSet &other) const;

    // Remove the difference between this set and other from this set.
    // The removed elements are returned.
    ScopeSet DifferenceUpdate(const ScopeSet &other);

    // Add the elements from another ScopeSet to this one
    void Update(const ScopeSet &other) {
      m_scopes.insert(other.m_scopes.begin(), other.m_scopes.end());
    }

    // Return the set of scopes as an escaped string, ready for use in an SLP
    // packet.
    string AsEscapedString() const;

    ScopeSet& operator=(const ScopeSet &other) {
      if (this != &other) {
        m_scopes = other.m_scopes;
      }
      return *this;
    }

    bool operator==(const ScopeSet &other) const {
      return m_scopes == other.m_scopes;
    }

    bool operator!=(const ScopeSet &other) const {
      return m_scopes != other.m_scopes;
    }

    void ToStream(ostream *out) const {
      *out << ola::StringJoin(",", m_scopes);
    }

    string ToString() const {
      std::ostringstream str;
      ToStream(&str);
      return str.str();
    }

    friend ostream& operator<<(ostream &out, const ScopeSet &entry) {
      entry.ToStream(&out);
      return out;
    }

 private:
    set<string> m_scopes;
};
}  // namespace slp
}  // namespace ola
#endif  // SLP_SCOPESET_H_
