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
 * PatchTest.cpp
 * Unittest for the Json Patch.
 * Copyright (C) 2014 Simon Newton
 */

#include <string>

#include "ola/testing/TestUtils.h"
#include "ola/web/Json.h"
#include "ola/web/JsonData.h"
#include "ola/web/JsonPatch.h"
#include "ola/web/JsonParser.h"
#include "ola/web/JsonWriter.h"
#include "ola/web/JsonPointer.h"

#include "ola/Logging.h"

using ola::web::JsonArray;
using ola::web::JsonBool;
using ola::web::JsonInt;
using ola::web::JsonNull;
using ola::web::JsonObject;
using ola::web::JsonParser;
using ola::web::JsonPatchAddOp;
using ola::web::JsonPatchCopyOp;
using ola::web::JsonPatchMoveOp;
using ola::web::JsonPatchRemoveOp;
using ola::web::JsonPatchReplaceOp;
using ola::web::JsonPatchSet;
using ola::web::JsonPatchTestOp;
using ola::web::JsonPointer;
using ola::web::JsonString;
using ola::web::JsonData;
using ola::web::JsonValue;
using ola::web::JsonWriter;
using std::unique_ptr;
using std::string;

class JsonPatchTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonPatchTest);
  CPPUNIT_TEST(testAddOp);
  CPPUNIT_TEST(testRemoveOp);
  CPPUNIT_TEST(testReplaceOp);
  CPPUNIT_TEST(testCopyOp);
  CPPUNIT_TEST(testMoveOp);
  CPPUNIT_TEST(testTestOp);
  CPPUNIT_TEST(testAtomicUpdates);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testAddOp();
  void testRemoveOp();
  void testReplaceOp();
  void testCopyOp();
  void testMoveOp();
  void testTestOp();
  void testAtomicUpdates();

 private:
  void CheckValuesMatch(const std::string &, const JsonValue *actual);
  void BuildSampleText(JsonData *text);
};

void JsonPatchTest::CheckValuesMatch(const std::string &input,
                                     const JsonValue *actual) {
  string error;
  unique_ptr<const JsonValue> expected_value(JsonParser::Parse(input, &error));
  if (expected_value.get()) {
    if (*actual != *expected_value.get()) {
      OLA_ASSERT_EQ(JsonWriter::AsString(*(expected_value.get())),
                    JsonWriter::AsString(*actual));
    }
  } else {
    OLA_ASSERT_EQ(expected_value.get(), actual);
  }
}

void JsonPatchTest::BuildSampleText(JsonData *text) {
  unique_ptr<JsonObject> object(new JsonObject());
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

CPPUNIT_TEST_SUITE_REGISTRATION(JsonPatchTest);

void JsonPatchTest::testAddOp() {
  JsonData text(NULL);

  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(JsonPointer("/foo"), new JsonNull()));
    OLA_ASSERT_FALSE(text.Apply(patch));
  }

  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(JsonPointer(""), new JsonObject()));
    OLA_ASSERT_TRUE(text.Apply(patch));
    CheckValuesMatch("{}", text.Value());
  }

  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/name"), new JsonString("simon")));
    OLA_ASSERT_TRUE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"simon\"}", text.Value());
  }

  // test replace
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/name"), new JsonString("james")));
    OLA_ASSERT_TRUE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\"}", text.Value());
  }

  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/numbers"), new JsonArray()));
    OLA_ASSERT_TRUE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\", \"numbers\": []}", text.Value());
  }

  // Append
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/numbers/-"), new JsonInt(1)));
    OLA_ASSERT_TRUE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\", \"numbers\": [1]}", text.Value());
  }

  // Array replace.
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/numbers/0"), new JsonInt(2)));
    OLA_ASSERT_TRUE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\", \"numbers\": [2, 1]}",
                     text.Value());
  }

  // out of bounds
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/numbers/2"), new JsonInt(3)));
    OLA_ASSERT_FALSE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\", \"numbers\": [2, 1]}",
                     text.Value());
  }

  // non-int array index
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/numbers/bar"), new JsonInt(3)));
    OLA_ASSERT_FALSE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\", \"numbers\": [2, 1]}",
                     text.Value());
  }

  // missing parent
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/pets/fluffy"), new JsonObject()));
    OLA_ASSERT_FALSE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\", \"numbers\": [2, 1]}",
                     text.Value());
  }

  // add to a leaf node
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(
          JsonPointer("/name/middle"), new JsonNull()));
    OLA_ASSERT_FALSE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\", \"numbers\": [2, 1]}",
                     text.Value());
  }

  // Add a null to an object, this isn't allowed.
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(JsonPointer("/foo"), NULL));
    OLA_ASSERT_FALSE(text.Apply(patch));
    CheckValuesMatch("{\"name\": \"james\", \"numbers\": [2, 1]}",
                     text.Value());
  }

  // Add a null
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchAddOp(JsonPointer(""), NULL));
    OLA_ASSERT_TRUE(text.Apply(patch));
    CheckValuesMatch("", text.Value());
  }
}

void JsonPatchTest::testRemoveOp() {
  JsonData text(NULL);
  BuildSampleText(&text);

  CheckValuesMatch(
    "{\"foo\": \"bar\", \"baz\": false, "
    " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
    text.Value());

  // Try removing /object/bat
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/object/bat")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Try removing /array/1
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/array/1")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {}, \"array\": [1,3] }",
      text.Value());
  }

  // Try removing /array/-
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/array/-")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {}, \"array\": [1] }",
      text.Value());
  }

  // Try removing /array/1
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/array/1")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {}, \"array\": [1] }",
      text.Value());
  }

  // Try removing /array/-
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/array/-")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {}, \"array\": [] }",
      text.Value());
  }

  // Try removing /array/-
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/array/1")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {}, \"array\": [] }",
      text.Value());
  }

  // Try removing /foo
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/foo")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"baz\": false, \"object\": {}, \"array\": [] }",
      text.Value());
  }

  // Try removing /array & /object
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/array")));
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/object")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch("{\"baz\": false }", text.Value());
  }

  // Try removing something that doesn't exist
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/foo")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch("{\"baz\": false }", text.Value());
  }

  // Finally remove the entire value.
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch("", text.Value());
  }

  OLA_ASSERT_NULL(text.Value());

  // Test we don't crash if we try to remove on an empty value
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/foo")));
    OLA_ASSERT_FALSE(text.Apply(patch));
    CheckValuesMatch("", text.Value());
  }
}

void JsonPatchTest::testReplaceOp() {
  JsonData text(NULL);
  BuildSampleText(&text);

  // Invalid pointer
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("foo"),
                new JsonString("test")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Simple key replace
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/foo"),
                new JsonString("test")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Replace an array index
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/array/1"),
                new JsonInt(4)));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,4,3] }",
      text.Value());
  }

  // Replace the last item in the array
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/array/-"),
                new JsonInt(5)));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,4,5] }",
      text.Value());
  }

  // Non-int index
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/array/foo"),
                new JsonInt(5)));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,4,5] }",
      text.Value());
  }

  // Out-of-range index
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/array/3"),
                new JsonInt(5)));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,4,5] }",
      text.Value());
  }

  // Missing parent
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/missing/3"),
                new JsonInt(5)));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,4,5] }",
      text.Value());
  }

  // 2 level path
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/object/bat"),
                new JsonInt(4)));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 4}, \"array\": [1,4,5] }",
      text.Value());
  }

  // Missing element
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/object/barrrr"),
                new JsonInt(4)));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 4}, \"array\": [1,4,5] }",
      text.Value());
  }

  // Replace entire array
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/array"), new JsonArray()));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 4}, \"array\": [] }",
      text.Value());
  }

  // Another out-of-range
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer("/array/0"),
                new JsonArray()));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"test\", \"baz\": false, "
      " \"object\": {\"bat\": 4}, \"array\": [] }",
      text.Value());
  }

  // Replace the entire text
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer(""), new JsonObject()));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch("{}", text.Value());
  }

  // Replace with a NULL
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer(""), NULL));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch("", text.Value());
  }

  // Replace a NULL
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(JsonPointer(""), new JsonObject()));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch("{}", text.Value());
  }
}

void JsonPatchTest::testMoveOp() {
  JsonData text(NULL);

  // Move a null value
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(JsonPointer("/foo"), JsonPointer("/foo")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch("", text.Value());
  }

  BuildSampleText(&text);

  // Invalid src pointer
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(JsonPointer("foo"), JsonPointer("/foo")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Invalid dst pointer
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(JsonPointer("/foo"), JsonPointer("baz")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Identity move
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(JsonPointer("/foo"), JsonPointer("/foo")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Test prefix handling, you can't move an object into itself
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(
          JsonPointer("/object"), JsonPointer("/object/bat")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Simple move (add)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(JsonPointer("/foo"), JsonPointer("/bar")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"bar\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Simple move (replace)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(JsonPointer("/bar"), JsonPointer("/baz")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"baz\": \"bar\", "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Replace an inner value with an outer (array)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(
          JsonPointer("/array/1"), JsonPointer("/array")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"baz\": \"bar\", "
      " \"object\": {\"bat\": 1}, \"array\": 2 }",
      text.Value());
  }

  // Replace an inner value with an outer (object)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(
          JsonPointer("/object/bat"), JsonPointer("/object")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"baz\": \"bar\", "
      " \"object\": 1, \"array\": 2 }",
      text.Value());
  }

  // Replace the root
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchMoveOp(
          JsonPointer("/baz"), JsonPointer("")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch("\"bar\"", text.Value());
  }
}

void JsonPatchTest::testCopyOp() {
  JsonData text(NULL);

  // Copy a null value
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(JsonPointer("/foo"), JsonPointer("/foo")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch("", text.Value());
  }

  BuildSampleText(&text);

  // Invalid src pointer
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(JsonPointer("foo"), JsonPointer("/foo")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Invalid dst pointer
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(JsonPointer("/foo"), JsonPointer("baz")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Identity copy
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(JsonPointer("/foo"), JsonPointer("/foo")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Basic copy (replace)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(JsonPointer("/foo"), JsonPointer("/baz")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Basic copy (add)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(JsonPointer("/foo"), JsonPointer("/qux")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", \"qux\": \"bar\", "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Copy into array
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(
          JsonPointer("/foo"), JsonPointer("/array/1")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", \"qux\": \"bar\", "
      " \"object\": {\"bat\": 1}, \"array\": [1,\"bar\",2, 3] }",
      text.Value());
  }

  // Copy into object (add)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(
          JsonPointer("/foo"), JsonPointer("/object/bar")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", \"qux\": \"bar\", "
      " \"object\": {\"bat\": 1, \"bar\": \"bar\"}, "
      " \"array\": [1,\"bar\",2, 3] }",
      text.Value());
  }

  // Copy into object (replace)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(
          JsonPointer("/foo"), JsonPointer("/object/bat")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", \"qux\": \"bar\", "
      " \"object\": {\"bat\": \"bar\", \"bar\": \"bar\"}, "
      " \"array\": [1,\"bar\",2, 3] }",
      text.Value());
  }

  // Replace an inner value with the object itself
  {
    // First some cleanup
    {
      JsonPatchSet patch;
      patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/object/bar")));
      patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/array/1")));
      patch.AddOp(new JsonPatchRemoveOp(JsonPointer("/qux")));
      OLA_ASSERT_TRUE(text.Apply(patch));

      CheckValuesMatch(
        "{\"foo\": \"bar\", \"baz\": \"bar\", "
        " \"object\": {\"bat\": \"bar\"}, "
        " \"array\": [1,2, 3] }",
        text.Value());
    }

    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(
          JsonPointer("/object"), JsonPointer("/object/bat")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", "
      " \"object\": {\"bat\": { \"bat\": \"bar\"} }, "
      " \"array\": [1,2, 3] }",
      text.Value());
  }

  // Replace an object with an inner value.
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(
          JsonPointer("/object/bat"), JsonPointer("/object")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", "
      " \"object\": { \"bat\": \"bar\"}, "
      " \"array\": [1,2, 3] }",
      text.Value());
  }

  // Copy an array to itself
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(
          JsonPointer("/array"), JsonPointer("/array/-")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", "
      " \"object\": { \"bat\": \"bar\"}, "
      " \"array\": [1,2, 3, [1,2,3]] }",
      text.Value());
  }

  // Replace an array with an inner element
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(
          JsonPointer("/array/3"), JsonPointer("/array")));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", "
      " \"object\": { \"bat\": \"bar\"}, "
      " \"array\": [1,2, 3] }",
      text.Value());
  }

  // Point to an invalid element (one past the end)
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchCopyOp(
          JsonPointer("/array/-"), JsonPointer("/array/1")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": \"bar\", "
      " \"object\": { \"bat\": \"bar\"}, "
      " \"array\": [1,2, 3] }",
      text.Value());
  }
}

void JsonPatchTest::testTestOp() {
  unique_ptr<JsonObject> object(new JsonObject());
  object->Add("foo", "bar");
  object->Add("baz", true);
  object->Add("bat", false);

  unique_ptr<JsonValue> original_object(object->Clone());
  JsonData text(object.release());

  JsonPointer pointer1("");
  JsonPointer pointer2("/foo");
  JsonPointer pointer3("/baz");
  JsonPointer pointer4("/bat");

  JsonPatchSet patch1;
  patch1.AddOp(new JsonPatchTestOp(pointer1, new JsonNull()));
  OLA_ASSERT_FALSE(text.Apply(patch1));

  JsonPatchSet patch2;
  patch2.AddOp(new JsonPatchTestOp(pointer2, new JsonBool(true)));
  OLA_ASSERT_FALSE(text.Apply(patch2));

  JsonPatchSet patch3;
  patch3.AddOp(new JsonPatchTestOp(pointer3, new JsonBool(true)));
  OLA_ASSERT_TRUE(text.Apply(patch3));

  JsonPatchSet patch4;
  patch4.AddOp(new JsonPatchTestOp(pointer4, new JsonBool(true)));
  OLA_ASSERT_FALSE(text.Apply(patch4));

  JsonPatchSet patch5;
  patch5.AddOp(new JsonPatchTestOp(pointer3, new JsonBool(false)));
  OLA_ASSERT_FALSE(text.Apply(patch5));

  // Now try a multi-element patch
  JsonPatchSet patch6;
  patch6.AddOp(new JsonPatchTestOp(pointer3, new JsonBool(true)));
  patch6.AddOp(new JsonPatchTestOp(pointer4, new JsonBool(false)));
  OLA_ASSERT_TRUE(text.Apply(patch6));

  JsonPatchSet patch7;
  patch7.AddOp(new JsonPatchTestOp(pointer3, new JsonBool(true)));
  patch7.AddOp(new JsonPatchTestOp(pointer4, new JsonBool(true)));
  OLA_ASSERT_FALSE(text.Apply(patch7));

  JsonPatchSet patch8;
  patch8.AddOp(new JsonPatchTestOp(pointer3, new JsonNull()));
  patch8.AddOp(new JsonPatchTestOp(pointer4, new JsonBool(false)));
  OLA_ASSERT_FALSE(text.Apply(patch8));

  // Finally check an invalid pointer
  JsonPointer invalid_pointer("foo");
  JsonPatchSet patch9;
  patch9.AddOp(new JsonPatchTestOp(invalid_pointer, new JsonNull()));
  OLA_ASSERT_FALSE(text.Apply(patch9));

  // Check no changes were made
  OLA_ASSERT_TRUE(*(original_object.get()) == *(text.Value()));
}

void JsonPatchTest::testAtomicUpdates() {
  JsonData text(NULL);
  BuildSampleText(&text);

  // Test a patch which will never pass. This is from section 5 of the RFC.
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchReplaceOp(
          JsonPointer("/foo"), new JsonInt(42)));
    patch.AddOp(new JsonPatchTestOp(
          JsonPointer("/foo"), new JsonString("C")));
    OLA_ASSERT_FALSE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": false, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }

  // Now try a test-and-patch sequence
  {
    JsonPatchSet patch;
    patch.AddOp(new JsonPatchTestOp(
          JsonPointer("/foo"), new JsonString("bar")));
    patch.AddOp(new JsonPatchReplaceOp(
          JsonPointer("/baz"), new JsonBool(true)));
    OLA_ASSERT_TRUE(text.Apply(patch));

    CheckValuesMatch(
      "{\"foo\": \"bar\", \"baz\": true, "
      " \"object\": {\"bat\": 1}, \"array\": [1,2,3] }",
      text.Value());
  }
}
