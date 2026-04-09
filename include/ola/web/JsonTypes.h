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
 * JsonTypes.h
 * Enums for the Json types.
 * See http://www.json.org/
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonTypes.h
 * @brief Enums used to identify JSON types.
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONTYPES_H_
#define INCLUDE_OLA_WEB_JSONTYPES_H_

#include <stdint.h>
#include <string>

namespace ola {
namespace web {

/**
 * @brief The type of JSON data element.
 *
 * This comes from section 3.5 of the JSON Schema core document.
 */
enum JsonType {
  JSON_ARRAY,  /**< An Array */
  JSON_BOOLEAN, /**< A boolean */
  JSON_INTEGER, /**< An integer */
  JSON_NULL, /**< A null value */
  JSON_NUMBER, /**< A number */
  JSON_OBJECT, /**< An object */
  JSON_STRING, /**< A string */
  JSON_UNDEFINED, /**< A unknown type */
};

/**
 * @brief Get the string corresponding to a JsonType.
 *
 * e.g. TypeToString(JSON_STRING) returns "string".
 */
std::string JsonTypeToString(JsonType type);

/**
 * @brief Convert a string to a JsonType.
 * @returns The JsonType, or JSON_UNDEFINED if the string isn't a valid type.
 *
 * e.g. StringToType("string") returns JSON_STRING.
 */
JsonType StringToJsonType(const std::string &type);

/**
 * @brief Given a value, return the JsonType this value corresponds to.
 */
template <typename T>
JsonType TypeFromValue(const T&);

/**
 * @brief Given a value, return the string name this value corresponds to.
 */
template <typename T>
std::string GetTypename(const T&t) {
  return TypeToString(TypeFromValue(t));
}

template <>
inline JsonType TypeFromValue<std::string>(const std::string&) {
  return JSON_STRING;
}

template <>
inline JsonType TypeFromValue<bool>(const bool&) { return JSON_BOOLEAN; }

template <>
inline JsonType TypeFromValue<uint32_t>(const uint32_t&) {
  return JSON_INTEGER;
}

template <>
inline JsonType TypeFromValue<int32_t>(const int32_t&) { return JSON_INTEGER; }

template <>
inline JsonType TypeFromValue<uint64_t>(const uint64_t&) {
  return JSON_INTEGER;
}

template <>
inline JsonType TypeFromValue<int64_t>(const int64_t&) { return JSON_INTEGER; }

template <>
inline JsonType TypeFromValue<double>(const double&) { return JSON_NUMBER; }

template <typename T>
inline JsonType TypeFromValue(const T&) { return JSON_UNDEFINED; }

}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONTYPES_H_
