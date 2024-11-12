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
 * SchemaTest.cpp
 * Unittest for the Json Schema.
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "ola/web/Json.h"
#include "ola/web/JsonParser.h"
#include "ola/web/JsonSchema.h"

using ola::web::AllOfValidator;
using ola::web::AnyOfValidator;
using ola::web::ArrayValidator;
using ola::web::BoolValidator;
using ola::web::ConjunctionValidator;
using ola::web::IntegerValidator;
using ola::web::JsonBool;
using ola::web::JsonDouble;
using ola::web::JsonInt;
using ola::web::JsonNull;
using ola::web::JsonParser;
using ola::web::JsonString;
using ola::web::JsonString;
using ola::web::JsonUInt;
using ola::web::JsonUInt;
using ola::web::JsonValue;
using ola::web::MaximumConstraint;
using ola::web::MinimumConstraint;
using ola::web::MultipleOfConstraint;
using ola::web::NotValidator;
using ola::web::NullValidator;
using ola::web::NumberValidator;
using ola::web::ObjectValidator;
using ola::web::OneOfValidator;
using ola::web::ReferenceValidator;
using ola::web::SchemaDefinitions;
using ola::web::StringValidator;
using ola::web::WildcardValidator;

using std::unique_ptr;
using std::set;
using std::string;
using std::vector;

class JsonSchemaTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonSchemaTest);
  CPPUNIT_TEST(testWildcardValidator);
  CPPUNIT_TEST(testReferenceValidator);
  CPPUNIT_TEST(testStringValidator);
  CPPUNIT_TEST(testBoolValidator);
  CPPUNIT_TEST(testNullValidator);
  CPPUNIT_TEST(testIntegerValidator);
  CPPUNIT_TEST(testNumberValidator);
  CPPUNIT_TEST(testObjectValidator);
  CPPUNIT_TEST(testArrayValidator);
  CPPUNIT_TEST(testAllOfValidator);
  CPPUNIT_TEST(testAnyOfValidator);
  CPPUNIT_TEST(testOneOfValidator);
  CPPUNIT_TEST(testNotValidator);
  CPPUNIT_TEST(testEnums);
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void testWildcardValidator();
  void testReferenceValidator();
  void testStringValidator();
  void testBoolValidator();
  void testNullValidator();
  void testIntegerValidator();
  void testNumberValidator();
  void testObjectValidator();
  void testArrayValidator();
  void testAllOfValidator();
  void testAnyOfValidator();
  void testOneOfValidator();
  void testNotValidator();
  void testEnums();

 private:
  unique_ptr<JsonBool> m_bool_value;
  unique_ptr<JsonValue> m_empty_array;
  unique_ptr<JsonValue> m_empty_object;
  unique_ptr<JsonInt> m_int_value;
  unique_ptr<JsonString> m_long_string_value;
  unique_ptr<JsonNull> m_null_value;
  unique_ptr<JsonDouble> m_number_value;
  unique_ptr<JsonString> m_string_value;
  unique_ptr<JsonUInt> m_uint_value;
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonSchemaTest);

void JsonSchemaTest::setUp() {
  string error;
  m_bool_value.reset(new JsonBool(true));
  m_empty_array.reset(JsonParser::Parse("[]", &error));
  m_empty_object.reset(JsonParser::Parse("{}", &error));
  m_int_value.reset(new JsonInt(-12));
  m_long_string_value.reset(new JsonString("This is a longer string"));
  m_null_value.reset(new JsonNull());
  m_number_value.reset(new JsonDouble(1.2));
  m_string_value.reset(new JsonString("foo"));
  m_uint_value.reset(new JsonUInt(4));
}

void JsonSchemaTest::testWildcardValidator() {
  WildcardValidator wildcard_validator;

  m_bool_value->Accept(&wildcard_validator);
  OLA_ASSERT_TRUE(wildcard_validator.IsValid());
  m_empty_array->Accept(&wildcard_validator);
  OLA_ASSERT_TRUE(wildcard_validator.IsValid());
  m_empty_object->Accept(&wildcard_validator);
  OLA_ASSERT_TRUE(wildcard_validator.IsValid());
  m_int_value->Accept(&wildcard_validator);
  OLA_ASSERT_TRUE(wildcard_validator.IsValid());
  m_null_value->Accept(&wildcard_validator);
  OLA_ASSERT_TRUE(wildcard_validator.IsValid());
  m_number_value->Accept(&wildcard_validator);
  OLA_ASSERT_TRUE(wildcard_validator.IsValid());
  m_string_value->Accept(&wildcard_validator);
  OLA_ASSERT_TRUE(wildcard_validator.IsValid());
  m_uint_value->Accept(&wildcard_validator);
  OLA_ASSERT_TRUE(wildcard_validator.IsValid());
}

void JsonSchemaTest::testReferenceValidator() {
  const string key = "#/definitions/testing";
  SchemaDefinitions definitions;
  definitions.Add(key, new IntegerValidator());

  ReferenceValidator validator(&definitions, key);

  m_int_value->Accept(&validator);
  OLA_ASSERT_TRUE(validator.IsValid());
  m_uint_value->Accept(&validator);
  OLA_ASSERT_TRUE(validator.IsValid());

  m_bool_value->Accept(&validator);
  OLA_ASSERT_FALSE(validator.IsValid());
  m_empty_array->Accept(&validator);
  OLA_ASSERT_FALSE(validator.IsValid());
  m_empty_object->Accept(&validator);
  OLA_ASSERT_FALSE(validator.IsValid());
  m_null_value->Accept(&validator);
  OLA_ASSERT_FALSE(validator.IsValid());
  m_number_value->Accept(&validator);
  OLA_ASSERT_FALSE(validator.IsValid());
  m_string_value->Accept(&validator);
  OLA_ASSERT_FALSE(validator.IsValid());
}

void JsonSchemaTest::testStringValidator() {
  StringValidator basic_string_validator((StringValidator::Options()));

  m_string_value->Accept(&basic_string_validator);
  OLA_ASSERT_TRUE(basic_string_validator.IsValid());
  m_long_string_value->Accept(&basic_string_validator);
  OLA_ASSERT_TRUE(basic_string_validator.IsValid());

  m_bool_value->Accept(&basic_string_validator);
  OLA_ASSERT_FALSE(basic_string_validator.IsValid());
  m_empty_array->Accept(&basic_string_validator);
  OLA_ASSERT_FALSE(basic_string_validator.IsValid());
  m_empty_object->Accept(&basic_string_validator);
  OLA_ASSERT_FALSE(basic_string_validator.IsValid());
  m_int_value->Accept(&basic_string_validator);
  OLA_ASSERT_FALSE(basic_string_validator.IsValid());
  m_null_value->Accept(&basic_string_validator);
  OLA_ASSERT_FALSE(basic_string_validator.IsValid());
  m_number_value->Accept(&basic_string_validator);
  OLA_ASSERT_FALSE(basic_string_validator.IsValid());
  m_uint_value->Accept(&basic_string_validator);
  OLA_ASSERT_FALSE(basic_string_validator.IsValid());

  // test a string with a min length
  StringValidator::Options min_length_options;
  min_length_options.min_length = 5;

  StringValidator min_length_string_validator(min_length_options);

  m_string_value->Accept(&min_length_string_validator);
  OLA_ASSERT_FALSE(min_length_string_validator.IsValid());
  m_long_string_value->Accept(&min_length_string_validator);
  OLA_ASSERT_TRUE(min_length_string_validator.IsValid());

  // test a string with a max length
  StringValidator::Options max_length_options;
  max_length_options.max_length = 10;

  StringValidator max_length_string_validator(max_length_options);

  m_string_value->Accept(&max_length_string_validator);
  OLA_ASSERT_TRUE(max_length_string_validator.IsValid());
  m_long_string_value->Accept(&max_length_string_validator);
  OLA_ASSERT_FALSE(max_length_string_validator.IsValid());
}

void JsonSchemaTest::testBoolValidator() {
  BoolValidator bool_validator;

  m_bool_value->Accept(&bool_validator);
  OLA_ASSERT_TRUE(bool_validator.IsValid());

  m_empty_array->Accept(&bool_validator);
  OLA_ASSERT_FALSE(bool_validator.IsValid());
  m_empty_object->Accept(&bool_validator);
  OLA_ASSERT_FALSE(bool_validator.IsValid());
  m_int_value->Accept(&bool_validator);
  OLA_ASSERT_FALSE(bool_validator.IsValid());
  m_null_value->Accept(&bool_validator);
  OLA_ASSERT_FALSE(bool_validator.IsValid());
  m_number_value->Accept(&bool_validator);
  OLA_ASSERT_FALSE(bool_validator.IsValid());
  m_string_value->Accept(&bool_validator);
  OLA_ASSERT_FALSE(bool_validator.IsValid());
  m_uint_value->Accept(&bool_validator);
  OLA_ASSERT_FALSE(bool_validator.IsValid());
}

void JsonSchemaTest::testNullValidator() {
  NullValidator null_validator;

  m_null_value->Accept(&null_validator);
  OLA_ASSERT_TRUE(null_validator.IsValid());

  m_bool_value->Accept(&null_validator);
  OLA_ASSERT_FALSE(null_validator.IsValid());
  m_empty_array->Accept(&null_validator);
  OLA_ASSERT_FALSE(null_validator.IsValid());
  m_empty_object->Accept(&null_validator);
  OLA_ASSERT_FALSE(null_validator.IsValid());
  m_int_value->Accept(&null_validator);
  OLA_ASSERT_FALSE(null_validator.IsValid());
  m_number_value->Accept(&null_validator);
  OLA_ASSERT_FALSE(null_validator.IsValid());
  m_string_value->Accept(&null_validator);
  OLA_ASSERT_FALSE(null_validator.IsValid());
  m_uint_value->Accept(&null_validator);
  OLA_ASSERT_FALSE(null_validator.IsValid());
}

void JsonSchemaTest::testIntegerValidator() {
  IntegerValidator integer_validator;

  m_int_value->Accept(&integer_validator);
  OLA_ASSERT_TRUE(integer_validator.IsValid());
  m_uint_value->Accept(&integer_validator);
  OLA_ASSERT_TRUE(integer_validator.IsValid());

  m_bool_value->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_empty_array->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_empty_object->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_null_value->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_number_value->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_string_value->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());

  // Now test some constraints.
  // Maximum
  IntegerValidator max_int_validator, exclusive_max_int_validator;
  max_int_validator.AddConstraint(new MaximumConstraint(
      new JsonInt(4), false));
  exclusive_max_int_validator.AddConstraint(new MaximumConstraint(
      new JsonInt(4), true));

  unique_ptr<JsonInt> int_value1(new JsonInt(3));
  unique_ptr<JsonInt> int_value2(new JsonInt(-11));
  unique_ptr<JsonInt> int_value3(new JsonInt(-13));
  unique_ptr<JsonInt> uint_value1(new JsonInt(5));

  // closed maximum
  int_value1->Accept(&max_int_validator);
  OLA_ASSERT_TRUE(max_int_validator.IsValid());
  int_value2->Accept(&max_int_validator);
  OLA_ASSERT_TRUE(max_int_validator.IsValid());
  int_value3->Accept(&max_int_validator);
  OLA_ASSERT_TRUE(max_int_validator.IsValid());
  m_int_value->Accept(&max_int_validator);
  OLA_ASSERT_TRUE(max_int_validator.IsValid());
  m_uint_value->Accept(&max_int_validator);
  OLA_ASSERT_TRUE(max_int_validator.IsValid());

  uint_value1->Accept(&max_int_validator);
  OLA_ASSERT_FALSE(max_int_validator.IsValid());

  // open maximum
  int_value1->Accept(&exclusive_max_int_validator);
  OLA_ASSERT_TRUE(exclusive_max_int_validator.IsValid());
  int_value2->Accept(&exclusive_max_int_validator);
  OLA_ASSERT_TRUE(exclusive_max_int_validator.IsValid());
  int_value3->Accept(&exclusive_max_int_validator);
  OLA_ASSERT_TRUE(exclusive_max_int_validator.IsValid());
  m_int_value->Accept(&exclusive_max_int_validator);
  OLA_ASSERT_TRUE(exclusive_max_int_validator.IsValid());

  m_uint_value->Accept(&exclusive_max_int_validator);
  OLA_ASSERT_FALSE(exclusive_max_int_validator.IsValid());
  uint_value1->Accept(&max_int_validator);
  OLA_ASSERT_FALSE(max_int_validator.IsValid());

  // Minimum
  IntegerValidator min_int_validator, exclusive_min_int_validator;
  min_int_validator.AddConstraint(new MinimumConstraint(
      new JsonInt(-12), false));
  exclusive_min_int_validator.AddConstraint(new MinimumConstraint(
      new JsonInt(-12), true));

  // closed minimum
  int_value1->Accept(&min_int_validator);
  OLA_ASSERT_TRUE(min_int_validator.IsValid());
  int_value2->Accept(&min_int_validator);
  OLA_ASSERT_TRUE(min_int_validator.IsValid());
  int_value3->Accept(&min_int_validator);
  OLA_ASSERT_FALSE(min_int_validator.IsValid());
  m_int_value->Accept(&min_int_validator);
  OLA_ASSERT_TRUE(min_int_validator.IsValid());
  m_uint_value->Accept(&min_int_validator);
  OLA_ASSERT_TRUE(min_int_validator.IsValid());

  // open minimum
  int_value1->Accept(&exclusive_min_int_validator);
  OLA_ASSERT_TRUE(exclusive_min_int_validator.IsValid());
  int_value2->Accept(&exclusive_min_int_validator);
  OLA_ASSERT_TRUE(exclusive_min_int_validator.IsValid());
  int_value3->Accept(&exclusive_min_int_validator);
  OLA_ASSERT_FALSE(exclusive_min_int_validator.IsValid());
  m_int_value->Accept(&exclusive_min_int_validator);
  OLA_ASSERT_FALSE(exclusive_min_int_validator.IsValid());
  m_uint_value->Accept(&exclusive_min_int_validator);
  OLA_ASSERT_TRUE(exclusive_min_int_validator.IsValid());

  // MultipleOf
  IntegerValidator multiple_of_validator;
  multiple_of_validator.AddConstraint(
      new MultipleOfConstraint(new JsonInt(2)));

  int_value1->Accept(&multiple_of_validator);
  OLA_ASSERT_FALSE(multiple_of_validator.IsValid());
  int_value2->Accept(&multiple_of_validator);
  OLA_ASSERT_FALSE(multiple_of_validator.IsValid());
  int_value3->Accept(&multiple_of_validator);
  OLA_ASSERT_FALSE(multiple_of_validator.IsValid());
  m_int_value->Accept(&multiple_of_validator);
  OLA_ASSERT_TRUE(multiple_of_validator.IsValid());
  m_uint_value->Accept(&multiple_of_validator);
  OLA_ASSERT_TRUE(multiple_of_validator.IsValid());

  unique_ptr<JsonInt> int_value4(new JsonInt(4));
  unique_ptr<JsonInt> int_value5(new JsonInt(8));
  unique_ptr<JsonInt> int_value6(new JsonInt(-4));

  int_value4->Accept(&multiple_of_validator);
  OLA_ASSERT_TRUE(multiple_of_validator.IsValid());
  int_value5->Accept(&multiple_of_validator);
  OLA_ASSERT_TRUE(multiple_of_validator.IsValid());
  int_value6->Accept(&multiple_of_validator);
  OLA_ASSERT_TRUE(multiple_of_validator.IsValid());
}

void JsonSchemaTest::testNumberValidator() {
  NumberValidator integer_validator;

  m_int_value->Accept(&integer_validator);
  OLA_ASSERT_TRUE(integer_validator.IsValid());
  m_uint_value->Accept(&integer_validator);
  OLA_ASSERT_TRUE(integer_validator.IsValid());
  m_number_value->Accept(&integer_validator);
  OLA_ASSERT_TRUE(integer_validator.IsValid());

  m_bool_value->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_empty_array->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_empty_object->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_null_value->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_string_value->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
}

void JsonSchemaTest::testObjectValidator() {
  ObjectValidator object_validator((ObjectValidator::Options()));

  m_empty_object->Accept(&object_validator);
  OLA_ASSERT_TRUE(object_validator.IsValid());

  m_bool_value->Accept(&object_validator);
  OLA_ASSERT_FALSE(object_validator.IsValid());
  m_empty_array->Accept(&object_validator);
  OLA_ASSERT_FALSE(object_validator.IsValid());
  m_int_value->Accept(&object_validator);
  OLA_ASSERT_FALSE(object_validator.IsValid());
  m_number_value->Accept(&object_validator);
  OLA_ASSERT_FALSE(object_validator.IsValid());
  m_number_value->Accept(&object_validator);
  OLA_ASSERT_FALSE(object_validator.IsValid());
  m_string_value->Accept(&object_validator);
  OLA_ASSERT_FALSE(object_validator.IsValid());
  m_uint_value->Accept(&object_validator);
  OLA_ASSERT_FALSE(object_validator.IsValid());

  string error;
  unique_ptr<JsonValue> object1(JsonParser::Parse("{\"a\": 1}", &error));
  unique_ptr<JsonValue> object2(
      JsonParser::Parse("{\"a\": 1, \"b\": 2}", &error));
  unique_ptr<JsonValue> object3(
      JsonParser::Parse("{\"a\": 1, \"b\": 2, \"c\": 3}", &error));
  unique_ptr<JsonValue> object4(
      JsonParser::Parse("{\"a\": 1, \"b\": true, \"c\": 3}", &error));
  unique_ptr<JsonValue> object5(
      JsonParser::Parse("{\"a\": 1, \"b\": 2, \"c\": false}", &error));

  // test maxProperties
  ObjectValidator::Options max_properties_options;
  max_properties_options.max_properties = 1;

  ObjectValidator max_properties_validator(max_properties_options);

  m_empty_object->Accept(&max_properties_validator);
  OLA_ASSERT_TRUE(max_properties_validator.IsValid());
  object1->Accept(&max_properties_validator);
  OLA_ASSERT_TRUE(max_properties_validator.IsValid());
  object2->Accept(&max_properties_validator);
  OLA_ASSERT_FALSE(max_properties_validator.IsValid());

  // test minProperties
  ObjectValidator::Options min_properties_options;
  min_properties_options.min_properties = 2;

  ObjectValidator min_properties_validator(min_properties_options);

  m_empty_object->Accept(&min_properties_validator);
  OLA_ASSERT_FALSE(min_properties_validator.IsValid());
  object1->Accept(&min_properties_validator);
  OLA_ASSERT_FALSE(min_properties_validator.IsValid());
  object2->Accept(&min_properties_validator);
  OLA_ASSERT_TRUE(min_properties_validator.IsValid());
  object3->Accept(&min_properties_validator);
  OLA_ASSERT_TRUE(min_properties_validator.IsValid());

  // test required
  set<string> required_properties;
  required_properties.insert("c");
  ObjectValidator::Options required_properties_options;
  required_properties_options.SetRequiredProperties(required_properties);

  ObjectValidator required_properties_validator(required_properties_options);

  m_empty_object->Accept(&required_properties_validator);
  OLA_ASSERT_FALSE(required_properties_validator.IsValid());
  object1->Accept(&required_properties_validator);
  OLA_ASSERT_FALSE(required_properties_validator.IsValid());
  object2->Accept(&required_properties_validator);
  OLA_ASSERT_FALSE(required_properties_validator.IsValid());
  object3->Accept(&required_properties_validator);
  OLA_ASSERT_TRUE(required_properties_validator.IsValid());

  // test dependencies
  // property dependencies first
  set<string> dependencies;
  // If we have b, then c is required
  dependencies.insert("c");

  ObjectValidator dependency_validator((ObjectValidator::Options()));
  dependency_validator.AddPropertyDependency("b", dependencies);

  m_empty_object->Accept(&dependency_validator);
  OLA_ASSERT_TRUE(dependency_validator.IsValid());
  object1->Accept(&dependency_validator);
  OLA_ASSERT_TRUE(dependency_validator.IsValid());
  object2->Accept(&dependency_validator);
  OLA_ASSERT_FALSE(dependency_validator.IsValid());
  object3->Accept(&dependency_validator);
  OLA_ASSERT_TRUE(dependency_validator.IsValid());

  // schema dependency
  // If c is present, b must be a bool
  ObjectValidator *sub_validator =
      new ObjectValidator((ObjectValidator::Options()));
  sub_validator->AddValidator("b", new BoolValidator());

  ObjectValidator schema_dependency_validator((ObjectValidator::Options()));
  schema_dependency_validator.AddSchemaDependency("c", sub_validator);

  m_empty_object->Accept(&schema_dependency_validator);
  OLA_ASSERT_TRUE(schema_dependency_validator.IsValid());
  object1->Accept(&schema_dependency_validator);
  OLA_ASSERT_TRUE(schema_dependency_validator.IsValid());
  object2->Accept(&schema_dependency_validator);
  OLA_ASSERT_TRUE(schema_dependency_validator.IsValid());
  object3->Accept(&schema_dependency_validator);
  OLA_ASSERT_FALSE(schema_dependency_validator.IsValid());
  object4->Accept(&schema_dependency_validator);
  OLA_ASSERT_TRUE(schema_dependency_validator.IsValid());

  // test properties
  // check b is a int
  ObjectValidator property_validator((ObjectValidator::Options()));
  property_validator.AddValidator("b", new IntegerValidator());

  m_empty_object->Accept(&property_validator);
  OLA_ASSERT_TRUE(property_validator.IsValid());
  object1->Accept(&property_validator);
  OLA_ASSERT_TRUE(property_validator.IsValid());
  object2->Accept(&property_validator);
  OLA_ASSERT_TRUE(property_validator.IsValid());
  object3->Accept(&property_validator);
  OLA_ASSERT_TRUE(property_validator.IsValid());
  object4->Accept(&property_validator);
  OLA_ASSERT_FALSE(property_validator.IsValid());

  // Now check a is also an int, and prevent any other properties
  ObjectValidator::Options no_additional_properties_options;
  no_additional_properties_options.SetAdditionalProperties(false);

  ObjectValidator property_validator2(no_additional_properties_options);
  property_validator2.AddValidator("b", new IntegerValidator());
  property_validator2.AddValidator("a", new IntegerValidator());

  m_empty_object->Accept(&property_validator2);
  OLA_ASSERT_TRUE(property_validator2.IsValid());
  object1->Accept(&property_validator2);
  OLA_ASSERT_TRUE(property_validator2.IsValid());
  object2->Accept(&property_validator2);
  OLA_ASSERT_TRUE(property_validator2.IsValid());
  object3->Accept(&property_validator2);
  OLA_ASSERT_FALSE(property_validator2.IsValid());
  object4->Accept(&property_validator2);
  OLA_ASSERT_FALSE(property_validator2.IsValid());

  ObjectValidator::Options allow_additional_properties_options;
  allow_additional_properties_options.SetAdditionalProperties(true);

  ObjectValidator property_validator3(allow_additional_properties_options);
  property_validator3.AddValidator("b", new IntegerValidator());
  property_validator3.AddValidator("a", new IntegerValidator());

  m_empty_object->Accept(&property_validator3);
  OLA_ASSERT_TRUE(property_validator3.IsValid());
  object1->Accept(&property_validator3);
  OLA_ASSERT_TRUE(property_validator3.IsValid());
  object2->Accept(&property_validator3);
  OLA_ASSERT_TRUE(property_validator3.IsValid());
  object3->Accept(&property_validator3);
  OLA_ASSERT_TRUE(property_validator3.IsValid());
  object4->Accept(&property_validator3);
  OLA_ASSERT_FALSE(property_validator3.IsValid());

  // try an additionalProperty validator
  ObjectValidator property_validator4((ObjectValidator::Options()));
  property_validator4.AddValidator("a", new IntegerValidator());
  property_validator4.AddValidator("b", new IntegerValidator());
  property_validator4.SetAdditionalValidator(new IntegerValidator());

  m_empty_object->Accept(&property_validator4);
  OLA_ASSERT_TRUE(property_validator4.IsValid());
  object1->Accept(&property_validator4);
  OLA_ASSERT_TRUE(property_validator4.IsValid());
  object2->Accept(&property_validator4);
  OLA_ASSERT_TRUE(property_validator4.IsValid());
  object3->Accept(&property_validator4);
  OLA_ASSERT_TRUE(property_validator4.IsValid());
  object4->Accept(&property_validator4);
  OLA_ASSERT_FALSE(property_validator4.IsValid());
  object5->Accept(&property_validator4);
  OLA_ASSERT_FALSE(property_validator4.IsValid());
}

void JsonSchemaTest::testArrayValidator() {
  ArrayValidator array_validator(NULL, NULL, ArrayValidator::Options());

  m_empty_array->Accept(&array_validator);
  OLA_ASSERT_TRUE(array_validator.IsValid());

  m_bool_value->Accept(&array_validator);
  OLA_ASSERT_FALSE(array_validator.IsValid());
  m_empty_object->Accept(&array_validator);
  OLA_ASSERT_FALSE(array_validator.IsValid());
  m_int_value->Accept(&array_validator);
  OLA_ASSERT_FALSE(array_validator.IsValid());
  m_number_value->Accept(&array_validator);
  OLA_ASSERT_FALSE(array_validator.IsValid());
  m_number_value->Accept(&array_validator);
  OLA_ASSERT_FALSE(array_validator.IsValid());
  m_string_value->Accept(&array_validator);
  OLA_ASSERT_FALSE(array_validator.IsValid());
  m_uint_value->Accept(&array_validator);
  OLA_ASSERT_FALSE(array_validator.IsValid());

  string error;
  unique_ptr<JsonValue> small_array(JsonParser::Parse("[1]", &error));
  unique_ptr<JsonValue> medium_array(JsonParser::Parse("[1, 2]", &error));
  unique_ptr<JsonValue> large_array(JsonParser::Parse("[1, 2, 3]", &error));
  unique_ptr<JsonValue> duplicate_items_array(
      JsonParser::Parse("[1, 2, 1]", &error));

  // test maxItems
  ArrayValidator::Options max_options;
  max_options.max_items = 2;

  ArrayValidator max_items_validator(NULL, NULL, max_options);

  m_empty_array->Accept(&max_items_validator);
  OLA_ASSERT_TRUE(max_items_validator.IsValid());
  small_array->Accept(&max_items_validator);
  OLA_ASSERT_TRUE(max_items_validator.IsValid());
  medium_array->Accept(&max_items_validator);
  OLA_ASSERT_TRUE(max_items_validator.IsValid());
  large_array->Accept(&max_items_validator);
  OLA_ASSERT_FALSE(max_items_validator.IsValid());

  // test minItems
  ArrayValidator::Options min_options;
  min_options.min_items = 2;

  ArrayValidator min_items_validator(NULL, NULL, min_options);

  m_empty_array->Accept(&min_items_validator);
  OLA_ASSERT_FALSE(min_items_validator.IsValid());
  small_array->Accept(&min_items_validator);
  OLA_ASSERT_FALSE(min_items_validator.IsValid());
  medium_array->Accept(&min_items_validator);
  OLA_ASSERT_TRUE(min_items_validator.IsValid());
  large_array->Accept(&min_items_validator);
  OLA_ASSERT_TRUE(min_items_validator.IsValid());

  // test uniqueItems
  ArrayValidator::Options unique_items_options;
  unique_items_options.unique_items = true;

  ArrayValidator unique_items_validator(NULL, NULL, unique_items_options);

  m_empty_array->Accept(&unique_items_validator);
  OLA_ASSERT_TRUE(unique_items_validator.IsValid());
  small_array->Accept(&unique_items_validator);
  OLA_ASSERT_TRUE(unique_items_validator.IsValid());
  medium_array->Accept(&unique_items_validator);
  OLA_ASSERT_TRUE(unique_items_validator.IsValid());
  large_array->Accept(&unique_items_validator);
  OLA_ASSERT_TRUE(unique_items_validator.IsValid());
  duplicate_items_array->Accept(&unique_items_validator);
  OLA_ASSERT_FALSE(unique_items_validator.IsValid());
}

void JsonSchemaTest::testAllOfValidator() {
  // 1 <= x <= 5
  IntegerValidator *range1 = new IntegerValidator();
  range1->AddConstraint(
      new MinimumConstraint(new JsonInt(1), false));
  range1->AddConstraint(
      new MaximumConstraint(new JsonInt(5), false));

  // 4 <= x <= 8
  IntegerValidator *range2 = new IntegerValidator();
  range2->AddConstraint(
      new MinimumConstraint(new JsonInt(4), false));
  range2->AddConstraint(
      new MaximumConstraint(new JsonInt(8), false));

  ConjunctionValidator::ValidatorList validators;
  validators.push_back(range1);
  validators.push_back(range2);

  AllOfValidator all_of_validator(&validators);

  m_string_value->Accept(&all_of_validator);
  OLA_ASSERT_FALSE(all_of_validator.IsValid());
  m_long_string_value->Accept(&all_of_validator);
  OLA_ASSERT_FALSE(all_of_validator.IsValid());
  m_bool_value->Accept(&all_of_validator);
  OLA_ASSERT_FALSE(all_of_validator.IsValid());
  m_empty_array->Accept(&all_of_validator);
  OLA_ASSERT_FALSE(all_of_validator.IsValid());
  m_empty_object->Accept(&all_of_validator);
  OLA_ASSERT_FALSE(all_of_validator.IsValid());
  m_int_value->Accept(&all_of_validator);
  OLA_ASSERT_FALSE(all_of_validator.IsValid());
  m_null_value->Accept(&all_of_validator);
  OLA_ASSERT_FALSE(all_of_validator.IsValid());
  m_number_value->Accept(&all_of_validator);
  OLA_ASSERT_FALSE(all_of_validator.IsValid());
  m_uint_value->Accept(&all_of_validator);
  OLA_ASSERT_TRUE(all_of_validator.IsValid());  // 4
}

void JsonSchemaTest::testAnyOfValidator() {
  ConjunctionValidator::ValidatorList validators;
  validators.push_back(new StringValidator((StringValidator::Options())));
  validators.push_back(new BoolValidator());
  validators.push_back(new NullValidator());

  AnyOfValidator any_of_validator(&validators);

  m_string_value->Accept(&any_of_validator);
  OLA_ASSERT_TRUE(any_of_validator.IsValid());
  m_long_string_value->Accept(&any_of_validator);
  OLA_ASSERT_TRUE(any_of_validator.IsValid());
  m_bool_value->Accept(&any_of_validator);
  OLA_ASSERT_TRUE(any_of_validator.IsValid());
  m_null_value->Accept(&any_of_validator);
  OLA_ASSERT_TRUE(any_of_validator.IsValid());

  m_empty_array->Accept(&any_of_validator);
  OLA_ASSERT_FALSE(any_of_validator.IsValid());
  m_empty_object->Accept(&any_of_validator);
  OLA_ASSERT_FALSE(any_of_validator.IsValid());
  m_int_value->Accept(&any_of_validator);
  OLA_ASSERT_FALSE(any_of_validator.IsValid());
  m_number_value->Accept(&any_of_validator);
  OLA_ASSERT_FALSE(any_of_validator.IsValid());
  m_uint_value->Accept(&any_of_validator);
  OLA_ASSERT_FALSE(any_of_validator.IsValid());
}

void JsonSchemaTest::testOneOfValidator() {
  // 1 <= x <= 5
  IntegerValidator *range1 = new IntegerValidator();
  range1->AddConstraint(
      new MinimumConstraint(new JsonInt(1), false));
  range1->AddConstraint(
      new MaximumConstraint(new JsonInt(5), false));

  // 4 <= x <= 8
  IntegerValidator *range2 = new IntegerValidator();
  range2->AddConstraint(
      new MinimumConstraint(new JsonInt(4), false));
  range2->AddConstraint(
      new MaximumConstraint(new JsonInt(8), false));

  ConjunctionValidator::ValidatorList validators;
  validators.push_back(range1);
  validators.push_back(range2);

  OneOfValidator one_of_validator(&validators);

  m_bool_value->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());
  m_empty_array->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());
  m_empty_object->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());
  m_int_value->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());
  m_null_value->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());
  m_number_value->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());
  m_string_value->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());
  m_uint_value->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());

  unique_ptr<JsonInt> int_value1(new JsonInt(3));
  unique_ptr<JsonInt> int_value2(new JsonInt(5));
  unique_ptr<JsonInt> int_value3(new JsonInt(6));

  int_value1->Accept(&one_of_validator);
  OLA_ASSERT_TRUE(one_of_validator.IsValid());
  int_value2->Accept(&one_of_validator);
  OLA_ASSERT_FALSE(one_of_validator.IsValid());
  int_value3->Accept(&one_of_validator);
  OLA_ASSERT_TRUE(one_of_validator.IsValid());
}

void JsonSchemaTest::testNotValidator() {
  NotValidator not_validator(new BoolValidator());

  // Anything but a bool
  m_bool_value->Accept(&not_validator);
  OLA_ASSERT_FALSE(not_validator.IsValid());
  m_empty_array->Accept(&not_validator);
  OLA_ASSERT_TRUE(not_validator.IsValid());
  m_empty_object->Accept(&not_validator);
  OLA_ASSERT_TRUE(not_validator.IsValid());
  m_int_value->Accept(&not_validator);
  OLA_ASSERT_TRUE(not_validator.IsValid());
  m_null_value->Accept(&not_validator);
  OLA_ASSERT_TRUE(not_validator.IsValid());
  m_number_value->Accept(&not_validator);
  OLA_ASSERT_TRUE(not_validator.IsValid());
  m_string_value->Accept(&not_validator);
  OLA_ASSERT_TRUE(not_validator.IsValid());
  m_uint_value->Accept(&not_validator);
  OLA_ASSERT_TRUE(not_validator.IsValid());
}

void JsonSchemaTest::testEnums() {
  StringValidator string_validator((StringValidator::Options()));
  string_validator.AddEnumValue(new JsonString("foo"));
  string_validator.AddEnumValue(new JsonString("bar"));

  JsonString bar_value("bar");
  JsonString baz_value("baz");

  m_string_value->Accept(&string_validator);
  OLA_ASSERT_TRUE(string_validator.IsValid());
  m_long_string_value->Accept(&string_validator);
  OLA_ASSERT_FALSE(string_validator.IsValid());
  bar_value.Accept(&string_validator);
  OLA_ASSERT_TRUE(string_validator.IsValid());
  baz_value.Accept(&string_validator);
  OLA_ASSERT_FALSE(string_validator.IsValid());

  IntegerValidator integer_validator;
  integer_validator.AddEnumValue(new JsonInt(1));
  integer_validator.AddEnumValue(new JsonInt(2));
  integer_validator.AddEnumValue(new JsonInt(4));

  JsonInt int_value1(2);
  JsonInt int_value2(3);
  JsonInt uint_value1(2);
  JsonInt uint_value2(3);

  m_int_value->Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  m_uint_value->Accept(&integer_validator);
  OLA_ASSERT_TRUE(integer_validator.IsValid());

  int_value1.Accept(&integer_validator);
  OLA_ASSERT_TRUE(integer_validator.IsValid());
  int_value2.Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
  uint_value1.Accept(&integer_validator);
  OLA_ASSERT_TRUE(integer_validator.IsValid());
  uint_value2.Accept(&integer_validator);
  OLA_ASSERT_FALSE(integer_validator.IsValid());
}
