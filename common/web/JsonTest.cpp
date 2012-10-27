/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
using ola::web::JsonIntValue;
using ola::web::JsonNullValue;
using ola::web::JsonObject;
using ola::web::JsonStringValue;
using ola::web::JsonUIntValue;
using ola::web::JsonValue;
using ola::web::JsonWriter;
using std::string;
using std::stringstream;

class JsonTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonTest);
  CPPUNIT_TEST(testString);
  CPPUNIT_TEST(testNumberValues);
  CPPUNIT_TEST(testBool);
  CPPUNIT_TEST(testNull);
  CPPUNIT_TEST(testSimpleArray);
  CPPUNIT_TEST(testSimpleObject);
  CPPUNIT_TEST(testComplexObject);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testString();
    void testNumberValues();
    void testBool();
    void testNull();
    void testSimpleArray();
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
 * Test numbers.
 */
void JsonTest::testNumberValues() {
  JsonUIntValue uint_value(10);
  string expected = "10";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(uint_value));

  JsonIntValue int_value(-10);
  expected = "-10";
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(int_value));
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
      "}"
  );
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
      "}"
  );
  OLA_ASSERT_EQ(expected, JsonWriter::AsString(object));
}
