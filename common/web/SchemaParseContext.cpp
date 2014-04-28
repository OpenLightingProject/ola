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
 * SchemaParseContext.cpp
 * Stores the state required as we walk the JSON schema document.
 * Copyright (C) 2014 Simon Newton
 */
#include "common/web/SchemaParseContext.h"

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

SchemaParseContextInterface* DefinitionsParseContext::OpenObject(
    const string &path) {
  OLA_INFO << "Starting a new definition at " << path;
  m_current_schema.reset(new SchemaParseContext(m_schema_defs));
  return m_current_schema.get();
}

void DefinitionsParseContext::CloseObject(const string &path) {
  if (!(HasKey() && m_current_schema.get())) {
    OLA_WARN << "Bad call to DefinitionsParseContext::CloseObject";
    return;
  }

  string key = TakeKey();

  ValidatorInterface *schema = m_current_schema->GetValidator();
  OLA_INFO << "Setting validator for " << key << " to " << schema;
  m_schema_defs->Add(TakeKey(), schema);
  m_current_schema.release();
  (void) path;
}


ValidatorInterface* SchemaParseContext::GetValidator() {
  ObjectValidator *validator = new ObjectValidator(ObjectValidator::Options());

  if (m_title.IsSet()) {
    validator->SetTitle(m_title.Value());
    m_title.Reset();
  }
  OLA_INFO << "returning new ObjectValidator";
  return validator;
}

void SchemaParseContext::String(const string &path, const string &value) {
  if (!HasKey()) {
    OLA_WARN << "No key defined for " << value;
    return;
  }
  const string key = TakeKey();
  OLA_INFO << key << " -> " << value;
  if (key == "title") {
    OLA_INFO << "set title to " << value;
    m_title.Set(value);
  } else {
    OLA_WARN << "Unknown key in schema " << key;
  }

  (void) path;
}

void SchemaParseContext::Number(const string &path, uint32_t value) {
  if (!TypeIsValidForCurrentKey(JSON_NUMBER)) {
    return;
  }

  (void) value;
  (void) path;
}

void SchemaParseContext::Number(const string &path, int32_t value) {
  (void) value;
  (void) path;
}

void SchemaParseContext::Number(const string &path, uint64_t value) {
  (void) value;
  (void) path;
}

void SchemaParseContext::Number(const string &path, int64_t value) {
  (void) value;
  (void) path;
}

void SchemaParseContext::Number(const string &path, long double value) {
  (void) value;
  (void) path;
}

void SchemaParseContext::Bool(const string &path, bool value) {
  (void) value;
  (void) path;
}

void SchemaParseContext::OpenArray(const string &path) {
  (void) path;
}

void SchemaParseContext::CloseArray(const string &path) {
  (void) path;
}

SchemaParseContextInterface* SchemaParseContext::OpenObject(
    const string &path) {
  if (!TypeIsValidForCurrentKey(JSON_OBJECT)) {
    return NULL;
  }

  string key = TakeKey();
  if (key == "definitions") {
    return new DefinitionsParseContext(m_schema_defs);
  }
  OLA_INFO << "unknown open schema obj";
  return NULL;
  (void) path;
}

void SchemaParseContext::CloseObject(const string &path) {
  OLA_INFO << "SchemaParseContext: closing object " << path;
}

bool SchemaParseContext::TypeIsValidForCurrentKey(JsonType type) {
  if (!HasKey()) {
    OLA_WARN << "Missing key before type " << type
             << ". This is a bug in the parser!";
    return false;
  }

  string key = Key();
  if (key == "id" || key == "title") {
    return type == JSON_STRING;
  } else if (key == "definitions") {
    return type == JSON_OBJECT;
  }
  OLA_WARN << "Unknown type for key " << key;
  return false;
}
}  // namespace web
}  // namespace ola
