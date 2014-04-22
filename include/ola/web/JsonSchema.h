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
 * JsonSchema.h
 * A Json Schema, see json-schema.org
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonSchema.h
 * @brief A Json Schema, see www.json-schema.org
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONSCHEMA_H_
#define INCLUDE_OLA_WEB_JSONSCHEMA_H_

#include <ola/base/Macro.h>
#include <ola/web/Json.h>
#include <deque>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ola {
namespace web {

/**
 * @addtogroup json
 * @{
 */

class SchemaDefintions;

/**
 * @brief The interface Json Schema Validators.
 */
class ValidatorInterface : public JsonValueVisitorInterface {
 public:
  typedef std::vector<ValidatorInterface*> ValidatorList;

  virtual ~ValidatorInterface() {}

  virtual bool IsValid() const = 0;

  // Do we need a "GetError" here?

  virtual void Visit(const JsonStringValue&) = 0;
  virtual void Visit(const JsonBoolValue&) = 0;
  virtual void Visit(const JsonNullValue&) = 0;
  virtual void Visit(const JsonRawValue&) = 0;
  virtual void Visit(const JsonObject&) = 0;
  virtual void Visit(const JsonArray&) = 0;
  virtual void Visit(const JsonUIntValue&) = 0;
  virtual void Visit(const JsonUInt64Value&) = 0;
  virtual void Visit(const JsonIntValue&) = 0;
  virtual void Visit(const JsonInt64Value&) = 0;
  virtual void Visit(const JsonDoubleValue&) = 0;
};

/**
 * @brief The base class for Json BaseValidators.
 * All Visit methods return false.
 */
class BaseValidator : public ValidatorInterface {
 public:
  BaseValidator()
      : m_is_valid(true) {
  }

  virtual ~BaseValidator() {}

  virtual bool IsValid() const { return m_is_valid; }

  virtual void Visit(const JsonStringValue&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonBoolValue&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonNullValue&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonRawValue&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonObject&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonArray&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonUIntValue&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonUInt64Value&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonIntValue&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonInt64Value&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonDoubleValue&) {
    m_is_valid = false;
  }

 protected:
  bool m_is_valid;
};

/**
 * @brief The wildcard validator matches everything.
 * This corresponds to the empty schema, i.e. {}
 */
class WildcardValidator : public BaseValidator {
 public:
  WildcardValidator() : BaseValidator() {}

  bool IsValid() const { return true; }
};

/**
 * @brief A reference validator holds a pointer to another schema.
 */
class ReferenceValidator : public ValidatorInterface {
 public:
  /**
   * @param definitions A SchemaDefintions object with which to resolve
   * references.
   * @param schema The $ref link to the other schema.
   */
  ReferenceValidator(const SchemaDefintions *definitions,
                     const std::string &schema);

  bool IsValid() const;

  void Visit(const JsonStringValue &value);
  void Visit(const JsonBoolValue &value);
  void Visit(const JsonNullValue &value);
  void Visit(const JsonRawValue &value);
  void Visit(const JsonObject &value);
  void Visit(const JsonArray &value);
  void Visit(const JsonUIntValue &value);
  void Visit(const JsonUInt64Value &value);
  void Visit(const JsonIntValue &value);
  void Visit(const JsonInt64Value &value);
  void Visit(const JsonDoubleValue &value);

 private:
  const SchemaDefintions *m_definitions;
  const std::string m_schema;
  ValidatorInterface *m_validator;

  template <typename T>
  void Validate(const T &value);
};

/**
 * @brief The validator for JsonStringValue.
 */
class StringValidator : public BaseValidator {
 public:
  struct Options {
    Options()
      : min_length(0),
        max_length(-1) {
    }

    unsigned int min_length;
    int max_length;
    // Regexes aren't supported.
    // std::string pattern
  };

  explicit StringValidator(const Options &options)
      : BaseValidator(),
        m_options(options) {
  }

  void Visit(const JsonStringValue &str);

 private:
  const Options m_options;
  DISALLOW_COPY_AND_ASSIGN(StringValidator);
};

/**
 * @brief The validator for JsonBoolValue.
 */
class BoolValidator : public BaseValidator {
 public:
  BoolValidator() : BaseValidator() {}

  void Visit(const JsonBoolValue&) { m_is_valid = true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(BoolValidator);
};

/**
 * @brief The validator for JsonNullValue.
 */
class NullValidator : public BaseValidator {
 public:
  NullValidator() : BaseValidator() {}

  void Visit(const JsonNullValue&) { m_is_valid = true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(NullValidator);
};

/**
 * @brief The base class for contraints that can be applies to the Json number
 * type.
 */
class NumberConstraint {
 public:
  virtual ~NumberConstraint() {}

  virtual bool IsValid(int i) = 0;
  virtual bool IsValid(unsigned int i) = 0;
  virtual bool IsValid(uint64_t i) = 0;
  virtual bool IsValid(int64_t i) = 0;
  virtual bool IsValid(long double d) = 0;
};

/**
 * @brief Confirms the valid is a multiple of the specified value.
 */
class MultipleOfConstraint : public NumberConstraint {
 public:
  MultipleOfConstraint(int multiple_of)
      : m_multiple_of(multiple_of) {
  }

  bool IsValid(int i) {
    return (i % m_multiple_of == 0);
  }

  bool IsValid(unsigned int i) {
    return (i % m_multiple_of == 0);
  }

  bool IsValid(uint64_t i) {
    return (i % m_multiple_of == 0);
  }
  bool IsValid(int64_t i) {
    return (i % m_multiple_of == 0);
  }

  bool IsValid(long double d);

 private:
  int m_multiple_of;
};

/**
 * @brief Enforces a maximum
 */
class MaximumConstraint : public NumberConstraint {
 public:
  MaximumConstraint(int limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  MaximumConstraint(unsigned int limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  MaximumConstraint(uint64_t limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  MaximumConstraint(int64_t limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  MaximumConstraint(long double limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  bool IsValid(int i) {
    return m_is_exclusive ? i < m_limit : i <= m_limit;
  }

  bool IsValid(unsigned int i) {
    return m_is_exclusive ? i < m_limit : i <= m_limit;
  }

  bool IsValid(uint64_t i) {
    return m_is_exclusive ? i < m_limit : i <= m_limit;
  }
  bool IsValid(int64_t i) {
    return m_is_exclusive ? i < m_limit : i <= m_limit;
  }

  bool IsValid(long double d) {
    return m_is_exclusive ? d < m_limit : d <= m_limit;
  }

 private:
  long double m_limit;
  bool m_is_exclusive;
};

/**
 * @brief Enforces a minimum
 */
class MinimumConstraint : public NumberConstraint {
 public:
  MinimumConstraint(int limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  MinimumConstraint(unsigned int limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  MinimumConstraint(uint64_t limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  MinimumConstraint(int64_t limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  MinimumConstraint(long double limit, bool is_exclusive)
      : m_limit(limit),
        m_is_exclusive(is_exclusive) {
  }

  bool IsValid(int i) {
    return m_is_exclusive ? i > m_limit : i >= m_limit;
  }

  bool IsValid(unsigned int i) {
    return m_is_exclusive ? i > m_limit : i >= m_limit;
  }

  bool IsValid(uint64_t i) {
    return m_is_exclusive ? i > m_limit : i >= m_limit;
  }
  bool IsValid(int64_t i) {
    return m_is_exclusive ? i > m_limit : i >= m_limit;
  }

  bool IsValid(long double d) {
    return m_is_exclusive ? d > m_limit : d >= m_limit;
  }

 private:
  long double m_limit;
  bool m_is_exclusive;
};

/**
 * @brief The validator for Json numbers.
 */
class NumberValidator : public BaseValidator {
 public:
  NumberValidator() : BaseValidator() {}
  ~NumberValidator();

  /**
   * @brief Add a constraint to this validator.
   * @param constraint the contraint to add, ownership is transferred.
   */
  void AddConstraint(NumberConstraint *constraint);

  void Visit(const JsonUIntValue&);
  void Visit(const JsonIntValue&);
  void Visit(const JsonUInt64Value&);
  void Visit(const JsonInt64Value&);
  void Visit(const JsonDoubleValue&);

 private:
  std::vector<NumberConstraint*> m_constraints;

  template <typename T>
  void CheckValue(T t);

  DISALLOW_COPY_AND_ASSIGN(NumberValidator);
};

/**
 * @brief The validator for JsonObject.
 */
class ObjectValidator : public BaseValidator, JsonObjectPropertyVisitor {
 public:
  struct Options {
    Options()
      : max_properties(-1),
        min_properties(0) {
    }

    int max_properties;
    unsigned int min_properties;
    std::set<std::string> required_properties;
  };
  ObjectValidator(const Options &options);
  ~ObjectValidator();

  /**
   * @brief Add a validator for a property.
   * @param property the name of the property to use this validator for
   * @param validator the validator, ownership is transferred.
   */
  void AddValidator(const std::string &property, ValidatorInterface *validator);

  void Visit(const JsonObject &obj);

  void VisitProperty(const std::string &property, const JsonValue &value);

 private:
  const Options m_options;

  std::map<std::string, ValidatorInterface*> m_property_validators;
  std::set<std::string> m_seen_properties;

  DISALLOW_COPY_AND_ASSIGN(ObjectValidator);
};

/**
 * @brief The validator for JsonArray.
 */
class ArrayValidator : public BaseValidator {
 public:
  struct Options {
    Options()
      : max_items(-1),
        min_items(0) {
    }

    int max_items;
    unsigned int min_items;
    bool unique_items;
  };

  /**
   * @brief Validate all elements of the array against the given schema.
   * @param validator The schema to validate elements against, ownership is
   * transferred.
   * @param options Extra contstraints on the Array.
   */
  ArrayValidator(ValidatorInterface *validator, const Options &options);

  /**
   * @brief Validate the first N items against the schemas in the
   * ValidatorList, and the remainder against schema.
   * @param validators the list of schemas to validate the first N items
   *   against. Ownership of the validators in the list is transferred.
   * @param schema the schema used to validate any additional items. If null,
   *   additional items are forbidden. Ownership is transferred.
   * @param options Extra contstraints on the Array.
   */
  ArrayValidator(ValidatorList *validators, ValidatorInterface *schema,
                 const Options &options);

  ~ArrayValidator();

  void Visit(const JsonArray &array);

 private:
  typedef std::deque<ValidatorInterface*> ValidatorQueue;

  ValidatorList m_validators;
  const std::auto_ptr<ValidatorInterface> m_default_validator;
  const Options m_options;

  class ArrayElementValidator : public BaseValidator {
   public:
    ArrayElementValidator(const ValidatorList &validators,
                          ValidatorInterface *default_validator);

    void Visit(const JsonStringValue&);
    void Visit(const JsonBoolValue&);
    void Visit(const JsonNullValue&);
    void Visit(const JsonRawValue&);
    void Visit(const JsonObject&);
    void Visit(const JsonArray &array);
    void Visit(const JsonUIntValue&);
    void Visit(const JsonUInt64Value&);
    void Visit(const JsonIntValue&);
    void Visit(const JsonInt64Value&);
    void Visit(const JsonDoubleValue&);

   private:
    ValidatorQueue m_item_validators;
    ValidatorInterface *m_default_validator;

    template <typename T>
    void ValidateItem(const T &item);

    DISALLOW_COPY_AND_ASSIGN(ArrayElementValidator);
  };

  DISALLOW_COPY_AND_ASSIGN(ArrayValidator);
};


/**
 * @brief The base class for validators that operate with a list of child
 * validators (allOf, anyOf, oneOf).
 */
class ConjunctionValidator : public BaseValidator {
 public:
  /**
   * @brief
   * @param validators the list of schemas to validate against. Ownership of
   *   the validators in the list is transferred.
   */
  ConjunctionValidator(ValidatorList *validators);
  virtual ~ConjunctionValidator();

  void Visit(const JsonStringValue &value) {
    Validate(value);
  }

  void Visit(const JsonBoolValue &value) {
    Validate(value);
  }

  void Visit(const JsonNullValue &value) {
    Validate(value);
  }

  void Visit(const JsonRawValue &value) {
    Validate(value);
  }

  void Visit(const JsonObject &value) {
    Validate(value);
  }

  void Visit(const JsonArray &value) {
    Validate(value);
  }

  void Visit(const JsonUIntValue &value) {
    Validate(value);
  }

  void Visit(const JsonUInt64Value &value) {
    Validate(value);
  }

  void Visit(const JsonIntValue &value) {
    Validate(value);
  }

  void Visit(const JsonInt64Value &value) {
    Validate(value);
  }

  void Visit(const JsonDoubleValue &value) {
    Validate(value);
  }

 protected:
  ValidatorList m_validators;

  virtual void Validate(const JsonValue &value) = 0;
};

/**
 * @brief A validator which ensures all child validators pass (allOf).
 */
class AllOfValidator : public ConjunctionValidator {
 public:
  /**
   * @brief
   * @param validators the list of schemas to validate against. Ownership of
   *   the validators in the list is transferred.
   */
  AllOfValidator(ValidatorList *validators)
      : ConjunctionValidator(validators) {
  }

 protected:
  void Validate(const JsonValue &value);

 private:
  DISALLOW_COPY_AND_ASSIGN(AllOfValidator);
};

/**
 * @brief A validator which ensures at least one of the child validators pass
 * (anyOf).
 */
class AnyOfValidator : public ConjunctionValidator {
 public:
  /**
   * @brief
   * @param validators the list of schemas to validate against. Ownership of
   *   the validators in the list is transferred.
   */
  AnyOfValidator(ValidatorList *validators)
      : ConjunctionValidator(validators) {
  }

 protected:
  void Validate(const JsonValue &value);

 private:
  DISALLOW_COPY_AND_ASSIGN(AnyOfValidator);
};

/**
 * @brief A validator which ensures at only one of the child validators pass
 * (oneOf).
 */
class OneOfValidator : public ConjunctionValidator {
 public:
  /**
   * @brief
   * @param validators the list of schemas to validate against. Ownership of
   *   the validators in the list is transferred.
   */
  OneOfValidator(ValidatorList *validators)
      : ConjunctionValidator(validators) {
  }

 protected:
  void Validate(const JsonValue &value);

 private:
  DISALLOW_COPY_AND_ASSIGN(OneOfValidator);
};

/**
 * @brief A validator that inverts the result of the child (not).
 */
class NotValidator : public BaseValidator {
 public:
  NotValidator(ValidatorInterface *validator)
      : BaseValidator(),
        m_validator(validator) {
  }

  void Visit(const JsonStringValue &value) {
    Validate(value);
  }

  void Visit(const JsonBoolValue &value) {
    Validate(value);
  }

  void Visit(const JsonNullValue &value) {
    Validate(value);
  }

  void Visit(const JsonRawValue &value) {
    Validate(value);
  }

  void Visit(const JsonObject &value) {
    Validate(value);
  }

  void Visit(const JsonArray &value) {
    Validate(value);
  }

  void Visit(const JsonUIntValue &value) {
    Validate(value);
  }

  void Visit(const JsonUInt64Value &value) {
    Validate(value);
  }

  void Visit(const JsonIntValue &value) {
    Validate(value);
  }

  void Visit(const JsonInt64Value &value) {
    Validate(value);
  }

  void Visit(const JsonDoubleValue &value) {
    Validate(value);
  }

 private:
  std::auto_ptr<ValidatorInterface> m_validator;

  void Validate(const JsonValue &value);

  DISALLOW_COPY_AND_ASSIGN(NotValidator);
};

/**
 * @brief A JsonHandlerInterface implementation that builds a parse tree.
class JsonSchema {
 public:
  JsonSchema() {}
  ~JsonSchema();

   * @brief The URI which defines which version of the schema this is
  std::string SchemaURI() const;

  std::string Description();
  std::string Type();

  bool Visit(const JsonValue *value);

 private:
  bool VisitMember(const std::string &property, const JsonObject &object) {
    BaseValidator *validator = m_validators[property];
    validator->Visit(object);
  }

  bool VisitMember(const std::string &property, const JsonArray &array);
  // etc..

  DISALLOW_COPY_AND_ASSIGN(JsonSchema);
};
 */


class SchemaDefintions {
 public:
  SchemaDefintions() {}
  ~SchemaDefintions();

  void Add(const std::string &schema_name, ValidatorInterface *validator);
  ValidatorInterface *Lookup(const std::string &schema_name) const;

 private:
  std::map<std::string, ValidatorInterface*> m_validators;


};
/*
class JsonSchemaCache {
 public:
  JsonSchemaCache() {}

   * @brief Add a schema to the cache.
   * @param schema the JsonSchema to add, ownership is transferred
  void AddSchema(const std::string& schema_uri, JsonSchema *schema);

  JsonSchema* Lookup(const std::string& schema_uri) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(JsonSchemaCache);
};
*/

/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONSCHEMA_H_
