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
 * JsonPointer.h
 * An implementation of RFC 6901.
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonPointer.h
 * @brief An implementation of Json Pointers (RFC 6901).
 */

#ifndef INCLUDE_OLA_WEB_JSONPOINTER_H_
#define INCLUDE_OLA_WEB_JSONPOINTER_H_

#include <ola/base/Macro.h>
#include <sstream>
#include <string>
#include <vector>

namespace ola {
namespace web {

/**
 * @brief A JSON pointer (RFC 6901) refers to a possible element in a JSON
 * data structure.
 *
 * The element referenced by the pointer may or may not exist.
 *
 * Given the JSON:
 * ~~~~~~~~~~~~~~~~~~~~~
   {
     "foo": {
       "bar": 1,
       "baz": true
     },
     "bat": [0, 1, 2]
   }
   ~~~~~~~~~~~~~~~~~~~~~
 *
 * The JSON pointers for each element are:
 *   - ""
 *   - "/foo"
 *   - "/foo/bar"
 *   - "/foo/baz"
 *   - "/bat"
 *   - "/bat/0"
 *   - "/bat/1"
 *   - "/bat/2"
 */
class JsonPointer {
 public:
  /**
   * @brief An iterator for traversing a JsonPointer.
   *
   * The iterator allows forward iteration only.
   * Iterators don't allow modifications to the underlying JsonPointer.
   */
  class Iterator {
   public:
     explicit Iterator(const JsonPointer *pointer)
        : m_pointer(pointer),
          m_index(0) {
     }

     /**
      * @brief Check if this iterator is valid
      *
      * An iterator is invalid if it refers to a token path the end of the
      * pointer.
      */
     bool IsValid() const {
       return m_index < m_pointer->TokenCount();
     }

     /**
      * @brief Check if the iterator is pointing to the last token.
      */
     bool AtEnd() const {
      return m_index + 1 == m_pointer->TokenCount();
     }

     /**
      * @brief Move the iterator to the next token in the pointer.
      */
     Iterator& operator++() {
        m_index++;
        return *this;
     }

     /**
      * @brief Move the iterator to the next token in the pointer.
      */
     Iterator& operator++(int) {
        m_index++;
        return *this;
     }

     /**
      * @brief Return the current token.
      */
     std::string operator*() const {
       return m_pointer->TokenAt(m_index);
     }

   private:
    const JsonPointer *m_pointer;
    unsigned int m_index;
  };

  JsonPointer();
  JsonPointer(const JsonPointer &other);

  /**
   * @brief Construct a new JsonPointer from a string representing the path.
   * @param path A Json Pointer path. The path should either be empty, or start
   * with a /.
   */
  explicit JsonPointer(const std::string &path);

  /**
   * @brief Equality operator
   */
  bool operator==(const JsonPointer &other) const;

  /**
   * @brief Returns true if this pointer is valid.
   *
   * Invalid pointers are ones that don't start with a /.
   * If the pointer is invalid, the result of all other methods is undefined.
   */
  bool IsValid() const { return m_is_valid; }

  /**
   * @brief Return an iterator pointing to the first token in the JsonPointer.
   */
  Iterator begin() const {
    return Iterator(this);
  }

  /**
   * @brief The number of tokens in the Json Pointer.
   *
   * A valid pointer has at least one token (""). The number of tokens is the
   * number of / plus one.
   */
  unsigned int TokenCount() const {
    return m_tokens.size() + 1;
  }

  /**
   * @brief Return the token at the specified index.
   * @param i the index of the token to return.
   * @returns the un-encoded representation of the token.
   */
  std::string TokenAt(unsigned int i) const {
    if (i >= m_tokens.size()) {
      return "";
    } else {
      return m_tokens[i];
    }
  }

  /**
   * @brief Append a token to the pointer path.
   * @param token The un-escaped token to add to this JsonPointer.
   */
  void Push(const std::string &token);

  /**
   * @brief Pop the last token from the pointer.
   */
  void Pop();

  /**
   * @brief Returns the string representation of the pointer
   */
  std::string ToString() const;

  /**
   * @brief Check if this pointer is a prefix of another
   * @param other the JsonPointer to compare to.
   * @returns true if this object is a prefix of the parameter.
   */
  bool IsPrefixOf(const JsonPointer &other) const;

 private:
  typedef std::vector<std::string> Tokens;

  bool m_is_valid;
  Tokens m_tokens;

  static std::string EscapeString(const std::string &input);
  static std::string UnEscapeString(const std::string &input);

  void operator=(const JsonPointer&);
};
}  // namespace web
}  // namespace ola
/**@}*/
#endif  // INCLUDE_OLA_WEB_JSONPOINTER_H_
