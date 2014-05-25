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
 * JsonSchemaTest.cpp
 * Unittest for the Json Schema.
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
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
using ola::web::JsonBoolValue;
using ola::web::JsonDoubleValue;
using ola::web::JsonIntValue;
using ola::web::JsonNullValue;
using ola::web::JsonParser;
using ola::web::JsonStringValue;
using ola::web::JsonStringValue;
using ola::web::JsonUIntValue;
using ola::web::JsonUIntValue;
using ola::web::JsonValue;
using ola::web::MaximumConstraint;
using ola::web::MinimumConstraint;
using ola::web::MultipleOfConstraint;
using ola::web::NotValidator;
using ola::web::NullValidator;
using ola::web::NumberValidator;
using ola::web::ObjectValidator;
using ola::web::OneOfValidator;
using ola::web::StringValidator;
using ola::web::WildcardValidator;

using std::auto_ptr;
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

 private:
  auto_ptr<JsonBoolValue> m_bool_value;
  auto_ptr<JsonValue> m_empty_array;
  auto_ptr<JsonValue> m_empty_object;
  auto_ptr<JsonIntValue> m_int_value;
  auto_ptr<JsonStringValue> m_long_string_value;
  auto_ptr<JsonNullValue> m_null_value;
  auto_ptr<JsonDoubleValue> m_number_value;
  auto_ptr<JsonStringValue> m_string_value;
  auto_ptr<JsonUIntValue> m_uint_value;
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonSchemaTest);

void JsonSchemaTest::setUp() {
  string error;
  m_bool_value.reset(new JsonBoolValue(true));
  m_empty_array.reset(JsonParser::Parse("[]", &error));
  m_empty_object.reset(JsonParser::Parse("{}", &error));
  m_int_value.reset(new JsonIntValue(-12));
  m_long_string_value.reset(new JsonStringValue("This is a longer string"));
  m_null_value.reset(new JsonNullValue());
  m_number_value.reset(new JsonDoubleValue(1.2));
  m_string_value.reset(new JsonStringValue("foo"));
  m_uint_value.reset(new JsonUIntValue(4));
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
      new JsonIntValue(4), false));
  exclusive_max_int_validator.AddConstraint(new MaximumConstraint(
      new JsonIntValue(4), true));

  auto_ptr<JsonIntValue> int_value1(new JsonIntValue(3));
  auto_ptr<JsonIntValue> int_value2(new JsonIntValue(-11));
  auto_ptr<JsonIntValue> int_value3(new JsonIntValue(-13));
  auto_ptr<JsonIntValue> uint_value1(new JsonIntValue(5));

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
      new JsonIntValue(-12), false));
  exclusive_min_int_validator.AddConstraint(new MinimumConstraint(
      new JsonIntValue(-12), true));

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
      new MultipleOfConstraint(new JsonIntValue(2)));

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

  auto_ptr<JsonIntValue> int_value4(new JsonIntValue(4));
  auto_ptr<JsonIntValue> int_value5(new JsonIntValue(8));
  auto_ptr<JsonIntValue> int_value6(new JsonIntValue(-4));

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
}

void JsonSchemaTest::testArrayValidator() {
  ArrayValidator array_validdator(NULL, NULL, ArrayValidator::Options());

  m_empty_array->Accept(&array_validdator);
  OLA_ASSERT_TRUE(array_validdator.IsValid());

  m_bool_value->Accept(&array_validdator);
  OLA_ASSERT_FALSE(array_validdator.IsValid());
  m_empty_object->Accept(&array_validdator);
  OLA_ASSERT_FALSE(array_validdator.IsValid());
  m_int_value->Accept(&array_validdator);
  OLA_ASSERT_FALSE(array_validdator.IsValid());
  m_number_value->Accept(&array_validdator);
  OLA_ASSERT_FALSE(array_validdator.IsValid());
  m_number_value->Accept(&array_validdator);
  OLA_ASSERT_FALSE(array_validdator.IsValid());
  m_string_value->Accept(&array_validdator);
  OLA_ASSERT_FALSE(array_validdator.IsValid());
  m_uint_value->Accept(&array_validdator);
  OLA_ASSERT_FALSE(array_validdator.IsValid());
}

void JsonSchemaTest::testAllOfValidator() {
  // 1 <= x <= 5
  IntegerValidator *range1 = new IntegerValidator();
  range1->AddConstraint(
      new MinimumConstraint(new JsonIntValue(1), false));
  range1->AddConstraint(
      new MaximumConstraint(new JsonIntValue(5), false));

  // 4 <= x <= 8
  IntegerValidator *range2 = new IntegerValidator();
  range2->AddConstraint(
      new MinimumConstraint(new JsonIntValue(4), false));
  range2->AddConstraint(
      new MaximumConstraint(new JsonIntValue(8), false));

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
      new MinimumConstraint(new JsonIntValue(1), false));
  range1->AddConstraint(
      new MaximumConstraint(new JsonIntValue(5), false));

  // 4 <= x <= 8
  IntegerValidator *range2 = new IntegerValidator();
  range2->AddConstraint(
      new MinimumConstraint(new JsonIntValue(4), false));
  range2->AddConstraint(
      new MaximumConstraint(new JsonIntValue(8), false));

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

  auto_ptr<JsonIntValue> int_value1(new JsonIntValue(3));
  auto_ptr<JsonIntValue> int_value2(new JsonIntValue(5));
  auto_ptr<JsonIntValue> int_value3(new JsonIntValue(6));

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
