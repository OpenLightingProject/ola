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
 * UIDSet.h
 * A Set of UIDs
 * Copyright (C) 2005-2010 Simon Newton
 */

/**
 * @addtogroup rdm_uid
 * @{
 * @file UIDSet.h
 * @brief A Set of UIDs
 * @}
 */

#ifndef INCLUDE_OLA_RDM_UIDSET_H_
#define INCLUDE_OLA_RDM_UIDSET_H_

#include <ola/rdm/UID.h>
#include <algorithm>
#include <iomanip>
#include <set>
#include <string>

namespace ola {
namespace rdm {

using std::ostream;
using std::set;


/*
 * Represents a set of RDM UIDs.
 */
class UIDSet {
  public:
    typedef set<UID>::const_iterator Iterator;

    UIDSet() {
    }

    UIDSet(const UIDSet &other):
      m_uids(other.m_uids) {
    }

    UIDSet& operator=(const UIDSet &other) {
      if (this != &other) {
        m_uids = other.m_uids;
      }
      return *this;
    }

    void Clear() {
      m_uids.clear();
    }

    // Number of UIDs in the set
    unsigned int Size() const {
      return m_uids.size();
    }

    // Add this UID to the set
    void AddUID(const UID &uid) {
      m_uids.insert(uid);
    }

    // Remove this UID from the set
    void RemoveUID(const UID &uid) {
      m_uids.erase(uid);
    }

    // Return true if the set contains this UID
    bool Contains(const UID &uid) const {
      return m_uids.find(uid) != m_uids.end();
    }

    void Union(const UIDSet &other) {
      set<UID> result = m_uids;
      set_union(m_uids.begin(),
                m_uids.end(),
                other.Begin(),
                other.End(),
                inserter(result, result.begin()));
      m_uids = result;
    }

    Iterator Begin() const {
      return m_uids.begin();
    }

    Iterator End() const {
      return m_uids.end();
    }

    // Return the UIDs in this set that don't exist in other
    UIDSet SetDifference(const UIDSet &other) {
      set<UID> difference;
      std::set_difference(m_uids.begin(),
                          m_uids.end(),
                          other.m_uids.begin(),
                          other.m_uids.end(),
                          std::inserter(difference, difference.begin()));
      return UIDSet(difference);
    }

    bool operator==(const UIDSet &other) const {
      return m_uids == other.m_uids;
    }

    bool operator!=(const UIDSet &other) const {
      return !(*this == other);
    }

    std::string ToString() const {
      std::stringstream str;
      set<UID>::const_iterator iter;
      for (iter = m_uids.begin(); iter != m_uids.end(); ++iter) {
        if (iter != m_uids.begin())
          str << ",";
        str << *iter;
      }
      return str.str();
    }

    friend ostream& operator<< (ostream &out, const UIDSet &uid_set) {
      return out << uid_set.ToString();
    }

  private:
    set<UID> m_uids;

    explicit UIDSet(const set<UID> uids) {
      m_uids = uids;
    }
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_UIDSET_H_
