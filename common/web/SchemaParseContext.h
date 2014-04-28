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
 * SchemaParseContext.h
 * Stores the state required as we walk the JSON schema document.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef COMMON_WEB_SCHEMAPARSECONTEXT_H_
#define COMMON_WEB_SCHEMAPARSECONTEXT_H_

#include <ola/base/Macro.h>

#include <memory>
#include <string>

#include "ola/web/JsonSchema.h"

namespace ola {
namespace web {

class SchemaParseContext;

template <typename T>
class OptionalItem {
 public:
  OptionalItem() : m_is_set(false) {}

  void Reset() { m_is_set = false; }

  void Set(const T &value) {
    m_is_set = true;
    m_value = value;
  }

  bool IsSet() const { return m_is_set; }
  const T& Value() const { return m_value; }

 private:
  bool m_is_set;
  T m_value;
};

/**
 * @brief The interface all SchemaParseContext classes inherit from.
 */
class SchemaParseContextInterface {
 public:
  SchemaParseContextInterface() {}
  virtual ~SchemaParseContextInterface() {}

  virtual void String(const std::string &path, const std::string &value) = 0;
  virtual void Number(const std::string &path, uint32_t value) = 0;
  virtual void Number(const std::string &path, int32_t value) = 0;
  virtual void Number(const std::string &path, uint64_t value) = 0;
  virtual void Number(const std::string &path, int64_t value) = 0;
  virtual void Number(const std::string &path, long double value) = 0;
  virtual void Bool(const std::string &path, bool value) = 0;
  virtual void Null(const std::string &path) = 0;
  virtual void OpenArray(const std::string &path) = 0;
  virtual void CloseArray(const std::string &path) = 0;
  virtual SchemaParseContextInterface* OpenObject(const std::string &path) = 0;
  virtual void ObjectKey(const std::string &path, const std::string &key) = 0;
  virtual void CloseObject(const std::string &path) = 0;
};

/**
 * @brief A SchemaParseContext that keeps track of the last property seen.
 */
class BaseParseContext : public SchemaParseContextInterface {
 public:
  BaseParseContext()
      : SchemaParseContextInterface(),
        m_property_set(false) {
  }

  /**
   * @brief Called when we encouter a property
   */
  void ObjectKey(const std::string &path, const std::string &property) {
    m_property_set = true;
    m_property = property;
    (void) path;
  }

 protected:
  /**
   * @brief Check if a 
   */
  bool HasKey() const { return m_property_set; }

  /**
   *
   */
  std::string TakeKey() {
    m_property_set = false;
    return m_property;
  }

  /**
   *
   */
  std::string Key() const { return m_property; }

 private:
  bool m_property_set;
  std::string m_property;
};

/**
 * @brief
 */
class DefinitionsParseContext : public BaseParseContext {
 public:
  /**
   * @brief Create a new DefinitionsParseContext
   * @param definitions, the SchemaDefinitions cache, ownership is not
   *   transferred.
   */
  explicit DefinitionsParseContext(SchemaDefinitions *definitions)
      : BaseParseContext(),
        m_schema_defs(definitions) {
  }

  // These are all invalid in 'definitions'
  void String(const std::string &, const std::string &) {}
  void Number(const std::string &, uint32_t) {}
  void Number(const std::string &, int32_t) {}
  void Number(const std::string &, uint64_t) {}
  void Number(const std::string &, int64_t) {}
  void Number(const std::string &, long double) {}
  void Bool(const std::string &, bool b) {
    (void) b;
  }
  void Null(const std::string &) {}
  void OpenArray(const std::string &) {}
  void CloseArray(const std::string &) {}

  SchemaParseContextInterface* OpenObject(const std::string &path);
  void CloseObject(const std::string &path);

 private:
  SchemaDefinitions *m_schema_defs;
  std::auto_ptr<SchemaParseContext> m_current_schema;

  DISALLOW_COPY_AND_ASSIGN(DefinitionsParseContext);
};

/**
 * @brief
 */
class SchemaParseContext : public BaseParseContext {
 public:
  /**
   * @brief Create a new SchemaParseContext
   * @param definitions, the SchemaDefinitions cache, ownership is not
   *   transferred.
   */
  explicit SchemaParseContext(SchemaDefinitions *definitions)
      : BaseParseContext(),
        m_schema_defs(definitions) {
  }

  virtual ValidatorInterface* GetValidator();

  // id, title, etc.
  void String(const std::string &path, const std::string &value);

  // minimum etc.
  void Number(const std::string &path, uint32_t value);
  void Number(const std::string &path, int32_t value);
  void Number(const std::string &path, uint64_t value);
  void Number(const std::string &path, int64_t value);
  void Number(const std::string &path, long double value);

  // exclusiveMin / Max
  void Bool(const std::string &path, bool value);

  void Null(const std::string &) {}  // this shouldn't happen in a schema

  // enums
  void OpenArray(const std::string &path);
  void CloseArray(const std::string &path);

  // properties, etc.
  SchemaParseContextInterface* OpenObject(const std::string &path);
  void CloseObject(const std::string &path);

 private:
  enum JsonType {
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOL,
    JSON_NULL,
    JSON_ARRAY,
    JSON_OBJECT
  };

  SchemaDefinitions *m_schema_defs;
  OptionalItem<std::string> m_title;
  OptionalItem<std::string> m_description;

  bool TypeIsValidForCurrentKey(JsonType type);

  DISALLOW_COPY_AND_ASSIGN(SchemaParseContext);
};
}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_SCHEMAPARSECONTEXT_H_
