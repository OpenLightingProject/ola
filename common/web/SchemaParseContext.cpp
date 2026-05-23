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
 * SchemaParseContext.cpp
 * Stores the state required as we walk the JSON schema document.
 * Copyright (C) 2014 Simon Newton
 */
#include "common/web/SchemaParseContext.h"

#include <limits>
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
#include "ola/web/JsonTypes.h"

namespace ola {
namespace web {

namespace {

/*
 * A cool hack to determine if a type is positive.
 * We do this to avoid:
 *   comparison of unsigned expression >= 0 is always true
 * warnings. See http://gcc.gnu.org/ml/gcc-help/2010-08/msg00286.html
 */
template<typename T, bool sign>
struct positive {
  bool operator()(const T &x) { return x >= 0; }
};

template<typename T>
struct positive<T, false>{
  bool operator()(const T &) {
    return true;
  }
};
}  // namespace

using std::unique_ptr;
using std::pair;
using std::set;
using std::string;
using std::vector;

// DefinitionsParseContext
// Used for parsing an object with key : json schema pairs, within 'definitions'
SchemaParseContextInterface* DefinitionsParseContext::OpenObject(
    OLA_UNUSED SchemaErrorLogger *logger) {
  m_current_schema.reset(new SchemaParseContext(m_schema_defs));
  return m_current_schema.get();
}

void DefinitionsParseContext::CloseObject(SchemaErrorLogger *logger) {
  string key = TakeKeyword();

  ValidatorInterface *schema = m_current_schema->GetValidator(logger);
  m_schema_defs->Add(key, schema);
  m_current_schema.reset();
}

// StrictTypedParseContext
// Used as a parent class for many of the other contexts.
void StrictTypedParseContext::String(SchemaErrorLogger *logger,
                                     const string &value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void StrictTypedParseContext::Number(SchemaErrorLogger *logger,
                                     uint32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void StrictTypedParseContext::Number(SchemaErrorLogger *logger, int32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void StrictTypedParseContext::Number(SchemaErrorLogger *logger,
                                     uint64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void StrictTypedParseContext::Number(SchemaErrorLogger *logger, int64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void StrictTypedParseContext::Number(SchemaErrorLogger *logger, double value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void StrictTypedParseContext::Bool(SchemaErrorLogger *logger, bool value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void StrictTypedParseContext::Null(SchemaErrorLogger *logger) {
  ReportErrorForType(logger, JSON_NULL);
}

SchemaParseContextInterface* StrictTypedParseContext::OpenArray(
    SchemaErrorLogger *logger) {
  ReportErrorForType(logger, JSON_ARRAY);
  return NULL;
}

void StrictTypedParseContext::CloseArray(
    OLA_UNUSED SchemaErrorLogger *logger) {
}

SchemaParseContextInterface* StrictTypedParseContext::OpenObject(
    SchemaErrorLogger *logger) {
  ReportErrorForType(logger, JSON_ARRAY);
  return NULL;
}

void StrictTypedParseContext::CloseObject(
    OLA_UNUSED SchemaErrorLogger *logger) {
}

void StrictTypedParseContext::ReportErrorForType(
    SchemaErrorLogger *logger,
    JsonType type) {
  logger->Error() << "Invalid type '" << JsonTypeToString(type)
                  << "' in 'required', elements must be strings";
}

// SchemaParseContext
// Used for parsing an object that describes a JSON schema.
ValidatorInterface* SchemaParseContext::GetValidator(
    SchemaErrorLogger *logger) {
  if (m_ref_schema.IsSet()) {
    return new ReferenceValidator(m_schema_defs, m_ref_schema.Value());
  }

  BaseValidator *validator = NULL;
  unique_ptr<IntegerValidator> int_validator;
  switch (m_type) {
    case JSON_UNDEFINED:
      break;
    case JSON_ARRAY:
      validator = BuildArrayValidator(logger);
      break;
    case JSON_BOOLEAN:
      validator = new BoolValidator();
      break;
    case JSON_INTEGER:
      int_validator.reset(new IntegerValidator());
      break;
    case JSON_NULL:
      validator = new NullValidator();
      break;
    case JSON_NUMBER:
      int_validator.reset(new NumberValidator());
      break;
    case JSON_OBJECT:
      validator = BuildObjectValidator(logger);
      break;
    case JSON_STRING:
      validator = BuildStringValidator(logger);
      break;
    default:
      {}
  }

  if (int_validator.get()) {
    if (!AddNumberConstraints(int_validator.get(), logger)) {
      return NULL;
    }
    validator = int_validator.release();
  }

  if (!validator && m_allof_context.get()) {
    ValidatorInterface::ValidatorList all_of_validators;
    m_allof_context->GetValidators(logger, &all_of_validators);
    if (all_of_validators.empty()) {
      logger->Error() << "allOf must contain at least one schema";
      return NULL;
    }
    validator = new AllOfValidator(&all_of_validators);
  }

  if (!validator && m_anyof_context.get()) {
    ValidatorInterface::ValidatorList any_of_validators;
    m_anyof_context->GetValidators(logger, &any_of_validators);
    if (any_of_validators.empty()) {
      logger->Error() << "anyOf must contain at least one schema";
      return NULL;
    }
    validator = new AnyOfValidator(&any_of_validators);
  }

  if (!validator && m_oneof_context.get()) {
    ValidatorInterface::ValidatorList one_of_validators;
    m_oneof_context->GetValidators(logger, &one_of_validators);
    if (one_of_validators.empty()) {
      logger->Error() << "oneOf must contain at least one schema";
      return NULL;
    }
    validator = new OneOfValidator(&one_of_validators);
  }

  if (!validator && m_not_context.get()) {
    validator = new NotValidator(m_not_context->GetValidator(logger));
  }

  if (validator == NULL) {
    if (m_type == JSON_UNDEFINED) {
      validator = new WildcardValidator();
    } else {
      logger->Error() << "Unknown type: " << JsonTypeToString(m_type);
      return NULL;
    }
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
  if (m_default_value.get()) {
    validator->SetDefaultValue(m_default_value.release());
  }

  if (m_enum_context.get()) {
    m_enum_context->AddEnumsToValidator(validator);
  }

  return validator;
}


void SchemaParseContext::ObjectKey(OLA_UNUSED SchemaErrorLogger *logger,
                                   const string &keyword) {
  m_keyword = LookupKeyword(keyword);
}

void SchemaParseContext::String(SchemaErrorLogger *logger,
                                const string &value) {
  if (!ValidTypeForKeyword(logger, m_keyword, TypeFromValue(value))) {
    return;
  }

  switch (m_keyword) {
    case SCHEMA_REF:
      m_ref_schema.Set(value);
      break;
    case SCHEMA_SCHEMA:
      m_schema.Set(value);
      break;
    case SCHEMA_DESCRIPTION:
      m_description.Set(value);
      break;
    case SCHEMA_DEFAULT:
      m_default_value.reset(new JsonString(value));
      break;
    case SCHEMA_FORMAT:
      m_format.Set(value);
      break;
    case SCHEMA_ID:
      m_id.Set(value);
      break;
    case SCHEMA_TITLE:
      m_title.Set(value);
      break;
    case SCHEMA_TYPE:
      m_type = StringToJsonType(value);
      if (m_type == JSON_UNDEFINED) {
        logger->Error() <<  "Invalid type: " << value;
      }
      break;
    default:
      // Nothing, we ignore keywords we don't support.
      {}
  }
}

void SchemaParseContext::Number(SchemaErrorLogger *logger, uint32_t value) {
  ProcessInt(logger, value);
}

void SchemaParseContext::Number(SchemaErrorLogger *logger, int32_t value) {
  ProcessInt(logger, value);
}

void SchemaParseContext::Number(SchemaErrorLogger *logger, uint64_t value) {
  ProcessInt(logger, value);
}

void SchemaParseContext::Number(SchemaErrorLogger *logger, int64_t value) {
  ProcessInt(logger, value);
}

void SchemaParseContext::Number(SchemaErrorLogger *logger, double value) {
  ValidTypeForKeyword(logger, m_keyword, TypeFromValue(value));

  switch (m_keyword) {
    case SCHEMA_DEFAULT:
      m_default_value.reset(new JsonDouble(value));
      break;
    case SCHEMA_MAXIMUM:
      m_maximum.reset(JsonValue::NewNumberValue(value));
      break;
    case SCHEMA_MINIMUM:
      m_minimum.reset(JsonValue::NewNumberValue(value));
      break;
    case SCHEMA_MULTIPLEOF:
      if (value <= 0) {
        logger->Error() << KeywordToString(m_keyword) << " can't be negative";
      } else {
        m_multiple_of.reset(JsonValue::NewNumberValue(value));
      }
      return;
    default:
      {}
  }
}

void SchemaParseContext::Bool(SchemaErrorLogger *logger, bool value) {
  if (!ValidTypeForKeyword(logger, m_keyword, TypeFromValue(value))) {
    OLA_INFO << "type was not valid";
    return;
  }

  switch (m_keyword) {
    case SCHEMA_DEFAULT:
      m_default_value.reset(new JsonBool(value));
      break;
    case SCHEMA_EXCLUSIVE_MAXIMUM:
      m_exclusive_maximum.Set(value);
      break;
    case SCHEMA_EXCLUSIVE_MINIMUM:
      m_exclusive_minimum.Set(value);
      break;
    case SCHEMA_UNIQUE_ITEMS:
      m_unique_items.Set(value);
      break;
    case SCHEMA_ADDITIONAL_ITEMS:
      m_additional_items.Set(value);
      break;
    case SCHEMA_ADDITIONAL_PROPERTIES:
      m_additional_properties.Set(value);
    default:
      {}
  }
}

void SchemaParseContext::Null(SchemaErrorLogger *logger) {
  ValidTypeForKeyword(logger, m_keyword, JSON_NULL);

  switch (m_keyword) {
    case SCHEMA_DEFAULT:
      m_default_value.reset(new JsonNull());
      break;
    default:
      {}
  }
}

SchemaParseContextInterface* SchemaParseContext::OpenArray(
    SchemaErrorLogger *logger) {
  if (!ValidTypeForKeyword(logger, m_keyword, JSON_ARRAY)) {
    return NULL;
  }

  switch (m_keyword) {
    case SCHEMA_DEFAULT:
      m_default_value_context.reset(new JsonValueContext());
      m_default_value_context->OpenArray(logger);
      return m_default_value_context.get();
    case SCHEMA_ITEMS:
      m_items_context_array.reset(new ArrayOfSchemaContext(m_schema_defs));
      return m_items_context_array.get();
    case SCHEMA_REQUIRED:
      m_required_items.reset(new ArrayOfStringsContext());
      return m_required_items.get();
    case SCHEMA_ENUM:
      m_enum_context.reset(new ArrayOfJsonValuesContext());
      return m_enum_context.get();
    case SCHEMA_ALL_OF:
      m_allof_context.reset(new ArrayOfSchemaContext(m_schema_defs));
      return m_allof_context.get();
    case SCHEMA_ANY_OF:
      m_anyof_context.reset(new ArrayOfSchemaContext(m_schema_defs));
      return m_anyof_context.get();
    case SCHEMA_ONE_OF:
      m_oneof_context.reset(new ArrayOfSchemaContext(m_schema_defs));
      return m_oneof_context.get();
    default:
      {}
  }
  return NULL;
}

void SchemaParseContext::CloseArray(SchemaErrorLogger *logger) {
  if (m_default_value_context.get()) {
    m_default_value_context->CloseArray(logger);
    m_default_value.reset(m_default_value_context->ClaimValue(logger));
    m_default_value_context.reset();
  }

  if (m_keyword == SCHEMA_ENUM) {
    if (m_enum_context->Empty()) {
      logger->Error() << "enum must contain at least one value";
    }
  }
}

SchemaParseContextInterface* SchemaParseContext::OpenObject(
    SchemaErrorLogger *logger) {
  if (!ValidTypeForKeyword(logger, m_keyword, JSON_OBJECT)) {
    return NULL;
  }

  switch (m_keyword) {
    case SCHEMA_DEFAULT:
      m_default_value_context.reset(new JsonValueContext());
      m_default_value_context->OpenObject(logger);
      return m_default_value_context.get();
    case SCHEMA_DEFINITIONS:
      m_definitions_context.reset(new DefinitionsParseContext(m_schema_defs));
      return m_definitions_context.get();
    case SCHEMA_PROPERTIES:
      m_properties_context.reset(new PropertiesParseContext(m_schema_defs));
      return m_properties_context.get();
    case SCHEMA_ADDITIONAL_PROPERTIES:
      m_additional_properties_context.reset(
          new SchemaParseContext(m_schema_defs));
      return m_additional_properties_context.get();
    case SCHEMA_ITEMS:
      m_items_single_context.reset(new SchemaParseContext(m_schema_defs));
      return m_items_single_context.get();
    case SCHEMA_ADDITIONAL_ITEMS:
      m_additional_items_context.reset(new SchemaParseContext(m_schema_defs));
      return m_additional_items_context.get();
    case SCHEMA_DEPENDENCIES:
      m_dependency_context.reset(new DependencyParseContext(m_schema_defs));
      return m_dependency_context.get();
    case SCHEMA_NOT:
      m_not_context.reset(new SchemaParseContext(m_schema_defs));
      return m_not_context.get();
    default:
      {}
  }
  return NULL;
}

void SchemaParseContext::CloseObject(SchemaErrorLogger *logger) {
  if (m_default_value_context.get()) {
    m_default_value_context->CloseObject(logger);
    m_default_value.reset(m_default_value_context->ClaimValue(logger));
    m_default_value_context.reset();
  }
}

void SchemaParseContext::ProcessPositiveInt(
    OLA_UNUSED SchemaErrorLogger *logger,
    uint64_t value) {
  switch (m_keyword) {
    case SCHEMA_MULTIPLEOF:
      m_multiple_of.reset(JsonValue::NewNumberValue(value));
      return;
    case SCHEMA_MIN_ITEMS:
      m_min_items.Set(value);
      break;
    case SCHEMA_MAX_ITEMS:
      m_max_items.Set(value);
      break;
    case SCHEMA_MAX_LENGTH:
      m_max_length.Set(value);
      break;
    case SCHEMA_MIN_LENGTH:
      m_min_length.Set(value);
      break;
    case SCHEMA_MAX_PROPERTIES:
      m_max_properties.Set(value);
      break;
    case SCHEMA_MIN_PROPERTIES:
      m_min_properties.Set(value);
      break;
    default:
      {}
  }
}

template <typename T>
void SchemaParseContext::ProcessInt(SchemaErrorLogger *logger, T value) {
  if (!ValidTypeForKeyword(logger, m_keyword, TypeFromValue(value))) {
    return;
  }

  switch (m_keyword) {
    case SCHEMA_DEFAULT:
      m_default_value.reset(JsonValue::NewValue(value));
      return;
    case SCHEMA_MAXIMUM:
      m_maximum.reset(JsonValue::NewNumberValue(value));
      return;
    case SCHEMA_MINIMUM:
      m_minimum.reset(JsonValue::NewNumberValue(value));
      return;
    default:
      {}
  }

  if (positive<T, std::numeric_limits<T>::is_signed>()(value)) {
    ProcessPositiveInt(logger, static_cast<uint64_t>(value));
    return;
  }

  logger->Error() << KeywordToString(m_keyword) << " can't be negative";
}

bool SchemaParseContext::AddNumberConstraints(IntegerValidator *validator,
                                              SchemaErrorLogger *logger) {
  if (m_exclusive_maximum.IsSet() && !m_maximum.get()) {
    logger->Error() << "exclusiveMaximum requires maximum to be defined";
    return false;
  }

  if (m_maximum.get()) {
    if (m_exclusive_maximum.IsSet()) {
      validator->AddConstraint(new MaximumConstraint(
            m_maximum.release(), m_exclusive_maximum.Value()));
    } else {
      validator->AddConstraint(new MaximumConstraint(m_maximum.release()));
    }
  }

  if (m_exclusive_minimum.IsSet() && !m_minimum.get()) {
    logger->Error() << "exclusiveMinimum requires minimum to be defined";
    return false;
  }

  if (m_minimum.get()) {
    if (m_exclusive_minimum.IsSet()) {
      validator->AddConstraint(new MinimumConstraint(
            m_minimum.release(), m_exclusive_minimum.Value()));
    } else {
      validator->AddConstraint(new MinimumConstraint(m_minimum.release()));
    }
  }

  if (m_multiple_of.get()) {
    validator->AddConstraint(new MultipleOfConstraint(m_multiple_of.release()));
  }
  return true;
}

BaseValidator* SchemaParseContext::BuildArrayValidator(
    SchemaErrorLogger *logger) {
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

  unique_ptr<ArrayValidator::Items> items;
  unique_ptr<ArrayValidator::AdditionalItems> additional_items;

  // items
  if (m_items_single_context.get() && m_items_context_array.get()) {
    logger->Error() << "'items' is somehow both a schema and an array!";
    return NULL;
  } else if (m_items_single_context.get()) {
    // 8.2.3.1
    items.reset(new ArrayValidator::Items(
          m_items_single_context->GetValidator(logger)));
  } else if (m_items_context_array.get()) {
    // 8.2.3.2
    ValidatorInterface::ValidatorList item_validators;
    m_items_context_array->GetValidators(logger, &item_validators);
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

BaseValidator* SchemaParseContext::BuildObjectValidator(
    SchemaErrorLogger* logger) {
  ObjectValidator::Options options;
  if (m_max_properties.IsSet()) {
    options.max_properties = m_max_properties.Value();
  }

  if (m_min_properties.IsSet()) {
    options.min_properties = m_min_properties.Value();
  }

  if (m_required_items.get()) {
    set<string> required_properties;
    m_required_items->GetStringSet(&required_properties);
    options.SetRequiredProperties(required_properties);
  }

  if (m_additional_properties.IsSet()) {
    options.SetAdditionalProperties(m_additional_properties.Value());
  }

  ObjectValidator *object_validator = new ObjectValidator(options);

  if (m_additional_properties_context.get()) {
    object_validator->SetAdditionalValidator(
        m_additional_properties_context->GetValidator(logger));
  }

  if (m_properties_context.get()) {
    m_properties_context->AddPropertyValidators(object_validator, logger);
  }

  if (m_dependency_context.get()) {
    m_dependency_context->AddDependenciesToValidator(object_validator);
  }
  return object_validator;
}

BaseValidator* SchemaParseContext::BuildStringValidator(
    OLA_UNUSED SchemaErrorLogger *logger) {
  StringValidator::Options options;

  if (m_max_length.IsSet()) {
    options.max_length = m_max_length.Value();
  }

  if (m_min_length.IsSet()) {
    options.min_length = m_min_length.Value();
  }

  return new StringValidator(options);
}

/*
 * Verify the type is valid for the given keyword.
 * If the type isn't valid, an error is logged.
 * @returns false if the type isn't valid or if the keyword is SCHEMA_UNKNOWN.
 */
bool SchemaParseContext::ValidTypeForKeyword(SchemaErrorLogger *logger,
                                             SchemaKeyword keyword,
                                             JsonType type) {
  switch (keyword) {
    case SCHEMA_UNKNOWN:
      return false;
    case SCHEMA_ID:
      return CheckTypeAndLog(logger, keyword, type, JSON_STRING);
    case SCHEMA_SCHEMA:
      return CheckTypeAndLog(logger, keyword, type, JSON_STRING);
    case SCHEMA_REF:
      return CheckTypeAndLog(logger, keyword, type, JSON_STRING);
    case SCHEMA_TITLE:
      return CheckTypeAndLog(logger, keyword, type, JSON_STRING);
    case SCHEMA_DESCRIPTION:
      return CheckTypeAndLog(logger, keyword, type, JSON_STRING);
    case SCHEMA_DEFAULT:
      return true;
    case SCHEMA_MULTIPLEOF:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER, JSON_NUMBER);
    case SCHEMA_MAXIMUM:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER, JSON_NUMBER);
    case SCHEMA_EXCLUSIVE_MAXIMUM:
      return CheckTypeAndLog(logger, keyword, type, JSON_BOOLEAN);
    case SCHEMA_MINIMUM:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER, JSON_NUMBER);
    case SCHEMA_EXCLUSIVE_MINIMUM:
      return CheckTypeAndLog(logger, keyword, type, JSON_BOOLEAN);
    case SCHEMA_MAX_LENGTH:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER);
    case SCHEMA_MIN_LENGTH:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER);
    case SCHEMA_PATTERN:
      return CheckTypeAndLog(logger, keyword, type, JSON_STRING);
    case SCHEMA_ADDITIONAL_ITEMS:
      return CheckTypeAndLog(logger, keyword, type, JSON_BOOLEAN, JSON_OBJECT);
    case SCHEMA_ITEMS:
      return CheckTypeAndLog(logger, keyword, type, JSON_ARRAY, JSON_OBJECT);
    case SCHEMA_MAX_ITEMS:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER);
    case SCHEMA_MIN_ITEMS:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER);
    case SCHEMA_UNIQUE_ITEMS:
      return CheckTypeAndLog(logger, keyword, type, JSON_BOOLEAN);
    case SCHEMA_MAX_PROPERTIES:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER);
    case SCHEMA_MIN_PROPERTIES:
      return CheckTypeAndLog(logger, keyword, type, JSON_INTEGER);
    case SCHEMA_REQUIRED:
      return CheckTypeAndLog(logger, keyword, type, JSON_ARRAY);
    case SCHEMA_ADDITIONAL_PROPERTIES:
      return CheckTypeAndLog(logger, keyword, type, JSON_BOOLEAN, JSON_OBJECT);
    case SCHEMA_DEFINITIONS:
      return CheckTypeAndLog(logger, keyword, type, JSON_OBJECT);
    case SCHEMA_PROPERTIES:
      return CheckTypeAndLog(logger, keyword, type, JSON_OBJECT);
    case SCHEMA_PATTERN_PROPERTIES:
      return CheckTypeAndLog(logger, keyword, type, JSON_OBJECT);
    case SCHEMA_DEPENDENCIES:
      return CheckTypeAndLog(logger, keyword, type, JSON_OBJECT);
    case SCHEMA_ENUM:
      return CheckTypeAndLog(logger, keyword, type, JSON_ARRAY);
    case SCHEMA_TYPE:
      return CheckTypeAndLog(logger, keyword, type, JSON_STRING, JSON_ARRAY);
    case SCHEMA_ALL_OF:
      return CheckTypeAndLog(logger, keyword, type, JSON_ARRAY);
    case SCHEMA_ANY_OF:
      return CheckTypeAndLog(logger, keyword, type, JSON_ARRAY);
    case SCHEMA_ONE_OF:
      return CheckTypeAndLog(logger, keyword, type, JSON_ARRAY);
    case SCHEMA_NOT:
      return CheckTypeAndLog(logger, keyword, type, JSON_OBJECT);
    default:
      return false;
  }
}

bool SchemaParseContext::CheckTypeAndLog(
    SchemaErrorLogger *logger, SchemaKeyword keyword,
    JsonType type, JsonType expected_type) {
  if (type == expected_type) {
    return true;
  } else {
    logger->Error() << "Invalid type for " << KeywordToString(keyword)
        << ", got " << JsonTypeToString(type) << ", expected "
        << JsonTypeToString(expected_type);
    return false;
  }
}

bool SchemaParseContext::CheckTypeAndLog(
    SchemaErrorLogger *logger, SchemaKeyword keyword,
    JsonType type, JsonType expected_type1, JsonType expected_type2) {
  if (type == expected_type1 || type == expected_type2) {
    return true;
  } else {
    logger->Error() << "Invalid type for " << KeywordToString(keyword)
        << ", got " << JsonTypeToString(type) << ", expected "
        << JsonTypeToString(expected_type1) << " or "
        << JsonTypeToString(expected_type2);
    return false;
  }
}


// PropertiesParseContext
// Used for parsing an object with key : json schema pairs, within 'properties'
void PropertiesParseContext::AddPropertyValidators(
    ObjectValidator *object_validator,
    SchemaErrorLogger *logger) {
  SchemaMap::iterator iter = m_property_contexts.begin();
  for (; iter != m_property_contexts.end(); ++iter) {
    ValidatorInterface *validator = iter->second->GetValidator(logger);
    if (validator) {
      object_validator->AddValidator(iter->first, validator);
    }
  }
}

PropertiesParseContext::~PropertiesParseContext() {
  STLDeleteValues(&m_property_contexts);
}

SchemaParseContextInterface* PropertiesParseContext::OpenObject(
    SchemaErrorLogger *logger) {
  const string key = TakeKeyword();
  pair<SchemaMap::iterator, bool> r = m_property_contexts.insert(
      pair<string, SchemaParseContext*>(key, NULL));

  if (r.second) {
    r.first->second = new SchemaParseContext(m_schema_defs);
  } else {
    logger->Error() << "Duplicate key " << key;
  }
  return r.first->second;
}

// ArrayOfSchemaContext
// Used for parsing an array of JSON schema within 'items'
ArrayOfSchemaContext::~ArrayOfSchemaContext() {
  STLDeleteElements(&m_item_schemas);
}

void ArrayOfSchemaContext::GetValidators(
    SchemaErrorLogger *logger,
    ValidatorInterface::ValidatorList *validators) {
  ItemSchemas::iterator iter = m_item_schemas.begin();
  for (; iter != m_item_schemas.end(); ++iter) {
    validators->push_back((*iter)->GetValidator(logger));
  }
}

SchemaParseContextInterface* ArrayOfSchemaContext::OpenObject(
    OLA_UNUSED SchemaErrorLogger *logger) {
  m_item_schemas.push_back(new SchemaParseContext(m_schema_defs));
  return m_item_schemas.back();
}

// ArrayOfStringsContext
// Used for parsing an array of strings.
void ArrayOfStringsContext::GetStringSet(StringSet *items) {
  *items = m_items;
}

void ArrayOfStringsContext::String(SchemaErrorLogger *logger,
                                   const string &value) {
  if (!m_items.insert(value).second) {
    logger->Error() << value << " appeared more than once in the array";
  }
}

// JsonValueContext
// Used for parsing a default value.
JsonValueContext::JsonValueContext() : SchemaParseContextInterface() {
  m_parser.Begin();
}

const JsonValue* JsonValueContext::ClaimValue(SchemaErrorLogger *logger) {
  m_parser.End();
  const JsonValue *value = m_parser.ClaimRoot();
  if (!value) {
    logger->Error() << " is invalid: " << m_parser.GetError();
  }
  return value;
}

void JsonValueContext::String(SchemaErrorLogger *, const string &value) {
  m_parser.String(value);
}

void JsonValueContext::Number(SchemaErrorLogger *, uint32_t value) {
  m_parser.Number(value);
}

void JsonValueContext::Number(SchemaErrorLogger *, int32_t value) {
  m_parser.Number(value);
}

void JsonValueContext::Number(SchemaErrorLogger *, uint64_t value) {
  m_parser.Number(value);
}

void JsonValueContext::Number(SchemaErrorLogger *, int64_t value) {
  m_parser.Number(value);
}

void JsonValueContext::Number(SchemaErrorLogger *, double value) {
  m_parser.Number(value);
}

void JsonValueContext::Bool(SchemaErrorLogger *, bool value) {
  m_parser.Bool(value);
}

void JsonValueContext::Null(OLA_UNUSED SchemaErrorLogger *logger) {
  m_parser.Null();
}

SchemaParseContextInterface* JsonValueContext::OpenArray(
    SchemaErrorLogger *) {
  m_parser.OpenArray();
  return this;
}

void JsonValueContext::CloseArray(OLA_UNUSED SchemaErrorLogger *logger) {
  m_parser.CloseArray();
}

SchemaParseContextInterface* JsonValueContext::OpenObject(
    SchemaErrorLogger *) {
  m_parser.OpenObject();
  return this;
}

void JsonValueContext::ObjectKey(SchemaErrorLogger *, const string &key) {
  m_parser.ObjectKey(key);
}

void JsonValueContext::CloseObject(OLA_UNUSED SchemaErrorLogger *logger) {
  m_parser.CloseObject();
}

// ArrayOfJsonValuesContext
// Used for parsing a list of enums.
ArrayOfJsonValuesContext::~ArrayOfJsonValuesContext() {
  STLDeleteElements(&m_enums);
}

void ArrayOfJsonValuesContext::AddEnumsToValidator(BaseValidator *validator) {
  vector<const JsonValue*>::const_iterator iter = m_enums.begin();
  for (; iter != m_enums.end(); ++iter) {
    validator->AddEnumValue(*iter);
  }
  m_enums.clear();
}

void ArrayOfJsonValuesContext::String(SchemaErrorLogger *logger,
                                      const string &value) {
  CheckForDuplicateAndAdd(logger, JsonValue::NewValue(value));
}

void ArrayOfJsonValuesContext::Number(SchemaErrorLogger *logger,
                                     uint32_t value) {
  CheckForDuplicateAndAdd(logger, JsonValue::NewValue(value));
}

void ArrayOfJsonValuesContext::Number(SchemaErrorLogger *logger,
                                      int32_t value) {
  CheckForDuplicateAndAdd(logger, JsonValue::NewValue(value));
}

void ArrayOfJsonValuesContext::Number(SchemaErrorLogger *logger,
                                      uint64_t value) {
  CheckForDuplicateAndAdd(logger, JsonValue::NewValue(value));
}

void ArrayOfJsonValuesContext::Number(SchemaErrorLogger *logger,
                                      int64_t value) {
  CheckForDuplicateAndAdd(logger, JsonValue::NewValue(value));
}

void ArrayOfJsonValuesContext::Number(SchemaErrorLogger *logger,
                                      double value) {
  CheckForDuplicateAndAdd(logger, JsonValue::NewValue(value));
}

void ArrayOfJsonValuesContext::Bool(SchemaErrorLogger *logger, bool value) {
  CheckForDuplicateAndAdd(logger, JsonValue::NewValue(value));
}

void ArrayOfJsonValuesContext::Null(SchemaErrorLogger *logger) {
  CheckForDuplicateAndAdd(logger, new JsonNull());
}

SchemaParseContextInterface* ArrayOfJsonValuesContext::OpenArray(
    OLA_UNUSED SchemaErrorLogger *logger) {
  m_value_context.reset(new JsonValueContext());
  return m_value_context.get();
}

void ArrayOfJsonValuesContext::CloseArray(SchemaErrorLogger *logger) {
  CheckForDuplicateAndAdd(logger, m_value_context->ClaimValue(logger));
}

SchemaParseContextInterface* ArrayOfJsonValuesContext::OpenObject(
    OLA_UNUSED SchemaErrorLogger *logger) {
  m_value_context.reset(new JsonValueContext());
  return m_value_context.get();
}

void ArrayOfJsonValuesContext::CloseObject(SchemaErrorLogger *logger) {
  CheckForDuplicateAndAdd(logger, m_value_context->ClaimValue(logger));
}

void ArrayOfJsonValuesContext::CheckForDuplicateAndAdd(
    SchemaErrorLogger *logger,
    const JsonValue *value) {
  vector<const JsonValue*>::const_iterator iter = m_enums.begin();
  for (; iter != m_enums.end(); ++iter) {
    if (**iter == *value) {
      logger->Error() << "Duplicate entries in enum array: " << value;
      delete value;
      return;
    }
  }
  m_enums.push_back(value);
}

// ArrayOfJsonValuesContext
// Used for parsing a list of enums.
DependencyParseContext::~DependencyParseContext() {
  STLDeleteValues(&m_schema_dependencies);
}

void DependencyParseContext::AddDependenciesToValidator(
    ObjectValidator *validator) {

  PropertyDependencies::const_iterator prop_iter =
    m_property_dependencies.begin();
  for (; prop_iter != m_property_dependencies.end(); ++prop_iter) {
    validator->AddPropertyDependency(prop_iter->first, prop_iter->second);
  }

  // Check Schema Dependencies
  SchemaDependencies::const_iterator schema_iter =
    m_schema_dependencies.begin();
  for (; schema_iter != m_schema_dependencies.end(); ++schema_iter) {
    validator->AddSchemaDependency(schema_iter->first, schema_iter->second);
  }
  m_schema_dependencies.clear();
}

SchemaParseContextInterface* DependencyParseContext::OpenArray(
    OLA_UNUSED SchemaErrorLogger *logger) {
  m_property_context.reset(new ArrayOfStringsContext());
  return m_property_context.get();
}

void DependencyParseContext::CloseArray(SchemaErrorLogger *logger) {
  StringSet &properties = m_property_dependencies[Keyword()];
  m_property_context->GetStringSet(&properties);

  if (properties.empty()) {
    logger->Error()
      << " property dependency lists must contain at least one item";
  }
  m_property_context.reset();
}

SchemaParseContextInterface* DependencyParseContext::OpenObject(
    OLA_UNUSED SchemaErrorLogger *logger) {
  m_schema_context.reset(new SchemaParseContext(m_schema_defs));
  return m_schema_context.get();
}

void DependencyParseContext::CloseObject(SchemaErrorLogger *logger) {
  STLReplaceAndDelete(&m_schema_dependencies, Keyword(),
                      m_schema_context->GetValidator(logger));
  m_schema_context.reset();
}
}  // namespace web
}  // namespace ola
