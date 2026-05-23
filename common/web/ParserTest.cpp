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
 * ParserTest.cpp
 * Unittest for the Json Parser.
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>

#include "ola/web/Json.h"
#include "ola/web/JsonLexer.h"
#include "ola/web/JsonParser.h"
#include "ola/web/JsonWriter.h"
#include "ola/testing/TestUtils.h"

using ola::web::JsonArray;
using ola::web::JsonBool;
using ola::web::JsonInt;
using ola::web::JsonLexer;
using ola::web::JsonNull;
using ola::web::JsonObject;
using ola::web::JsonParser;
using ola::web::JsonString;
using ola::web::JsonUInt;
using ola::web::JsonValue;
using ola::web::JsonWriter;
using std::unique_ptr;
using std::string;

class JsonParserTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonParserTest);
  CPPUNIT_TEST(testParseBool);
  CPPUNIT_TEST(testParseNull);
  CPPUNIT_TEST(testParseString);
  CPPUNIT_TEST(testParseNumber);
  CPPUNIT_TEST(testArray);
  CPPUNIT_TEST(testObject);
  CPPUNIT_TEST(testInvalidInput);
  CPPUNIT_TEST(testStressTests);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testParseBool();
    void testParseNull();
    void testParseString();
    void testParseNumber();
    void testArray();
    void testObject();
    void testInvalidInput();
    void testStressTests();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonParserTest);


void JsonParserTest::testParseBool() {
  string error;
  unique_ptr<const JsonValue> value(JsonParser::Parse("  true  ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("true"), JsonWriter::AsString(*value.get()));

  // no whitespace
  value.reset(JsonParser::Parse("true", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("true"), JsonWriter::AsString(*value.get()));

  // tabs
  value.reset(JsonParser::Parse("\ttrue\r\n", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("true"), JsonWriter::AsString(*value.get()));

  // false values
  value.reset(JsonParser::Parse("false", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("false"), JsonWriter::AsString(*value.get()));
}

void JsonParserTest::testParseNull() {
  string error;
  unique_ptr<const JsonValue> value(JsonParser::Parse("  null  ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("null"), JsonWriter::AsString(*value.get()));
}

void JsonParserTest::testParseString() {
  string error;
  unique_ptr<const JsonValue> value(JsonParser::Parse("\"test\"", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("\"test\""), JsonWriter::AsString(*value.get()));

  // test a missing "
  value.reset(JsonParser::Parse("\"test", &error));
  OLA_ASSERT_NULL(value.get());

  // test escaping
  value.reset(JsonParser::Parse("\"test\\\" escape\"", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("\"test\\\" escape\""),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("\"<br>\n\\n<br/>\"", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  /*
  OLA_ASSERT_EQ(string("\"<br>\n\\n<br/>\""),
                JsonWriter::AsString(*value.get()));
  */
}

void JsonParserTest::testParseNumber() {
  string error;
  unique_ptr<const JsonValue> value(JsonParser::Parse(" 0", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(" -0", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("  1  ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("1"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("4096 ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("4096"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(" -1", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-1"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(" -345", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-345"), JsonWriter::AsString(*value.get()));

  // Decimals
  value.reset(JsonParser::Parse(" 0.0", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(" 0.1", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0.1"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("0.123456", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0.123456"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-0.123456", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-0.123456"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("3.14159", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("3.14159"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-3.14159", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-3.14159"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-17.079", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-17.079"), JsonWriter::AsString(*value.get()));

  // Exponents
  value.reset(JsonParser::Parse("0e1", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("0e-2", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-0E1", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-0e-3", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("1E4", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("1e4"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("5e-4", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("5e-4"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("912E-2", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("912e-2"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-23e4", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-23e4"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-912E-2", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-912e-2"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("14e0", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("14"), JsonWriter::AsString(*value.get()));

  // exponents with decimals
  value.reset(JsonParser::Parse("-3.1e2", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-3.1e2"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("3.14E3", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("3.14e3"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("3.14e-1", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("3.14e-1"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-2.718e-1", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-2.718e-1"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(" -0.2718E+2 ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-0.2718e2"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("-0.2718e-1 ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("-0.2718e-1"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("0.2718e+1", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0.2718e1"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("0.2718e-2  ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("0.2718e-2"), JsonWriter::AsString(*value.get()));

  // Invalid inputs
  value.reset(JsonParser::Parse("-", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("1e", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("1E", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("1e-", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("1E-", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("1e+", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("1E+", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("-", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("a", &error));
  OLA_ASSERT_NULL(value.get());
}

void JsonParserTest::testArray() {
  string error;
  unique_ptr<const JsonValue> value(JsonParser::Parse(" [  ]", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("[]"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("[ 1] ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("[1]"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("[  true,\tfalse ,null ] ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("[true, false, null]"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("[ 1\n,  2 \t, 3,4 ] ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("[1, 2, 3, 4]"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(
        "[\"test]\", 1, [\"nested\"], 4] ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("[\n  \"test]\",\n  1,\n  [\"nested\"],\n  4\n]"),
                JsonWriter::AsString(*value.get()));

  // Invalid input
  value.reset(JsonParser::Parse("[abc] ", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("[,] ", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse(" [\n, ] ", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse(" [", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse(" [1", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse(" [1,", &error));
  OLA_ASSERT_NULL(value.get());
}

void JsonParserTest::testObject() {
  string error;
  unique_ptr<const JsonValue> value(JsonParser::Parse("{}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{}"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{\"key\"  : 1} ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"key\": 1\n}"),
                JsonWriter::AsString(*value.get()));

  // Multiple keys
  value.reset(JsonParser::Parse(
        "{\"key1\"  : 1, \"key2\"  : 2} ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"key1\": 1,\n  \"key2\": 2\n}"),
                JsonWriter::AsString(*value.get()));

  // Nested
  value.reset(JsonParser::Parse(
        "{\"key1\"  : 1, \"key2\"  : {\"age\": 24}} ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(
      string("{\n  \"key1\": 1,\n  \"key2\": {\n    \"age\": 24\n  }\n}"),
      JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(
        "{\"key1\"  : 1, \"key2\"  : [1,  2 ] } ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"key1\": 1,\n  \"key2\": [1, 2]\n}"),
                JsonWriter::AsString(*value.get()));

  // double key test
  value.reset(JsonParser::Parse("{\"key\"  : 1, \"key\"  : 2} ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"key\": 2\n}"),
                JsonWriter::AsString(*value.get()));

  // trailing comma test. This is outside the standard, but seems to be
  // commonly used.
  /*
  value.reset(JsonParser::Parse("{\"key\"  : 1, } ", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"key\": 2\n}"),
                JsonWriter::AsString(*value.get()));
  */

  value.reset(JsonParser::Parse("{", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{1", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{true", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{\"", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{\"key", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{\"key\"", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{\"key\":", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{\"key\" : }", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{'key': 1}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse(
        "{\"default\": {\"pets\": [1, 2, {null}]}}}", &error));
  OLA_ASSERT_NULL(value.get());
}

void JsonParserTest::testInvalidInput() {
  string error;
  unique_ptr<const JsonValue> value(JsonParser::Parse(" ", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("TRUE", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("FALSE", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("NULL", &error));
  OLA_ASSERT_NULL(value.get());
}

/**
 * From https://code.google.com/p/json-smart/wiki/FeaturesTests
 * Some of these overlap with the ones above.
 */
void JsonParserTest::testStressTests() {
  string error;
  unique_ptr<const JsonValue> value(JsonParser::Parse("{}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{}"), JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"v\":\"1\"}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": \"1\"\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"v\":\"1\"\r\n}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": \"1\"\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"v\":1}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": 1\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"v\":\"ab'c\"}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": \"ab'c\"\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"PI\":3.141E-10}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"PI\": 3.141e-10\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"PI\":3.141e-10}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"PI\": 3.141e-10\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"v\":12345123456789}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": 12345123456789\n}"),
                JsonWriter::AsString(*value.get()));

  /*
  // We don't pass the big-int test.
  value.reset(JsonParser::Parse("{ \"v\":123456789123456789123456789}",
                                &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": 123456789123456789123456789\n}"),
                JsonWriter::AsString(*value.get()));
  */

  value.reset(JsonParser::Parse("[ 1,2,3,4]", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("[1, 2, 3, 4]"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("[ \"1\",\"2\",\"3\",\"4\"]", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("[\"1\", \"2\", \"3\", \"4\"]"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("[ { }, { }, []]", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("[\n  {},\n  {},\n  []\n]"),
                JsonWriter::AsString(*value.get()));

  /*
  value.reset(JsonParser::Parse("{ \"v\":\"\u2000\u20ff\"}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": \"\"\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"v\":\"\u2000\u20FF\"}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": \"\"\n}"),
                JsonWriter::AsString(*value.get()));
  */

  value.reset(JsonParser::Parse("{ \"a\":\"hp://foo\"}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"a\": \"hp:\\/\\/foo\"\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"a\":null}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"a\": null\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"a\":true}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"a\": true\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{ \"a\" : true }", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"a\": true\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(
        "{ \"v\" : 1.7976931348623157E308}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": 1.7976931348623157e308\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse(
        "{ \"v\" : 1.79E08}", &error));
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{\n  \"v\": 1.79e8\n}"),
                JsonWriter::AsString(*value.get()));

  value.reset(JsonParser::Parse("{'X' : 's }", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{\"X\" : \"s }", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{'X", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{\"X", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"v\":'ab\"c'}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"v\":str}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"v\":It's'Work}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ a:1234}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("[ a,bc]", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"v\":s1 s2}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"v\":s1   s2  }", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"a\":\"foo.bar\"}#toto", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ 'value':'string'}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{v:15-55}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{v:15%}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{v:15.06%}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"v\":s1' s2}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"v\":s1\" \"s2}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ \"NaN\":NaN}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("[ a},b]", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("[ a:,b]", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ a,b:123}", &error));
  OLA_ASSERT_NULL(value.get());

  value.reset(JsonParser::Parse("{ a]b:123}", &error));
  OLA_ASSERT_NULL(value.get());
}
