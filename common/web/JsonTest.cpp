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
 * JsonTest.cpp
 * Unittest for Json classses.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <sstream>

#include "ola/web/Json.h"
#include "ola/testing/TestUtils.h"


using ola::web::JsonArray;
using ola::web::JsonBoolValue;
using ola::web::JsonDoubleValue;
using ola::web::JsonIntValue;
using ola::web::JsonNullValue;
using ola::web::JsonObject;
using ola::web::JsonRawValue;
using ola::web::JsonStringValue;
using ola::web::JsonUIntValue;
using ola::web::JsonValue;
using ola::web::JsonWriter;
using std::string;

class JsonTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonTest);
  CPPUNIT_TEST(testString);
  CPPUNIT_TEST(testIntegerValues);
  CPPUNIT_TEST(testNumberValues);
  CPPUNIT_TEST(testRaw);
  CPPUNIT_TEST(testBool);
  CPPUNIT_TEST(testNull);
  CPPUNIT_TEST(testSimpleArray);
  CPPUNIT_TEST(testEmptyObject);
  CPPUNIT_TEST(testSimpleObject);
  CPPUNIT_TEST(testComplexObject);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testString();
    void testIntegerValues();
    void testNumberValues();
    void testRaw();
    void testBool();
    void testNull();
    void testSimpleArray();
    void testEmptyObject();
    void testSimpleObject();
    void testComplexObject();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonTest);

/*
 * Test a string.
 */
void JsonTest::testString() {
  JsonStringValue value("foo");
  string expected = "\"foo\"";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(value));

  // test escaping
  JsonStringValue value2("foo\"bar\"");
  expected = "\"foo\\\"bar\\\"\"";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(value2));
}


/*
 * Test ints.
 */
void JsonTest::testIntegerValues() {
  JsonUIntValue uint_value(10);
  string expected = "10";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(uint_value));

  JsonIntValue int_value(-10);
  expected = "-10";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(int_value));
}

/*
 * Test numbers (doubles).
 */
void JsonTest::testNumberValues() {
  // For JsonDoubleValue constructed with a double, the string representation
  // depends on the platform. For example 1.23-e2 could be any of 1.23-e2,
  // 0.00123 or 1.23e-002.
  // So we skip this test.
  JsonDoubleValue d1(12.234);
  OLA_ASSERT_EQ(12.234, d1.Value());

  JsonDoubleValue d2(-1.23e-12);
  OLA_ASSERT_EQ(-1.23e-12, d2.Value());

  // For JsonDoubleValue created using DoubleRepresentation, the string will be
  // well defined, but the Value() may differ. Just do our best here.
  JsonDoubleValue::DoubleRepresentation rep1 = {
    false, 12, 1, 345, 0
  };
  JsonDoubleValue d3(rep1);
  OLA_ASSERT_EQ(string("12.0345"), JsonWriter::AsString(d3));
  OLA_ASSERT_EQ(12.0345, d3.Value());

  JsonDoubleValue::DoubleRepresentation rep2 = {
    true, 345, 3, 789, 2
  };
  JsonDoubleValue d4(rep2);
  OLA_ASSERT_EQ(string("-345.000789e2"), JsonWriter::AsString(d4));
  OLA_ASSERT_DOUBLE_EQ(-345.000789e2, d4.Value(), 0.001);

  JsonDoubleValue::DoubleRepresentation rep3 = {
    true, 345, 3, 0, -2
  };
  JsonDoubleValue d5(rep3);
  OLA_ASSERT_EQ(string("-345e-2"), JsonWriter::AsString(d5));
  OLA_ASSERT_EQ(-3.45, d5.Value());
}

/*
 * Test raw.
 * This is used by the web DMX console
 */
void JsonTest::testRaw() {
  // A printable character
  JsonRawValue value("\x41");
  string expected = "\x41";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(value));

  // And an unprintable one
  JsonRawValue value2("\x7f");
  expected = "\x7f";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(value2));
}

/*
 * Test bools.
 */
void JsonTest::testBool() {
  JsonBoolValue true_value(true);
  string expected = "true";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(true_value));

  JsonBoolValue false_value(false);
  expected = "false";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(false_value));
}


/*
 * Test a null.
 */
void JsonTest::testNull() {
  JsonNullValue value;
  string expected = "null";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(value));
}


/*
 * Test a simple array.
 * Json arrays can be mixed types.
 */
void JsonTest::testSimpleArray() {
  JsonArray array;
  array.Append();
  array.Append(true);
  array.Append(1);
  array.Append("foo");
  array.Append(10);
  array.Append(-10);

  string expected = "[null, true, 1, \"foo\", 10, -10]";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(array));
}

/*
 * Test an empty object.
 */
void JsonTest::testEmptyObject() {
  JsonObject object;

  string expected = "{}";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(object));
}

/*
 * Test a simple object.
 */
void JsonTest::testSimpleObject() {
  JsonObject object;
  object.Add("age", 10);
  object.Add("name", "simon");
  object.Add("male", true);

  string expected = (
      "{\n"
      "  \"age\": 10,\n"
      "  \"male\": true,\n"
      "  \"name\": \"simon\"\n"
      "}");
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(object));
}

/*
 * Test a complex object.
 */
void JsonTest::testComplexObject() {
  JsonObject object;
  object.Add("age", 10);
  object.Add("name", "simon");
  object.Add("male", true);

  JsonArray *array = object.AddArray("lucky numbers");
  array->Append(2);
  array->Append(5);

  string expected = (
      "{\n"
      "  \"age\": 10,\n"
      "  \"lucky numbers\": [2, 5],\n"
      "  \"male\": true,\n"
      "  \"name\": \"simon\"\n"
      "}");
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(object));
}
