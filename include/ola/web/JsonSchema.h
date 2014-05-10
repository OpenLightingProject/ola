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
#include <ola/stl/STLUtils.h>
#include <ola/web/Json.h>
#include <deque>
#include <map>
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

class SchemaDefinitions;

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

  // Returns the Schema as a JsonObject.
  virtual JsonObject* GetSchema() const = 0;

  virtual void SetSchema(const std::string &schema) = 0;
  virtual void SetId(const std::string &id) = 0;
  virtual void SetTitle(const std::string &title) = 0;
  virtual void SetDescription(const std::string &title) = 0;
};

/**
 * @brief The base class for Json BaseValidators.
 * All Visit methods return false.
 */
class BaseValidator : public ValidatorInterface {
 protected:
  enum JsonType {
    ARRAY_TYPE,
    BOOLEAN_TYPE,
    INTEGER_TYPE,
    NULL_TYPE,
    NUMBER_TYPE,
    OBJECT_TYPE,
    STRING_TYPE,
    NONE_TYPE,
  };

  explicit BaseValidator(JsonType type)
      : m_is_valid(true),
        m_type(type) {
  }

 public:
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

  virtual JsonObject* GetSchema() const;

  void SetSchema(const std::string &schema);
  void SetId(const std::string &id);
  void SetTitle(const std::string &title);
  void SetDescription(const std::string &title);

 protected:
  bool m_is_valid;
  JsonType m_type;
  std::string m_schema;
  std::string m_id;
  std::string m_title;
  std::string m_description;

  // Child classes can hook in here to extend the schema.
  virtual void ExtendSchema(JsonObject *schema) const {
    (void) schema;
  }
};

/**
 * @brief The wildcard validator matches everything.
 * This corresponds to the empty schema, i.e. {}
 */
class WildcardValidator : public BaseValidator {
 public:
  WildcardValidator() : BaseValidator(NONE_TYPE) {}

  bool IsValid() const { return true; }
};

/**
 * @brief A reference validator holds a pointer to another schema.
 */
class ReferenceValidator : public ValidatorInterface {
 public:
  /**
   * @param definitions A SchemaDefinitions object with which to resolve
   * references.
   * @param schema The $ref link to the other schema.
   */
  ReferenceValidator(const SchemaDefinitions *definitions,
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

  JsonObject* GetSchema() const;

  void SetSchema(const std::string &schema);
  void SetId(const std::string &id);
  void SetTitle(const std::string &title);
  void SetDescription(const std::string &title);

 private:
  const SchemaDefinitions *m_definitions;
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
    // Formats & Regexes aren't supported.
    // std::string pattern
    // std::string format
  };

  explicit StringValidator(const Options &options)
      : BaseValidator(STRING_TYPE),
        m_options(options) {
  }

  void Visit(const JsonStringValue &str);

 private:
  const Options m_options;

  void ExtendSchema(JsonObject *schema) const;

  DISALLOW_COPY_AND_ASSIGN(StringValidator);
};

/**
 * @brief The validator for JsonBoolValue.
 */
class BoolValidator : public BaseValidator {
 public:
  BoolValidator() : BaseValidator(BOOLEAN_TYPE) {}

  void Visit(const JsonBoolValue&) { m_is_valid = true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(BoolValidator);
};

/**
 * @brief The validator for JsonNullValue.
 */
class NullValidator : public BaseValidator {
 public:
  NullValidator() : BaseValidator(NULL_TYPE) {}

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
  virtual bool IsValid(double d) = 0;

  virtual void ExtendSchema(JsonObject *schema) const = 0;
};

/**
 * @brief Confirms the valid is a multiple of the specified value.
 */
class MultipleOfConstraint : public NumberConstraint {
 public:
  explicit MultipleOfConstraint(int multiple_of)
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

  bool IsValid(double d);

  void ExtendSchema(JsonObject *schema) const {
    schema->Add("multipleOf", m_multiple_of);
  }

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

  MaximumConstraint(double limit, bool is_exclusive)
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

  bool IsValid(double d) {
    return m_is_exclusive ? d < m_limit : d <= m_limit;
  }

  void ExtendSchema(JsonObject *schema) const {
    schema->Add("maximum", m_limit);
    if (m_is_exclusive) {
      schema->Add("exclusiveMaximum", true);
    }
  }

 private:
  double m_limit;
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

  MinimumConstraint(double limit, bool is_exclusive)
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

  bool IsValid(double d) {
    return m_is_exclusive ? d > m_limit : d >= m_limit;
  }

  void ExtendSchema(JsonObject *schema) const {
    schema->Add("minimum", m_limit);
    if (m_is_exclusive) {
      schema->Add("exclusiveMinimum", true);
    }
  }

 private:
  double m_limit;
  bool m_is_exclusive;
};

/**
 * @brief The validator for Json integers.
 */
class IntegerValidator : public BaseValidator {
 public:
  IntegerValidator() : BaseValidator(INTEGER_TYPE) {}
  virtual ~IntegerValidator();

  /**
   * @brief Add a constraint to this validator.
   * @param constraint the contraint to add, ownership is transferred.
   */
  void AddConstraint(NumberConstraint *constraint);

  void Visit(const JsonUIntValue&);
  void Visit(const JsonIntValue&);
  void Visit(const JsonUInt64Value&);
  void Visit(const JsonInt64Value&);
  virtual void Visit(const JsonDoubleValue&);

 protected:
  explicit IntegerValidator(JsonType type) : BaseValidator(type) {}

  template <typename T>
  void CheckValue(T t);

 private:
  std::vector<NumberConstraint*> m_constraints;

  void ExtendSchema(JsonObject *schema) const;

  DISALLOW_COPY_AND_ASSIGN(IntegerValidator);
};


/**
 * @brief The validator for Json numbers.
 *
 * This is an IntegerValidator that is extended to allow doubles.
 */
class NumberValidator : public IntegerValidator {
 public:
  NumberValidator() : IntegerValidator(NUMBER_TYPE) {}

  void Visit(const JsonDoubleValue&);

 private:
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
        min_properties(0),
        has_required_properties(false) {
    }

    void SetRequiredProperties(
        const std::set<std::string> &required_properties_arg) {
      required_properties = required_properties_arg;
      has_required_properties = true;
    }

    int max_properties;
    unsigned int min_properties;
    bool has_required_properties;
    std::set<std::string> required_properties;
  };

  explicit ObjectValidator(const Options &options);
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
  typedef std::map<std::string, ValidatorInterface*> PropertyValidators;
  const Options m_options;

  std::map<std::string, ValidatorInterface*> m_property_validators;
  std::set<std::string> m_seen_properties;

  void ExtendSchema(JsonObject *schema) const;

  DISALLOW_COPY_AND_ASSIGN(ObjectValidator);
};

/**
 * @brief The validator for JsonArray.
 */
class ArrayValidator : public BaseValidator {
 public:
   /**
    * The items parameter. This can be either a validator or a list of
    * validators.
    */
  class Items {
   public:
    explicit Items(ValidatorInterface *validator)
      : m_validator(validator) {
    }

    explicit Items(ValidatorList *validators)
      : m_validator(NULL),
        m_validator_list(*validators) {
    }

    ~Items() {
      STLDeleteElements(&m_validator_list);
    }

    ValidatorInterface* Validator() const { return m_validator.get(); }
    const ValidatorList& Validators() const { return m_validator_list; }

   private:
    std::auto_ptr<ValidatorInterface> m_validator;
    ValidatorList m_validator_list;

    DISALLOW_COPY_AND_ASSIGN(Items);
  };

  /**
   * The additionalItems parameter. This can be either a bool or a validator.
   */
  class AdditionalItems {
   public:
    explicit AdditionalItems(bool allow_additional)
        : m_allowed(allow_additional),
          m_validator(NULL) {
    }

    explicit AdditionalItems(ValidatorInterface *validator)
        : m_allowed(true),
          m_validator(validator) {
    }

    ValidatorInterface* Validator() const { return m_validator.get(); }
    bool AllowAdditional() const { return m_allowed; }

   private:
    bool m_allowed;
    std::auto_ptr<ValidatorInterface> m_validator;

    DISALLOW_COPY_AND_ASSIGN(AdditionalItems);
  };

  struct Options {
    Options()
      : max_items(-1),
        min_items(0),
        unique_items(false) {
    }

    int max_items;
    unsigned int min_items;
    bool unique_items;
  };

  /**
   * @brief Validate all elements of the array against the given schema.
   * @param items  , ownership is transferred.
   * @param additional_items , ownership is transferred.
   * @param options Extra constraints on the Array.
   */
  ArrayValidator(Items *items, AdditionalItems *additional_items,
                 const Options &options);

  ~ArrayValidator();

  void Visit(const JsonArray &array);

 private:
  typedef std::deque<ValidatorInterface*> ValidatorQueue;

  const std::auto_ptr<Items> m_items;
  const std::auto_ptr<AdditionalItems> m_additional_items;
  const Options m_options;

  // This is used if items is missing, or if additionalItems is true.
  std::auto_ptr<WildcardValidator> m_wildcard_validator;

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

  void ExtendSchema(JsonObject *schema) const;
  ArrayElementValidator* ConstructElementValidator() const;

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
   * @param keyword the name of the conjunction.
   * @param validators the list of schemas to validate against. Ownership of
   *   the validators in the list is transferred.
   */
  ConjunctionValidator(const std::string &keyword, ValidatorList *validators);
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
  std::string m_keyword;
  ValidatorList m_validators;

  void ExtendSchema(JsonObject *schema) const;

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
  explicit AllOfValidator(ValidatorList *validators)
      : ConjunctionValidator("allOf", validators) {
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
  explicit AnyOfValidator(ValidatorList *validators)
      : ConjunctionValidator("anyOf", validators) {
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
  explicit OneOfValidator(ValidatorList *validators)
      : ConjunctionValidator("oneOf", validators) {
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
  explicit NotValidator(ValidatorInterface *validator)
      : BaseValidator(NONE_TYPE),
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

  void ExtendSchema(JsonObject *schema) const;

  DISALLOW_COPY_AND_ASSIGN(NotValidator);
};

class SchemaDefinitions {
 public:
  SchemaDefinitions() {}
  ~SchemaDefinitions();

  void Add(const std::string &schema_name, ValidatorInterface *validator);
  ValidatorInterface *Lookup(const std::string &schema_name) const;

  void AddToJsonObject(JsonObject *json) const;

  bool HasDefinitions() const { return !m_validators.empty(); }

 private:
  typedef std::map<std::string, ValidatorInterface*> SchemaMap;

  SchemaMap m_validators;

  DISALLOW_COPY_AND_ASSIGN(SchemaDefinitions);
};


/**
 * @brief A JsonHandlerInterface implementation that builds a parse tree.
 */
class JsonSchema {
 public:
  ~JsonSchema() {}

  /*
   * @brief The URI which defines which version of the schema this is.
   */
  std::string SchemaURI() const;

  /**
   * @brief Validate a JsonValue against this schema
   */
  bool IsValid(const JsonValue &value);

  /**
   * @brief Return the schema as Json.
   */
  const JsonObject* AsJson() const;

  /**
   * @brief Parse a string and return a new schema
   * @returns A JsonSchema object, or NULL if the string wasn't a valid schema.
   */
  static JsonSchema* FromString(const std::string& schema_string,
                                std::string *error);

 private:
  std::string m_schema_uri;
  std::auto_ptr<ValidatorInterface> m_root_validator;
  std::auto_ptr<SchemaDefinitions> m_schema_defs;

  JsonSchema(const std::string &schema_url,
             ValidatorInterface *root_validator,
             SchemaDefinitions *schema_defs);

  DISALLOW_COPY_AND_ASSIGN(JsonSchema);
};

/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONSCHEMA_H_
