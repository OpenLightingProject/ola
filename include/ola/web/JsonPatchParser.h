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
 * JsonPatchParser.h
 * Create a JsonPatchSet from a string.
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonPatchParser.h
 * @brief Create a JsonPatchSet from a string.
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONPATCHPARSER_H_
#define INCLUDE_OLA_WEB_JSONPATCHPARSER_H_

#include <ola/base/Macro.h>
#include <ola/web/JsonLexer.h>
#include <ola/web/JsonPatch.h>
#include <ola/web/JsonParser.h>
#include <ola/web/OptionalItem.h>
#include <memory>
#include <stack>
#include <string>

namespace ola {
namespace web {

/**
 * @addtogroup json
 * @{
 */

/**
 * @brief Parse a JSON Patch document (RFC 6902).
 */
class JsonPatchParser : public JsonParserInterface {
 public:
  explicit JsonPatchParser(JsonPatchSet *patch_set)
      : JsonParserInterface(),
        m_patch_set(patch_set),
        m_parser_depth(0),
        m_state(TOP) {
  }

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
   * @brief Check if this patch document was valid
   */
  bool IsValid() const;

  /**
   * @brief Build a JsonPatchSet from a JSON document.
   * @param input the JSON text to parse
   * @param patch_set the JsonPatchSet to populate
   * @param error where to store any errors.
   */
  static bool Parse(const std::string &input,
                    JsonPatchSet *patch_set,
                    std::string *error);

 private:
  enum State {
    TOP,
    PATCH_LIST,
    PATCH,
    VALUE,
  };

  std::string m_error;
  JsonPatchSet *m_patch_set;
  std::string m_key;

  JsonParser m_parser;
  unsigned int m_parser_depth;
  State m_state;

  // Patch members;
  std::string m_op;
  OptionalItem<std::string> m_path;
  OptionalItem<std::string> m_from;
  std::auto_ptr<JsonValue> m_value;

  template <typename T>
  void HandleNumber(const T &value);

  void HandlePatchString(const std::string &value);
  void HandlePatch();

  static const char kFromKey[];
  static const char kMissingFrom[];
  static const char kMissingPath[];
  static const char kMissingValue[];
  static const char kOpKey[];
  static const char kPatchElementError[];
  static const char kPatchListError[];
  static const char kPathKey[];
  static const char kValueKey[];
  static const char kAddOp[];
  static const char kRemoveOp[];
  static const char kReplaceOp[];
  static const char kMoveOp[];
  static const char kCopyOp[];
  static const char kTestOp[];

  DISALLOW_COPY_AND_ASSIGN(JsonPatchParser);
};
/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONPATCHPARSER_H_
