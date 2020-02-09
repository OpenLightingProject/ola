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
 * UIDSet.h
 * A Set of UIDs
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @addtogroup rdm_uid
 * @{
 * @file UIDSet.h
 * @brief A set of UIDs.
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

/**
 * @addtogroup rdm_uid
 * @{
 * @class UIDSet
 * @brief Represents a set of RDM UIDs.
 * @}
 */
class UIDSet {
 public:
    /**
     * @brief the Iterator for a UIDSets
     */
    typedef std::set<UID>::const_iterator Iterator;

    /**
     * @brief Construct an empty set
     */
    UIDSet() {}

    /**
     * @brief Copy constructor.
     * @param other the UIDSet to copy.
     */
    UIDSet(const UIDSet &other):
      m_uids(other.m_uids) {
    }

    /**
     * @brief Construct a new UIDSet from binary data.
     * @param data a pointer to the memory containing the UIDSet data. The data
     * should be most significant byte first.
     * @param length is the length of the data you wish to retrieve
     */
    explicit UIDSet(const uint8_t *data, unsigned int *length) {
      unsigned int used_length = 0;
      while ((*length - used_length) >= UID::LENGTH) {
        m_uids.insert(UID(data + used_length));
        used_length += UID::LENGTH;
      }
      *length = used_length;
    }

    /**
     * @brief Assignment operator
     */
    UIDSet& operator=(const UIDSet &other) {
      if (this != &other) {
        m_uids = other.m_uids;
      }
      return *this;
    }

    /**
     * @brief Remove all members from the set.
     */
    void Clear() {
      m_uids.clear();
    }

    /**
     * @brief Return the number of UIDs in the set.
     * @return the number of UIDs in the set.
     */
    unsigned int Size() const {
      return static_cast<unsigned int>(m_uids.size());
    }

    /**
     * @brief Return whether the UID set is empty.
     * @return true if there are no UIDs in the set.
     */
    bool Empty() const {
      return m_uids.empty();
    }

    /**
     * @brief Add a UID to the set.
     * @param uid the UID to add.
     */
    void AddUID(const UID &uid) {
      m_uids.insert(uid);
    }

    /**
     * @brief Remove a UID from the set.
     * @param uid the UID to remove.
     */
    void RemoveUID(const UID &uid) {
      m_uids.erase(uid);
    }

    /**
     * @brief Check if the set contains a UID.
     * @param uid the UID to check for.
     * @return true if the set contains this UID.
     */
    bool Contains(const UID &uid) const {
      return m_uids.find(uid) != m_uids.end();
    }

    /**
     * @brief Return the union of this set and another UIDSet.
     * @param other the UIDSet to perform the union with.
     * @return the union of the two UIDSets.
     */
    UIDSet Union(const UIDSet &other) {
      std::set<UID> result;
      set_union(m_uids.begin(),
                m_uids.end(),
                other.Begin(),
                other.End(),
                inserter(result, result.begin()));
      return UIDSet(result);
    }

    /**
     * @brief Return an Iterator to the first member of the set.
     */
    Iterator Begin() const {
      return m_uids.begin();
    }

    /**
     * @brief Return an Iterator to one-pass-the-last member of the set.
     */
    Iterator End() const {
      return m_uids.end();
    }

    /**
     * @brief Return the UIDs in this set that don't exist in other.
     * @param other the UIDSet to subtract from this set.
     * @return the difference between this UIDSet and other.
     */
    UIDSet SetDifference(const UIDSet &other) {
      std::set<UID> difference;
      std::set_difference(m_uids.begin(),
                          m_uids.end(),
                          other.m_uids.begin(),
                          other.m_uids.end(),
                          std::inserter(difference, difference.begin()));
      return UIDSet(difference);
    }

    /**
     * @brief Equality operator.
     * @param other the UIDSet to compare to.
     */
    bool operator==(const UIDSet &other) const {
      return m_uids == other.m_uids;
    }

    /**
     * @brief Inequality operator.
     * @param other the UIDSet to compare to.
     */
    bool operator!=(const UIDSet &other) const {
      return !(*this == other);
    }

    /**
     * @brief Convert a UIDSet to a human readable string.
     * @returns a comma separated string with the UIDs from the set.
     */
    std::string ToString() const {
      std::ostringstream str;
      std::set<UID>::const_iterator iter;
      for (iter = m_uids.begin(); iter != m_uids.end(); ++iter) {
        if (iter != m_uids.begin())
          str << ",";
        str << *iter;
      }
      return str.str();
    }

    /**
     * @brief A helper function to write a UIDSet to an ostream.
     * @param out the ostream
     * @param uid_set the UIDSet to write.
     */
    friend std::ostream& operator<< (std::ostream &out, const UIDSet &uid_set) {
      return out << uid_set.ToString();
    }

    /**
     * @brief Write the binary representation of the UID to memory.
     * @param buffer a pointer to memory to write the UID to
     * @param length the size of the memory block, should be at least UID_SIZE.
     * @returns true if length was >= UID_SIZE, false otherwise.
     */
    bool Pack(uint8_t *buffer, unsigned int length) const {
      if (static_cast<size_t>(length) < (m_uids.size() * UID::UID_SIZE)) {
        return false;
      }
      uint8_t *ptr = buffer;
      std::set<UID>::const_iterator iter;
      for (iter = m_uids.begin(); iter != m_uids.end(); ++iter) {
        iter->Pack(ptr, UID::UID_SIZE);
        ptr += UID::UID_SIZE;
      }
      return true;
    }

 private:
    std::set<UID> m_uids;

    explicit UIDSet(const std::set<UID> uids) {
      m_uids = uids;
    }
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_UIDSET_H_
