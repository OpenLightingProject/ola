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
#include "ola/web/JsonSchema.h"


using ola::web::AllOfValidator;
using ola::web::AnyOfValidator;
using ola::web::ArrayValidator;
using ola::web::JsonArray;
using ola::web::JsonValue;
using ola::web::JsonBoolValue;
using ola::web::JsonIntValue;
using ola::web::JsonNullValue;
using ola::web::JsonObject;
using ola::web::JsonSchema;
using ola::web::JsonStringValue;
using ola::web::JsonUIntValue;
using ola::web::MaximumConstraint;
using ola::web::MinimumConstraint;
using ola::web::NotValidator;
using ola::web::NumberValidator;
using ola::web::ObjectValidator;
using ola::web::OneOfValidator;
using ola::web::StringValidator;
using ola::web::ValidatorInterface;
using ola::web::WildcardValidator;
using std::auto_ptr;
using std::string;
using std::vector;

class JsonSchemaTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonSchemaTest);
  CPPUNIT_TEST(testParseBool);
  CPPUNIT_TEST(testParseFromString);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testParseBool();
    void testParseFromString();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonSchemaTest);


void JsonSchemaTest::testParseBool() {
  ObjectValidator::Options root_options;
  // root_options.required_properties.insert("name");
  root_options.required_properties.insert("age");

  /*
  vector<ValidatorInterface*> validators;
  NumberValidator *number_validator = new NumberValidator();
  number_validator->AddConstraint(new MaximumConstraint(7, true));
  validators.push_back(number_validator);

  number_validator = new NumberValidator();
  number_validator->AddConstraint(new MinimumConstraint(3, true));
  validators.push_back(number_validator);
  */

  ObjectValidator root_validator(root_options);
  // root_validator.AddValidator("age", new OneOfValidator(&validators));
  /*
  root_validator.AddValidator(
      "age",
      new NotValidator(new NumberValidator()));
  */
  /*
  validators.push_back(
      new StringValidator(StringValidator::Options()));
  ArrayValidator array_validator(
      &validators,
      new NumberValidator(),
      ArrayValidator::Options());

  ArrayValidator::Options array_options;
  ArrayValidator array_validator(
      new ArrayValidator(new WildcardValidator(), ArrayValidator::Options()),
      array_options);
  root_validator.AddValidator("array", &array_validator);

  NumberValidator number_validator;
  */
  NumberValidator *number_validator = new NumberValidator();
  number_validator->AddConstraint(new MaximumConstraint(
        JsonValue::NewNumberValue(7), true));
  number_validator->SetTitle("Age");
  number_validator->SetDescription("Age of the Human");

  root_validator.AddValidator("age", new NotValidator(number_validator));
  auto_ptr<JsonObject> schema(root_validator.GetSchema());

  // OLA_INFO << ola::web::JsonWriter::AsString(*schema.get());
  /*
  JsonObject object;
  object.Add("age", true);
  */

  /*
  JsonArray *array = object.AddArray("array");
  JsonArray *inner_array = array->AppendArray();
  // *inner_array = array->AppendArray();
  inner_array->Append("test");
  // array->Append(1);
  */

  // object.Accept(&root_validator);
  // OLA_ASSERT_TRUE(root_validator.IsValid());
}

void JsonSchemaTest::testParseFromString() {
  /*
  auto_ptr<JsonSchema> schema(JsonSchema::FromString(
        "{"
        " \"title\": \"Hi\""
        "}"
  ));
  */

  /*
  auto_ptr<JsonSchema> schema(JsonSchema::FromString(
        "{"
        "  \"definitions\": {"
        "    \"foo\": {"
        "    }"
        "  },"
        "  \"title\": \"Hi\""
        ""
  ));

  auto_ptr<JsonSchema> schema(JsonSchema::FromString(
        "{"
        "  \"definitions\": {"
        "    \"foo\": {"
        "    }"
        "  },"
        "  \"title\": \"Hi\""
        ""));

  if (schema.get()) {
    OLA_INFO << "Json Schema was valid";
    const JsonObject *value = schema->AsJson();
    if (value) {
      OLA_INFO << ola::web::JsonWriter::AsString(*value);
    }
  } else {
    OLA_WARN << "Parsing failed";
  }
  */
}
