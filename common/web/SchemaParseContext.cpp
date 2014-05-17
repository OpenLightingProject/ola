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

using std::auto_ptr;
using std::make_pair;
using std::pair;
using std::set;
using std::string;
using std::vector;

// DefinitionsParseContext
// Used for parsing an object with key : json schema pairs, within 'definitions'
SchemaParseContextInterface* DefinitionsParseContext::OpenObject(
    SchemaErrorLogger *logger) {
  m_current_schema.reset(new SchemaParseContext(m_schema_defs));
  return m_current_schema.get();
  (void) logger;
}

void DefinitionsParseContext::CloseObject(SchemaErrorLogger *logger) {
  if (!(HasKeyword() && m_current_schema.get())) {
    OLA_WARN << "Bad call to DefinitionsParseContext::CloseObject";
    return;
  }

  string key = TakeKeyword();

  ValidatorInterface *schema = m_current_schema->GetValidator(logger);
  m_schema_defs->Add(TakeKeyword(), schema);
  m_current_schema.release();
}

// SchemaParseContext
// Used for parsing an object that describes a JSON schema.
SchemaParseContext::~SchemaParseContext() {
  // STLDeleteElements(&m_number_constraints);
}

ValidatorInterface* SchemaParseContext::GetValidator(
    SchemaErrorLogger *logger) {
  if (m_ref_schema.IsSet()) {
    return new ReferenceValidator(m_schema_defs, m_ref_schema.Value());
  }

  ValidatorInterface *validator = NULL;
  switch (m_type) {
    case JSON_UNDEFINED:
      validator = new WildcardValidator();
      break;
    case JSON_ARRAY:
      validator = BuildArrayValidator(logger);
      break;
    case JSON_BOOLEAN:
      validator = new BoolValidator();
      break;
    case JSON_INTEGER:
      validator = new IntegerValidator();
      break;
    case JSON_NULL:
      validator = new NullValidator();
      break;
    case JSON_NUMBER:
      validator = new NumberValidator();
      break;
    case JSON_OBJECT:
      validator = BuildObjectValidator(logger);
      break;
    case JSON_STRING:
      validator = BuildStringValidator(logger);
      break;
    default:
      logger->Error() << "Unknown type: " << JsonTypeToString(m_type);
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
  if (m_default_value.get()) {
    validator->SetDefaultValue(m_default_value.release());
  }

  return validator;
}


void SchemaParseContext::ObjectKey(SchemaErrorLogger *logger,
                                   const string &keyword) {
  m_keyword = LookupKeyword(keyword);
  (void) logger;
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
      m_default_value.reset(new JsonStringValue(value));
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
      m_default_value.reset(new JsonDoubleValue(value));
      break;
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
      m_default_value.reset(new JsonBoolValue(value));
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
    default:
      {}
  }
}

void SchemaParseContext::Null(SchemaErrorLogger *logger) {
  ValidTypeForKeyword(logger, m_keyword, JSON_NULL);

  switch (m_keyword) {
    case SCHEMA_DEFAULT:
      m_default_value.reset(new JsonNullValue());
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
      m_default_value_context.reset(new DefaultValueParseContext());
      m_default_value_context->OpenArray(logger);
      return m_default_value_context.get();
    case SCHEMA_ITEMS:
      m_items_context_array.reset(new ArrayItemsParseContext(m_schema_defs));
      return m_items_context_array.get();
    case SCHEMA_REQUIRED:
      m_required_items.reset(new RequiredPropertiesParseContext());
      return m_required_items.get();
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
}

SchemaParseContextInterface* SchemaParseContext::OpenObject(
    SchemaErrorLogger *logger) {
  if (!ValidTypeForKeyword(logger, m_keyword, JSON_OBJECT)) {
    return NULL;
  }

  switch (m_keyword) {
    case SCHEMA_DEFAULT:
      m_default_value_context.reset(new DefaultValueParseContext());
      m_default_value_context->OpenObject(logger);
      return m_default_value_context.get();
    case SCHEMA_DEFINITIONS:
      if (m_definitions_context.get()) {
        logger->Error() << "Duplicate key 'definitions'";
        return NULL;
      }
      m_definitions_context.reset(new DefinitionsParseContext(m_schema_defs));
      return m_definitions_context.get();
    case SCHEMA_PROPERTIES:
      if (m_properties_context.get()) {
        logger->Error() << "Duplicate key 'properties'";
        return NULL;
      }
      m_properties_context.reset(new PropertiesParseContext(m_schema_defs));
      return m_properties_context.get();
    case SCHEMA_ITEMS:
      m_items_single_context.reset(new SchemaParseContext(m_schema_defs));
      return m_items_single_context.get();
    case SCHEMA_ADDITIONAL_ITEMS:
      m_additional_items_context.reset(new SchemaParseContext(m_schema_defs));
      return m_additional_items_context.get();
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

void SchemaParseContext::ProcessPositiveInt(SchemaErrorLogger *logger,
                                            uint64_t value) {
  switch (m_keyword) {
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
  (void) logger;
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
    default:
      {}
  }

  if (positive<T, std::numeric_limits<T>::is_signed>()(value)) {
    ProcessPositiveInt(logger, static_cast<uint64_t>(value));
    return;
  }

  logger->Error() << KeywordToString(m_keyword) << " can't be negative";
}

ValidatorInterface* SchemaParseContext::BuildArrayValidator(
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

  auto_ptr<ArrayValidator::Items> items;
  auto_ptr<ArrayValidator::AdditionalItems> additional_items;

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
    m_required_items->GetRequiredItems(&required_properties);
    options.SetRequiredProperties(required_properties);
  }

  ObjectValidator *object_validator = new ObjectValidator(options);

  if (m_properties_context.get()) {
    m_properties_context->AddPropertyValidators(object_validator, logger);
  }
  return object_validator;
}

ValidatorInterface* SchemaParseContext::BuildStringValidator(
    SchemaErrorLogger *logger) {
  StringValidator::Options options;

  if (m_max_length.IsSet()) {
    options.max_length = m_max_length.Value();
  }

  if (m_min_length.IsSet()) {
    options.min_length = m_min_length.Value();
  }

  return new StringValidator(options);
  (void) logger;
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

void PropertiesParseContext::String(SchemaErrorLogger *logger,
                                    const string &value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(SchemaErrorLogger *logger, uint32_t value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(SchemaErrorLogger *logger, int32_t value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(SchemaErrorLogger *logger, uint64_t value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(SchemaErrorLogger *logger, int64_t value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Number(SchemaErrorLogger *logger, double value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Bool(SchemaErrorLogger *logger, bool value) {
  (void) logger;
  (void) value;
}

void PropertiesParseContext::Null(SchemaErrorLogger *logger) {
  (void) logger;
}

SchemaParseContextInterface* PropertiesParseContext::OpenArray(
    SchemaErrorLogger *logger) {
  (void) logger;
  return NULL;
}

void PropertiesParseContext::CloseArray(SchemaErrorLogger *logger) {
  (void) logger;
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

void PropertiesParseContext::CloseObject(SchemaErrorLogger *logger) {
  (void) logger;
}

// ArrayItemsParseContext
// Used for parsing an array of JSON schema within 'items'
ArrayItemsParseContext::~ArrayItemsParseContext() {
  STLDeleteElements(&m_item_schemas);
}

void ArrayItemsParseContext::AddValidators(
    SchemaErrorLogger *logger,
    vector<ValidatorInterface*> *validators) {
  ItemSchemas::iterator iter = m_item_schemas.begin();
  for (; iter != m_item_schemas.end(); ++iter) {
    validators->push_back((*iter)->GetValidator(logger));
  }
}

void ArrayItemsParseContext::String(SchemaErrorLogger *logger,
                                    const string &value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(SchemaErrorLogger *logger, uint32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(SchemaErrorLogger *logger, int32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(SchemaErrorLogger *logger, uint64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(SchemaErrorLogger *logger, int64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Number(SchemaErrorLogger *logger, double value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Bool(SchemaErrorLogger *logger, bool value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void ArrayItemsParseContext::Null(SchemaErrorLogger *logger) {
  ReportErrorForType(logger, JSON_NULL);
}

SchemaParseContextInterface* ArrayItemsParseContext::OpenArray(
    SchemaErrorLogger *logger) {
  ReportErrorForType(logger, JSON_ARRAY);
  return NULL;
}

SchemaParseContextInterface* ArrayItemsParseContext::OpenObject(
    SchemaErrorLogger *logger) {
  m_item_schemas.push_back(new SchemaParseContext(m_schema_defs));
  return m_item_schemas.back();
  (void) logger;
}

void ArrayItemsParseContext::ReportErrorForType(SchemaErrorLogger *logger,
                                                JsonType type) {
  logger->Error() << "Invalid type '" << JsonTypeToString(type)
      << "' in 'items', elements must be a valid JSON schema";
}

// RequiredPropertiesParseContext
// Used for parsing an array of strings within 'required'
void RequiredPropertiesParseContext::GetRequiredItems(
    RequiredItems *required_items) {
  *required_items = m_required_items;
}

void RequiredPropertiesParseContext::String(SchemaErrorLogger *logger,
                                            const string &value) {
  if (!m_required_items.insert(value).second) {
    logger->Error() << value << " appeared more than once in 'required'";
  }
}

void RequiredPropertiesParseContext::Number(SchemaErrorLogger *logger,
                                            uint32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Number(SchemaErrorLogger *logger,
                                            int32_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Number(SchemaErrorLogger *logger,
                                            uint64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Number(SchemaErrorLogger *logger,
                                            int64_t value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Number(SchemaErrorLogger *logger,
                                            double value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Bool(SchemaErrorLogger *logger,
                                          bool value) {
  ReportErrorForType(logger, TypeFromValue(value));
}

void RequiredPropertiesParseContext::Null(SchemaErrorLogger *logger) {
  ReportErrorForType(logger, JSON_NULL);
}

SchemaParseContextInterface* RequiredPropertiesParseContext::OpenArray(
    SchemaErrorLogger *logger) {
  ReportErrorForType(logger, JSON_ARRAY);
  return NULL;
}

SchemaParseContextInterface* RequiredPropertiesParseContext::OpenObject(
    SchemaErrorLogger *logger) {
  ReportErrorForType(logger, JSON_OBJECT);
  return NULL;
}

void RequiredPropertiesParseContext::ReportErrorForType(
    SchemaErrorLogger *logger,
    JsonType type) {
  logger->Error() << "Invalid type '" << JsonTypeToString(type)
                  << "' in 'required', elements must be strings";
}

// DefaultValueParseContext
// Used for parsing a default value.
DefaultValueParseContext::DefaultValueParseContext()
    : SchemaParseContextInterface() {
  m_parser.Begin();
}

const JsonValue* DefaultValueParseContext::ClaimValue(
    SchemaErrorLogger *logger) {
  m_parser.End();
  const JsonValue *value = m_parser.ClaimRoot();
  if (!value) {
    logger->Error() << " is invalid: " << m_parser.GetError();
  }
  return value;
}

void DefaultValueParseContext::String(SchemaErrorLogger *,
                                      const string &value) {
  m_parser.String(value);
}

void DefaultValueParseContext::Number(SchemaErrorLogger *,
                                      uint32_t value) {
  m_parser.Number(value);
}

void DefaultValueParseContext::Number(SchemaErrorLogger *,
                                      int32_t value) {
  m_parser.Number(value);
}

void DefaultValueParseContext::Number(SchemaErrorLogger *,
                                      uint64_t value) {
  m_parser.Number(value);
}

void DefaultValueParseContext::Number(SchemaErrorLogger *,
                                      int64_t value) {
  m_parser.Number(value);
}

void DefaultValueParseContext::Number(SchemaErrorLogger *, double value) {
  m_parser.Number(value);
}

void DefaultValueParseContext::Bool(SchemaErrorLogger *, bool value) {
  m_parser.Bool(value);
}

void DefaultValueParseContext::Null(SchemaErrorLogger *logger) {
  m_parser.Null();
  (void) logger;
}

SchemaParseContextInterface* DefaultValueParseContext::OpenArray(
    SchemaErrorLogger *) {
  m_parser.OpenArray();
  return this;
}

void DefaultValueParseContext::CloseArray(SchemaErrorLogger *logger) {
  m_parser.CloseArray();
  (void) logger;
}

SchemaParseContextInterface* DefaultValueParseContext::OpenObject(
    SchemaErrorLogger *) {
  m_parser.OpenObject();
  return this;
}

void DefaultValueParseContext::ObjectKey(SchemaErrorLogger *,
                                         const string &key) {
  m_parser.ObjectKey(key);
}

void DefaultValueParseContext::CloseObject(SchemaErrorLogger *logger) {
  m_parser.CloseObject();
  (void) logger;
}
}  // namespace web
}  // namespace ola
