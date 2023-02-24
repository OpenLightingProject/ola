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
 * SchemaKeywords.cpp
 * The keywords used in JSON Schema.
 * Copyright (C) 2014 Simon Newton
 */

#include "common/web/SchemaKeywords.h"

#include <string>

namespace ola {
namespace web {

using std::string;

string KeywordToString(SchemaKeyword keyword) {
  switch (keyword) {
    case SCHEMA_ID:
      return "id";
    case SCHEMA_SCHEMA:
      return "$schema";
    case SCHEMA_REF:
      return "$ref";
    case SCHEMA_TITLE:
      return "title";
    case SCHEMA_DESCRIPTION:
      return "description";
    case SCHEMA_DEFAULT:
      return "default";
    case SCHEMA_FORMAT:
      return "format";
    case SCHEMA_MULTIPLEOF:
      return "multipleOf";
    case SCHEMA_MAXIMUM:
      return "maximum";
    case SCHEMA_EXCLUSIVE_MAXIMUM:
      return "exclusiveMaximum";
    case SCHEMA_MINIMUM:
      return "minimum";
    case SCHEMA_EXCLUSIVE_MINIMUM:
      return "exclusiveMinimum";
    case SCHEMA_MAX_LENGTH:
      return "maxLength";
    case SCHEMA_MIN_LENGTH:
      return "minLength";
    case SCHEMA_PATTERN:
      return "pattern";
    case SCHEMA_ADDITIONAL_ITEMS:
      return "additionalItems";
    case SCHEMA_ITEMS:
      return "items";
    case SCHEMA_MAX_ITEMS:
      return "maxItems";
    case SCHEMA_MIN_ITEMS:
      return "minItems";
    case SCHEMA_UNIQUE_ITEMS:
      return "uniqueItems";
    case SCHEMA_MAX_PROPERTIES:
      return "maxProperties";
    case SCHEMA_MIN_PROPERTIES:
      return "minProperties";
    case SCHEMA_REQUIRED:
      return "required";
    case SCHEMA_ADDITIONAL_PROPERTIES:
      return "additionalProperties";
    case SCHEMA_DEFINITIONS:
      return "definitions";
    case SCHEMA_PROPERTIES:
      return "properties";
    case SCHEMA_PATTERN_PROPERTIES:
      return "patternProperties";
    case SCHEMA_DEPENDENCIES:
      return "dependencies";
    case SCHEMA_ENUM:
      return "enum";
    case SCHEMA_TYPE:
      return "type";
    case SCHEMA_ALL_OF:
      return "allOf";
    case SCHEMA_ANY_OF:
      return "anyOf";
    case SCHEMA_ONE_OF:
      return "oneOf";
    case SCHEMA_NOT:
      return "not";
    case SCHEMA_UNKNOWN:
    default:
      return "";
  }
}

SchemaKeyword LookupKeyword(const string& keyword) {
  if (keyword == "id") {
    return SCHEMA_ID;
  } else if (keyword == "$schema") {
    return SCHEMA_SCHEMA;
  } else if (keyword == "$ref") {
    return SCHEMA_REF;
  } else if (keyword == "title") {
    return SCHEMA_TITLE;
  } else if (keyword == "description") {
    return SCHEMA_DESCRIPTION;
  } else if (keyword == "default") {
    return SCHEMA_DEFAULT;
  } else if (keyword == "format") {
    return SCHEMA_FORMAT;
  } else if (keyword == "multipleOf") {
    return SCHEMA_MULTIPLEOF;
  } else if (keyword == "maximum") {
    return SCHEMA_MAXIMUM;
  } else if (keyword == "exclusiveMaximum") {
    return SCHEMA_EXCLUSIVE_MAXIMUM;
  } else if (keyword == "minimum") {
    return SCHEMA_MINIMUM;
  } else if (keyword == "exclusiveMinimum") {
    return SCHEMA_EXCLUSIVE_MINIMUM;
  } else if (keyword == "maxLength") {
    return SCHEMA_MAX_LENGTH;
  } else if (keyword == "minLength") {
    return SCHEMA_MIN_LENGTH;
  } else if (keyword == "pattern") {
    return SCHEMA_PATTERN;
  } else if (keyword == "additionalItems") {
    return SCHEMA_ADDITIONAL_ITEMS;
  } else if (keyword == "items") {
    return SCHEMA_ITEMS;
  } else if (keyword == "maxItems") {
    return SCHEMA_MAX_ITEMS;
  } else if (keyword == "minItems") {
    return SCHEMA_MIN_ITEMS;
  } else if (keyword == "uniqueItems") {
    return SCHEMA_UNIQUE_ITEMS;
  } else if (keyword == "maxProperties") {
    return SCHEMA_MAX_PROPERTIES;
  } else if (keyword == "minProperties") {
    return SCHEMA_MIN_PROPERTIES;
  } else if (keyword == "required") {
    return SCHEMA_REQUIRED;
  } else if (keyword == "additionalProperties") {
    return SCHEMA_ADDITIONAL_PROPERTIES;
  } else if (keyword == "definitions") {
    return SCHEMA_DEFINITIONS;
  } else if (keyword == "properties") {
    return SCHEMA_PROPERTIES;
  } else if (keyword == "patternProperties") {
    return SCHEMA_PATTERN_PROPERTIES;
  } else if (keyword == "dependencies") {
    return SCHEMA_DEPENDENCIES;
  } else if (keyword == "enum") {
    return SCHEMA_ENUM;
  } else if (keyword == "type") {
    return SCHEMA_TYPE;
  } else if (keyword == "allOf") {
    return SCHEMA_ALL_OF;
  } else if (keyword == "anyOf") {
    return SCHEMA_ANY_OF;
  } else if (keyword == "oneOf") {
    return SCHEMA_ONE_OF;
  } else if (keyword == "not") {
    return SCHEMA_NOT;
  } else {
    return SCHEMA_UNKNOWN;
  }
}
}  // namespace web
}  // namespace ola
