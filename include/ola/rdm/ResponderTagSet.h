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
 * ResponderTagSet.h
 * A Set of Tags
 * Copyright (C) 2025 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file ResponderTagSet.h
 * @brief A set of Tags.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERTAGSET_H_
#define INCLUDE_OLA_RDM_RESPONDERTAGSET_H_

#include <ola/Logging.h>
#include <ola/strings/Utils.h>

#include <algorithm>
#include <iomanip>
#include <set>
#include <string>

namespace ola {
namespace rdm {

/**
 * @addtogroup rdm_resp
 * @{
 * @class TagSet
 * @brief Represents a set of Tags.
 * @}
 */
class TagSet {
 public:
    /**
     * @brief the Iterator for a TagSets
     */
    typedef std::set<std::string>::const_iterator Iterator;

    /**
     * @brief Construct an empty set
     */
    TagSet() {}

    /**
     * @brief Copy constructor.
     * @param other the TagSet to copy.
     */
    TagSet(const TagSet &other):
      m_tags(other.m_tags) {
    }

    /**
     * @brief Assignment operator
     */
    TagSet& operator=(const TagSet &other) {
      if (this != &other) {
        m_tags = other.m_tags;
      }
      return *this;
    }

    /**
     * @brief Remove all members from the set.
     */
    void Clear() {
      m_tags.clear();
    }

    /**
     * @brief Return the number of Tags in the set.
     * @return the number of Tags in the set.
     */
    unsigned int Size() const {
      return static_cast<unsigned int>(m_tags.size());
    }

    /**
     * @brief Return whether the Tag set is empty.
     * @return true if there are no Tags in the set.
     */
    bool Empty() const {
      return m_tags.empty();
    }

    /**
     * @brief Add a Tag to the set.
     * @param tag the Tag to add.
     */
    void AddTag(const std::string &tag) {
      m_tags.insert(tag);
    }

    /**
     * @brief Remove a Tag from the set.
     * @param tag the Tag to remove.
     */
    void RemoveTag(const std::string &tag) {
      m_tags.erase(tag);
    }

    /**
     * @brief Check if the set contains a Tag.
     * @param tag the Tag to check for.
     * @return true if the set contains this Tag.
     */
    bool Contains(const std::string &tag) const {
      return m_tags.find(tag) != m_tags.end();
    }

    /**
     * @brief Return an Iterator to the first member of the set.
     */
    Iterator Begin() const {
      return m_tags.begin();
    }

    /**
     * @brief Return an Iterator to one-past-the-last member of the set.
     */
    Iterator End() const {
      return m_tags.end();
    }

    /**
     * @brief Equality operator.
     * @param other the TagSet to compare to.
     */
    bool operator==(const TagSet &other) const {
      return m_tags == other.m_tags;
    }

    /**
     * @brief Inequality operator.
     * @param other the TagSet to compare to.
     */
    bool operator!=(const TagSet &other) const {
      return !(*this == other);
    }

    /**
     * @brief Convert a TagSet to a human readable string.
     * @returns a comma separated string with the Tags from the set.
     * @note Commas can also exist within the tags themselves...
     */
    std::string ToString() const {
      std::ostringstream str;
      std::set<std::string>::const_iterator iter;
      for (iter = m_tags.begin(); iter != m_tags.end(); ++iter) {
        if (iter != m_tags.begin())
          str << ",";
        str << *iter;
      }
      return str.str();
    }

    /**
     * @brief A helper function to write a TagSet to an ostream.
     * @param out the ostream
     * @param tag_set the TagSet to write.
     */
    friend std::ostream& operator<< (std::ostream &out, const TagSet &tag_set) {
      return out << tag_set.ToString();
    }

    /**
     * @brief Write the binary representation of the Tag set to memory.
     * @param[in] buffer a pointer to memory to write the Tag set to
     * @param[in,out] length the size of the memory block, should be at least
     *   (32 + 1) * TagSet size. Set to the used size.
     * @returns true if length was >= (32 + 1) * set size, false otherwise.
     */
    bool Pack(uint8_t *buffer, unsigned int *length) const {
      if (static_cast<size_t>(*length) < (m_tags.size() * (32 + 1))) {
        return false;
      }
      uint8_t *ptr = buffer;
      unsigned int i = 0;
      std::set<std::string>::const_iterator iter;
      for (iter = m_tags.begin(); iter != m_tags.end(); ++iter) {
        // We can't use StrNCopy as we want to specify the iter length each go
        strncpy(reinterpret_cast<char*>(ptr + i),
                iter->c_str(),
                iter->length());
        // Set the NULL termination!
        ptr[i + iter->length()] = 0;
        // Increment past the tag and NULL
        i += iter->length() + 1;
      }
      *length = i;
      return true;
    }

 private:
    std::set<std::string> m_tags;

    explicit TagSet(const std::set<std::string> tags) {
      m_tags = tags;
    }
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERTAGSET_H_
