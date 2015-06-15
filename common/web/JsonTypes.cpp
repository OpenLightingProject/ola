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
 * JsonTypes.cpp
 * Utils for dealing with JsonTypes.
 * Copyright (C) 2014 Simon Newton
 */

#include <string>
#include "ola/web/JsonTypes.h"

namespace ola {
namespace web {

using std::string;

string JsonTypeToString(JsonType type) {
  switch (type) {
    case JSON_ARRAY:
      return "array";
    case JSON_BOOLEAN:
      return "boolean";
    case JSON_INTEGER:
      return "integer";
    case JSON_NULL:
      return "null";
    case JSON_NUMBER:
      return "number";
    case JSON_OBJECT:
      return "object";
    case JSON_STRING:
      return "string";
    case JSON_UNDEFINED:
      return "";
    default:
      return "Unknown type";
  }
}

JsonType StringToJsonType(const string &type) {
  if (type == "array") {
    return JSON_ARRAY;
  } else if (type == "boolean") {
    return JSON_BOOLEAN;
  } else if (type == "integer") {
    return JSON_INTEGER;
  } else if (type == "null") {
    return JSON_NULL;
  } else if (type == "number") {
    return JSON_NUMBER;
  } else if (type == "object") {
    return JSON_OBJECT;
  } else if (type == "string") {
    return JSON_STRING;
  } else {
    return JSON_UNDEFINED;
  }
}
}  // namespace web
}  // namespace ola
