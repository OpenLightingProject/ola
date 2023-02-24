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
 * SchemaKeywords.h
 * The keywords used in JSON Schema.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef COMMON_WEB_SCHEMAKEYWORDS_H_
#define COMMON_WEB_SCHEMAKEYWORDS_H_

#include <string>

namespace ola {
namespace web {

/**
 * @brief The list of valid JSon Schema keywords.
 */
enum SchemaKeyword {
  SCHEMA_UNKNOWN,  /**< Keywords we don't understand */
  SCHEMA_ID,
  SCHEMA_SCHEMA,
  SCHEMA_REF,
  SCHEMA_TITLE,
  SCHEMA_DESCRIPTION,
  SCHEMA_DEFAULT,
  SCHEMA_FORMAT,
  SCHEMA_MULTIPLEOF,
  SCHEMA_MAXIMUM,
  SCHEMA_EXCLUSIVE_MAXIMUM,
  SCHEMA_MINIMUM,
  SCHEMA_EXCLUSIVE_MINIMUM,
  SCHEMA_MAX_LENGTH,
  SCHEMA_MIN_LENGTH,
  SCHEMA_PATTERN,
  SCHEMA_ADDITIONAL_ITEMS,
  SCHEMA_ITEMS,
  SCHEMA_MAX_ITEMS,
  SCHEMA_MIN_ITEMS,
  SCHEMA_UNIQUE_ITEMS,
  SCHEMA_MAX_PROPERTIES,
  SCHEMA_MIN_PROPERTIES,
  SCHEMA_REQUIRED,
  SCHEMA_ADDITIONAL_PROPERTIES,
  SCHEMA_DEFINITIONS,
  SCHEMA_PROPERTIES,
  SCHEMA_PATTERN_PROPERTIES,
  SCHEMA_DEPENDENCIES,
  SCHEMA_ENUM,
  SCHEMA_TYPE,
  SCHEMA_ALL_OF,
  SCHEMA_ANY_OF,
  SCHEMA_ONE_OF,
  SCHEMA_NOT,
};

/**
 * Return the string used by the SchemaKeyword.
 */
std::string KeywordToString(SchemaKeyword keyword);

/**
 * @brief Map a string to a SchemaKeyword.
 * @returns the SchemaKeyword corresponding to the string, or SCHEMA_UNDEFINED.
 */
SchemaKeyword LookupKeyword(const std::string& keyword);

}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_SCHEMAKEYWORDS_H_
