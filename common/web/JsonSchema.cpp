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
#include "ola/web/JsonParser.h"
#include "ola/web/JsonSchema.h"

namespace ola {
namespace web {

using std::set;
using std::string;
using std::vector;

JsonObject* BaseValidator::GetSchema() const {
  JsonObject *schema = new JsonObject();
  if (!m_title.empty()) {
    schema->Add("title", m_title);
  }
  if (!m_description.empty()) {
    schema->Add("description", m_description);
  }
  ExtendSchema(schema);
  return schema;
}

void BaseValidator::SetTitle(const string &title) {
  m_title = title;
}

void BaseValidator::SetDescription(const string &description) {
  m_description = description;
}

ReferenceValidator::ReferenceValidator(const SchemaDefinitions *definitions,
                                       const string &schema)
    : m_definitions(definitions),
      m_schema(schema),
      m_validator(NULL) {
}

bool ReferenceValidator::IsValid() const {
  return m_validator ? m_validator->IsValid() : false;
}

void ReferenceValidator::Visit(const JsonStringValue &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonBoolValue &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonNullValue &value) {
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

void ReferenceValidator::Visit(const JsonUIntValue &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonUInt64Value &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonIntValue &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonInt64Value &value) {
  Validate(value);
}

void ReferenceValidator::Visit(const JsonDoubleValue &value) {
  Validate(value);
}

JsonObject* ReferenceValidator::GetSchema() const {
  JsonObject *schema = new JsonObject();
  schema->Add("$ref", m_schema);
  return schema;
}

template <typename T>
void ReferenceValidator::Validate(const T &value) {
  if (!m_validator) {
    m_validator = m_definitions->Lookup(m_schema);
  }

  if (m_validator) {
    value.Accept(m_validator);
  }
}

void StringValidator::Visit(const JsonStringValue &str) {
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

  m_is_valid = true;
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

bool MultipleOfConstraint::IsValid(long double d) {
  return (fmod(d, m_multiple_of) == 0);
}

NumberValidator::~NumberValidator() {
  STLDeleteElements(&m_constraints);
}

void NumberValidator::AddConstraint(NumberConstraint *constraint) {
  m_constraints.push_back(constraint);
}

void NumberValidator::Visit(const JsonUIntValue &value) {
  CheckValue(value.Value());
}

void NumberValidator::Visit(const JsonIntValue &value) {
  CheckValue(value.Value());
}

void NumberValidator::Visit(const JsonUInt64Value &value) {
  CheckValue(value.Value());
}

void NumberValidator::Visit(const JsonInt64Value &value) {
  CheckValue(value.Value());
}

void NumberValidator::Visit(const JsonDoubleValue &value) {
  CheckValue(value.Value());
}

void NumberValidator::ExtendSchema(JsonObject *schema) const {
  vector<NumberConstraint*>::const_iterator iter = m_constraints.begin();
  for (; iter != m_constraints.end(); ++iter) {
    (*iter)->ExtendSchema(schema);
  }
}

template <typename T>
void NumberValidator::CheckValue(T t) {
  vector<NumberConstraint*>::const_iterator iter = m_constraints.begin();
  for (; iter != m_constraints.end(); ++iter) {
    if (!(*iter)->IsValid(t)) {
      m_is_valid = false;
      return;
    }
  }
  m_is_valid = true;
}

ObjectValidator::ObjectValidator(const Options &options)
    : BaseValidator(),
      m_options(options) {
}

ObjectValidator::~ObjectValidator() {
  STLDeleteValues(&m_property_validators);
}

void ObjectValidator::AddValidator(const std::string &property,
                                   ValidatorInterface *validator) {
  STLReplaceAndDelete(&m_property_validators, property, validator);
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

  set<string> missing_properties;
  std::set_difference(m_options.required_properties.begin(),
                      m_options.required_properties.end(),
                      m_seen_properties.begin(),
                      m_seen_properties.end(),
                      std::inserter(missing_properties,
                                    missing_properties.end()));
  if (!missing_properties.empty()) {
    OLA_INFO << "Missing " << missing_properties.size()
             << " required properties";
    m_is_valid = false;
  }
}

void ObjectValidator::VisitProperty(const std::string &property,
                                    const JsonValue &value) {
  OLA_INFO << "Looking for property " << property;
  m_seen_properties.insert(property);

  ValidatorInterface *validator = STLFindOrNull(
      m_property_validators, property);
  if (!validator) {
    OLA_WARN << "No such property " << property;
    m_is_valid &= false;
    return;
  }

  value.Accept(validator);
  m_is_valid &= validator->IsValid();
}

void ObjectValidator::ExtendSchema(JsonObject *schema) const {
  if (m_options.min_properties > 0) {
    schema->Add("minProperties", m_options.min_properties);
  }

  if (m_options.max_properties >= 0) {
    schema->Add("maxProperties", m_options.max_properties);
  }

  if (!m_options.required_properties.empty()) {
    JsonArray *required_properties = schema->AddArray("required");
    set<string>::const_iterator iter = m_options.required_properties.begin();
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
}

ArrayValidator::ArrayValidator(ValidatorInterface *validator,
                               const Options &options)
  : m_default_validator(validator),
    m_options(options) {
}

ArrayValidator::ArrayValidator(ValidatorList *validators,
                               ValidatorInterface *schema,
                               const Options &options)
    : m_validators(*validators),
      m_default_validator(schema),
      m_options(options) {
  validators->clear();
}

ArrayValidator::~ArrayValidator() {
  STLDeleteElements(&m_validators);
}

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

  // TODO(simon): implement unique_items here.

  ArrayElementValidator element_validator(
      m_validators, m_default_validator.get());

  for (unsigned int i = 0; i < array.Size(); i++) {
    OLA_INFO << "Checking element at " << i;
    array.ElementAt(i)->Accept(&element_validator);
    if (!element_validator.IsValid()) {
      break;
    }
  }
  m_is_valid = element_validator.IsValid();
}

ArrayValidator::ArrayElementValidator::ArrayElementValidator(
    const ValidatorList &validators,
    ValidatorInterface *default_validator)
    : m_item_validators(validators.begin(), validators.end()),
      m_default_validator(default_validator) {
}

void ArrayValidator::ArrayElementValidator::Visit(
    const JsonStringValue &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonBoolValue &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonNullValue &value) {
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

void ArrayValidator::ArrayElementValidator::Visit(const JsonUIntValue &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(
    const JsonUInt64Value &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonIntValue &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(const JsonInt64Value &value) {
  ValidateItem(value);
}

void ArrayValidator::ArrayElementValidator::Visit(
    const JsonDoubleValue &value) {
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

  if (!m_validators.empty()) {
    JsonArray *items = schema->AddArray("items");
    ValidatorList::const_iterator iter = m_validators.begin();
    for (; iter != m_validators.end(); iter++) {
      JsonObject *child_schema = (*iter)->GetSchema();
      items->Append(child_schema);
    }
  } else if (m_default_validator.get()) {
    JsonObject *child_schema = m_default_validator->GetSchema();
    schema->Add("items", child_schema);
  }
}

ConjunctionValidator::ConjunctionValidator(const string &keyword,
                                           ValidatorList *validators)
    : m_keyword(keyword),
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


void NotValidator::Validate(const JsonValue &value) {
  value.Accept(m_validator.get());
  m_is_valid = !m_validator->IsValid();
}

void NotValidator::ExtendSchema(JsonObject *schema) const {
  JsonObject *child_schema = m_validator->GetSchema();
  schema->AddValue("not", child_schema);
}

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
  if (json) {
    JsonObject *definitions = json->AddObject("definitions");
    m_schema_defs->AddToJsonObject(definitions);
  }
  return json;
}

JsonSchema* JsonSchema::FromString(const string& schema_string,
                                   string *error) {
  *error = "";
  SchemaParser schema_parser;
  bool ok = JsonParser::Parse(schema_string, &schema_parser);
  if (!ok || !schema_parser.IsValidSchema()) {
    OLA_INFO << "Error " << schema_parser.Error();
    *error = schema_parser.Error();
    return NULL;
  }
  return new JsonSchema("", schema_parser.ClaimRootValidator(),
                        schema_parser.ClaimSchemaDefs());
}
}  // namespace web
}  // namespace ola
