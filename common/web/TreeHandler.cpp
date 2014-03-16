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
 * TreeHandler.cpp
 * A Json Parser.
 * Copyright (C) 2013 Simon Newton
 */
#include "ola/web/TreeHandler.h"

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"

namespace ola {
namespace web {

using std::string;

TreeHandler::~TreeHandler() {
  STLEmptyStackAndDelete(&m_array_stack);
  STLEmptyStackAndDelete(&m_object_stack);
}

void TreeHandler::Begin() {
  m_error = "";
  m_root.reset();
  m_key = "";
  STLEmptyStack(&m_container_stack);
  STLEmptyStackAndDelete(&m_array_stack);
  STLEmptyStackAndDelete(&m_object_stack);
}

void TreeHandler::End() {
  if (!m_container_stack.empty()) {
    OLA_WARN << "Json container stack is not empty";
  }
  STLEmptyStack(&m_container_stack);
  if (!m_array_stack.empty()) {
    OLA_WARN << "JsonArray stack is not empty";
  }
  STLEmptyStackAndDelete(&m_array_stack);
  if (!m_object_stack.empty()) {
    OLA_WARN << "JsonObject stack is not empty";
  }
  STLEmptyStackAndDelete(&m_object_stack);
}

void TreeHandler::String(const string &value) {
  AddValue(new JsonStringValue(value));
}

void TreeHandler::Number(uint32_t value) {
  AddValue(new JsonUIntValue(value));
}

void TreeHandler::Number(int32_t value) {
  AddValue(new JsonIntValue(value));
}

void TreeHandler::Number(uint64_t value) {
  AddValue(new JsonUInt64Value(value));
}

void TreeHandler::Number(int64_t value) {
  AddValue(new JsonInt64Value(value));
}

void TreeHandler::Number(long double value) {
  AddValue(new JsonDoubleValue(value));
}

void TreeHandler::Bool(bool value) {
  AddValue(new JsonBoolValue(value));
}

void TreeHandler::Null() {
  AddValue(new JsonNullValue());
}

void TreeHandler::OpenArray() {
  if (m_container_stack.empty()) {
    m_array_stack.push(new JsonArray());
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

void TreeHandler::CloseArray() {
  if (m_container_stack.empty() || m_container_stack.top() != ARRAY ||
      m_array_stack.empty()) {
    OLA_WARN << "Mismatched CloseArray()";
    m_error = "Internal error";
    return;
  }

  m_container_stack.pop();
  const JsonArray *array = m_array_stack.top();
  m_array_stack.pop();
  if (m_container_stack.empty() && !m_root.get()) {
    m_root.reset(array);
  }
}

void TreeHandler::OpenObject() {
  if (m_container_stack.empty()) {
    m_object_stack.push(new JsonObject());
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

void TreeHandler::ObjectKey(const std::string &key) {
  if (!m_key.empty()) {
    OLA_WARN << "Json Key should be empty, was " << key;
  }
  m_key = key;
}

void TreeHandler::CloseObject() {
  if (m_container_stack.empty() || m_container_stack.top() != OBJECT ||
      m_object_stack.empty()) {
    OLA_WARN << "Mismatched CloseObject()";
    m_error = "Internal error";
    return;
  }

  m_container_stack.pop();
  const JsonObject *object = m_object_stack.top();
  m_object_stack.pop();
  if (m_container_stack.empty() && !m_root.get()) {
    m_root.reset(object);
  }
}

void TreeHandler::SetError(const string &error) {
  m_error = error;
  m_root.reset();
}

string TreeHandler::GetError() const {
  return m_error;
}

const JsonValue *TreeHandler::GetRoot() const {
  return m_root.get();
}

const JsonValue *TreeHandler::ClaimRoot() {
  return m_root.release();
}

void TreeHandler::AddValue(const JsonValue *value) {
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
}  // namespace web
}  // namespace ola
