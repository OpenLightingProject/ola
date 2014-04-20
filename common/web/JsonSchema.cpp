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
#include <string>
#include <vector>
#include "ola/stl/STLUtils.h"
#include "ola/web/JsonSchema.h"
#include "ola/Logging.h"

namespace ola {
namespace web {

using std::string;
using std::vector;

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

void ObjectValidator::AddValidator(const std::string &property,
                                   BaseValidator *validator) {
  STLReplace(&m_property_validators, property, validator);
}

void ObjectValidator::Visit(const JsonObject &obj) {
  m_is_valid = true;
  obj.VisitProperties(this);
}

void ObjectValidator::VisitProperty(const std::string &property,
                                    const JsonValue &value) {
  OLA_INFO << "Looking for property " << property;
  BaseValidator *validator = STLFindOrNull(m_property_validators, property);
  if (!validator) {
    OLA_WARN << "No such property " << property;
    m_is_valid &= false;
    return;
  }

  value.Accept(validator);
  m_is_valid &= validator->IsValid();
}

}  // namespace web
}  // namespace ola
