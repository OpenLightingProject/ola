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
 * PatchParserTest.cpp
 * Unittest for the JSON Patch Parser.
 * Copyright (C) 2014 Simon Newton
 */

#include <string>

#include "ola/testing/TestUtils.h"
#include "ola/web/Json.h"
#include "ola/web/JsonData.h"
#include "ola/web/JsonParser.h"
#include "ola/web/JsonPatch.h"
#include "ola/web/JsonPatchParser.h"
#include "ola/web/JsonPointer.h"
#include "ola/web/JsonWriter.h"

#include "ola/Logging.h"

using ola::web::JsonArray;
using ola::web::JsonObject;
using ola::web::JsonParser;
using ola::web::JsonPatchAddOp;
using ola::web::JsonPatchCopyOp;
using ola::web::JsonPatchMoveOp;
using ola::web::JsonPatchParser;
using ola::web::JsonPatchRemoveOp;
using ola::web::JsonPatchReplaceOp;
using ola::web::JsonPatchSet;
using ola::web::JsonPatchTestOp;
using ola::web::JsonData;
using ola::web::JsonValue;
using ola::web::JsonWriter;
using std::auto_ptr;
using std::string;

class JsonPatchParserTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonPatchParserTest);
  CPPUNIT_TEST(testInvalid);
  CPPUNIT_TEST(testAdd);
  CPPUNIT_TEST(testRemove);
  CPPUNIT_TEST(testReplace);
  CPPUNIT_TEST(testMove);
  CPPUNIT_TEST(testCopy);
  CPPUNIT_TEST(testTest);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testInvalid();
  void testAdd();
  void testRemove();
  void testReplace();
  void testMove();
  void testCopy();
  void testTest();

 private:
    void CheckValuesMatch(const std::string &input, const JsonValue *actual);
    void BuildSampleText(JsonData *text);
    void CheckInvalid(const string &input);
    void CheckValid(const string &input, JsonPatchSet *patch_set);
};

void JsonPatchParserTest::CheckValuesMatch(const std::string &input,
                                           const JsonValue *actual) {
  string error;
  auto_ptr<const JsonValue> expected_value(JsonParser::Parse(input, &error));
  if (expected_value.get()) {
    if (*actual != *expected_value.get()) {
      OLA_ASSERT_EQ(JsonWriter::AsString(*(expected_value.get())),
                    JsonWriter::AsString(*actual));
    }
  } else {
    OLA_ASSERT_EQ(expected_value.get(), actual);
  }
}

void JsonPatchParserTest::BuildSampleText(JsonData *text) {
  auto_ptr<JsonObject> object(new JsonObject());
  object->Add("foo", "bar");
  object->Add("baz", false);

  JsonObject *child_object = new JsonObject();
  child_object->Add("bat", 1);
  object->AddValue("object", child_object);

  JsonArray *child_array = new JsonArray();
  child_array->Append(1);
  child_array->Append(2);
  child_array->Append(3);
  object->AddValue("array", child_array);

  text->SetValue(object.release());
}

void JsonPatchParserTest::CheckInvalid(const string &input) {
  JsonPatchSet patch_set;
  string error;
  OLA_ASSERT_FALSE(JsonPatchParser::Parse(input, &patch_set, &error));
  OLA_ASSERT_TRUE(patch_set.Empty());
  OLA_ASSERT_NE(string(""), error);
}

void JsonPatchParserTest::CheckValid(const string &input,
                                     JsonPatchSet *patch_set) {
  string error;
  OLA_ASSERT_TRUE(JsonPatchParser::Parse(input, patch_set, &error));
  OLA_ASSERT_FALSE(patch_set->Empty());
  OLA_ASSERT_EQ(string(""), error);
}

CPPUNIT_TEST_SUITE_REGISTRATION(JsonPatchParserTest);


void JsonPatchParserTest::testInvalid() {
  CheckInvalid("");
  CheckInvalid("{}");
  CheckInvalid("null");
  CheckInvalid("1");
  CheckInvalid("\"foo\"");
  CheckInvalid("true");
  CheckInvalid("[null]");
  CheckInvalid("[1]");
  CheckInvalid("[1.2]");
  CheckInvalid("[\"foo\"]");
  CheckInvalid("[[]]");

  CheckInvalid("[{}]");
  CheckInvalid("[{\"op\": \"\"}]");
  CheckInvalid("[{\"op\": \"foo\"}]");
}

void JsonPatchParserTest::testAdd() {
  CheckInvalid("[{\"op\": \"add\"}]");
  // Invalid and missing paths
  CheckInvalid("[{\"op\": \"add\", \"path\": null, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"add\", \"path\": true, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"add\", \"path\": 1, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"add\", \"path\": 1.2, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"add\", \"path\": {}, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"add\", \"path\": [], \"value\": {}}]");
  CheckInvalid("[{\"op\": \"add\", \"value\": {}}]");

  // Missing value
  CheckInvalid("[{\"op\": \"add\", \"path\": \"/foo\"}]");

  // Valid patches
  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"add\", \"path\": \"/foo\", \"value\": {}}]", &patch_set);

    JsonData text(new JsonObject());
    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
        "{\"foo\": {}}", text.Value());
  }

  // Complex nested value
  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"add\", \"path\": \"/foo\", "
        "\"value\": [{\"foo\": [[]]}] }]", &patch_set);

    JsonData text(new JsonObject());
    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
        "{\"foo\": [{\"foo\": [[]]}] }", text.Value());
  }
}

void JsonPatchParserTest::testRemove() {
  CheckInvalid("[{\"op\": \"remove\"}]");
  // Invalid and missing paths
  CheckInvalid("[{\"op\": \"remove\", \"path\": null}]");
  CheckInvalid("[{\"op\": \"remove\", \"path\": true}]");
  CheckInvalid("[{\"op\": \"remove\", \"path\": 1}]");
  CheckInvalid("[{\"op\": \"remove\", \"path\": 1.2}]");
  CheckInvalid("[{\"op\": \"remove\", \"path\": {}}]");
  CheckInvalid("[{\"op\": \"remove\", \"path\": []}]");
  CheckInvalid("[{\"op\": \"remove\"}]");

  JsonData text(NULL);
  BuildSampleText(&text);

  // Valid patches
  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"remove\", \"path\": \"/foo\"}]", &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"remove\", \"path\": \"/object\"},"
        " {\"op\": \"remove\", \"path\": \"/array\"}]", &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch("{\"baz\": false }", text.Value());
  }
}

void JsonPatchParserTest::testReplace() {
  CheckInvalid("[{\"op\": \"replace\"}]");
  // Invalid and missing paths
  CheckInvalid("[{\"op\": \"replace\", \"path\": null, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"replace\", \"path\": true, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"replace\", \"path\": 1, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"replace\", \"path\": 1.2, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"replace\", \"path\": {}, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"replace\", \"path\": [], \"value\": {}}]");
  CheckInvalid("[{\"op\": \"replace\", \"value\": {}}]");

  // Missing value
  CheckInvalid("[{\"op\": \"replace\", \"path\": \"/foo\"}]");

  JsonData text(NULL);
  BuildSampleText(&text);

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"replace\", \"path\": \"/foo\", \"value\": 42}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": 42, \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"replace\", \"path\": \"/foo\", \"value\": true}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": true, \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"replace\", \"path\": \"/foo\", \"value\": \"bar\"}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"replace\", \"path\": \"/foo\", \"value\": []}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": [], \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"replace\", \"path\": \"/foo\", \"value\": {}}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": {}, \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }
}

void JsonPatchParserTest::testMove() {
  CheckInvalid("[{\"op\": \"move\"}]");
  // Invalid and missing paths
  CheckInvalid("[{\"op\": \"move\", \"path\": null, \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": true, \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": 1, \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": {}, \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": [], \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"move\", \"from\": {}}]");

  // Missing or invalid from
  CheckInvalid("[{\"op\": \"move\", \"path\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": \"/foo\", \"from\": null}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": \"/foo\", \"from\": true}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": \"/foo\", \"from\": 1}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": \"/foo\", \"from\": []}]");
  CheckInvalid("[{\"op\": \"move\", \"path\": \"/foo\", \"from\": {}}]");

  JsonData text(NULL);
  BuildSampleText(&text);

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"move\", \"path\": \"/foo\", \"from\": \"/baz\"}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"move\", \"path\": \"/foo\", \"from\": \"/array/1\"}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": 2, "
      " \"object\": {\"bat\": 1}, \"array\": [1,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"move\", \"path\": \"/foo\", \"from\": \"/object/bat\"}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": 1, "
      " \"object\": {}, \"array\": [1,3] }",
      text.Value());
  }
}

void JsonPatchParserTest::testCopy() {
  CheckInvalid("[{\"op\": \"copy\"}]");
  // Invalid and missing paths
  CheckInvalid("[{\"op\": \"copy\", \"path\": null, \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": true, \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": 1, \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": {}, \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": [], \"from\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"copy\", \"from\": {}}]");

  // Missing or invalid from
  CheckInvalid("[{\"op\": \"copy\", \"path\": \"/foo\"}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": \"/foo\", \"from\": null}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": \"/foo\", \"from\": true}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": \"/foo\", \"from\": 1}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": \"/foo\", \"from\": []}]");
  CheckInvalid("[{\"op\": \"copy\", \"path\": \"/foo\", \"from\": {}}]");

  JsonData text(NULL);
  BuildSampleText(&text);

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"copy\", \"path\": \"/foo\", \"from\": \"/baz\"}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": false, \"baz\": false,"
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"copy\", \"path\": \"/foo\", \"from\": \"/array/1\"}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": 2, \"baz\": false,"
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"copy\", \"path\": \"/foo\", \"from\": \"/object/bat\"}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": 1, \"baz\": false,"
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }
}

void JsonPatchParserTest::testTest() {
  CheckInvalid("[{\"op\": \"test\"}]");
  // Invalid and missing paths
  CheckInvalid("[{\"op\": \"test\", \"path\": null, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"test\", \"path\": true, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"test\", \"path\": 1, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"test\", \"path\": 1.2, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"test\", \"path\": {}, \"value\": {}}]");
  CheckInvalid("[{\"op\": \"test\", \"path\": [], \"value\": {}}]");
  CheckInvalid("[{\"op\": \"test\", \"value\": {}}]");

  // Missing value
  CheckInvalid("[{\"op\": \"test\", \"path\": \"/foo\"}]");

  JsonData text(NULL);
  BuildSampleText(&text);

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"test\", \"path\": \"/foo\", \"value\": \"bar\"}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"test\", \"path\": \"/array\", \"value\": [1,2,3]}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"test\", \"path\": \"/object/bat\", \"value\": 1}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  {
    JsonPatchSet patch_set;
    CheckValid(
        "[{\"op\": \"test\", \"path\": \"/baz\", \"value\": false}]",
        &patch_set);

    OLA_ASSERT_TRUE(text.Apply(patch_set));
    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }
}
