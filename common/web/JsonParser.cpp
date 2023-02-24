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
 * JsonParser.cpp
 * A Json Parser.
 * Copyright (C) 2014 Simon Newton
 */
#include "ola/web/JsonParser.h"

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"
#include "ola/web/JsonLexer.h"

namespace ola {
namespace web {

using std::string;

void JsonParser::Begin() {
  m_error = "";
  m_root.reset();
  m_key = "";

  STLEmptyStack(&m_container_stack);
  STLEmptyStack(&m_array_stack);
  STLEmptyStack(&m_object_stack);
}

void JsonParser::End() {
  if (!m_container_stack.empty()) {
    OLA_WARN << "Json container stack is not empty";
  }
  STLEmptyStack(&m_container_stack);

  if (!m_array_stack.empty()) {
    OLA_WARN << "JsonArray stack is not empty";
  }
  STLEmptyStack(&m_array_stack);

  if (!m_object_stack.empty()) {
    OLA_WARN << "JsonObject stack is not empty";
  }
  STLEmptyStack(&m_object_stack);
}

void JsonParser::String(const string &value) {
  AddValue(new JsonString(value));
}

void JsonParser::Number(uint32_t value) {
  AddValue(new JsonUInt(value));
}

void JsonParser::Number(int32_t value) {
  AddValue(new JsonInt(value));
}

void JsonParser::Number(uint64_t value) {
  AddValue(new JsonUInt64(value));
}

void JsonParser::Number(int64_t value) {
  AddValue(new JsonInt64(value));
}

void JsonParser::Number(const JsonDouble::DoubleRepresentation &rep) {
  AddValue(new JsonDouble(rep));
}

void JsonParser::Number(double value) {
  AddValue(new JsonDouble(value));
}

void JsonParser::Bool(bool value) {
  AddValue(new JsonBool(value));
}

void JsonParser::Null() {
  AddValue(new JsonNull());
}

void JsonParser::OpenArray() {
  if (m_container_stack.empty()) {
    m_array_stack.push(new JsonArray());
    m_root.reset(m_array_stack.top());
  } else if (m_container_stack.top() == ARRAY && !m_array_stack.empty()) {
    m_array_stack.push(m_array_stack.top()->AppendArray());
  } else if (m_container_stack.top() == OBJECT && !m_object_stack.empty()) {
    m_array_stack.push(m_object_stack.top()->AddArray(m_key));
    m_key = "";
  } else {
    OLA_WARN << "Can't find where to start array";
    m_error = "Internal error";
  }
  m_container_stack.push(ARRAY);
}

void JsonParser::CloseArray() {
  if (m_container_stack.empty() || m_container_stack.top() != ARRAY ||
      m_array_stack.empty()) {
    OLA_WARN << "Mismatched CloseArray()";
    m_error = "Internal error";
    return;
  }

  m_container_stack.pop();
  m_array_stack.pop();
}

void JsonParser::OpenObject() {
  if (m_container_stack.empty()) {
    m_object_stack.push(new JsonObject());
    m_root.reset(m_object_stack.top());
  } else if (m_container_stack.top() == ARRAY && !m_array_stack.empty()) {
    m_object_stack.push(m_array_stack.top()->AppendObject());
  } else if (m_container_stack.top() == OBJECT && !m_object_stack.empty()) {
    m_object_stack.push(m_object_stack.top()->AddObject(m_key));
    m_key = "";
  } else {
    OLA_WARN << "Can't find where to start object";
    m_error = "Internal error";
  }
  m_container_stack.push(OBJECT);
}

void JsonParser::ObjectKey(const string &key) {
  if (!m_key.empty()) {
    OLA_WARN << "Json Key should be empty, was " << key;
  }
  m_key = key;
}

void JsonParser::CloseObject() {
  if (m_container_stack.empty() || m_container_stack.top() != OBJECT ||
      m_object_stack.empty()) {
    OLA_WARN << "Mismatched CloseObject()";
    m_error = "Internal error";
    return;
  }

  m_container_stack.pop();
  m_object_stack.pop();
}

void JsonParser::SetError(const string &error) {
  m_error = error;
}

string JsonParser::GetError() const {
  return m_error;
}

JsonValue *JsonParser::GetRoot() {
  return m_root.get();
}

JsonValue *JsonParser::ClaimRoot() {
  if (m_error.empty()) {
    return m_root.release();
  } else {
    return NULL;
  }
}

void JsonParser::AddValue(JsonValue *value) {
  if (!m_container_stack.empty() && m_container_stack.top() == ARRAY) {
    if (m_array_stack.empty()) {
      OLA_WARN << "Missing JsonArray, parsing is broken!";
      m_error = "Internal error";
      delete value;
    } else {
      m_array_stack.top()->Append(value);
    }
  } else if (!m_container_stack.empty() && m_container_stack.top() == OBJECT) {
    if (m_object_stack.empty()) {
      OLA_WARN << "Missing JsonObject, parsing is broken!";
      m_error = "Internal error";
      delete value;
    } else {
      m_object_stack.top()->AddValue(m_key, value);
      m_key = "";
    }
  } else if (!m_root.get()) {
    m_root.reset(value);
    return;
  } else {
    OLA_WARN << "Parse stack broken";
    m_error = "Internal error";
    delete value;
  }
}

JsonValue* JsonParser::Parse(const string &input, string *error) {
  JsonParser parser;
  if (JsonLexer::Parse(input, &parser)) {
    return parser.ClaimRoot();
  } else {
    *error = parser.GetError();
    return NULL;
  }
}
}  // namespace web
}  // namespace ola
