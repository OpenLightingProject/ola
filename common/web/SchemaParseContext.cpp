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

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "ola/Logging.h"


#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"

namespace ola {
namespace web {

using std::auto_ptr;
using std::make_pair;
using std::pair;
using std::string;

SchemaParseContextInterface* DefinitionsParseContext::OpenObject(
    ErrorLogger *logger) {
  OLA_INFO << "Starting a new definition at " << logger->GetPointer();
  m_current_schema.reset(new SchemaParseContext(m_schema_defs));
  return m_current_schema.get();
}

void DefinitionsParseContext::CloseObject(ErrorLogger *logger) {
  if (!(HasKey() && m_current_schema.get())) {
    OLA_WARN << "Bad call to DefinitionsParseContext::CloseObject";
    return;
  }

  string key = TakeKey();

  ValidatorInterface *schema = m_current_schema->GetValidator(logger);
  OLA_INFO << "Setting validator for " << key << " to " << schema;
  m_schema_defs->Add(TakeKey(), schema);
  m_current_schema.release();
}


ValidatorInterface* SchemaParseContext::GetValidator(ErrorLogger *logger) {
  if (!m_type.IsSet()) {
    logger->Error() << "Missing 'type' property";
    return NULL;
  }

  ValidatorInterface *validator = NULL;
  const string &type = m_type.Value();
  OLA_INFO << " type is " << type;
  if (type == "number") {
    OLA_INFO << "new NumberValidator";
    validator = new NumberValidator();
  } else if (type == "string") {
    validator = new StringValidator(StringValidator::Options());
  } else if (type == "boolean") {
    validator = new BoolValidator();
  } else if (type == "object") {
    ObjectValidator *object_validator = new ObjectValidator(
        ObjectValidator::Options());
    if (m_properties_context.get()) {
      m_properties_context->AddPropertyValidaators(object_validator, logger);
    }
    validator = object_validator;
  } else {
    logger->Error() << "Unknown type: " << type;
    return NULL;
  }

  if (m_schema.IsSet()) {
    validator->SetSchema(m_schema.Value());
    m_schema.Reset();
  }
  if (m_id.IsSet()) {
    validator->SetId(m_id.Value());
    m_id.Reset();
  }
  if (m_title.IsSet()) {
    validator->SetTitle(m_title.Value());
    m_title.Reset();
  }
  if (m_description.IsSet()) {
    validator->SetDescription(m_description.Value());
    m_description.Reset();
  }

  OLA_INFO << "returning new Validator";
  return validator;
}

void SchemaParseContext::String(ErrorLogger *logger, const string &value) {
  if (HasKey()) {
    const string key = TakeKey();
    OLA_INFO << key << " -> " << value;
    if (key == "id") {
      m_id.Set(value);
    } else if (key == "$schema") {
      m_schema.Set(value);
    } else if (key == "title") {
      m_title.Set(value);
    } else if (key == "description") {
      m_description.Set(value);
    } else if (key == "type") {
      if (ValidType(value)) {
        m_type.Set(value);
      } else {
        logger->Error() <<  "Invalid type: " << value;
      }
    } else {
      logger->Error() << "Unknown key in schema: " << key;
    }
  } else {
    logger->Error() <<  "Invalid type string";
  }
}

void SchemaParseContext::Number(ErrorLogger *logger, uint32_t value) {
  logger->Error() << "Invalid type number: "<< value;
}

void SchemaParseContext::Number(ErrorLogger *logger, int32_t value) {
  logger->Error() << "Invalid type number: "<< value;
}

void SchemaParseContext::Number(ErrorLogger *logger, uint64_t value) {
  logger->Error() << "Invalid type number: "<< value;
}

void SchemaParseContext::Number(ErrorLogger *logger, int64_t value) {
  logger->Error() << "Invalid type number: "<< value;
}

void SchemaParseContext::Number(ErrorLogger *logger, long double value) {
  logger->Error() << "Invalid type number: "<< value;
}

void SchemaParseContext::Bool(ErrorLogger *logger, bool value) {
  logger->Error() << "Invalid type bool: " << value;
}

void SchemaParseContext::OpenArray(ErrorLogger *logger) {
  logger->Error() << "Invalid type array";
}

void SchemaParseContext::CloseArray(ErrorLogger *logger) {
  (void) logger;
}

SchemaParseContextInterface* SchemaParseContext::OpenObject(
    ErrorLogger *logger) {
  if (HasKey()) {
    const string key = Key();
    if (key == "definitions") {
      if (m_definitions_context.get()) {
        logger->Error() << "Duplicate key 'definitions'";
        return NULL;
      }
      m_definitions_context.reset(new DefinitionsParseContext(m_schema_defs));
      return m_definitions_context.get();
    } else if (key == "properties") {
      if (m_properties_context.get()) {
        logger->Error() << "Duplicate key 'properties'";
        return NULL;
      }
      m_properties_context.reset(new PropertiesParseContext(m_schema_defs));
      return m_properties_context.get();
    } else {
      logger->Error() << "Unknown key in schema: " << key;
    }
  } else {
    logger->Error() << "Invalid type object";
  }
  return NULL;
}

void SchemaParseContext::CloseObject(ErrorLogger *logger) {
  (void) logger;
}


bool SchemaParseContext::ValidType(const string& type) {
  return (type == "array" ||
          type == "boolean" ||
          type == "integer" ||
          type == "null" ||
          type == "number" ||
          type == "object" ||
          type == "string");
}

void PropertiesParseContext::AddPropertyValidaators(
    ObjectValidator *object_validator,
    ErrorLogger *logger) {
  SchemaMap::iterator iter = m_property_contexts.begin();
  OLA_INFO << m_property_contexts.size();
  for (; iter != m_property_contexts.end(); ++iter) {
    ValidatorInterface *validator = iter->second->GetValidator(logger);
    if (validator) {
      object_validator->AddValidator(iter->first, validator);
    }
  }
}

void PropertiesParseContext::String(ErrorLogger *logger, const string &value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(ErrorLogger *logger, uint32_t value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(ErrorLogger *logger, int32_t value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(ErrorLogger *logger, uint64_t value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(ErrorLogger *logger, int64_t value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(ErrorLogger *logger, long double value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Bool(ErrorLogger *logger, bool value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Null(ErrorLogger *logger) {
  (void) logger;
}

void PropertiesParseContext::OpenArray(ErrorLogger *logger) {
  (void) logger;
}

void PropertiesParseContext::CloseArray(ErrorLogger *logger) {
  (void) logger;
}

SchemaParseContextInterface* PropertiesParseContext::OpenObject(
    ErrorLogger *logger) {
  const string key = TakeKey();
  pair<SchemaMap::iterator, bool> r = m_property_contexts.insert(
      pair<string, SchemaParseContext*>(key, NULL));

  if (r.second) {
    OLA_INFO << "Started context for property " << key;
    r.first->second = new SchemaParseContext(m_schema_defs);
  } else {
    logger->Error() << "Duplicate key " << key;
  }
  return r.first->second;
}

void PropertiesParseContext::CloseObject(ErrorLogger *logger) {
  (void) logger;
}

}  // namespace web
}  // namespace ola
