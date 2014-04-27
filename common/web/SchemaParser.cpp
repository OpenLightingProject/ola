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
 * SchemaParser.cpp
 * A json-schema Parser.
 * Copyright (C) 2014 Simon Newton
 */
#include "common/web/SchemaParser.h"

#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"

namespace ola {
namespace web {

using std::auto_ptr;
using std::string;

class SchemaParseContext;

SchemaParser::SchemaParser()
    : JsonHandlerInterface(),
      m_verified(false) {
}

SchemaParser::~SchemaParser() {
}

void SchemaParser::Begin() {
  m_verified = false;
  m_root_validator.reset();
  m_error.str("");
}

void SchemaParser::End() {
  OLA_INFO << "End()";
}

void SchemaParser::String(const string &value) {
  m_pointer.IncrementIndex();

  if (!m_context_stack.empty()) {
    if (m_context_stack.top()) {
      m_context_stack.top()->String(value);
    } else {
      OLA_INFO << "In null context, skipping value " << value;
    }
  } else {
    OLA_WARN << "Invalid schema : " << value;
  }
}

void SchemaParser::Number(uint32_t value) {
  return HandleNumber(value);
}

void SchemaParser::Number(int32_t value) {
  return HandleNumber(value);
}

void SchemaParser::Number(uint64_t value) {
  return HandleNumber(value);
}

void SchemaParser::Number(int64_t value) {
  return HandleNumber(value);
}

void SchemaParser::Number(long double value) {
  return HandleNumber(value);
}

void SchemaParser::Bool(bool value) {
  m_pointer.IncrementIndex();
  if (!m_context_stack.empty()) {
    m_context_stack.top()->Bool(value);
  } else {
    OLA_WARN << "Invalid schema : " << value;
  }
}

void SchemaParser::Null() {
  m_pointer.IncrementIndex();
  if (!m_context_stack.empty()) {
    m_context_stack.top()->Null();
  } else {
    OLA_WARN << "Invalid schema with null";
  }
}

void SchemaParser::OpenArray() {
  m_pointer.OpenArray();
  if (!m_context_stack.empty()) {
    m_context_stack.top()->OpenArray();
  } else {
    OLA_WARN << "Invalid schema with array";
  }
}

void SchemaParser::CloseArray() {
  m_pointer.CloseArray();
  if (!m_context_stack.empty()) {
    m_context_stack.top()->CloseArray();
  } else {
    OLA_WARN << "Invalid schema with array";
  }
}

void SchemaParser::OpenObject() {
  m_pointer.OpenObject();
  OLA_INFO << "open obj";
  if (m_context_stack.empty()) {
    m_context_stack.push(new SchemaParseContext());
  } else {
    if (m_context_stack.top()) {
      m_context_stack.push(m_context_stack.top()->OpenObject());
      OLA_INFO << "pushed " << m_context_stack.top();
    } else {
      OLA_INFO << "In null context, skipping open ";
      m_context_stack.push(NULL);
    }
  }
}

void SchemaParser::ObjectKey(const std::string &key) {
  m_pointer.SetProperty(key);
  OLA_INFO << "setting key " << key;

  if (!m_context_stack.empty()) {
    if (m_context_stack.top()) {
      m_context_stack.top()->ObjectKey(key);
    } else {
      OLA_INFO << "In null context, skipping key " << key;
    }
  } else {
    OLA_WARN << "Invalid schema : " << key;
  }
}

void SchemaParser::CloseObject() {
  m_pointer.CloseObject();

  OLA_INFO << "close, size is " << m_context_stack.size();
  if (m_context_stack.empty()) {
    OLA_INFO << "Missing ParseContext!";
    m_verified = false;
    return;
  }

  if (m_context_stack.size() == 1) {
    // We're at the root
    auto_ptr<SchemaParseContextInterface> context(m_context_stack.top());
    m_context_stack.pop();

    // TODO(simonn): FIX ME
    SchemaParseContext *root = dynamic_cast<SchemaParseContext*>(context.get());
    m_root_validator.reset(root->GetValidator());
    m_verified = true;

  } else {
    m_context_stack.pop();
    OLA_INFO << "closing context, size is now " << m_context_stack.size();
    if (m_context_stack.top()) {
      m_context_stack.top()->CloseObject();
    }
  }
}

void SchemaParser::SetError(const string &error) {
  m_error.str(error);
}

bool SchemaParser::IsValidSchema() {
  return m_verified;
}

std::string SchemaParser::Error() const {
  return m_error.str();
}

ValidatorInterface* SchemaParser::ClaimRootValidator() {
  if (m_verified) {
    return m_root_validator.release();
  } else {
    return NULL;
  }
}

SchemaDefintions* SchemaParser::ClaimSchemaDefs() {
  return NULL;
}

template <typename T>
void SchemaParser::HandleNumber(T t) {
  m_pointer.IncrementIndex();
  if (!m_context_stack.empty()) {
    if (m_context_stack.top()) {
      m_context_stack.top()->Number(t);
    } else {
      OLA_INFO << "In null context, skipping value " << t;
    }
  } else {
    OLA_WARN << "Invalid schema : " << t;
  }
}
}  // namespace web
}  // namespace ola
