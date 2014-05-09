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
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ola/Logging.h"


#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/Json.h"

namespace ola {
namespace web {

using std::auto_ptr;
using std::make_pair;
using std::pair;
using std::set;
using std::string;
using std::vector;

/**
 * @brief Given a type, return the JSON name for the type
 */
template <typename T>
string TypeFromValue(const T& v);

template <>
string TypeFromValue<string>(const string&) {
  return "string";
}

template <>
string TypeFromValue<bool>(const bool&) {
  return "bool";
}

template <>
string TypeFromValue<uint32_t>(const uint32_t&) {
  return "integer";
}

template <>
string TypeFromValue<int32_t>(const int32_t&) {
  return "integer";
}

template <>
string TypeFromValue<uint64_t>(const uint64_t&) {
  return "integer";
}

template <>
string TypeFromValue<int64_t>(const int64_t&) {
  return "integer";
}

template <>
string TypeFromValue<long double>(const long double&) {
  return "number";
}

template <typename T>
string TypeFromValue(const T&) {
  return "unknown type";
}


// DefinitionsParseContext
// Used for parsing an object with key : json schema pairs, within 'definitions'
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

// SchemaParseContext
// Used for parsing an object that describes a JSON schema.
SchemaParseContext::~SchemaParseContext() {
  // STLDeleteElements(&m_number_constraints);
}

ValidatorInterface* SchemaParseContext::GetValidator(ErrorLogger *logger) {
  OLA_INFO << "GetValidator";
  if (m_ref_schema.IsSet()) {
    return new ReferenceValidator(m_schema_defs, m_ref_schema.Value());
  }

  if (!m_type.IsSet()) {
    OLA_INFO << logger->GetPointer();
    OLA_INFO << "no type";
    logger->Error() << "Missing 'type' property";
    // TODO(simonn): this probably needs to be fixed.
    return new WildcardValidator();
  }

  ValidatorInterface *validator = NULL;
  const string &type = m_type.Value();
  OLA_INFO << " type is " << type;
  if (type == "array") {
    validator = BuildArrayValidator(logger);
  } else if (type == "boolean") {
    validator = new BoolValidator();
  } else if (type == "integer") {
    validator = new IntegerValidator();
  } else if (type == "null") {
    validator = new NullValidator();
  } else if (type == "number") {
    validator = new NumberValidator();
  } else if (type == "object") {
    validator = BuildObjectValidator(logger);
  } else if (type == "string") {
    validator = new StringValidator(StringValidator::Options());
  } else {
    logger->Error() << "Unknown type: " << type;
    return NULL;
  }

  if (!validator) {
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
  if (!HasKey()) {
    logger->Error() <<  "Invalid type string";
    return;
  }

  const string key = TakeKey();
  OLA_INFO << key << " -> " << value;
  if (key == "$ref") {
    m_ref_schema.Set(value);
  } else if (key == "$schema") {
    m_schema.Set(value);
  } else if (key == "description") {
    m_description.Set(value);
  } else if (key == "format") {
    m_format.Set(value);
  } else if (key == "id") {
    m_id.Set(value);
  } else if (key == "title") {
    m_title.Set(value);
  } else if (key == "type") {
    if (ValidType(value)) {
      m_type.Set(value);
    } else {
      logger->Error() <<  "Invalid type: " << value;
    }
  } else {
    logger->Error() << "Unknown key " << key << " for string type";
  }
}

void SchemaParseContext::Number(ErrorLogger *logger, uint32_t value) {
  ProcessInt(logger, value);
}

void SchemaParseContext::Number(ErrorLogger *logger, int32_t value) {
  ProcessInt(logger, value);
}

void SchemaParseContext::Number(ErrorLogger *logger, uint64_t value) {
  ProcessInt(logger, value);
}

void SchemaParseContext::Number(ErrorLogger *logger, int64_t value) {
  ProcessInt(logger, value);
}

void SchemaParseContext::Number(ErrorLogger *logger, long double value) {
  logger->Error() << "Invalid type number: "<< value;
}

void SchemaParseContext::Bool(ErrorLogger *logger, bool value) {
  if (!HasKey()) {
    logger->Error() <<  "Invalid type bool";
    return;
  }

  const string key = TakeKey();
  OLA_INFO << key << " -> " << value;
  if (key == "exclusiveMaximum") {
    m_exclusive_maximum.Set(value);
  } else if (key == "exclusiveMinimum") {
    m_exclusive_minimum.Set(value);
  } else if (key == "uniqueItems") {
    m_unique_items.Set(value);
  } else if (key == "additionalItems") {
    m_additional_items.Set(value);
  } else {
    logger->Error() << "Unknown key " << key << " for bool type";
  }
}

void SchemaParseContext::Null(ErrorLogger *logger) {
  logger->Error() << "Invalid value 'null'";
}

SchemaParseContextInterface* SchemaParseContext::OpenArray(
    ErrorLogger *logger) {
  if (!HasKey()) {
    logger->Error() << "Invalid type array";
    return NULL;
  }

  const string key = TakeKey();
  if (key == "items") {
    m_items_context_array.reset(new ArrayItemsParseContext(m_schema_defs));
    return m_items_context_array.get();
  } else if (key == "required") {
    m_required_items.reset(new RequiredPropertiesParseContext());
    return m_required_items.get();
  } else {
    logger->Error() << "Unknown key in schema: " << key;
    return NULL;
  }
}

void SchemaParseContext::CloseArray(ErrorLogger *logger) {
  (void) logger;
}

SchemaParseContextInterface* SchemaParseContext::OpenObject(
    ErrorLogger *logger) {
  if (!HasKey()) {
    logger->Error() << "Invalid type object";
    return NULL;
  }

  const string key = TakeKey();
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
  } else if (key == "items") {
    m_items_single_context.reset(new SchemaParseContext(m_schema_defs));
    return m_items_single_context.get();
  } else if (key == "additionalItems") {
    m_additional_items_context.reset(new SchemaParseContext(m_schema_defs));
    return m_additional_items_context.get();
  } else {
    logger->Error() << "Unknown key in schema: " << key;
    return NULL;
  }
}

void SchemaParseContext::CloseObject(ErrorLogger *logger) {
  (void) logger;
}

template <typename T>
void SchemaParseContext::ProcessInt(ErrorLogger *logger, T value) {
  if (!HasKey()) {
    logger->Error() <<  "Invalid type integer";
    return;
  }

  const string key = TakeKey();
  OLA_INFO << key << " -> " << value;
  if (key == "minItems") {
    m_min_items.Set(value);
  } else if (key == "maxItems") {
    m_max_items.Set(value);
  } else if (key == "maxLength") {
    m_max_length.Set(value);
  } else if (key == "minLength") {
    m_min_length.Set(value);
  } else if (key == "maxProperties") {
    if (value < 0) {
      logger->Error() << "maxProperties must be >= 0";
    } else {
      m_max_properties.Set(value);
    }
  } else if (key == "minProperties") {
    m_min_properties.Set(value);
    if (value < 0) {
      logger->Error() << "minProperties must be >= 0";
    } else {
      m_min_properties.Set(value);
    }
  } else {
    logger->Error() << "Unknown key in schema: " << key;
  }
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

ValidatorInterface* SchemaParseContext::BuildArrayValidator(
    ErrorLogger *logger) {
  ArrayValidator::Options options;
  if (m_min_items.IsSet()) {
    options.min_items = m_min_items.Value();
  }

  if (m_max_items.IsSet()) {
    options.max_items = m_max_items.Value();
  }

  if (m_unique_items.IsSet()) {
    options.unique_items = m_unique_items.Value();
  }

  std::auto_ptr<ArrayValidator::Items> items;
  std::auto_ptr<ArrayValidator::AdditionalItems> additional_items;

  // items
  if (m_items_single_context.get() && m_items_context_array.get()) {
    logger->Error() << "'items' is somehow both a schema and an array!";
    return NULL;
  } else if (m_items_single_context.get()) {
    // 8.2.3.1
    items.reset(new ArrayValidator::Items(
          m_items_single_context->GetValidator(logger)));
    OLA_INFO << items->Validator();
  } else if (m_items_context_array.get()) {
    // 8.2.3.2
    vector<ValidatorInterface*> item_validators;
    m_items_context_array->AddValidators(logger, &item_validators);
    items.reset(new ArrayValidator::Items(&item_validators));
  }

  // additionalItems
  if (m_additional_items_context.get()) {
    additional_items.reset(new ArrayValidator::AdditionalItems(
        m_additional_items_context->GetValidator(logger)));
  } else if (m_additional_items.IsSet()) {
    additional_items.reset(
        new ArrayValidator::AdditionalItems(m_additional_items.Value()));
  }

  return new ArrayValidator(items.release(), additional_items.release(),
                            options);
}

ValidatorInterface* SchemaParseContext::BuildObjectValidator(
    ErrorLogger* logger) {
  ObjectValidator::Options options;
  if (m_max_properties.IsSet()) {
    options.max_properties = m_max_properties.Value();
  }

  if (m_min_properties.IsSet()) {
    options.min_properties = m_min_properties.Value();
  }

  if (m_required_items.get()) {
    set<string> required_properties;
    m_required_items->GetRequiredStrings(&required_properties);
    options.SetRequiredProperties(required_properties);
  }

  ObjectValidator *object_validator = new ObjectValidator(options);

  if (m_properties_context.get()) {
    m_properties_context->AddPropertyValidaators(object_validator, logger);
  }
  return object_validator;
}

// PropertiesParseContext
// Used for parsing an object with key : json schema pairs, within 'properties'
void PropertiesParseContext::AddPropertyValidaators(
    ObjectValidator *object_validator,
    ErrorLogger *logger) {
  SchemaMap::iterator iter = m_property_contexts.begin();
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

SchemaParseContextInterface* PropertiesParseContext::OpenArray(
    ErrorLogger *logger) {
  (void) logger;
  return NULL;
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

// ArrayItemsParseContext
// Used for parsing an array of JSON schema within 'items'

ArrayItemsParseContext::~ArrayItemsParseContext() {
  STLDeleteElements(&m_item_schemas);
}

void ArrayItemsParseContext::AddValidators(
    ErrorLogger *logger,
    vector<ValidatorInterface*> *validators) {
  ItemSchemas::iterator iter = m_item_schemas.begin();
  for (; iter != m_item_schemas.end(); ++iter) {
    validators->push_back((*iter)->GetValidator(logger));
  }
}

void ArrayItemsParseContext::String(ErrorLogger *logger,
                                    const std::string &value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(ErrorLogger *logger, uint32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(ErrorLogger *logger, int32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(ErrorLogger *logger, uint64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(ErrorLogger *logger, int64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(ErrorLogger *logger, long double value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Bool(ErrorLogger *logger, bool value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Null(ErrorLogger *logger) {
  ReportErrorForType(logger, "null");
}

SchemaParseContextInterface* ArrayItemsParseContext::OpenArray(
    ErrorLogger *logger) {
  ReportErrorForType(logger, "array");
  return NULL;
}

SchemaParseContextInterface* ArrayItemsParseContext::OpenObject(
    ErrorLogger *logger) {
  m_item_schemas.push_back(new SchemaParseContext(m_schema_defs));
  return m_item_schemas.back();
  (void) logger;
}

void ArrayItemsParseContext::ReportErrorForType(ErrorLogger *logger,
                                                const string& type) {
  logger->Error() << "Invalid type '" << type
      << "' in 'items', elements must be a valid JSON schema";
}

// RequiredPropertiesParseContext
// Used for parsing an array of strings within 'required'
void RequiredPropertiesParseContext::GetRequiredStrings(
    RequiredItems *required_items) {
  *required_items = m_required_items;
}

void RequiredPropertiesParseContext::String(ErrorLogger *logger,
                                            const std::string &value) {
  if (!m_required_items.insert(value).second) {
    logger->Error() << "The property " << value << " appeared more than once";
  }
}

void RequiredPropertiesParseContext::Number(ErrorLogger *logger,
                                            uint32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Number(ErrorLogger *logger,
                                            int32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Number(ErrorLogger *logger,
                                            uint64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Number(ErrorLogger *logger,
                                            int64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Number(ErrorLogger *logger,
                                            long double value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Bool(ErrorLogger *logger,
                                          bool value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Null(ErrorLogger *logger) {
  ReportErrorForType(logger, "null");
}

SchemaParseContextInterface* RequiredPropertiesParseContext::OpenArray(
    ErrorLogger *logger) {
  ReportErrorForType(logger, "array");
  return NULL;
}

SchemaParseContextInterface* RequiredPropertiesParseContext::OpenObject(
    ErrorLogger *logger) {
  ReportErrorForType(logger, "object");
  return NULL;
}

void RequiredPropertiesParseContext::ReportErrorForType(ErrorLogger *logger,
                                                        const string& type) {
  logger->Error() << "Invalid type '" << type
      << "' in 'required', elements must be strings";
}

}  // namespace web
}  // namespace ola
