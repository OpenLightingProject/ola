/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Json.cpp
 * A simple set of classes for generating JSON.
 * See http://www.json.org/
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include "ola/web/Json.h"
#include "ola/StringUtils.h"


namespace ola {
namespace web {

/**
 * Clean up a JsonObject
 */
JsonObject::~JsonObject() {
  MemberMap::iterator iter = m_members.begin();
  for (; iter != m_members.end(); ++iter)
    delete iter->second;
  m_members.clear();
}


void JsonObject::Add(const string &key, const string &value) {
  FreeIfExists(key);
  m_members[key] = new JsonStringValue(value);
}


void JsonObject::Add(const string &key, const char *value) {
  Add(key, string(value));
}


void JsonObject::Add(const string &key, unsigned int i) {
  FreeIfExists(key);
  m_members[key] = new JsonUIntValue(i);
}


void JsonObject::Add(const string &key, int i) {
  FreeIfExists(key);
  m_members[key] = new JsonUIntValue(i);
}


void JsonObject::Add(const string &key, bool value) {
  FreeIfExists(key);
  m_members[key] = new JsonBoolValue(value);
}


void JsonObject::Add(const string &key) {
  FreeIfExists(key);
  m_members[key] = new JsonNullValue();
}


void JsonObject::AddRaw(const string &key, const string &value) {
  FreeIfExists(key);
  m_members[key] = new JsonRawValue(value);
}


JsonObject* JsonObject::AddObject(const string &key) {
  FreeIfExists(key);
  JsonObject *obj = new JsonObject();
  m_members[key] = obj;
  return obj;
}


JsonArray* JsonObject::AddArray(const string &key) {
  FreeIfExists(key);
  JsonArray *array = new JsonArray();
  m_members[key] = array;
  return array;
}


/**
 * Write a JsonObject to a stream.
 */
void JsonObject::ToString(ostream *output, unsigned int indent) const {
  Indent(output, indent);
  *output << "{\n";
  MemberMap::const_iterator iter = m_members.begin();
  string separator = "";
  for (; iter != m_members.end(); ++iter) {
    *output << separator;
    Indent(output, indent + INDENT);
    *output << '"' << EscapeString(iter->first) << "\": ";
    iter->second->ToString(output, indent + INDENT);
    separator = ",\n";
  }
  *output << "\n";
  Indent(output, indent);
  *output << "}";
}


/**
 * Check if a key exists.
 */
void JsonObject::FreeIfExists(const string &key) const {
  MemberMap::const_iterator iter = m_members.find(key);
  if (iter != m_members.end())
    delete iter->second;
}


/**
 * Clean up a JsonArray
 */
JsonArray::~JsonArray() {
  ValuesVector::iterator iter = m_values.begin();
  for (; iter != m_values.end(); ++iter)
    delete *iter;
  m_values.clear();
}


/**
 * Write a JsonArray to a stream.
 */
void JsonArray::ToString(ostream *output, unsigned int indent) const {
  *output << "[";
  ValuesVector::const_iterator iter = m_values.begin();
  string separator = m_complex_type ? "\n" : "";
  unsigned int child_indent = m_complex_type ? indent + INDENT : 0;
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


/**
 * Write the json to a stream.
 */
void JsonWriter::Write(ostream *output, const JsonValue &obj) {
  obj.ToString(output, 0);
}


string JsonWriter::AsString(const JsonValue &obj) {
  stringstream str;
  obj.ToString(&str, 0);
  return str.str();
}
}  // web
}  // ola
