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
 * JsonWriter.cpp
 * Serialize JSON data.
 * Copyright (C) 2012 Simon Newton
 */

#include <string>
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/JsonWriter.h"

namespace ola {
namespace web {

using std::ostream;
using std::ostringstream;
using std::string;

void JsonWriter::Write(ostream *output, const JsonValue &obj) {
  JsonWriter writer(output);
  obj.Accept(&writer);
}

string JsonWriter::AsString(const JsonValue &obj) {
  ostringstream str;
  JsonWriter writer(&str);
  obj.Accept(&writer);
  return str.str();
}

void JsonWriter::Visit(const JsonString &value) {
  *m_output << '"' << EscapeString(EncodeString(value.Value())) << '"';
}

void JsonWriter::Visit(const JsonBool &value) {
  *m_output << (value.Value() ? "true" : "false");
}

void JsonWriter::Visit(const JsonNull &) {
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

void JsonWriter::Visit(const JsonUInt &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonUInt64 &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonInt &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonInt64 &value) {
  *m_output << value.Value();
}

void JsonWriter::Visit(const JsonDouble &value) {
  *m_output << value.ToString();
}

void JsonWriter::VisitProperty(const string &property,
                               const JsonValue &value) {
  *m_output << m_separator << string(m_indent, ' ') << "\""
            << EscapeString(property) << "\": ";
  value.Accept(this);
  m_separator = ",\n";
}
}  // namespace web
}  // namespace ola
