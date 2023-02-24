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
 * JsonSchema.cpp
 * Json Schema validation.
 * See http://www.json-schema.org/
 * Copyright (C) 2014 Simon Newton
 */

#include <math.h>
#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "common/web/SchemaParser.h"
#include "ola/stl/STLUtils.h"
#include "ola/web/JsonLexer.h"
#include "ola/web/JsonSchema.h"
#include "ola/web/JsonTypes.h"

namespace ola {
namespace web {

using std::auto_ptr;
using std::set;
using std::string;
using std::vector;

// BaseValidator
// -----------------------------------------------------------------------------
BaseValidator::~BaseValidator() {
  STLDeleteElements(&m_enums);
}

JsonObject* BaseValidator::GetSchema() const {
  JsonObject *schema = new JsonObject();
  if (!m_schema.empty()) {
    schema->Add("$schema", m_schema);
  }
  if (!m_id.empty()) {
    schema->Add("id", m_id);
  }
  if (!m_title.empty()) {
    schema->Add("title", m_title);
  }
  if (!m_description.empty()) {
    schema->Add("description", m_description);
  }
  const string type = JsonTypeToString(m_type);
  if (!type.empty()) {
    schema->Add("type", type);
  }

  if (m_default_value.get()) {
    schema->AddValue("default", m_default_value.get()->Clone());
  }

  if (!m_enums.empty()) {
    JsonArray *enum_array = schema->AddArray("enum");
    vector<const JsonValue*>::const_iterator iter = m_enums.begin();
    for (; iter != m_enums.end(); ++iter) {
      enum_array->AppendValue((*iter)->Clone());
    }
  }
  ExtendSchema(schema);
  return schema;
}

void BaseValidator::SetSchema(const string &schema) {
  m_schema = schema;
}

void BaseValidator::SetId(const string &id) {
  m_id = id;
}

void BaseValidator::SetTitle(const string &title) {
  m_title = title;
}

void BaseValidator::SetDescription(const string &description) {
  m_description = description;
}

void BaseValidator::SetDefaultValue(const JsonValue *value) {
  m_default_value.reset(value);
}

const JsonValue *BaseValidator::GetDefaultValue() const {
  return m_default_value.get();
}

void BaseValidator::AddEnumValue(const JsonValue *value) {
  m_enums.push_back(value);
}

bool BaseValidator::CheckEnums(const JsonValue &value) {
  if (m_enums.empty()) {
    return true;
  }
  vector<const JsonValue*>::const_iterator iter = m_enums.begin();
  for (; iter != m_enums.end(); ++iter) {
    if (**iter == value) {
      return true;
    }
  }
  return false;
}

// ReferenceValidator
// -----------------------------------------------------------------------------
ReferenceValidator::ReferenceValidator(const SchemaDefinitions *definitions,
                                       const string &schema)
    : m_definitions(definitions),
      m_schema(schema),
      m_validator(NULL) {
}

bool ReferenceValidator::IsValid() const {
  return m_validator ? m_validator->IsValid() : false;
}

void ReferenceValidator::Visit(const JsonString &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonBool &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonNull &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonRawValue &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonObject &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonArray &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonUInt &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonUInt64 &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonInt &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonInt64 &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonDouble &value) {
  Validate(value);
}

JsonObject* ReferenceValidator::GetSchema() const {
  JsonObject *schema = new JsonObject();
  schema->Add("$ref", m_schema);
  return schema;
}

void ReferenceValidator::SetSchema(const std::string&) {}
void ReferenceValidator::SetId(const std::string &) {}
void ReferenceValidator::SetTitle(const std::string &) {}
void ReferenceValidator::SetDescription(const std::string &) {}
void ReferenceValidator::SetDefaultValue(const JsonValue *) {}
const JsonValue *ReferenceValidator::GetDefaultValue() const { return NULL; }

template <typename T>
void ReferenceValidator::Validate(const T &value) {
  if (!m_validator) {
    m_validator = m_definitions->Lookup(m_schema);
  }

  if (m_validator) {
    value.Accept(m_validator);
  }
}

// StringValidator
// -----------------------------------------------------------------------------
void StringValidator::Visit(const JsonString &str) {
  const std::string& value = str.Value();
  size_t str_size = value.size();
  if (str_size < m_options.min_length) {
    m_is_valid = false;
    return;
  }

  if (m_options.max_length >= 0 &&
      str_size > static_cast<size_t>(m_options.max_length)) {
    m_is_valid = false;
    return;
  }

  m_is_valid = CheckEnums(str);
}

void StringValidator::ExtendSchema(JsonObject *schema) const {
  if (m_options.min_length > 0) {
    schema->Add("minLength", m_options.min_length);
  }

  if (m_options.max_length >= 0) {
    schema->Add("maxLength", m_options.max_length);
  }

  // TODO(simon): Add pattern here?
  // TODO(simon): Add format here?
}

// IntegerValidator
// -----------------------------------------------------------------------------
IntegerValidator::~IntegerValidator() {
  STLDeleteElements(&m_constraints);
}

void IntegerValidator::AddConstraint(NumberConstraint *constraint) {
  m_constraints.push_back(constraint);
}

void IntegerValidator::Visit(const JsonUInt &value) {
  CheckValue(value);
}

void IntegerValidator::Visit(const JsonInt &value) {
  CheckValue(value);
}

void IntegerValidator::Visit(const JsonUInt64 &value) {
  CheckValue(value);
}

void IntegerValidator::Visit(const JsonInt64 &value) {
  CheckValue(value);
}

void IntegerValidator::Visit(const JsonDouble &value) {
  BaseValidator::Visit(value);
}

void IntegerValidator::ExtendSchema(JsonObject *schema) const {
  vector<NumberConstraint*>::const_iterator iter = m_constraints.begin();
  for (; iter != m_constraints.end(); ++iter) {
    (*iter)->ExtendSchema(schema);
  }
}

void IntegerValidator::CheckValue(const JsonNumber &value) {
  vector<NumberConstraint*>::const_iterator iter = m_constraints.begin();
  for (; iter != m_constraints.end(); ++iter) {
    if (!(*iter)->IsValid(value)) {
      m_is_valid = false;
      return;
    }
  }
  m_is_valid = CheckEnums(value);
}

void NumberValidator::Visit(const JsonDouble &value) {
  CheckValue(value);
}

// ObjectValidator
// -----------------------------------------------------------------------------
ObjectValidator::ObjectValidator(const Options &options)
    : BaseValidator(JSON_OBJECT),
      m_options(options) {
}

ObjectValidator::~ObjectValidator() {
  STLDeleteValues(&m_property_validators);
  STLDeleteValues(&m_schema_dependencies);
}

void ObjectValidator::AddValidator(const std::string &property,
                                   ValidatorInterface *validator) {
  STLReplaceAndDelete(&m_property_validators, property, validator);
}

void ObjectValidator::SetAdditionalValidator(ValidatorInterface *validator) {
  m_additional_property_validator.reset(validator);
}

void ObjectValidator::AddSchemaDependency(const string &property,
                                          ValidatorInterface *validator) {
  STLReplaceAndDelete(&m_schema_dependencies, property, validator);
}

void ObjectValidator::AddPropertyDependency(const string &property,
                                            const StringSet &properties) {
  m_property_dependencies[property] = properties;
}

void ObjectValidator::Visit(const JsonObject &obj) {
  m_is_valid = true;

  if (obj.Size() < m_options.min_properties) {
    m_is_valid = false;
    return;
  }

  if (m_options.max_properties > 0 &&
      obj.Size() > static_cast<size_t>(m_options.max_properties)) {
    m_is_valid = false;
    return;
  }

  m_seen_properties.clear();
  obj.VisitProperties(this);

  StringSet missing_properties;
  std::set_difference(m_options.required_properties.begin(),
                      m_options.required_properties.end(),
                      m_seen_properties.begin(),
                      m_seen_properties.end(),
                      std::inserter(missing_properties,
                                    missing_properties.end()));
  if (!missing_properties.empty()) {
    m_is_valid = false;
  }

  // Check PropertyDependencies
  PropertyDependencies::const_iterator prop_iter =
    m_property_dependencies.begin();
  for (; prop_iter != m_property_dependencies.end() && m_is_valid;
       ++prop_iter) {
    if (!STLContains(m_seen_properties, prop_iter->first)) {
      continue;
    }

    StringSet::const_iterator iter = prop_iter->second.begin();
    for (; iter != prop_iter->second.end(); ++iter) {
      if (!STLContains(m_seen_properties, *iter)) {
        m_is_valid = false;
        break;
      }
    }
  }

  // Check Schema Dependencies
  SchemaDependencies::const_iterator schema_iter =
    m_schema_dependencies.begin();
  for (; schema_iter != m_schema_dependencies.end() && m_is_valid;
       ++schema_iter) {
    if (STLContains(m_seen_properties, schema_iter->first)) {
      obj.Accept(schema_iter->second);
      if (!schema_iter->second->IsValid()) {
        m_is_valid = false;
        break;
      }
    }
  }
}

void ObjectValidator::VisitProperty(const std::string &property,
                                    const JsonValue &value) {
  m_seen_properties.insert(property);

  // The algorithm is described in section 8.3.3
  ValidatorInterface *validator = STLFindOrNull(
      m_property_validators, property);

  // patternProperties would be added here if supported

  if (!validator) {
    // try the additional validator
    validator = m_additional_property_validator.get();
  }

  if (validator) {
    value.Accept(validator);
    m_is_valid &= validator->IsValid();
  } else {
    // No validator found
    if (m_options.has_allow_additional_properties &&
        !m_options.allow_additional_properties) {
      m_is_valid &= false;
    }
  }
}

void ObjectValidator::ExtendSchema(JsonObject *schema) const {
  if (m_options.min_properties > 0) {
    schema->Add("minProperties", m_options.min_properties);
  }

  if (m_options.max_properties >= 0) {
    schema->Add("maxProperties", m_options.max_properties);
  }

  if (m_options.has_required_properties) {
    JsonArray *required_properties = schema->AddArray("required");
    StringSet::const_iterator iter = m_options.required_properties.begin();
    for (; iter != m_options.required_properties.end(); ++iter) {
      required_properties->Append(*iter);
    }
  }

  if (!m_property_validators.empty()) {
    JsonObject *properties = schema->AddObject("properties");
    PropertyValidators::const_iterator iter = m_property_validators.begin();
    for (; iter != m_property_validators.end(); iter++) {
      JsonObject *child_schema = iter->second->GetSchema();
      properties->AddValue(iter->first, child_schema);
    }
  }

  if (m_options.has_allow_additional_properties) {
    schema->Add("additionalProperties", m_options.allow_additional_properties);
  } else if (m_additional_property_validator.get()) {
    schema->AddValue("additionalProperties",
                     m_additional_property_validator->GetSchema());
  }

  if (!(m_property_dependencies.empty() && m_schema_dependencies.empty())) {
    JsonObject *dependencies = schema->AddObject("dependencies");
    PropertyDependencies::const_iterator prop_iter =
      m_property_dependencies.begin();
    for (; prop_iter != m_property_dependencies.end(); ++prop_iter) {
      JsonArray *properties = dependencies->AddArray(prop_iter->first);
      StringSet::const_iterator iter = prop_iter->second.begin();
      for (; iter != prop_iter->second.end(); ++iter) {
        properties->Append(*iter);
      }
    }

    SchemaDependencies::const_iterator schema_iter =
      m_schema_dependencies.begin();
    for (; schema_iter != m_schema_dependencies.end(); ++schema_iter) {
      dependencies->AddValue(schema_iter->first,
          schema_iter->second->GetSchema());
    }
  }
}

// ArrayValidator
// -----------------------------------------------------------------------------
ArrayValidator::ArrayValidator(Items *items, AdditionalItems *additional_items,
                               const Options &options)
  : BaseValidator(JSON_ARRAY),
    m_items(items),
    m_additional_items(additional_items),
    m_options(options),
    m_wildcard_validator(new WildcardValidator()) {
}

ArrayValidator::~ArrayValidator() {}

// items: array or schema (object)
// additionalItems : schema (object) or bool
//
// items = object
// items = array, additional = bool
// items = array, additional = schema
void ArrayValidator::Visit(const JsonArray &array) {
  if (array.Size() < m_options.min_items) {
    m_is_valid = false;
    return;
  }

  if (m_options.max_items > 0 &&
      array.Size() > static_cast<size_t>(m_options.max_items)) {
    m_is_valid = false;
    return;
  }


  auto_ptr<ArrayElementValidator> element_validator(
      ConstructElementValidator());

  for (unsigned int i = 0; i < array.Size(); i++) {
    array.ElementAt(i)->Accept(element_validator.get());
    if (!element_validator->IsValid()) {
      break;
    }
  }
  m_is_valid = element_validator->IsValid();
  if (!m_is_valid) {
    return;
  }

  if (m_options.unique_items) {
    for (unsigned int i = 0; i < array.Size(); i++) {
      for (unsigned int j = 0; j < i; j++) {
        if (*(array.ElementAt(i)) == *(array.ElementAt(j))) {
          m_is_valid = false;
          return;
        }
      }
    }
  }
}

void ArrayValidator::ExtendSchema(JsonObject *schema) const {
  if (m_options.min_items > 0) {
    schema->Add("minItems", m_options.min_items);
  }

  if (m_options.max_items >= 0) {
    schema->Add("maxItems", m_options.max_items);
  }

  if (m_options.unique_items) {
    schema->Add("uniqueItems", m_options.unique_items);
  }

  if (m_items.get()) {
    // items exists
    if (m_items->Validator()) {
      // items is an object
      JsonObject *child_schema = m_items->Validator()->GetSchema();
      schema->AddValue("items", child_schema);
    } else {
      // items is an array
      const ValidatorList &validators = m_items->Validators();
      JsonArray *items = schema->AddArray("items");
      ValidatorList::const_iterator iter = validators.begin();
      for (; iter != validators.end(); iter++) {
        JsonObject *child_schema = (*iter)->GetSchema();
        items->Append(child_schema);
      }
    }
  }

  if (m_additional_items.get()) {
    // additionalItems exists
    if (m_additional_items->Validator()) {
      // additionalItems is an object
      JsonObject *child_schema = m_additional_items->Validator()->GetSchema();
      schema->AddValue("additionalItems", child_schema);
    } else {
      // additionalItems is a bool
      schema->Add("additionalItems", m_additional_items->AllowAdditional());
    }
  }
}

// TODO(simon): do this once at construction, rather than on every visit?
ArrayValidator::ArrayElementValidator*
    ArrayValidator::ConstructElementValidator() const {
  if (m_items.get()) {
    if (m_items->Validator()) {
      // 8.2.3.1, items is an object.
      ValidatorList empty_validators;
      return new ArrayElementValidator(empty_validators, m_items->Validator());
    } else {
      // 8.2.3.3, items is an array.
      const ValidatorList &validators = m_items->Validators();
      ValidatorInterface *default_validator = NULL;

      // Check to see if additionalItems it defined.
      if (m_additional_items.get()) {
        if (m_additional_items->Validator()) {
          // additionalItems is an object
          default_validator = m_additional_items->Validator();
        } else if (m_additional_items->AllowAdditional()) {
          // additionalItems is a bool, and true
          default_validator = m_wildcard_validator.get();
        }
      } else {
        // additionalItems not provided, so it defaults to the empty schema
        // (wildcard).
        default_validator = m_wildcard_validator.get();
      }
      return new ArrayElementValidator(validators, default_validator);
    }
  } else {
    // no items, therefore it defaults to the empty (wildcard) schema.
    ValidatorList empty_validators;
    return new ArrayElementValidator(
        empty_validators, m_wildcard_validator.get());
  }
}

// ArrayValidator::ArrayElementValidator
// -----------------------------------------------------------------------------
ArrayValidator::ArrayElementValidator::ArrayElementValidator(
    const ValidatorList &validators,
    ValidatorInterface *default_validator)
    : BaseValidator(JSON_UNDEFINED),
      m_item_validators(validators.begin(), validators.end()),
      m_default_validator(default_validator) {
}

void ArrayValidator::ArrayElementValidator::Visit(
    const JsonString &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonBool &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonNull &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonRawValue &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonObject &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonArray &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonUInt &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(
    const JsonUInt64 &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonInt &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonInt64 &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(
    const JsonDouble &value) {
  ValidateItem(value);
}

template <typename T>
void ArrayValidator::ArrayElementValidator::ValidateItem(const T &item) {
  ValidatorInterface *validator = NULL;

  if (!m_item_validators.empty()) {
    validator = m_item_validators.front();
    m_item_validators.pop_front();
  } else if (!m_default_validator) {
    // additional items aren't allowed
    m_is_valid = false;
    return;
  } else {
    validator = m_default_validator;
  }
  item.Accept(validator);
  m_is_valid = validator->IsValid();
}

// ConjunctionValidator
// -----------------------------------------------------------------------------
ConjunctionValidator::ConjunctionValidator(const string &keyword,
                                           ValidatorList *validators)
    : BaseValidator(JSON_UNDEFINED),
      m_keyword(keyword),
      m_validators(*validators) {
  validators->clear();
}

ConjunctionValidator::~ConjunctionValidator() {
  STLDeleteElements(&m_validators);
}

void ConjunctionValidator::ExtendSchema(JsonObject *schema) const {
  JsonArray *items = schema->AddArray(m_keyword);
  ValidatorList::const_iterator iter = m_validators.begin();
  for (; iter != m_validators.end(); ++iter) {
    JsonObject *child_schema = (*iter)->GetSchema();
    items->Append(child_schema);
  }
}

// AllOfValidator
// -----------------------------------------------------------------------------
void AllOfValidator::Validate(const JsonValue &value) {
  ValidatorList::iterator iter = m_validators.begin();
  for (; iter != m_validators.end(); ++iter) {
    value.Accept(*iter);
    if (!(*iter)->IsValid()) {
      m_is_valid = false;
      return;
    }
  }
  m_is_valid = true;
}

// AnyOfValidator
// -----------------------------------------------------------------------------
void AnyOfValidator::Validate(const JsonValue &value) {
  ValidatorList::iterator iter = m_validators.begin();
  for (; iter != m_validators.end(); ++iter) {
    value.Accept(*iter);
    if ((*iter)->IsValid()) {
      m_is_valid = true;
      return;
    }
  }
  m_is_valid = false;
}

// OneOfValidator
// -----------------------------------------------------------------------------
void OneOfValidator::Validate(const JsonValue &value) {
  bool matched = false;
  ValidatorList::iterator iter = m_validators.begin();
  for (; iter != m_validators.end(); ++iter) {
    value.Accept(*iter);
    if ((*iter)->IsValid()) {
      if (matched) {
        m_is_valid = false;
        return;
      } else {
        matched = true;
      }
    }
  }
  m_is_valid = matched;
}

// NotValidator
// -----------------------------------------------------------------------------
void NotValidator::Validate(const JsonValue &value) {
  value.Accept(m_validator.get());
  m_is_valid = !m_validator->IsValid();
}

void NotValidator::ExtendSchema(JsonObject *schema) const {
  JsonObject *child_schema = m_validator->GetSchema();
  schema->AddValue("not", child_schema);
}

// SchemaDefinitions
// -----------------------------------------------------------------------------
SchemaDefinitions::~SchemaDefinitions() {
  STLDeleteValues(&m_validators);
}

void SchemaDefinitions::Add(const string &schema_name,
                           ValidatorInterface *validator) {
  STLReplaceAndDelete(&m_validators, schema_name, validator);
}

ValidatorInterface *SchemaDefinitions::Lookup(const string &schema_name) const {
  return STLFindOrNull(m_validators, schema_name);
}

void SchemaDefinitions::AddToJsonObject(JsonObject *json) const {
  SchemaMap::const_iterator iter = m_validators.begin();
  for (; iter != m_validators.end(); ++iter) {
    JsonObject *schema = iter->second->GetSchema();
    json->AddValue(iter->first, schema);
  }
}

// JsonSchema
// -----------------------------------------------------------------------------
JsonSchema::JsonSchema(const std::string &schema_url,
                       ValidatorInterface *root_validator,
                       SchemaDefinitions *schema_defs)
    : m_schema_uri(schema_url),
      m_root_validator(root_validator),
      m_schema_defs(schema_defs) {
}

string JsonSchema::SchemaURI() const {
  return m_schema_uri;
}

bool JsonSchema::IsValid(const JsonValue &value) {
  value.Accept(m_root_validator.get());
  return m_root_validator->IsValid();
}

const JsonObject* JsonSchema::AsJson() const {
  JsonObject *json =  m_root_validator->GetSchema();
  if (json && m_schema_defs->HasDefinitions()) {
    JsonObject *definitions = json->AddObject("definitions");
    m_schema_defs->AddToJsonObject(definitions);
  }
  return json;
}

JsonSchema* JsonSchema::FromString(const string& schema_string,
                                   string *error) {
  *error = "";
  SchemaParser schema_parser;
  bool ok = JsonLexer::Parse(schema_string, &schema_parser);
  if (!ok || !schema_parser.IsValidSchema()) {
    *error = schema_parser.Error();
    return NULL;
  }
  return new JsonSchema("", schema_parser.ClaimRootValidator(),
                        schema_parser.ClaimSchemaDefs());
}
}  // namespace web
}  // namespace ola
