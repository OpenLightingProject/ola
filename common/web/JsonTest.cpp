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
#include <vector>

#include "ola/testing/TestUtils.h"
#include "ola/web/Json.h"
#include "ola/web/JsonPointer.h"
#include "ola/web/JsonWriter.h"

using ola::web::JsonArray;
using ola::web::JsonBoolValue;
using ola::web::JsonDoubleValue;
using ola::web::JsonInt64Value;
using ola::web::JsonIntValue;
using ola::web::JsonNullValue;
using ola::web::JsonObject;
using ola::web::JsonPointer;
using ola::web::JsonRawValue;
using ola::web::JsonStringValue;
using ola::web::JsonUInt64Value;
using ola::web::JsonUIntValue;
using ola::web::JsonValue;
using ola::web::JsonWriter;
using std::string;
using std::vector;

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
  CPPUNIT_TEST(testEquality);
  CPPUNIT_TEST(testLookups);
  CPPUNIT_TEST(testClone);
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
    void testEquality();
    void testLookups();
    void testClone();
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

/*
 * Test a complex object.
 */
void JsonTest::testEquality() {
  JsonStringValue string1("foo");
  JsonStringValue string2("foo");
  JsonStringValue string3("bar");
  JsonBoolValue bool1(true);
  JsonBoolValue bool2(false);
  JsonNullValue null1;
  JsonDoubleValue double1(1.0);
  JsonDoubleValue double2(1.0);
  JsonDoubleValue double3(2.1);

  JsonUIntValue uint1(10);
  JsonUIntValue uint2(99);

  JsonIntValue int1(10);
  JsonIntValue int2(99);
  JsonIntValue int3(-99);

  JsonInt64Value int64_1(-99);
  JsonInt64Value int64_2(10);
  JsonInt64Value int64_3(99);

  JsonInt64Value uint64_1(10);
  JsonInt64Value uint64_2(99);

  vector<JsonValue*> all_values;
  all_values.push_back(&string1);
  all_values.push_back(&string2);
  all_values.push_back(&string3);
  all_values.push_back(&bool1);
  all_values.push_back(&bool2);
  all_values.push_back(&null1);
  all_values.push_back(&double1);
  all_values.push_back(&double2);
  all_values.push_back(&double3);
  all_values.push_back(&uint1);
  all_values.push_back(&uint2);
  all_values.push_back(&int1);
  all_values.push_back(&int2);
  all_values.push_back(&int3);
  all_values.push_back(&int64_1);
  all_values.push_back(&int64_2);
  all_values.push_back(&int64_3);
  all_values.push_back(&uint64_1);
  all_values.push_back(&uint64_2);

  OLA_ASSERT_EQ(string1, string2);
  OLA_ASSERT_NE(string1, string3);

  OLA_ASSERT_NE(bool1, bool2);

  OLA_ASSERT_EQ(double1, double2);
  OLA_ASSERT_NE(double1, double3);

  OLA_ASSERT_NE(uint1, uint2);

  OLA_ASSERT_NE(uint1, uint2);

  // Test the tricky cases:
  OLA_ASSERT(int1 == uint1);
  OLA_ASSERT(int2 == uint2);
  OLA_ASSERT(uint1 == int64_2);
  OLA_ASSERT(uint2 == int64_3);
  OLA_ASSERT(int3 == int64_1);
  OLA_ASSERT(uint1 == uint64_1);
  OLA_ASSERT(uint2 == uint64_2);
  OLA_ASSERT(int1 == uint64_1);
  OLA_ASSERT(int2 == uint64_2);
  OLA_ASSERT(int64_2 == uint64_1);
  OLA_ASSERT(int64_3 == uint64_2);

  // Test Array equality.
  JsonArray array1;
  array1.Append(true);
  array1.Append(1);
  array1.Append("foo");

  JsonArray array2;
  array2.Append(true);
  array2.Append(1);
  array2.Append("foo");
  array2.Append(-1);

  JsonArray array3;
  array3.Append(true);
  array3.Append(1);
  array3.Append("bar");

  all_values.push_back(&array1);
  all_values.push_back(&array2);
  all_values.push_back(&array3);

  OLA_ASSERT_FALSE(array1 == array2);
  OLA_ASSERT_FALSE(array1 == array3);

  // Object equality
  JsonObject object1;
  object1.Add("age", 10);
  object1.Add("name", "simon");
  object1.Add("male", true);

  JsonObject object2;
  object1.Add("age", 10);
  object1.Add("name", "simon");
  object1.Add("male", true);
  object1.Add("nationality", "Australia");

  JsonObject object3;
  object3.Add("age", 10);
  object3.Add("name", "james");
  object3.Add("male", true);

  all_values.push_back(&object1);
  all_values.push_back(&object2);
  all_values.push_back(&object3);

  OLA_ASSERT_FALSE(object1 == object2);
  OLA_ASSERT_FALSE(object1 == object3);

  // verify identity equality
  for (unsigned int i = 0; i < all_values.size(); ++i) {
    OLA_ASSERT(*(all_values[i]) == *(all_values[i]));
  }
}

/*
 * Test looking up a value with a pointer.
 */
void JsonTest::testLookups() {
  JsonPointer empty_pointer;
  JsonPointer invalid_pointer("/invalid/path");
  JsonPointer name_pointer("/name");

  JsonStringValue string1("foo");
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(&string1),
                string1.LookupElement(empty_pointer));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
                string1.LookupElement(invalid_pointer));

  // Now try an object
  JsonStringValue *name_value = new JsonStringValue("simon");
  JsonObject object;
  object.Add("age", 10);
  object.AddValue("name", name_value);
  object.Add("male", true);
  object.Add("", "foo");

  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(&object),
                object.LookupElement(empty_pointer));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(name_value),
                object.LookupElement(name_pointer));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
                object.LookupElement(invalid_pointer));

  // Now try an array
  JsonArray *array = new JsonArray();
  JsonStringValue *string2 = new JsonStringValue("cat");
  JsonStringValue *string3 = new JsonStringValue("dog");
  JsonStringValue *string4 = new JsonStringValue("mouse");
  array->AppendValue(string2);
  array->AppendValue(string3);
  array->AppendValue(string4);

  JsonPointer first("/0");
  JsonPointer middle("/1");
  JsonPointer last("/2");
  JsonPointer one_past_last("/-");
  JsonPointer invalid("/a");

  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(array),
                array->LookupElement(empty_pointer));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
                array->LookupElement(invalid_pointer));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(string2),
                array->LookupElement(first));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(string3),
                array->LookupElement(middle));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(string4),
                array->LookupElement(last));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
                array->LookupElement(one_past_last));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
                array->LookupElement(invalid));

  // now a nested case
  object.AddValue("pets", array);
  JsonPointer first_pet("/pets/0");
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(string2),
                object.LookupElement(first_pet));
}

/*
 * Test that clone() works.
 */
void JsonTest::testClone() {
  JsonStringValue string1("foo");
  JsonBoolValue bool1(true);
  JsonNullValue null1;
  JsonDoubleValue double1(1.0);
  JsonUIntValue uint1(10);
  JsonIntValue int1(10);
  JsonInt64Value int64_1(-99);
  JsonInt64Value uint64_1(10);

  JsonObject object;
  object.Add("age", 10);
  object.Add("name", "simon");
  object.Add("male", true);
  object.Add("", "foo");

  JsonArray array;
  array.Append(true);
  array.Append(1);
  array.Append("bar");

  vector<JsonValue*> all_values;
  all_values.push_back(&string1);
  all_values.push_back(&bool1);
  all_values.push_back(&null1);
  all_values.push_back(&double1);
  all_values.push_back(&uint1);
  all_values.push_back(&int1);
  all_values.push_back(&int64_1);
  all_values.push_back(&uint64_1);
  all_values.push_back(&object);
  all_values.push_back(&array);

  for (unsigned int i = 0; i < all_values.size(); ++i) {
    std::auto_ptr<JsonValue> value(all_values[i]->Clone());
    OLA_ASSERT(*(value.get()) == *(all_values[i]));
  }
}
