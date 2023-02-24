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
#include <ola/web/JsonTypes.h>
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
class ValidatorInterface : public JsonValueConstVisitorInterface {
 public:
  /**
   * @brief a list of Validators.
   */
  typedef std::vector<ValidatorInterface*> ValidatorList;

  virtual ~ValidatorInterface() {}

  /**
   * @brief Check if the value was valid according to this validator.
   */
  virtual bool IsValid() const = 0;

  // Do we need a "GetError" here?

  /**
   * @brief Returns the Schema as a JsonObject.
   * @returns A new JsonObject that represents the schema. Ownership of the
   *   JsonObject is transferred to the caller.
   */
  virtual JsonObject* GetSchema() const = 0;

  /**
   * @brief Set the $schema property for this validator.
   */
  virtual void SetSchema(const std::string &schema) = 0;

  /**
   * @brief Set the id property for this validator.
   */
  virtual void SetId(const std::string &id) = 0;

  /**
   * @brief Set the title property for this validator.
   */
  virtual void SetTitle(const std::string &title) = 0;

  /**
   * @brief Set the description property for this validator.
   */
  virtual void SetDescription(const std::string &title) = 0;

  /**
   * @brief Set the default value for this validator.
   * @param value The JsonValue to use as the default. Ownership is
   *   transferred.
   */
  virtual void SetDefaultValue(const JsonValue *value) = 0;

  /**
   * @brief Return the default value for this validator.
   * @returns A JsonValue, or NULL if no default value was specified. Ownership
   *   is not transferred.
   *
   * The value is only valid until the next call to SetDefaultValue or for the
   * lifetime of the validator.
   */
  virtual const JsonValue *GetDefaultValue() const = 0;
};

/**
 * @brief The base class for Json BaseValidators.
 * All Visit methods return false.
 */
class BaseValidator : public ValidatorInterface {
 protected:
  explicit BaseValidator(JsonType type)
      : m_is_valid(true),
        m_type(type) {
  }

 public:
  virtual ~BaseValidator();

  virtual bool IsValid() const { return m_is_valid; }

  virtual void Visit(const JsonString&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonBool&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonNull&) {
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

  virtual void Visit(const JsonUInt&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonUInt64&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonInt&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonInt64&) {
    m_is_valid = false;
  }

  virtual void Visit(const JsonDouble&) {
    m_is_valid = false;
  }

  virtual JsonObject* GetSchema() const;

  /**
   * @brief Set the schema.
   */
  void SetSchema(const std::string &schema);

  /**
   * @brief Set the id.
   */
  void SetId(const std::string &id);

  /**
   * @brief Set the title.
   */
  void SetTitle(const std::string &title);

  /**
   * @brief Set the description.
   */
  void SetDescription(const std::string &title);

  /**
   * @brief Set the default value for this validator.
   * @param value the default value, ownership is transferred.
   */
  void SetDefaultValue(const JsonValue *value);

  /**
   * @brief Return the default value
   * @returns The default value, or NULL if there isn't one.
   */
  const JsonValue *GetDefaultValue() const;

  /**
   * @brief Add a enum value to the list of allowed values.
   * @param value the JsonValue to add, ownership is transferred.
   */
  void AddEnumValue(const JsonValue *value);

 protected:
  bool m_is_valid;
  JsonType m_type;
  std::string m_schema;
  std::string m_id;
  std::string m_title;
  std::string m_description;
  std::auto_ptr<const JsonValue> m_default_value;
  std::vector<const JsonValue*> m_enums;

  bool CheckEnums(const JsonValue &value);

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
  WildcardValidator() : BaseValidator(JSON_UNDEFINED) {}

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

  void Visit(const JsonString &value);
  void Visit(const JsonBool &value);
  void Visit(const JsonNull &value);
  void Visit(const JsonRawValue &value);
  void Visit(const JsonObject &value);
  void Visit(const JsonArray &value);
  void Visit(const JsonUInt &value);
  void Visit(const JsonUInt64 &value);
  void Visit(const JsonInt &value);
  void Visit(const JsonInt64 &value);
  void Visit(const JsonDouble &value);

  JsonObject* GetSchema() const;

  void SetSchema(const std::string &schema);
  void SetId(const std::string &id);
  void SetTitle(const std::string &title);
  void SetDescription(const std::string &title);
  void SetDefaultValue(const JsonValue *value);
  const JsonValue *GetDefaultValue() const;

 private:
  const SchemaDefinitions *m_definitions;
  const std::string m_schema;
  ValidatorInterface *m_validator;

  template <typename T>
  void Validate(const T &value);
};

/**
 * @brief The validator for JsonString.
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
      : BaseValidator(JSON_STRING),
        m_options(options) {
  }

  void Visit(const JsonString &str);

 private:
  const Options m_options;

  void ExtendSchema(JsonObject *schema) const;

  DISALLOW_COPY_AND_ASSIGN(StringValidator);
};

/**
 * @brief The validator for JsonBool.
 */
class BoolValidator : public BaseValidator {
 public:
  BoolValidator() : BaseValidator(JSON_BOOLEAN) {}

  void Visit(const JsonBool &value) { m_is_valid = CheckEnums(value); }

 private:
  DISALLOW_COPY_AND_ASSIGN(BoolValidator);
};

/**
 * @brief The validator for JsonNull.
 */
class NullValidator : public BaseValidator {
 public:
  NullValidator() : BaseValidator(JSON_NULL) {}

  void Visit(const JsonNull &value) { m_is_valid = CheckEnums(value); }

 private:
  DISALLOW_COPY_AND_ASSIGN(NullValidator);
};

/**
 * @brief The base class for constraints that can be applies to the Json number
 * type.
 */
class NumberConstraint {
 public:
  virtual ~NumberConstraint() {}

  virtual bool IsValid(const JsonNumber &value) = 0;

  virtual void ExtendSchema(JsonObject *schema) const = 0;
};

/**
 * @brief Confirms the valid is a multiple of the specified value.
 */
class MultipleOfConstraint : public NumberConstraint {
 public:
  explicit MultipleOfConstraint(const JsonNumber *value)
      : m_multiple_of(value) {
  }

  bool IsValid(const JsonNumber &value) {
    return value.MultipleOf(*m_multiple_of);
  }

  void ExtendSchema(JsonObject *schema) const {
    schema->AddValue("multipleOf", m_multiple_of->Clone());
  }

 private:
  std::auto_ptr<const JsonNumber> m_multiple_of;
};

/**
 * @brief Enforces a maximum
 */
class MaximumConstraint : public NumberConstraint {
 public:
  /**
   * @brief Create a new MaximumConstraint.
   * @param limit the maximum, ownership is transferred.
   * @param is_exclusive true is the limit is exclusive, false if not.
   */
  MaximumConstraint(const JsonNumber *limit, bool is_exclusive)
      : m_limit(limit),
        m_has_exclusive(true),
        m_is_exclusive(is_exclusive) {
  }

  /**
   * @brief Create a new MaximumConstraint.
   * @param limit the maximum, ownership is transferred.
   */
  explicit MaximumConstraint(const JsonNumber *limit)
      : m_limit(limit),
        m_has_exclusive(false),
        m_is_exclusive(false) {
  }

  bool IsValid(const JsonNumber &value) {
    return (m_has_exclusive && m_is_exclusive) ? value < *m_limit
        : value <= *m_limit;
  }

  void ExtendSchema(JsonObject *schema) const {
    schema->AddValue("maximum", m_limit->Clone());
    if (m_has_exclusive) {
      schema->Add("exclusiveMaximum", m_is_exclusive);
    }
  }

 private:
  std::auto_ptr<const JsonNumber> m_limit;
  bool m_has_exclusive, m_is_exclusive;
};

/**
 * @brief Enforces a minimum
 */
class MinimumConstraint : public NumberConstraint {
 public:
  /**
   * @brief Create a new MaximumConstraint.
   * @param limit the minimum, ownership is transferred.
   * @param is_exclusive true is the limit is exclusive, false if not.
   */
  MinimumConstraint(const JsonNumber *limit, bool is_exclusive)
      : m_limit(limit),
        m_has_exclusive(true),
        m_is_exclusive(is_exclusive) {
  }

  /**
   * @brief Create a new MaximumConstraint.
   * @param limit the minimum, ownership is transferred.
   */
  explicit MinimumConstraint(const JsonNumber *limit)
      : m_limit(limit),
        m_has_exclusive(false),
        m_is_exclusive(false) {
  }

  bool IsValid(const JsonNumber &value) {
    return (m_has_exclusive && m_is_exclusive) ? value > *m_limit
        : value >= *m_limit;
  }

  void ExtendSchema(JsonObject *schema) const {
    schema->AddValue("minimum", m_limit->Clone());
    if (m_has_exclusive) {
      schema->Add("exclusiveMinimum", m_is_exclusive);
    }
  }

 private:
  std::auto_ptr<const JsonNumber> m_limit;
  bool m_has_exclusive, m_is_exclusive;
};

/**
 * @brief The validator for Json integers.
 */
class IntegerValidator : public BaseValidator {
 public:
  IntegerValidator() : BaseValidator(JSON_INTEGER) {}
  virtual ~IntegerValidator();

  /**
   * @brief Add a constraint to this validator.
   * @param constraint the constraint to add, ownership is transferred.
   */
  void AddConstraint(NumberConstraint *constraint);

  void Visit(const JsonUInt&);
  void Visit(const JsonInt&);
  void Visit(const JsonUInt64&);
  void Visit(const JsonInt64&);
  virtual void Visit(const JsonDouble&);

 protected:
  explicit IntegerValidator(JsonType type) : BaseValidator(type) {}

  void CheckValue(const JsonNumber &value);

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
  NumberValidator() : IntegerValidator(JSON_NUMBER) {}

  void Visit(const JsonDouble&);

 private:
  DISALLOW_COPY_AND_ASSIGN(NumberValidator);
};

/**
 * @brief The validator for JsonObject.
 *
 * @note This does not implement patternProperties. This is done because we
 * don't want to pull in a regex library just yet. Maybe some point in the
 * future we can use re2.
 */
class ObjectValidator : public BaseValidator, JsonObjectPropertyVisitor {
 public:
  struct Options {
    Options()
      : max_properties(-1),
        min_properties(0),
        has_required_properties(false),
        has_allow_additional_properties(false),
        allow_additional_properties(false) {
    }

    void SetRequiredProperties(
        const std::set<std::string> &required_properties_arg) {
      required_properties = required_properties_arg;
      has_required_properties = true;
    }

    void SetAdditionalProperties(bool allow_additional) {
      has_allow_additional_properties = true;
      allow_additional_properties = allow_additional;
    }

    int max_properties;
    unsigned int min_properties;
    bool has_required_properties;
    std::set<std::string> required_properties;
    bool has_allow_additional_properties;
    bool allow_additional_properties;
  };

  explicit ObjectValidator(const Options &options);
  ~ObjectValidator();

  /**
   * @brief Add a validator for a property.
   * @param property the name of the property to use this validator for
   * @param validator the validator, ownership is transferred.
   */
  void AddValidator(const std::string &property, ValidatorInterface *validator);

  /**
   * @brief Add the validator for additionalProperties.
   * @param validator the validator, ownership is transferred.
   */
  void SetAdditionalValidator(ValidatorInterface *validator);

  /**
   * @brief Add a schema dependency.
   * @param property the property name
   * @param validator the validator to check the object against. Ownership is
   *   transferred.
   *
   * As per Section 5.4.5.2.1, if the named property is present in the object,
   * the object itself will be validated against the supplied schema.
   */
  void AddSchemaDependency(const std::string &property,
                           ValidatorInterface *validator);

  /**
   * @brief Add a property dependency.
   * @param property the property name
   * @param properties the names of the properties that must exist in the
   *   object if the named property is present.
   *
   * As per Section 5.4.5.2.2, if the named property is present in the object,
   * the object must also contain all of the properties in the set.
   */
  void AddPropertyDependency(const std::string &property,
                             const std::set<std::string> &properties);

  void Visit(const JsonObject &obj);

  void VisitProperty(const std::string &property, const JsonValue &value);

 private:
  typedef std::set<std::string> StringSet;
  typedef std::map<std::string, ValidatorInterface*> PropertyValidators;
  typedef std::map<std::string, ValidatorInterface*> SchemaDependencies;
  typedef std::map<std::string, StringSet> PropertyDependencies;

  const Options m_options;

  PropertyValidators m_property_validators;
  std::auto_ptr<ValidatorInterface> m_additional_property_validator;
  PropertyDependencies m_property_dependencies;
  SchemaDependencies m_schema_dependencies;

  StringSet m_seen_properties;

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
   * @param items The items in the array, ownership is transferred.
   * @param additional_items Any additional (optional) items. ownership is
   *   transferred.
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

    void Visit(const JsonString&);
    void Visit(const JsonBool&);
    void Visit(const JsonNull&);
    void Visit(const JsonRawValue&);
    void Visit(const JsonObject&);
    void Visit(const JsonArray &array);
    void Visit(const JsonUInt&);
    void Visit(const JsonUInt64&);
    void Visit(const JsonInt&);
    void Visit(const JsonInt64&);
    void Visit(const JsonDouble&);

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

  void Visit(const JsonString &value) {
    Validate(value);
  }

  void Visit(const JsonBool &value) {
    Validate(value);
  }

  void Visit(const JsonNull &value) {
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

  void Visit(const JsonUInt &value) {
    Validate(value);
  }

  void Visit(const JsonUInt64 &value) {
    Validate(value);
  }

  void Visit(const JsonInt &value) {
    Validate(value);
  }

  void Visit(const JsonInt64 &value) {
    Validate(value);
  }

  void Visit(const JsonDouble &value) {
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
      : BaseValidator(JSON_UNDEFINED),
        m_validator(validator) {
  }

  void Visit(const JsonString &value) {
    Validate(value);
  }

  void Visit(const JsonBool &value) {
    Validate(value);
  }

  void Visit(const JsonNull &value) {
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

  void Visit(const JsonUInt &value) {
    Validate(value);
  }

  void Visit(const JsonUInt64 &value) {
    Validate(value);
  }

  void Visit(const JsonInt &value) {
    Validate(value);
  }

  void Visit(const JsonInt64 &value) {
    Validate(value);
  }

  void Visit(const JsonDouble &value) {
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
