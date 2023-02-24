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
 * PointerTracker.h
 * Maintains a JsonPointer from a series of parse events.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef COMMON_WEB_POINTERTRACKER_H_
#define COMMON_WEB_POINTERTRACKER_H_

#include <ola/base/Macro.h>
#include <ola/web/JsonPointer.h>
#include <sstream>
#include <string>
#include <vector>

namespace ola {
namespace web {

/**
 * @brief Maintains a Json Pointer (RFC 6901) given a set of Json parse events.
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
 * It has the pointers:
 *   - ""
 *   - "/foo"
 *   - "/foo/bar"
 *   - "/foo/baz"
 *   - "/bat"
 *   - "/bat/0"
 *   - "/bat/1"
 *   - "/bat/2"
 *
 * When parsing this example, the order of method invocation should be:
 * @code
     JsonPointer pointer;
     PointerTracker tracker(&pointer);

     tracker.OpenObject()
     tracker.SetProperty("foo");
     tracker.OpenObject();
     tracker.SetProperty("bar");
     tracker.SetProperty("baz");
     tracker.CloseObject();
     tracker.SetProperty("bat");
     tracker.OpenArray();
     tracker.IncrementIndex();
     tracker.IncrementIndex();
     tracker.IncrementIndex();
     tracker.CloseArray();
     tracker.CloseObject();
   @endcode
 */
class PointerTracker {
 public:
  explicit PointerTracker(JsonPointer *pointer)
     : m_pointer(pointer) {
  }

  /**
   * @brief Open a new Object Element
   */
  void OpenObject();

  /**
   * @brief Set the property name within an Object element.
   * Note if we're not currently in an object element this has no effect.
   */
  void SetProperty(const std::string &property);

  /**
   * @brief Close an Object element.
   * Note if we're not currently in an object element this has no effect.
   */
  void CloseObject();

  /**
   * @brief Open a new Array Element
   * Note that until IncrementIndex() is called, the array index will be -1.
   * This is so you can call IncrementIndex() on each element.
   */
  void OpenArray();

  /**
   * @brief Close an Array Element
   * If we're not currently in an array this doesn't do anything.
   */
  void CloseArray();

  /**
   * @brief Increment an array index.
   * If we're not current in an array this doesn't do anything.
   */
  void IncrementIndex();

 private:
  enum TokenType {
    TOKEN_OBJECT,
    TOKEN_ARRAY,
  };

  struct Token {
   public:
    TokenType type;
    int index;
    bool property_set;

    explicit Token(TokenType type)
      : type(type), index(-1), property_set(false) {}
  };

  JsonPointer *m_pointer;
  std::vector<Token> m_tokens;

  DISALLOW_COPY_AND_ASSIGN(PointerTracker);
};
}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_POINTERTRACKER_H_
