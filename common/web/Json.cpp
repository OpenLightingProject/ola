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
 * Json.cpp
 * A simple set of classes for generating JSON.
 * See http://www.json.org/
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"

namespace ola {
namespace web {

JsonObject::~JsonObject() {
  STLDeleteValues(&m_members);
}

void JsonObject::Add(const string &key, const string &value) {
  STLReplaceAndDelete(&m_members, key, new JsonStringValue(value));
}

void JsonObject::Add(const string &key, const char *value) {
  Add(key, string(value));
}

void JsonObject::Add(const string &key, unsigned int i) {
  STLReplaceAndDelete(&m_members, key, new JsonUIntValue(i));
}

void JsonObject::Add(const string &key, int i) {
  STLReplaceAndDelete(&m_members, key, new JsonIntValue(i));
}

void JsonObject::Add(const string &key, bool value) {
  STLReplaceAndDelete(&m_members, key, new JsonBoolValue(value));
}

void JsonObject::Add(const string &key) {
  STLReplaceAndDelete(&m_members, key, new JsonNullValue());
}

void JsonObject::AddRaw(const string &key, const string &value) {
  STLReplaceAndDelete(&m_members, key, new JsonRawValue(value));
}

JsonObject* JsonObject::AddObject(const string &key) {
  JsonObject *obj = new JsonObject();
  STLReplaceAndDelete(&m_members, key, obj);
  return obj;
}

JsonArray* JsonObject::AddArray(const string &key) {
  JsonArray *array = new JsonArray();
  STLReplaceAndDelete(&m_members, key, array);
  return array;
}

void JsonObject::ToString(ostream *output, unsigned int indent) const {
  Indent(output, indent);
  *output << "{\n";
  MemberMap::const_iterator iter = m_members.begin();
  string separator = "";
  for (; iter != m_members.end(); ++iter) {
    *output << separator;
    Indent(output, indent + DEFAULT_INDENT);
    *output << '"' << EscapeString(iter->first) << "\": ";
    iter->second->ToString(output, indent + DEFAULT_INDENT);
    separator = ",\n";
  }
  *output << "\n";
  Indent(output, indent);
  *output << "}";
}

JsonArray::~JsonArray() {
  STLDeleteElements(&m_values);
}

void JsonArray::ToString(ostream *output, unsigned int indent) const {
  *output << "[";
  ValuesVector::const_iterator iter = m_values.begin();
  string separator = m_complex_type ? "\n" : "";
  unsigned int child_indent = m_complex_type ? indent + DEFAULT_INDENT : 0;
  for (; iter != m_values.end(); ++iter) {
    *output << separator;
    (*iter)->ToString(output, child_indent);
    separator = (m_complex_type ? ",\n" : ", ");
  }
  if (m_complex_type) {
    *output << "\n";
    Indent(output, indent);
    *output << "]";
  } else {
    *output << "]";
  }
}

void JsonWriter::Write(ostream *output, const JsonValue &obj) {
  obj.ToString(output, 0);
}

string JsonWriter::AsString(const JsonValue &obj) {
  stringstream str;
  obj.ToString(&str, 0);
  return str.str();
}
}  // namespace web
}  // namespace ola
