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
#include <set>
#include <string>
#include <vector>

#include "ola/web/JsonSchema.h"
#include "common/web/PointerTracker.h"

namespace ola {
namespace web {

class ArrayItemsParseContext;
class DefinitionsParseContext;
class PropertiesParseContext;
class SchemaParseContext;
class RequiredPropertiesParseContext;

/**
 * @brief The list of valid JSon Schema keywords.
 */
enum SchemaKeyword {
  SCHEMA_UNKNOWN,  /**< Keywords we don't understand */
  SCHEMA_ID,
  SCHEMA_SCHEMA,
  SCHEMA_REF,
  SCHEMA_TITLE,
  SCHEMA_DESCRIPTION,
  SCHEMA_DEFAULT,
  SCHEMA_FORMAT,
  SCHEMA_MULTIPLEOF,
  SCHEMA_MAXIMUM,
  SCHEMA_EXCLUSIVE_MAXIMUM,
  SCHEMA_MINIMUM,
  SCHEMA_EXCLUSIVE_MINIMUM,
  SCHEMA_MAX_LENGTH,
  SCHEMA_MIN_LENGTH,
  SCHEMA_PATTERN,
  SCHEMA_ADDITIONAL_ITEMS,
  SCHEMA_ITEMS,
  SCHEMA_MAX_ITEMS,
  SCHEMA_MIN_ITEMS,
  SCHEMA_UNIQUE_ITEMS,
  SCHEMA_MAX_PROPERTIES,
  SCHEMA_MIN_PROPERTIES,
  SCHEMA_REQUIRED,
  SCHEMA_ADDITIONAL_PROPERTIES,
  SCHEMA_DEFINITIONS,
  SCHEMA_PROPERTIES,
  SCHEMA_PATTERN_PROPERTIES,
  SCHEMA_DEPENDENCIES,
  SCHEMA_ENUM,
  SCHEMA_TYPE,
  SCHEMA_ALL_OF,
  SCHEMA_ANY_OF,
  SCHEMA_ONE_OF,
  SCHEMA_NOT,
};

/**
 * Return the string used by the SchemaKeyword.
 */
std::string KeywordToString(SchemaKeyword keyword);

/**
 * @brief Map a string to a SchemaKeyword.
 * @returns the SchemaKeyword corresponding to the string, or SCHEMA_UNDEFINED.
 */
SchemaKeyword LookupKeyword(const std::string& keyword);

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
  virtual void Number(ErrorLogger *logger, double value) = 0;
  virtual void Bool(ErrorLogger *logger, bool value) = 0;
  virtual void Null(ErrorLogger *logger) = 0;
  virtual SchemaParseContextInterface* OpenArray(ErrorLogger *logger) = 0;
  virtual void CloseArray(ErrorLogger *logger) = 0;
  virtual SchemaParseContextInterface* OpenObject(ErrorLogger *logger) = 0;
  virtual void ObjectKey(ErrorLogger *logger, const std::string &key) = 0;
  virtual void CloseObject(ErrorLogger *logger) = 0;
};

/**
 * @brief A SchemaParseContext that keeps track of the last keyword seen.
 */
class BaseParseContext : public SchemaParseContextInterface {
 public:
  BaseParseContext() : SchemaParseContextInterface() {}

  /**
   * @brief Called when we encouter a property
   */
  void ObjectKey(ErrorLogger *logger, const std::string &keyword) {
    m_keyword.Set(keyword);
    (void) logger;
  }

 protected:
  /**
   * @brief Check if a
   */
  bool HasKeyword() const { return m_keyword.IsSet(); }

  /**
   *
   */
  std::string TakeKeyword() {
    std::string keyword = m_keyword.Value();
    m_keyword.Reset();
    return keyword;
  }

  /**
   *
   */
  const std::string& Keyword() const { return m_keyword.Value(); }

 private:
  OptionalItem<std::string> m_keyword;
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
  void Number(ErrorLogger *, double) {}
  void Bool(ErrorLogger *, bool) {}
  void Null(ErrorLogger *logger) { (void) logger; }

  SchemaParseContextInterface* OpenArray(ErrorLogger *logger) {
    return NULL;
    (void) logger;
  }

  void CloseArray(ErrorLogger *logger) {
    (void) logger;
  }

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
class SchemaParseContext : public SchemaParseContextInterface {
 public:
  /**
   * @brief Create a new SchemaParseContext
   * @param definitions, the SchemaDefinitions cache, ownership is not
   *   transferred.
   */
  explicit SchemaParseContext(SchemaDefinitions *definitions)
      : m_schema_defs(definitions),
        m_type(JSON_UNDEFINED) {
  }
  ~SchemaParseContext();

  /**
   * @brief Return the ValidatorInterface for this context.
   *
   * Ownership of the ValidatorInterface is transferred to the caller.
   * @returns A new ValidatorInterface or NULL if it was not possible to
   * construct a validator.
   */
  ValidatorInterface* GetValidator(ErrorLogger *logger);

  void ObjectKey(ErrorLogger *logger, const std::string &keyword);

  // id, title, etc.
  void String(ErrorLogger *logger, const std::string &value);

  // minimum etc.
  void Number(ErrorLogger *logger, uint32_t value);
  void Number(ErrorLogger *logger, int32_t value);
  void Number(ErrorLogger *logger, uint64_t value);
  void Number(ErrorLogger *logger, int64_t value);
  void Number(ErrorLogger *logger, double value);

  // exclusiveMin / Max
  void Bool(ErrorLogger *logger, bool value);
  void Null(ErrorLogger *logger);

  // enums
  SchemaParseContextInterface* OpenArray(ErrorLogger *logger);
  void CloseArray(ErrorLogger *logger);

  // properties, etc.
  SchemaParseContextInterface* OpenObject(ErrorLogger *logger);
  void CloseObject(ErrorLogger *logger);

 private:
  SchemaDefinitions *m_schema_defs;
  // Set to the last keyword reported to ObjectKey()
  SchemaKeyword m_keyword;

  // Members are arranged according to the order in which they appear in the
  // draft standard.

  // Common keywords
  OptionalItem<std::string> m_id;
  OptionalItem<std::string> m_schema;

  // 5.1 Number / integer keywords
  OptionalItem<bool> m_exclusive_maximum;
  OptionalItem<bool> m_exclusive_minimum;

  // 5.2 String keywords
  // TODO(simon): Implement pattern support?
  OptionalItem<std::string> m_pattern;
  OptionalItem<uint64_t> m_max_length;
  OptionalItem<uint64_t> m_min_length;

  // 5.3 Array keywords
  // 'additionalItems' can be either a bool or a schema
  OptionalItem<bool> m_additional_items;
  std::auto_ptr<SchemaParseContext> m_additional_items_context;

  // 'items' can be either a json schema, or an array of json schema.
  std::auto_ptr<SchemaParseContext> m_items_single_context;
  std::auto_ptr<ArrayItemsParseContext> m_items_context_array;

  OptionalItem<uint64_t> m_max_items;
  OptionalItem<uint64_t> m_min_items;
  OptionalItem<bool> m_unique_items;

  // 5.4 Object keywords
  OptionalItem<uint64_t> m_max_properties;
  OptionalItem<uint64_t> m_min_properties;
  std::auto_ptr<RequiredPropertiesParseContext> m_required_items;

  // 5.5 Keywords for multiple instance types
  JsonType m_type;

  // 6. Metadata keywords
  OptionalItem<std::string> m_description;
  OptionalItem<std::string> m_title;

  OptionalItem<std::string> m_ref_schema;

  // TODO(simon): Implement format support?
  OptionalItem<std::string> m_format;

  std::auto_ptr<DefinitionsParseContext> m_definitions_context;
  std::auto_ptr<PropertiesParseContext> m_properties_context;
  // vector<NumberConstraint> m_number_constraints;

  void ProcessPositiveInt(ErrorLogger *logger, uint64_t value);

  template <typename T>
  void ProcessInt(ErrorLogger *logger, T t);
  ValidatorInterface* BuildArrayValidator(ErrorLogger *logger);
  ValidatorInterface* BuildObjectValidator(ErrorLogger *logger);
  ValidatorInterface* BuildStringValidator(ErrorLogger *logger);

  static bool ValidTypeForKeyword(ErrorLogger *logger, SchemaKeyword keyword,
                                  JsonType type);
  // Verify that type == expected_type. If it doesn't report an error to the
  // logger.
  static bool CheckTypeAndLog(ErrorLogger *logger, SchemaKeyword keyword,
                              JsonType type, JsonType expected_type);
  // Same as above but the type can be either expected_type1 or expected_type2
  static bool CheckTypeAndLog(ErrorLogger *logger, SchemaKeyword keyword,
                              JsonType type, JsonType expected_type1,
                              JsonType expected_type2);

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
  void Number(ErrorLogger *logger, double value);
  void Bool(ErrorLogger *logger, bool value);
  void Null(ErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(ErrorLogger *logger);
  void CloseArray(ErrorLogger *logger);
  SchemaParseContextInterface* OpenObject(ErrorLogger *logger);
  void CloseObject(ErrorLogger *logger);

 private:
  typedef std::map<std::string, SchemaParseContext*> SchemaMap;

  SchemaDefinitions *m_schema_defs;
  SchemaMap m_property_contexts;

  DISALLOW_COPY_AND_ASSIGN(PropertiesParseContext);
};


/**
 * @brief Parse the array of objects in an 'items' property.
 */
class ArrayItemsParseContext : public BaseParseContext {
 public:
  explicit ArrayItemsParseContext(SchemaDefinitions *definitions)
      : BaseParseContext(),
        m_schema_defs(definitions) {
  }

  ~ArrayItemsParseContext();

  /**
   * @brief Populate a vector with validators for the elements in 'items'
   * @param[out] validators A vector fill with new validators. Ownership of the
   * validators is transferred to the caller.
   */
  void AddValidators(ErrorLogger *logger,
                     std::vector<ValidatorInterface*> *validators);

  void String(ErrorLogger *logger, const std::string &value);
  void Number(ErrorLogger *logger, uint32_t value);
  void Number(ErrorLogger *logger, int32_t value);
  void Number(ErrorLogger *logger, uint64_t value);
  void Number(ErrorLogger *logger, int64_t value);
  void Number(ErrorLogger *logger, double value);
  void Bool(ErrorLogger *logger, bool value);
  void Null(ErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(ErrorLogger *logger);
  void CloseArray(ErrorLogger *logger) {
    (void) logger;
  }
  SchemaParseContextInterface* OpenObject(ErrorLogger *logger);
  void CloseObject(ErrorLogger *logger) {
    (void) logger;
  }

 private:
  typedef std::vector<SchemaParseContext*> ItemSchemas;

  SchemaDefinitions *m_schema_defs;
  ItemSchemas m_item_schemas;

  void ReportErrorForType(ErrorLogger *logger, JsonType type);

  DISALLOW_COPY_AND_ASSIGN(ArrayItemsParseContext);
};


/**
 * @brief Parse an array of strings for the 'required' property.
 */
class RequiredPropertiesParseContext : public BaseParseContext {
 public:
  typedef std::set<std::string> RequiredItems;

  RequiredPropertiesParseContext() : BaseParseContext() {}

  /**
   * @brief Return the strings in the 'required' array
   */
  void GetRequiredItems(RequiredItems *required_items);

  void String(ErrorLogger *logger, const std::string &value);
  void Number(ErrorLogger *logger, uint32_t value);
  void Number(ErrorLogger *logger, int32_t value);
  void Number(ErrorLogger *logger, uint64_t value);
  void Number(ErrorLogger *logger, int64_t value);
  void Number(ErrorLogger *logger, double value);
  void Bool(ErrorLogger *logger, bool value);
  void Null(ErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(ErrorLogger *logger);
  void CloseArray(ErrorLogger *logger) {
    (void) logger;
  }
  SchemaParseContextInterface* OpenObject(ErrorLogger *logger);
  void CloseObject(ErrorLogger *logger) {
    (void) logger;
  }

 private:
  RequiredItems m_required_items;

  void ReportErrorForType(ErrorLogger *logger, JsonType type);

  DISALLOW_COPY_AND_ASSIGN(RequiredPropertiesParseContext);
};
}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_SCHEMAPARSECONTEXT_H_
