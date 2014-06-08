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
 * JsonData.cpp
 * Copyright (C) 2014 Simon Newton
 */
#include "ola/web/JsonData.h"

#include <string>

#include "ola/web/Json.h"
#include "ola/web/JsonPatch.h"

namespace ola {
namespace web {

using std::string;

bool JsonData::SetValue(JsonValue *value) {
  if (IsValid(value)) {
    m_value.reset(value);
    return true;
  } else {
    delete value;
    return false;
  }
}

bool JsonData::Apply(const JsonPatchSet &patch) {
  JsonValue *new_value = NULL;
  if (m_value.get()) {
    new_value = m_value->Clone();
  }

  // now apply the patch
  bool ok = patch.Apply(&new_value) && IsValid(new_value);
  if (ok) {
    m_value.reset(new_value);
  } else {
    delete new_value;
  }
  return ok;
}

bool JsonData::IsValid(const JsonValue *value) {
  if (!m_schema) {
    return true;
  }

  value->Accept(m_schema);
  return m_schema->IsValid();
}
}  // namespace web
}  // namespace ola
