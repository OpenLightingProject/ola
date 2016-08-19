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
 * OptionalItem.h
 * A value which may or may not be present.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef INCLUDE_OLA_WEB_OPTIONALITEM_H_
#define INCLUDE_OLA_WEB_OPTIONALITEM_H_

#include <ola/base/Macro.h>
#include <string>

namespace ola {
namespace web {

template <typename T>
class OptionalItem {
 public:
  OptionalItem();

  void Reset() { m_is_set = false; }

  void Set(const T &value) {
    m_is_set = true;
    m_value = value;
  }

  bool IsSet() const { return m_is_set; }
  const T& Value() const { return m_value; }

 private:
  bool m_is_set;
  T m_value;

  DISALLOW_COPY_AND_ASSIGN(OptionalItem);
};

template <>
inline OptionalItem<std::string>::OptionalItem()
    : m_is_set(false) {
}

template <typename T>
OptionalItem<T>::OptionalItem()
    : m_is_set(false),
      m_value(0) {
}


}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_OPTIONALITEM_H_
