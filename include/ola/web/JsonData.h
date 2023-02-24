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
 * JsonData.h
 * The top level container for JSON data.
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonData.h
 * @brief The top level container for JSON data.
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONDATA_H_
#define INCLUDE_OLA_WEB_JSONDATA_H_

#include <ola/base/Macro.h>
#include <ola/web/Json.h>
#include <ola/web/JsonPatch.h>
#include <ola/web/JsonSchema.h>
#include <memory>
#include <string>

namespace ola {
namespace web {

/**
 * @addtogroup json
 * @{
 */

/**
 * @brief Represents a JSON text as defined in section 2 of RFC7158.
 *
 * JsonData encapsulates a JsonValue and permits patch operations to be
 * applied.
 *
 * Clients should use this rather than JsonValues when using JSON patch
 * operations. This is because some patch ops may delete the entire value, so
 * you shouldn't really be passing JsonValue* around.
 */
class JsonData {
 public:
  /**
   * @brief Construct a new JsonData.
   * @param value The JsonValue to use, ownership is transferred.
   * @param schema The schema to validate this data against. Ownership is not
   *   transferred.
   */
  JsonData(const JsonValue *value,
           ValidatorInterface *schema = NULL)
      : m_value(value),
        m_schema(schema) {
  }

  /**
   * @brief Return the JsonValue for this text.
   *
   * The pointer is valid until the next patch operation or call to SetValue()
   */
  const JsonValue* Value() const { return m_value.get(); }

  /**
   * @brief Set the value for this JsonData.
   * @param value the new JsonValue. Ownership is transferred.
   */
  bool SetValue(JsonValue *value);

  /**
   * @brief Apply a set of JSON patches to the value.
   * @param patch The set of JsonPatch operations to apply.
   * @returns True if the patch set was successfully applied, false otherwise.
   */
  bool Apply(const JsonPatchSet &patch);

  /**
   * @brief Return the schema for this JSON data.
   */
  const ValidatorInterface* GetSchema() const { return m_schema; }

 private:
  std::auto_ptr<const JsonValue> m_value;
  ValidatorInterface *m_schema;

  bool IsValid(const JsonValue *value);
};

/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONDATA_H_
