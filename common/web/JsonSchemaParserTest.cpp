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
 * JsonSchemaParserTest.cpp
 * Unittest for the Json Schema.
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "ola/web/JsonSchema.h"


using ola::web::JsonSchema;
using ola::web::JsonObject;
using std::auto_ptr;
using std::string;

class JsonSchemaParserTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonSchemaParserTest);
  /*
  CPPUNIT_TEST(testPrimitiveTypes);
  CPPUNIT_TEST(testEmptySchema);
  CPPUNIT_TEST(testInvalidSchema);
  */
  CPPUNIT_TEST(testDefinitions);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testPrimitiveTypes();
  void testEmptySchema();
  void testInvalidSchema();
  void testDefinitions();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonSchemaParserTest);

void JsonSchemaParserTest::testPrimitiveTypes() {
  string error;
  auto_ptr<JsonSchema> schema(JsonSchema::FromString("null", &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_FALSE(error.empty());

  schema.reset(JsonSchema::FromString("1", &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_FALSE(error.empty());

  schema.reset(JsonSchema::FromString("-1", &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_FALSE(error.empty());

  schema.reset(JsonSchema::FromString("true", &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_FALSE(error.empty());

  schema.reset(JsonSchema::FromString("[1, 2]", &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_FALSE(error.empty());

  schema.reset(JsonSchema::FromString("\"foo\"", &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_FALSE(error.empty());

  schema.reset(JsonSchema::FromString("[null, [1], {} ]", &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_FALSE(error.empty());
}

void JsonSchemaParserTest::testEmptySchema() {
  string error;
  auto_ptr<JsonSchema> schema(JsonSchema::FromString("{}", &error));

  OLA_ASSERT_NOT_NULL(schema.get());
  OLA_ASSERT_TRUE(error.empty());

  auto_ptr<const JsonObject> value(schema->AsJson());
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_ASSERT_EQ(string("{}"), ola::web::JsonWriter::AsString(*value));
}

void JsonSchemaParserTest::testInvalidSchema() {
}

void JsonSchemaParserTest::testDefinitions() {
  string input =
      "{\n"
      "  \"$schema\": \"http:\\/\\/json-schema.org\\/draft-04\\/schema#\",\n"
      "  \"definitions\": {\n"
      "    \"foo\": {\n"
      "      \"title\": \"bar\",\n"
      "      \"type\": \"number\"\n"
      "    }\n"
      "  },\n"
      "  \"type\": \"object\",\n"
      "  \"description\": \"A sample schema\",\n"
      "  \"id\": \"http:\\/\\/json-schema.org\\/draft-04\\/schema#\",\n"
      "  \"title\": \"Hi\",\n"
      "  \"properties\": {\n"
      "    \"name\": {\n"
      "      \"type\": \"boolean\"\n"
      "    }\n"
      "  }\n"
      "}";
  string error;
  auto_ptr<JsonSchema> schema(JsonSchema::FromString(input, &error));

  if (!error.empty()) {
    OLA_INFO << "Schema parse error: " << error;
  }
  OLA_ASSERT_NOT_NULL(schema.get());
  OLA_ASSERT_TRUE(error.empty());

  auto_ptr<const JsonObject> value(schema->AsJson());
  OLA_ASSERT_NOT_NULL(value.get());
  OLA_INFO << ola::web::JsonWriter::AsString(*value);
  OLA_ASSERT_EQ(input, ola::web::JsonWriter::AsString(*value));
}
