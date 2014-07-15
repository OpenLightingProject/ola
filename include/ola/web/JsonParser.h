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
 * JsonParser.h
 * A JsonParserInterface implementation that builds a parse tree.
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonParser.h
 * @brief A JsonParserInterface implementation that builds a parse tree.
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONPARSER_H_
#define INCLUDE_OLA_WEB_JSONPARSER_H_

#include <ola/base/Macro.h>
#include <ola/web/JsonLexer.h>
#include <memory>
#include <stack>
#include <string>

namespace ola {
namespace web {

class JsonArray;
class JsonObject;
class JsonValue;

/**
 * @addtogroup json
 * @{
 */

/**
 * @brief A JsonParserInterface implementation that builds a tree of
 * JsonValues.
 *
 * This is the most common implementation of the JsonParserInterface but it's
 * also the least efficient since it loads the entire document into memory.
 */
class JsonParser : public JsonParserInterface {
 public:
  JsonParser() : JsonParserInterface() {}

  void Begin();
  void End();

  void String(const std::string &value);
  void Number(uint32_t value);
  void Number(int32_t value);
  void Number(uint64_t value);
  void Number(int64_t value);
  void Number(const JsonDouble::DoubleRepresentation &rep);
  void Number(double value);
  void Bool(bool value);
  void Null();
  void OpenArray();
  void CloseArray();
  void OpenObject();
  void ObjectKey(const std::string &key);
  void CloseObject();

  void SetError(const std::string &error);

  /**
   * @brief Check if parsing was successful.
   */
  std::string GetError() const;

  /**
   * @brief Get the root of the parse tree, or NULL if parsing failed.
   * @returns the root JsonValue. Ownership is not transferred.
   */
  JsonValue *GetRoot();

  /**
   * @brief Get the root of the parse tree, or NULL if parsing failed.
   * @returns the root JsonValue. Ownership is transferred to the caller.
   */
  JsonValue *ClaimRoot();

  /**
   * @brief Parse text and return a JsonValue representation
   */
  static JsonValue* Parse(const std::string &input,
                          std::string *error);

 private:
  // The container type identifies the type of container (object or array)
  // we're currently operating in. As we descend the parse tree we push types
  // onto m_container_stack so we can back track up the tree later.
  enum ContainerType {
    ARRAY,
    OBJECT,
  };

  std::string m_error;
  std::auto_ptr<JsonValue> m_root;
  std::string m_key;
  std::stack<ContainerType> m_container_stack;

  // The stacks below don't own the objects they point to.
  std::stack<JsonArray*> m_array_stack;
  std::stack<JsonObject*> m_object_stack;

  void AddValue(JsonValue *value);
  DISALLOW_COPY_AND_ASSIGN(JsonParser);
};
/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONPARSER_H_
