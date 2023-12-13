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
 * SchemaParser.h
 * Constructs a JsonSchema.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef COMMON_WEB_SCHEMAPARSER_H_
#define COMMON_WEB_SCHEMAPARSER_H_


#include <memory>
#include <stack>
#include <string>

#include "common/web/PointerTracker.h"
#include "common/web/SchemaErrorLogger.h"
#include "common/web/SchemaParseContext.h"
#include "ola/base/Macro.h"
#include "ola/web/JsonLexer.h"
#include "ola/web/JsonPointer.h"

namespace ola {
namespace web {

/**
 * @brief Build the tree of validators and a SchemaDefinitions object from a
 * JSON Schema.
 *
 * The SchemaParser is an implementation of JsonParserInterface. As the
 * JsonLexer encounters each token, it calls the appropriate method on the
 * SchemaParser. The SchemaParser maintains a stack of contexts, each of which
 * corresponds to a different part of the JSON schema. As objects / arrays are
 * opened / closed, new contexts are added / removed from the context stack.
 */
class SchemaParser : public JsonParserInterface {
 public:
  /**
   * @brief Create a new SchemaParser.
   */
  SchemaParser();

  /**
   * @brief Clean up
   */
  ~SchemaParser();

  // Methods from JsonParserInterface
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
   * @brief Check if the schema was valid.
   * @return true if the schema was valid, false otherwise.
   */
  bool IsValidSchema();

  /**
   * @brief Get the error message.
   */
  std::string Error() const;

  /**
   * @brief Claim the RootValidator that was created by parsing the schema.
   * @returns A new Validator, or NULL if the schema wasn't valid. Ownership of
   *   the validator is transferred to the caller.
   */
  ValidatorInterface* ClaimRootValidator();

  /**
   * @brief Claim the SchemaDefinitions that were created by parsing the schema.
   * @returns A SchemaDefinitions object, or NULL if the schema wasn't valid.
   *   Ownership of the SchemaDefinitions is transferred to the caller.
   */
  SchemaDefinitions* ClaimSchemaDefs();

 private:
  std::auto_ptr<SchemaDefinitions> m_schema_defs;
  std::auto_ptr<SchemaParseContext> m_root_context;

  std::auto_ptr<ValidatorInterface> m_root_validator;

  std::stack<class SchemaParseContextInterface*> m_context_stack;
  JsonPointer m_pointer;
  PointerTracker m_pointer_tracker;
  SchemaErrorLogger m_error_logger;

  template <typename T>
  void HandleNumber(T t);

  DISALLOW_COPY_AND_ASSIGN(SchemaParser);
};
}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_SCHEMAPARSER_H_
