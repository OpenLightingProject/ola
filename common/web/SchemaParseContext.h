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

  virtual void String(const std::string &value) = 0;
  virtual void Number(uint32_t value) = 0;
  virtual void Number(int32_t value) = 0;
  virtual void Number(uint64_t value) = 0;
  virtual void Number(int64_t value) = 0;
  virtual void Number(long double value) = 0;
  virtual void Bool(bool value) = 0;
  virtual void Null() = 0;
  virtual void OpenArray() = 0;
  virtual void CloseArray() = 0;
  virtual SchemaParseContextInterface* OpenObject() = 0;
  virtual void ObjectKey(const std::string &key) = 0;
  virtual void CloseObject() = 0;
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
  void ObjectKey(const std::string &property) {
    m_property_set = true;
    m_property = property;
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
  DefinitionsParseContext()
      : BaseParseContext(),
        m_schema_defs(new SchemaDefintions()) {
  }

  SchemaDefintions* GetSchemaDefinitions() {
    return NULL;
  }

  // These are all invalid in 'definitions'
  void String(const std::string &) {}
  void Number(uint32_t) {}
  void Number(int32_t) {}
  void Number(uint64_t) {}
  void Number(int64_t) {}
  void Number(long double) {}
  void Bool(bool b) {
    (void) b;
  }
  void Null() {}
  void OpenArray() {}
  void CloseArray() {}

  SchemaParseContextInterface* OpenObject();
  void CloseObject();

 private:
  std::auto_ptr<SchemaDefintions> m_schema_defs;
  std::auto_ptr<SchemaParseContext> m_current_schema;

  DISALLOW_COPY_AND_ASSIGN(DefinitionsParseContext);
};

/**
 * @brief
 */
class SchemaParseContext : public BaseParseContext {
 public:
  SchemaParseContext() : BaseParseContext() {}

  virtual ValidatorInterface* GetValidator();

  // id, title, etc.
  void String(const std::string &value);

  // minimum etc.
  void Number(uint32_t value);
  void Number(int32_t value);
  void Number(uint64_t value);
  void Number(int64_t value);
  void Number(long double value);

  // exclusiveMin / Max
  void Bool(bool value);

  void Null() {}  // this shouldn't happen in a schema

  // enums
  void OpenArray();
  void CloseArray();

  // properties, etc.
  SchemaParseContextInterface* OpenObject();
  void CloseObject();

 private:
  enum JsonType {
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOL,
    JSON_NULL,
    JSON_ARRAY,
    JSON_OBJECT
  };

  OptionalItem<std::string> m_title;
  OptionalItem<std::string> m_description;

  bool TypeIsValidForCurrentKey(JsonType type);

  DISALLOW_COPY_AND_ASSIGN(SchemaParseContext);
};
}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_SCHEMAPARSECONTEXT_H_
