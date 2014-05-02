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

#include <map>
#include <memory>
#include <ostream>
#include <string>

#include "ola/web/JsonSchema.h"
#include "common/web/PointerTracker.h"

namespace ola {
namespace web {

class DefinitionsParseContext;
class PropertiesParseContext;
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
 * @brief Captures errors while parsing the schema.
 * The ErrorLogger keeps track of where we are in the parse tree so that errors
 * can have helpful information.
 */
class ErrorLogger {
 public:
  ErrorLogger() {}

  bool HasError() const {
    return !m_first_error.str().empty();
  }

  std::string ErrorString() const {
    return m_first_error.str();
  }

  std::ostream& Error() {
    if (m_first_error.str().empty()) {
      m_first_error << m_pointer.GetPointer() << ": ";
      return m_first_error;
    } else {
      return m_extra_errors;
    }
  }

  void SetProperty(const std::string &property) {
    m_pointer.SetProperty(property);
  }

  void OpenObject() {
    m_pointer.OpenObject();
  }

  void CloseObject() {
    m_pointer.CloseObject();
  }

  void OpenArray() {
    m_pointer.OpenArray();
  }

  void CloseArray() {
    m_pointer.CloseArray();
  }

  void IncrementIndex() {
    m_pointer.IncrementIndex();
  }

  void Reset() {
    m_pointer.Reset();
  }

  std::string GetPointer() {
    return m_pointer.GetPointer();
  }

 private:
  std::ostringstream m_first_error;
  std::ostringstream m_extra_errors;
  PointerTracker m_pointer;
};

/**
 * @brief The interface all SchemaParseContext classes inherit from.
 */
class SchemaParseContextInterface {
 public:
  SchemaParseContextInterface() {}
  virtual ~SchemaParseContextInterface() {}

  virtual void String(ErrorLogger *logger, const std::string &value) = 0;
  virtual void Number(ErrorLogger *logger, uint32_t value) = 0;
  virtual void Number(ErrorLogger *logger, int32_t value) = 0;
  virtual void Number(ErrorLogger *logger, uint64_t value) = 0;
  virtual void Number(ErrorLogger *logger, int64_t value) = 0;
  virtual void Number(ErrorLogger *logger, long double value) = 0;
  virtual void Bool(ErrorLogger *logger, bool value) = 0;
  virtual void Null(ErrorLogger *logger) = 0;
  virtual void OpenArray(ErrorLogger *logger) = 0;
  virtual void CloseArray(ErrorLogger *logger) = 0;
  virtual SchemaParseContextInterface* OpenObject(ErrorLogger *logger) = 0;
  virtual void ObjectKey(ErrorLogger *logger, const std::string &key) = 0;
  virtual void CloseObject(ErrorLogger *logger) = 0;
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
  void ObjectKey(ErrorLogger *logger, const std::string &property) {
    m_property_set = true;
    m_property = property;
    (void) logger;
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
  void String(ErrorLogger *, const std::string &) {}
  void Number(ErrorLogger *, uint32_t) {}
  void Number(ErrorLogger *, int32_t) {}
  void Number(ErrorLogger *, uint64_t) {}
  void Number(ErrorLogger *, int64_t) {}
  void Number(ErrorLogger *, long double) {}
  void Bool(ErrorLogger *, bool b) {
    (void) b;
  }
  void Null(ErrorLogger *) {}
  void OpenArray(ErrorLogger *) {}
  void CloseArray(ErrorLogger *) {}

  SchemaParseContextInterface* OpenObject(ErrorLogger *logger);
  void CloseObject(ErrorLogger *logger);

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

  ValidatorInterface* GetValidator(ErrorLogger *logger);

  // id, title, etc.
  void String(ErrorLogger *logger, const std::string &value);

  // minimum etc.
  void Number(ErrorLogger *logger, uint32_t value);
  void Number(ErrorLogger *logger, int32_t value);
  void Number(ErrorLogger *logger, uint64_t value);
  void Number(ErrorLogger *logger, int64_t value);
  void Number(ErrorLogger *logger, long double value);

  // exclusiveMin / Max
  void Bool(ErrorLogger *logger, bool value);

  void Null(ErrorLogger *logger) {
    (void) logger;
  }  // this shouldn't happen in a schema

  // enums
  void OpenArray(ErrorLogger *logger);
  void CloseArray(ErrorLogger *logger);

  // properties, etc.
  SchemaParseContextInterface* OpenObject(ErrorLogger *logger);
  void CloseObject(ErrorLogger *logger);

 private:
  SchemaDefinitions *m_schema_defs;
  OptionalItem<std::string> m_id;
  OptionalItem<std::string> m_schema;
  OptionalItem<std::string> m_title;
  OptionalItem<std::string> m_description;
  OptionalItem<std::string> m_type;

  std::auto_ptr<DefinitionsParseContext> m_definitions_context;
  std::auto_ptr<PropertiesParseContext> m_properties_context;

  bool ValidType(const std::string& type);

  DISALLOW_COPY_AND_ASSIGN(SchemaParseContext);
};


/**
 * @brief
 */
class PropertiesParseContext : public BaseParseContext {
 public:
  explicit PropertiesParseContext(SchemaDefinitions *definitions)
      : BaseParseContext(),
        m_schema_defs(definitions) {
  }

  void AddPropertyValidaators(ObjectValidator *object_validator,
                              ErrorLogger *logger);

  void String(ErrorLogger *logger, const std::string &value);
  void Number(ErrorLogger *logger, uint32_t value);
  void Number(ErrorLogger *logger, int32_t value);
  void Number(ErrorLogger *logger, uint64_t value);
  void Number(ErrorLogger *logger, int64_t value);
  void Number(ErrorLogger *logger, long double value);
  void Bool(ErrorLogger *logger, bool value);
  void Null(ErrorLogger *logger);
  void OpenArray(ErrorLogger *logger);
  void CloseArray(ErrorLogger *logger);
  SchemaParseContextInterface* OpenObject(ErrorLogger *logger);
  void CloseObject(ErrorLogger *logger);

 private:
  typedef std::map<std::string, SchemaParseContext*> SchemaMap;

  SchemaDefinitions *m_schema_defs;
  SchemaMap m_property_contexts;

  DISALLOW_COPY_AND_ASSIGN(PropertiesParseContext);
};
}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_SCHEMAPARSECONTEXT_H_
