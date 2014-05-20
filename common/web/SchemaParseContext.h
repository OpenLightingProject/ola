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
#include <set>
#include <string>
#include <vector>

#include "common/web/PointerTracker.h"
#include "common/web/SchemaErrorLogger.h"
#include "common/web/SchemaKeywords.h"
#include "ola/web/JsonSchema.h"
#include "ola/web/JsonParser.h"

namespace ola {
namespace web {

class ArrayItemsParseContext;
class DefaultValueParseContext;
class DefinitionsParseContext;
class DependencyParseContext;
class PropertiesParseContext;
class SchemaParseContext;
class StringArrayContext;

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

  virtual void String(SchemaErrorLogger *logger, const std::string &value) = 0;
  virtual void Number(SchemaErrorLogger *logger, uint32_t value) = 0;
  virtual void Number(SchemaErrorLogger *logger, int32_t value) = 0;
  virtual void Number(SchemaErrorLogger *logger, uint64_t value) = 0;
  virtual void Number(SchemaErrorLogger *logger, int64_t value) = 0;
  virtual void Number(SchemaErrorLogger *logger, double value) = 0;
  virtual void Bool(SchemaErrorLogger *logger, bool value) = 0;
  virtual void Null(SchemaErrorLogger *logger) = 0;
  virtual SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger) = 0;
  virtual void CloseArray(SchemaErrorLogger *logger) = 0;
  virtual SchemaParseContextInterface* OpenObject(
      SchemaErrorLogger *logger) = 0;
  virtual void ObjectKey(SchemaErrorLogger *logger, const std::string &key) = 0;
  virtual void CloseObject(SchemaErrorLogger *logger) = 0;
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
  void ObjectKey(SchemaErrorLogger *logger, const std::string &keyword) {
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
  void String(SchemaErrorLogger *, const std::string &) {}
  void Number(SchemaErrorLogger *, uint32_t) {}
  void Number(SchemaErrorLogger *, int32_t) {}
  void Number(SchemaErrorLogger *, uint64_t) {}
  void Number(SchemaErrorLogger *, int64_t) {}
  void Number(SchemaErrorLogger *, double) {}
  void Bool(SchemaErrorLogger *, bool) {}
  void Null(SchemaErrorLogger *logger) { (void) logger; }

  SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger) {
    return NULL;
    (void) logger;
  }

  void CloseArray(SchemaErrorLogger *logger) {
    (void) logger;
  }

  SchemaParseContextInterface* OpenObject(SchemaErrorLogger *logger);
  void CloseObject(SchemaErrorLogger *logger);

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
  ValidatorInterface* GetValidator(SchemaErrorLogger *logger);

  void ObjectKey(SchemaErrorLogger *logger, const std::string &keyword);

  void String(SchemaErrorLogger *logger, const std::string &value);
  void Number(SchemaErrorLogger *logger, uint32_t value);
  void Number(SchemaErrorLogger *logger, int32_t value);
  void Number(SchemaErrorLogger *logger, uint64_t value);
  void Number(SchemaErrorLogger *logger, int64_t value);
  void Number(SchemaErrorLogger *logger, double value);
  void Bool(SchemaErrorLogger *logger, bool value);
  void Null(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger);
  void CloseArray(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenObject(SchemaErrorLogger *logger);
  void CloseObject(SchemaErrorLogger *logger);

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
  std::auto_ptr<JsonNumberValue> m_maximum;
  std::auto_ptr<JsonNumberValue> m_minimum;
  std::auto_ptr<JsonNumberValue> m_multiple_of;

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
  std::auto_ptr<StringArrayContext> m_required_items;
  std::auto_ptr<DependencyParseContext> m_dependency_context;

  // 5.5 Keywords for multiple instance types
  JsonType m_type;
  std::auto_ptr<class EnumParseContext> m_enum_context;

  // 6. Metadata keywords
  OptionalItem<std::string> m_description;
  OptionalItem<std::string> m_title;
  std::auto_ptr<const JsonValue> m_default_value;
  std::auto_ptr<DefaultValueParseContext> m_default_value_context;

  OptionalItem<std::string> m_ref_schema;

  // TODO(simon): Implement format support?
  OptionalItem<std::string> m_format;

  std::auto_ptr<DefinitionsParseContext> m_definitions_context;
  std::auto_ptr<PropertiesParseContext> m_properties_context;
  // vector<NumberConstraint> m_number_constraints;

  void ProcessPositiveInt(SchemaErrorLogger *logger, uint64_t value);

  template <typename T>
  void ProcessInt(SchemaErrorLogger *logger, T t);

  void AddNumberConstraints(IntegerValidator *validator);

  BaseValidator* BuildArrayValidator(SchemaErrorLogger *logger);
  BaseValidator* BuildObjectValidator(SchemaErrorLogger *logger);
  BaseValidator* BuildStringValidator(SchemaErrorLogger *logger);

  static bool ValidTypeForKeyword(SchemaErrorLogger *logger,
                                  SchemaKeyword keyword,
                                  JsonType type);
  // Verify that type == expected_type. If it doesn't report an error to the
  // logger.
  static bool CheckTypeAndLog(SchemaErrorLogger *logger, SchemaKeyword keyword,
                              JsonType type, JsonType expected_type);
  // Same as above but the type can be either expected_type1 or expected_type2
  static bool CheckTypeAndLog(SchemaErrorLogger *logger, SchemaKeyword keyword,
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
  ~PropertiesParseContext();

  void AddPropertyValidators(ObjectValidator *object_validator,
                              SchemaErrorLogger *logger);

  void String(SchemaErrorLogger *logger, const std::string &value);
  void Number(SchemaErrorLogger *logger, uint32_t value);
  void Number(SchemaErrorLogger *logger, int32_t value);
  void Number(SchemaErrorLogger *logger, uint64_t value);
  void Number(SchemaErrorLogger *logger, int64_t value);
  void Number(SchemaErrorLogger *logger, double value);
  void Bool(SchemaErrorLogger *logger, bool value);
  void Null(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger);
  void CloseArray(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenObject(SchemaErrorLogger *logger);
  void CloseObject(SchemaErrorLogger *logger);

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
  void AddValidators(SchemaErrorLogger *logger,
                     std::vector<ValidatorInterface*> *validators);

  void String(SchemaErrorLogger *logger, const std::string &value);
  void Number(SchemaErrorLogger *logger, uint32_t value);
  void Number(SchemaErrorLogger *logger, int32_t value);
  void Number(SchemaErrorLogger *logger, uint64_t value);
  void Number(SchemaErrorLogger *logger, int64_t value);
  void Number(SchemaErrorLogger *logger, double value);
  void Bool(SchemaErrorLogger *logger, bool value);
  void Null(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger);
  void CloseArray(SchemaErrorLogger *logger) {
    (void) logger;
  }
  SchemaParseContextInterface* OpenObject(SchemaErrorLogger *logger);
  void CloseObject(SchemaErrorLogger *logger) {
    (void) logger;
  }

 private:
  typedef std::vector<SchemaParseContext*> ItemSchemas;

  SchemaDefinitions *m_schema_defs;
  ItemSchemas m_item_schemas;

  void ReportErrorForType(SchemaErrorLogger *logger, JsonType type);

  DISALLOW_COPY_AND_ASSIGN(ArrayItemsParseContext);
};


/**
 * @brief Parse an array of strings.
 *
 * This is used for required and the property dependency list.
 */
class StringArrayContext : public BaseParseContext {
 public:
  typedef std::set<std::string> StringSet;

  StringArrayContext() : BaseParseContext() {}

  /**
   * @brief Return the strings in the string array
   */
  void GetStringSet(StringSet *stringd);

  void String(SchemaErrorLogger *logger, const std::string &value);
  void Number(SchemaErrorLogger *logger, uint32_t value);
  void Number(SchemaErrorLogger *logger, int32_t value);
  void Number(SchemaErrorLogger *logger, uint64_t value);
  void Number(SchemaErrorLogger *logger, int64_t value);
  void Number(SchemaErrorLogger *logger, double value);
  void Bool(SchemaErrorLogger *logger, bool value);
  void Null(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger);
  void CloseArray(SchemaErrorLogger *logger) {
    (void) logger;
  }
  SchemaParseContextInterface* OpenObject(SchemaErrorLogger *logger);
  void CloseObject(SchemaErrorLogger *logger) {
    (void) logger;
  }

 private:
  StringSet m_items;

  void ReportErrorForType(SchemaErrorLogger *logger, JsonType type);

  DISALLOW_COPY_AND_ASSIGN(StringArrayContext);
};

/**
 * @brief Parse a default value.
 *
 * Default values can be full fledged JSON documents. This context simply
 * passes the events through to a JsonParser to build the JsonValue.
 */
class DefaultValueParseContext : public SchemaParseContextInterface {
 public:
  DefaultValueParseContext();

  const JsonValue* ClaimValue(SchemaErrorLogger *logger);

  void String(SchemaErrorLogger *logger, const std::string &value);
  void Number(SchemaErrorLogger *logger, uint32_t value);
  void Number(SchemaErrorLogger *logger, int32_t value);
  void Number(SchemaErrorLogger *logger, uint64_t value);
  void Number(SchemaErrorLogger *logger, int64_t value);
  void Number(SchemaErrorLogger *logger, double value);
  void Bool(SchemaErrorLogger *logger, bool value);
  void Null(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger);
  void CloseArray(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenObject(SchemaErrorLogger *logger);
  void ObjectKey(SchemaErrorLogger *logger, const std::string &key);
  void CloseObject(SchemaErrorLogger *logger);

 private:
  JsonParser m_parser;

  DISALLOW_COPY_AND_ASSIGN(DefaultValueParseContext);
};

/**
 * @brief Parse an array of enums.
 *
 * Items in the array can be full fledged JSON documents.
 */
class EnumParseContext : public SchemaParseContextInterface {
 public:
  EnumParseContext() {}
  ~EnumParseContext();

  void AddEnumsToValidator(BaseValidator *validator);

  void String(SchemaErrorLogger *logger, const std::string &value);
  void Number(SchemaErrorLogger *logger, uint32_t value);
  void Number(SchemaErrorLogger *logger, int32_t value);
  void Number(SchemaErrorLogger *logger, uint64_t value);
  void Number(SchemaErrorLogger *logger, int64_t value);
  void Number(SchemaErrorLogger *logger, double value);
  void Bool(SchemaErrorLogger *logger, bool value);
  void Null(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger);
  void CloseArray(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenObject(SchemaErrorLogger *logger);
  void ObjectKey(SchemaErrorLogger*, const std::string &) {}
  void CloseObject(SchemaErrorLogger *logger);

  bool Empty() const { return m_enums.empty(); }

 private:
  std::vector<const JsonValue*> m_enums;
  std::auto_ptr<DefaultValueParseContext> m_value_context;

  void CheckForDuplicateAndAdd(SchemaErrorLogger *logger,
                               const JsonValue *value);

  DISALLOW_COPY_AND_ASSIGN(EnumParseContext);
};


/**
 * @brief Parse an object containing dependencies.
 *
 * Values in a dependency object can be either arrays of strings, or schema.
 */
class DependencyParseContext : public BaseParseContext {
 public:
  explicit DependencyParseContext(SchemaDefinitions *definitions)
    : m_schema_defs(definitions) {}
  ~DependencyParseContext();

  void AddDependenciesToValidator(ObjectValidator *validator);

  void String(SchemaErrorLogger *logger, const std::string &value);
  void Number(SchemaErrorLogger *logger, uint32_t value);
  void Number(SchemaErrorLogger *logger, int32_t value);
  void Number(SchemaErrorLogger *logger, uint64_t value);
  void Number(SchemaErrorLogger *logger, int64_t value);
  void Number(SchemaErrorLogger *logger, double value);
  void Bool(SchemaErrorLogger *logger, bool value);
  void Null(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenArray(SchemaErrorLogger *logger);
  void CloseArray(SchemaErrorLogger *logger);
  SchemaParseContextInterface* OpenObject(SchemaErrorLogger *logger);
  void CloseObject(SchemaErrorLogger *logger);

 private:
  typedef std::set<std::string> StringSet;
  typedef std::map<std::string, ValidatorInterface*> SchemaDependencies;
  typedef std::map<std::string, StringSet> PropertyDependencies;

  SchemaDefinitions *m_schema_defs;

  std::auto_ptr<StringArrayContext> m_property_context;
  std::auto_ptr<SchemaParseContext> m_schema_context;

  PropertyDependencies m_property_dependencies;
  SchemaDependencies m_schema_dependencies;

  void ReportErrorForType(SchemaErrorLogger *logger, JsonType type);

  DISALLOW_COPY_AND_ASSIGN(DependencyParseContext);
};
}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_SCHEMAPARSECONTEXT_H_
