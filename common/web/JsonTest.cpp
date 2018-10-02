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
 * JsonTest.cpp
 * Unittest for Json classes.
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
using ola::web::JsonBool;
using ola::web::JsonDouble;
using ola::web::JsonInt64;
using ola::web::JsonInt;
using ola::web::JsonNull;
using ola::web::JsonObject;
using ola::web::JsonPointer;
using ola::web::JsonRawValue;
using ola::web::JsonString;
using ola::web::JsonUInt64;
using ola::web::JsonUInt;
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
  CPPUNIT_TEST(testIntInequality);
  CPPUNIT_TEST(testMultipleOf);
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
    void testIntInequality();
    void testMultipleOf();
    void testLookups();
    void testClone();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonTest);

/*
 * Test a string.
 */
void JsonTest::testString() {
  JsonString value("foo");
  string expected = "\"foo\"";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(value));

  // test escaping
  JsonString value2("foo\"bar\"");
  expected = "\"foo\\\"bar\\\"\"";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(value2));
}


/*
 * Test ints.
 */
void JsonTest::testIntegerValues() {
  JsonUInt uint_value(10);
  string expected = "10";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(uint_value));

  JsonInt int_value(-10);
  expected = "-10";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(int_value));
}

/*
 * Test numbers (doubles).
 */
void JsonTest::testNumberValues() {
  // For JsonDouble constructed with a double, the string representation
  // depends on the platform. For example 1.23-e2 could be any of 1.23-e2,
  // 0.00123 or 1.23e-002.
  // So we skip this test.
  JsonDouble d1(12.234);
  OLA_ASSERT_EQ(12.234, d1.Value());

  JsonDouble d2(-1.23e-12);
  OLA_ASSERT_EQ(-1.23e-12, d2.Value());

  // For JsonDouble created using DoubleRepresentation, the string will be
  // well defined, but the Value() may differ. Just do our best here.
  JsonDouble::DoubleRepresentation rep1 = {
    false, 12, 1, 345, 0
  };
  JsonDouble d3(rep1);
  OLA_ASSERT_EQ(string("12.0345"), JsonWriter::AsString(d3));
  OLA_ASSERT_EQ(12.0345, d3.Value());

  JsonDouble::DoubleRepresentation rep2 = {
    true, 345, 3, 789, 2
  };
  JsonDouble d4(rep2);
  OLA_ASSERT_EQ(string("-345.000789e2"), JsonWriter::AsString(d4));
  OLA_ASSERT_DOUBLE_EQ(-345.000789e2, d4.Value(), 0.001);

  JsonDouble::DoubleRepresentation rep3 = {
    true, 345, 3, 0, -2
  };
  JsonDouble d5(rep3);
  OLA_ASSERT_EQ(string("-345e-2"), JsonWriter::AsString(d5));
  OLA_ASSERT_EQ(-3.45, d5.Value());

  JsonDouble::DoubleRepresentation rep4 = {
    false, 2, 0, 1, 0
  };
  JsonDouble d6(rep4);
  OLA_ASSERT_EQ(string("2.1"), JsonWriter::AsString(d6));
  OLA_ASSERT_EQ(2.1, d6.Value());
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
  JsonBool true_value(true);
  string expected = "true";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(true_value));

  JsonBool false_value(false);
  expected = "false";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(false_value));
}


/*
 * Test a null.
 */
void JsonTest::testNull() {
  JsonNull value;
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
 * Test for equality.
 */
void JsonTest::testEquality() {
  JsonString string1("foo");
  JsonString string2("foo");
  JsonString string3("bar");
  JsonBool bool1(true);
  JsonBool bool2(false);
  JsonNull null1;
  JsonDouble double1(1.0);
  JsonDouble double2(1.0);
  JsonDouble double3(2.1);

  JsonUInt uint1(10);
  JsonUInt uint2(99);

  JsonInt int1(10);
  JsonInt int2(99);
  JsonInt int3(-99);

  JsonInt64 int64_1(-99);
  JsonInt64 int64_2(10);
  JsonInt64 int64_3(99);

  JsonUInt64 uint64_1(10);
  JsonUInt64 uint64_2(99);

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
 * Test for integer / number inequality.
 */
void JsonTest::testIntInequality() {
  JsonDouble double1(1.0);
  JsonDouble double2(1.0);
  JsonDouble double3(11.1);
  JsonUInt uint1(10);
  JsonUInt uint2(99);
  JsonInt int1(10);
  JsonInt int2(99);
  JsonInt int3(-99);
  JsonInt64 int64_1(-99);
  JsonInt64 int64_2(10);
  JsonInt64 int64_3(99);
  JsonUInt64 uint64_1(10);
  JsonUInt64 uint64_2(99);

  OLA_ASSERT_LT(double1, double3);
  OLA_ASSERT_LTE(double1, double2);
  OLA_ASSERT_LTE(double1, double3);
  OLA_ASSERT_GT(double3, double1);
  OLA_ASSERT_GTE(double3, double1);
  OLA_ASSERT_GTE(double2, double1);
  OLA_ASSERT_LT(double1, uint1);
  OLA_ASSERT_LT(double1, int1);
  OLA_ASSERT_LT(double1, int64_2);
  OLA_ASSERT_LT(double1, uint64_1);
  OLA_ASSERT_LT(uint1, double3);
  OLA_ASSERT_LT(int1, double3);
  OLA_ASSERT_LT(int64_1, double3);
  OLA_ASSERT_LT(int64_2, double3);
  OLA_ASSERT_LT(uint64_1, double3);

  OLA_ASSERT_LT(uint1, uint2);
  OLA_ASSERT_LTE(uint1, uint1);
  OLA_ASSERT_LT(int1, int2);
  OLA_ASSERT_LTE(int1, int1);
  OLA_ASSERT_LT(int3, int1);
  OLA_ASSERT_LT(uint64_1, uint64_2);
  OLA_ASSERT_LTE(uint64_1, uint64_1);
  OLA_ASSERT_LT(int64_1, int64_2);
  OLA_ASSERT_LTE(int64_1, int64_1);
  OLA_ASSERT_LT(int64_2, int64_3);
  OLA_ASSERT_LT(uint64_1, uint2);
  OLA_ASSERT_LTE(uint64_1, uint1);
  OLA_ASSERT_LT(int64_1, int1);
  OLA_ASSERT_LTE(int64_1, int3);
  OLA_ASSERT_LT(uint1, uint64_2);
  OLA_ASSERT_LTE(uint1, uint64_1);
  OLA_ASSERT_LT(int3, int64_2);
  OLA_ASSERT_LTE(int3, int64_1);

  OLA_ASSERT_LT(int3, uint1);
  OLA_ASSERT_LTE(int1, uint1);
  OLA_ASSERT_LT(int64_1, uint1);
  OLA_ASSERT_LTE(int64_2, uint1);
  OLA_ASSERT_LT(uint1, int2);
  OLA_ASSERT_LTE(uint1, int1);
  OLA_ASSERT_LT(uint64_1, int2);
  OLA_ASSERT_LTE(uint64_1, int1);
  OLA_ASSERT_LT(int3, uint64_1);
  OLA_ASSERT_LTE(int1, uint64_1);
  OLA_ASSERT_LT(int64_1, uint64_1);
  OLA_ASSERT_LTE(int64_2, uint64_1);
  OLA_ASSERT_LT(uint1, int64_3);
  OLA_ASSERT_LTE(uint1, int64_2);
  OLA_ASSERT_LT(uint64_1, int64_3);
  OLA_ASSERT_LTE(uint64_1, int64_2);
}

/*
 * Test for mulitpleOf
 */
void JsonTest::testMultipleOf() {
  JsonDouble double1(10.0);
  JsonDouble double2(5);
  JsonDouble double3(11.0);
  JsonUInt uint1(10);
  JsonUInt uint2(5);
  JsonUInt uint3(11);
  JsonInt int1(10);
  JsonInt int2(5);
  JsonInt int3(11);
  JsonInt64 int64_1(10);
  JsonInt64 int64_2(5);
  JsonInt64 int64_3(11);
  JsonUInt64 uint64_1(10);
  JsonUInt64 uint64_2(5);
  JsonUInt64 uint64_3(11);

  OLA_ASSERT(double1.MultipleOf(double2));
  OLA_ASSERT(double1.MultipleOf(uint2));
  OLA_ASSERT(double1.MultipleOf(int2));
  OLA_ASSERT(double1.MultipleOf(uint64_2));
  OLA_ASSERT(double1.MultipleOf(int64_2));

  OLA_ASSERT(uint1.MultipleOf(double2));
  OLA_ASSERT(uint1.MultipleOf(uint2));
  OLA_ASSERT(uint1.MultipleOf(int2));
  OLA_ASSERT(uint1.MultipleOf(uint64_2));
  OLA_ASSERT(uint1.MultipleOf(int64_2));

  OLA_ASSERT(int1.MultipleOf(double2));
  OLA_ASSERT(int1.MultipleOf(uint2));
  OLA_ASSERT(int1.MultipleOf(int2));
  OLA_ASSERT(int1.MultipleOf(uint64_2));
  OLA_ASSERT(int1.MultipleOf(int64_2));

  OLA_ASSERT(int64_1.MultipleOf(double2));
  OLA_ASSERT(int64_1.MultipleOf(uint2));
  OLA_ASSERT(int64_1.MultipleOf(int2));
  OLA_ASSERT(int64_1.MultipleOf(uint64_2));
  OLA_ASSERT(int64_1.MultipleOf(int64_2));

  OLA_ASSERT(uint64_1.MultipleOf(double2));
  OLA_ASSERT(uint64_1.MultipleOf(uint2));
  OLA_ASSERT(uint64_1.MultipleOf(int2));
  OLA_ASSERT(uint64_1.MultipleOf(uint64_2));
  OLA_ASSERT(uint64_1.MultipleOf(int64_2));

  OLA_ASSERT_FALSE(double3.MultipleOf(double2));
  OLA_ASSERT_FALSE(double3.MultipleOf(uint2));
  OLA_ASSERT_FALSE(double3.MultipleOf(int2));
  OLA_ASSERT_FALSE(double3.MultipleOf(uint64_2));
  OLA_ASSERT_FALSE(double3.MultipleOf(int64_2));

  OLA_ASSERT_FALSE(uint3.MultipleOf(double2));
  OLA_ASSERT_FALSE(uint3.MultipleOf(uint2));
  OLA_ASSERT_FALSE(uint3.MultipleOf(int2));
  OLA_ASSERT_FALSE(uint3.MultipleOf(uint64_2));
  OLA_ASSERT_FALSE(uint3.MultipleOf(int64_2));

  OLA_ASSERT_FALSE(int3.MultipleOf(double2));
  OLA_ASSERT_FALSE(int3.MultipleOf(uint2));
  OLA_ASSERT_FALSE(int3.MultipleOf(int2));
  OLA_ASSERT_FALSE(int3.MultipleOf(uint64_2));
  OLA_ASSERT_FALSE(int3.MultipleOf(int64_2));

  OLA_ASSERT_FALSE(int64_3.MultipleOf(double2));
  OLA_ASSERT_FALSE(int64_3.MultipleOf(uint2));
  OLA_ASSERT_FALSE(int64_3.MultipleOf(int2));
  OLA_ASSERT_FALSE(int64_3.MultipleOf(uint64_2));
  OLA_ASSERT_FALSE(int64_3.MultipleOf(int64_2));

  OLA_ASSERT_FALSE(uint64_3.MultipleOf(double2));
  OLA_ASSERT_FALSE(uint64_3.MultipleOf(uint2));
  OLA_ASSERT_FALSE(uint64_3.MultipleOf(int2));
  OLA_ASSERT_FALSE(uint64_3.MultipleOf(uint64_2));
  OLA_ASSERT_FALSE(uint64_3.MultipleOf(int64_2));
}

/*
 * Test looking up a value with a pointer.
 */
void JsonTest::testLookups() {
  JsonPointer empty_pointer;
  JsonPointer invalid_pointer("/invalid/path");
  JsonPointer name_pointer("/name");

  JsonString string1("foo");
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(&string1),
                string1.LookupElement(empty_pointer));

#ifdef __FreeBSD__
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(0),
#else
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
#endif  // __FreeBSD__
                string1.LookupElement(invalid_pointer));

  // Now try an object
  JsonString *name_value = new JsonString("simon");
  JsonObject object;
  object.Add("age", 10);
  object.AddValue("name", name_value);
  object.Add("male", true);
  object.Add("", "foo");

  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(&object),
                object.LookupElement(empty_pointer));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(name_value),
                object.LookupElement(name_pointer));

#ifdef __FreeBSD__
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(0),
#else
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
#endif  // __FreeBSD__
                object.LookupElement(invalid_pointer));

  // Now try an array
  JsonArray *array = new JsonArray();
  JsonString *string2 = new JsonString("cat");
  JsonString *string3 = new JsonString("dog");
  JsonString *string4 = new JsonString("mouse");
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

#ifdef __FreeBSD__
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(0),
#else
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
#endif  // __FreeBSD__
                array->LookupElement(invalid_pointer));

  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(string2),
                array->LookupElement(first));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(string3),
                array->LookupElement(middle));
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(string4),
                array->LookupElement(last));

#ifdef __FreeBSD__
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(0),
#else
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
#endif  // __FreeBSD__
                array->LookupElement(one_past_last));

#ifdef __FreeBSD__
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(0),
#else
  OLA_ASSERT_EQ(reinterpret_cast<JsonValue*>(NULL),
#endif  // __FreeBSD__
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
  JsonString string1("foo");
  JsonBool bool1(true);
  JsonNull null1;
  JsonDouble double1(1.0);
  JsonUInt uint1(10);
  JsonInt int1(10);
  JsonInt64 int64_1(-99);
  JsonUInt64 uint64_1(10);

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
