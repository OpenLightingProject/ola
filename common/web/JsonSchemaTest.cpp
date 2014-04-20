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

#include "ola/web/Json.h"
#include "ola/web/JsonSchema.h"
#include "ola/testing/TestUtils.h"


using ola::web::JsonArray;
using ola::web::JsonBoolValue;
using ola::web::JsonIntValue;
using ola::web::JsonNullValue;
using ola::web::JsonObject;
using ola::web::JsonStringValue;
using ola::web::JsonUIntValue;
using ola::web::ObjectValidator;
using ola::web::StringValidator;
using ola::web::NumberValidator;
using ola::web::MultipleOfConstraint;
using ola::web::MaximumConstraint;
using ola::web::MinimumConstraint;
using std::auto_ptr;
using std::string;

class JsonSchemaTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonSchemaTest);
  CPPUNIT_TEST(testParseBool);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testParseBool();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonSchemaTest);


void JsonSchemaTest::testParseBool() {
  ObjectValidator root_validator;
  StringValidator name_validator((StringValidator::Options()));
  root_validator.AddValidator("test", &name_validator);

  NumberValidator number_validator;
  number_validator.AddConstraint(new MaximumConstraint(7, false));
  root_validator.AddValidator("age", &number_validator);

  JsonObject object;
  object.Add("age", 7);
  object.Accept(&root_validator);
  OLA_ASSERT_TRUE(root_validator.IsValid());
}
