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

#include "ola/stl/STLUtils.h"
#include "ola/web/JsonSchema.h"
#include "ola/Logging.h"

namespace ola {
namespace web {

using std::set;
using std::string;
using std::vector;

ReferenceValidator::ReferenceValidator(const SchemaDefintions *definitions,
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

SchemaDefintions::~SchemaDefintions() {
  STLDeleteValues(&m_validators);
}

void SchemaDefintions::Add(const string &schema_name,
                           ValidatorInterface *validator) {
  STLReplaceAndDelete(&m_validators, schema_name, validator);
}

ValidatorInterface *SchemaDefintions::Lookup(const string &schema_name) const {
  return STLFindOrNull(m_validators, schema_name);
}

}  // namespace web
}  // namespace ola
