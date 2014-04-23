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

using std::ostream;
using std::string;
using std::stringstream;

void JsonStringValue::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonBoolValue::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonNullValue::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonRawValue::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonIntValue::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonInt64Value::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonUIntValue::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonUInt64Value::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonDoubleValue::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

JsonObject::~JsonObject() {
  STLDeleteValues(&m_members);
}

void JsonObject::Add(const std::string &key, const std::string &value) {
  STLReplaceAndDelete(&m_members, key, new JsonStringValue(value));
}

void JsonObject::Add(const std::string &key, const char *value) {
  Add(key, string(value));
}

void JsonObject::Add(const std::string &key, unsigned int i) {
  STLReplaceAndDelete(&m_members, key, new JsonUIntValue(i));
}

void JsonObject::Add(const std::string &key, int i) {
  STLReplaceAndDelete(&m_members, key, new JsonIntValue(i));
}

void JsonObject::Add(const std::string &key, long double d) {
  STLReplaceAndDelete(&m_members, key, new JsonDoubleValue(d));
}

void JsonObject::Add(const std::string &key, bool value) {
  STLReplaceAndDelete(&m_members, key, new JsonBoolValue(value));
}

void JsonObject::Add(const std::string &key) {
  STLReplaceAndDelete(&m_members, key, new JsonNullValue());
}

void JsonObject::AddRaw(const std::string &key, const std::string &value) {
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

void JsonObject::AddValue(const string &key, const JsonValue *value) {
  STLReplaceAndDelete(&m_members, key, value);
}

void JsonObject::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

void JsonObject::VisitProperties(JsonObjectPropertyVisitor *visitor) const {
  MemberMap::const_iterator iter = m_members.begin();
  for (; iter != m_members.end(); ++iter) {
    visitor->VisitProperty(iter->first, *(iter->second));
  }
}

JsonArray::~JsonArray() {
  STLDeleteElements(&m_values);
}

void JsonArray::Accept(JsonValueVisitorInterface *visitor) const {
  visitor->Visit(*this);
}

const JsonValue *JsonArray::ElementAt(unsigned int i) const {
  if (i < m_values.size()) {
    return m_values[i];
  } else {
    return NULL;
  }
}

void JsonWriter::Write(ostream *output, const JsonValue &obj) {
  JsonWriter writer(output);
  obj.Accept(&writer);
}

string JsonWriter::AsString(const JsonValue &obj) {
  stringstream str;
  JsonWriter writer(&str);
  obj.Accept(&writer);
  return str.str();
}

void JsonWriter::Visit(const JsonStringValue &value) {
  *m_output << '"' << EscapeString(EncodeString(value.Value())) << '"';
}

void JsonWriter::Visit(const JsonBoolValue &value) {
  *m_output << (value.Value() ? "true" : "false");
}

void JsonWriter::Visit(const JsonNullValue &) {
  *m_output << "null";
}

void JsonWriter::Visit(const JsonRawValue &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonObject &value) {
  if (value.IsEmpty()) {
    *m_output << "{}";
    return;
  }

  string old_separator = m_separator;
  m_separator = "";
  m_indent += DEFAULT_INDENT;
  *m_output << "{\n";

  value.VisitProperties(this);
  m_indent -= DEFAULT_INDENT;

  *m_output << "\n" << string(m_indent, ' ');
  *m_output << "}";
  m_separator = old_separator;
}

void JsonWriter::Visit(const JsonArray &value) {
  *m_output << "[";
  string default_separator = ", ";

  if (value.IsComplexType()) {
    m_indent += DEFAULT_INDENT;
    *m_output << "\n" << string(m_indent, ' ');
    default_separator = ",\n";
    default_separator.append(m_indent, ' ');
  }

  string separator;

  for (unsigned int i = 0; i < value.Size(); i++) {
    *m_output << separator;
    value.ElementAt(i)->Accept(this);
    separator = default_separator;
  }

  if (value.IsComplexType()) {
    *m_output << "\n";
    m_indent -= DEFAULT_INDENT;
    *m_output << string(m_indent, ' ');
  }

  *m_output << "]";
}

void JsonWriter::Visit(const JsonUIntValue &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonUInt64Value &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonIntValue &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonInt64Value &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonDoubleValue &value) {
  *m_output << value.Value();
}

void JsonWriter::VisitProperty(const std::string &property,
                               const JsonValue &value) {
  *m_output << m_separator << string(m_indent, ' ') << "\""
            << EscapeString(property) << "\": ";
  value.Accept(this);
  m_separator = ",\n";
}
}  // namespace web
}  // namespace ola
