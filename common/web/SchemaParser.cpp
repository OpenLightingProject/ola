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

SchemaParser::SchemaParser()
    : JsonHandlerInterface() {
}

SchemaParser::~SchemaParser() {}

void SchemaParser::Begin() {
  m_schema_defs.reset();
  m_root_context.reset();
  m_root_validator.reset();
  STLEmptyStackAndDelete(&m_context_stack);
  m_error_logger.Reset();
}

void SchemaParser::End() {}

void SchemaParser::String(const string &value) {
  if (m_error_logger.HasError()) {
    return;
  }

  if (!m_root_context.get()) {
    m_error_logger.Error() << "Invalid string for first element: " << value;
    return;
  }

  m_error_logger.IncrementIndex();

  OLA_INFO << "Setting string for " << m_error_logger.GetPointer()
     << " size is " << m_context_stack.size();
  if (m_context_stack.top()) {
    m_context_stack.top()->String(&m_error_logger, value);
  } else {
    OLA_INFO << "In null context, skipping value " << value;
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
  if (m_error_logger.HasError()) {
    return;
  }

  if (!m_root_context.get()) {
    m_error_logger.Error() << "Invalid bool for first element:" << value;
    return;
  }

  m_error_logger.IncrementIndex();

  if (m_context_stack.top()) {
    m_context_stack.top()->Bool(&m_error_logger, value);
  } else {
    OLA_INFO << "In null context, skipping value " << value;
  }
}

void SchemaParser::Null() {
  if (m_error_logger.HasError()) {
    return;
  }

  if (!m_root_context.get()) {
    m_error_logger.Error() << "Invalid null for first element";
    return;
  }

  m_error_logger.IncrementIndex();

  if (m_context_stack.top()) {
    m_context_stack.top()->Null(&m_error_logger);
  } else {
    OLA_INFO << "In null context, skipping null";
  }
}

void SchemaParser::OpenArray() {
  if (m_error_logger.HasError()) {
    return;
  }

  if (!m_root_context.get()) {
    m_error_logger.Error() << "Invalid array for first element";
    return;
  }

  m_error_logger.OpenArray();

  if (m_context_stack.top()) {
    m_context_stack.top()->OpenArray(&m_error_logger);
  } else {
    OLA_INFO << "In null context, skipping OpenArray";
  }
}

void SchemaParser::CloseArray() {
  if (m_error_logger.HasError() || !m_root_context.get()) {
    return;
  }

  m_error_logger.CloseArray();
  if (!m_context_stack.top()) {
    m_context_stack.top()->CloseArray(&m_error_logger);
  } else {
    OLA_INFO << "In null context, skipping CloseArray";
  }
}

void SchemaParser::OpenObject() {
  if (m_error_logger.HasError()) {
    return;
  }

  m_error_logger.OpenObject();

  if (!m_root_context.get()) {
    m_schema_defs.reset(new SchemaDefinitions());
    m_root_context.reset(new SchemaParseContext(m_schema_defs.get()));
    m_context_stack.push(m_root_context.get());
  } else {
    if (m_context_stack.top()) {
      m_context_stack.push(
          m_context_stack.top()->OpenObject(&m_error_logger));
      OLA_INFO << "Opened " << m_error_logger.GetPointer() <<  " ("
          << m_context_stack.top() << ") size is now "
          << m_context_stack.size();
    } else {
      OLA_INFO << "In null context, skipping OpenObject";
      m_context_stack.push(NULL);
    }
  }
}

void SchemaParser::ObjectKey(const std::string &key) {
  if (m_error_logger.HasError()) {
    return;
  }

  m_error_logger.SetProperty(key);

  if (m_context_stack.top()) {
    OLA_INFO << "Setting key for " << m_error_logger.GetPointer();
    m_context_stack.top()->ObjectKey(&m_error_logger, key);
  } else {
    OLA_INFO << "In null context, skipping key " << key;
  }
}

void SchemaParser::CloseObject() {
  if (m_error_logger.HasError()) {
    return;
  }

  m_error_logger.CloseObject();

  m_context_stack.pop();

  const string json_pointer = m_error_logger.GetPointer();

  OLA_INFO << "CloseObject";
  if (m_context_stack.empty()) {
    // We're at the root
    m_root_validator.reset(m_root_context->GetValidator(&m_error_logger));
  } else {
    OLA_INFO << "closing context " << json_pointer << ", size is now "
        << m_context_stack.size();
    if (m_context_stack.top()) {
      m_context_stack.top()->CloseObject(&m_error_logger);
    }
  }
  OLA_INFO << "Close of " << json_pointer << " complete, size is "
      << m_context_stack.size();
}

void SchemaParser::SetError(const string &error) {
  m_error_logger.Error() << error;
}

bool SchemaParser::IsValidSchema() {
  return m_root_validator.get() != NULL;
}

std::string SchemaParser::Error() const {
  return m_error_logger.ErrorString();
}

ValidatorInterface* SchemaParser::ClaimRootValidator() {
  return m_root_validator.release();
}

SchemaDefinitions* SchemaParser::ClaimSchemaDefs() {
  return m_schema_defs.release();
}

template <typename T>
void SchemaParser::HandleNumber(T t) {
  if (m_error_logger.HasError()) {
    return;
  }

  if (!m_root_context.get()) {
    m_error_logger.Error() << "Invalid number for first element: " << t;
    return;
  }

  m_error_logger.IncrementIndex();
  if (m_context_stack.top()) {
    m_context_stack.top()->Number(&m_error_logger, t);
  } else {
    OLA_INFO << "In null context, skipping number " << t;
  }
}
}  // namespace web
}  // namespace ola
