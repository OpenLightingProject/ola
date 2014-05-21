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
 * Unittests for the Json Schema Parser.
 * This uses testcases from the testdata directory.
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/file/Util.h"
#include "ola/testing/TestUtils.h"
#include "ola/web/JsonSchema.h"
#include "ola/web/JsonWriter.h"

using ola::web::JsonObject;
using ola::web::JsonSchema;
using std::auto_ptr;
using std::ostringstream;
using std::string;
using std::vector;

class JsonSchemaParserTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonSchemaParserTest);
  CPPUNIT_TEST(testPrimitiveTypes);
  CPPUNIT_TEST(testEmptySchema);
  CPPUNIT_TEST(testBasicKeywords);
  CPPUNIT_TEST(testInvalidSchema);
  CPPUNIT_TEST(testInvalidTypes);
  CPPUNIT_TEST(testIntegers);
  CPPUNIT_TEST(testStrings);
  CPPUNIT_TEST(testArrays);
  CPPUNIT_TEST(testObjects);
  CPPUNIT_TEST(testMisc);
  CPPUNIT_TEST(testDefinitions);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testPrimitiveTypes();
  void testEmptySchema();
  void testBasicKeywords();
  void testInvalidSchema();
  void testInvalidTypes();
  void testIntegers();
  void testStrings();
  void testArrays();
  void testObjects();
  void testMisc();
  void testDefinitions();

 private:
  struct TestCase {
    string input;
    string expected;
  };

  typedef vector<TestCase> PositiveTests;
  typedef vector<string> NegativeTests;

  string ReadTestFile(const string &filename);
  void ReadTestCases(const string& filename, PositiveTests *positive_tests,
                     NegativeTests *negative_tests);

  void FinalizePositiveCase(TestCase *test, PositiveTests *positive_tests);
  void FinalizeNegativeCase(string *test, NegativeTests *negative_tests);

  void ParseSchemaAndConvertToJson(const string &input,
                                   const string &expected);
  void VerifyFailure(const string &input);

  void RunTestsInFile(const string &test_file);
};

CPPUNIT_TEST_SUITE_REGISTRATION(JsonSchemaParserTest);

/**
 * Read the contents of a file in testdata
 */
string JsonSchemaParserTest::ReadTestFile(const string &filename) {
  string file_path;
  file_path.append(TEST_SRC_DIR);
  file_path.push_back(ola::file::PATH_SEPARATOR);
  file_path.append("testdata");
  file_path.push_back(ola::file::PATH_SEPARATOR);
  file_path.append(filename);

  std::ifstream in(file_path.data(), std::ios::in);
  OLA_ASSERT_TRUE(in.is_open());

  string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();
  return contents;
}


void JsonSchemaParserTest::FinalizePositiveCase(
    TestCase *test,
    PositiveTests *positive_tests) {
  if (test->input.empty()) {
    return;
  }

  // If no expected string was supplied, it defaults to the input string.
  if (test->expected.empty()) {
    test->expected = test->input;
  }
  positive_tests->push_back(*test);
  test->input.clear();
  test->expected.clear();
}

void JsonSchemaParserTest::FinalizeNegativeCase(
    string *test,
    NegativeTests *negative_tests) {
  if (!test->empty()) {
    negative_tests->push_back(*test);
    test->clear();
  }
}

/**
 *
 * TestCases are separated by ======== (8 x =)
 * The input and expected result is separated by -------- (8 x -)
 * If the input and the expected is the same the -------- can be omitted.
 */
void JsonSchemaParserTest::ReadTestCases(const string& filename,
                                         PositiveTests *positive_tests,
                                         NegativeTests *negative_tests) {
  string file_path;
  file_path.append(TEST_SRC_DIR);
  file_path.push_back(ola::file::PATH_SEPARATOR);
  file_path.append("testdata");
  file_path.push_back(ola::file::PATH_SEPARATOR);
  file_path.append(filename);

  std::ifstream in(file_path.data(), std::ios::in);
  OLA_ASSERT_TRUE_MSG(in.is_open(), file_path);

  enum Mode {
    NEGATIVE_INPUT,
    POSITIVE_INPUT,
    POSITIVE_EXPECTED,
  };

  TestCase test_case;
  string negative_test;

  Mode mode = POSITIVE_INPUT;
  const string comment_prefix = "//";
  string line;
  while (getline(in, line)) {
    if (line.compare(0, comment_prefix.size(), comment_prefix) == 0) {
      continue;
    } else if (line == "--------") {
      mode = POSITIVE_EXPECTED;
    } else if (line == "=== POSITIVE ===") {
      FinalizePositiveCase(&test_case, positive_tests);
      FinalizeNegativeCase(&negative_test, negative_tests);
      mode = POSITIVE_INPUT;
    } else if (line == "=== NEGATIVE ===") {
      FinalizePositiveCase(&test_case, positive_tests);
      FinalizeNegativeCase(&negative_test, negative_tests);
      mode = NEGATIVE_INPUT;
    } else if (mode == POSITIVE_INPUT) {
      test_case.input.append(line);
      test_case.input.push_back('\n');
    } else if (mode == POSITIVE_EXPECTED) {
      test_case.expected.append(line);
      test_case.expected.push_back('\n');
    } else if (mode == NEGATIVE_INPUT) {
      negative_test.append(line);
      negative_test.push_back('\n');
    }
  }

  FinalizePositiveCase(&test_case, positive_tests);
  FinalizeNegativeCase(&negative_test, negative_tests);
  in.close();
}

/**
 * Parse a JSON schema, confirm there are no errors, serialize back to JSON and
 * compare with the expected schema.
 */
void JsonSchemaParserTest::ParseSchemaAndConvertToJson(const string &input,
                                                       const string &expected) {
  string error;
  auto_ptr<JsonSchema> schema(JsonSchema::FromString(input, &error));
  OLA_ASSERT_EQ(string(""), error);
  OLA_ASSERT_NOT_NULL(schema.get());

  auto_ptr<const JsonObject> schema_json(schema->AsJson());
  string actual = ola::web::JsonWriter::AsString(*schema_json);
  actual.push_back('\n');
  OLA_ASSERT_EQ(expected, actual);
}

/**
 * Verify that the given schema is invalid
 */
void JsonSchemaParserTest::VerifyFailure(const string &input) {
  string error;
  auto_ptr<JsonSchema> schema(JsonSchema::FromString(input, &error));
  bool failed = !error.empty();
  if (!failed) {
    ostringstream str;
    str << "Expected schema to fail parsing:\n" << input;
    OLA_FAIL(str.str());
  }
}

void JsonSchemaParserTest::RunTestsInFile(const string &test_file) {
  PositiveTests positive_tests;
  NegativeTests negative_tests;
  ReadTestCases(test_file, &positive_tests, &negative_tests);
  OLA_INFO << "Read " << positive_tests.size() << " positive tests, "
           << negative_tests.size() << " negative tests from " << test_file;

  PositiveTests::const_iterator iter = positive_tests.begin();
  for (; iter != positive_tests.end(); ++iter) {
    /*
    OLA_INFO << "Input: " << iter->input;
    OLA_INFO << "Expected: " << iter->expected;
    */
    ParseSchemaAndConvertToJson(iter->input, iter->expected);
  }

  NegativeTests::const_iterator neg_iter = negative_tests.begin();
  for (; neg_iter != negative_tests.end(); ++neg_iter) {
    VerifyFailure(*neg_iter);
  }
}

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

/**
 * Verify basic keywords like 'id', '$schema', 'title' & 'description' work
 * correctly.
 */
void JsonSchemaParserTest::testBasicKeywords() {
  RunTestsInFile("basic-keywords.test");
}

void JsonSchemaParserTest::testInvalidSchema() {
  string error;
  string input = "{\"type\": \"array\", \"items\": null}";
  auto_ptr<JsonSchema> schema(JsonSchema::FromString(input, &error));
  OLA_ASSERT_NOT_EMPTY(error);
}

void JsonSchemaParserTest::testInvalidTypes() {
  string error;
  string input = "{\"type\": \"foo\"}";
  auto_ptr<JsonSchema> schema(JsonSchema::FromString(input, &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_NOT_EMPTY(error);

  input = "{\"type\": null}";
  schema.reset(JsonSchema::FromString(input, &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_NOT_EMPTY(error);

  input = "{\"type\": true}";
  schema.reset(JsonSchema::FromString(input, &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_NOT_EMPTY(error);

  input = "{\"type\": 1}";
  schema.reset(JsonSchema::FromString(input, &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_NOT_EMPTY(error);

  input = "{\"type\": {}}";
  schema.reset(JsonSchema::FromString(input, &error));
  OLA_ASSERT_NULL(schema.get());
  OLA_ASSERT_NOT_EMPTY(error);
}

/**
 * Verify integers parse correctly.
 */
void JsonSchemaParserTest::testIntegers() {
  RunTestsInFile("integers.test");
}

/**
 * Verify strings parse correctly.
 */
void JsonSchemaParserTest::testStrings() {
  RunTestsInFile("strings.test");
}

/**
 * Verify arrays parse correctly.
 */
void JsonSchemaParserTest::testArrays() {
  // Test the various combinations of 'items' & 'additionalItems'
  // items can be either a schema (object) or an array
  // additionalItems can be either a bool or a schema.
  RunTestsInFile("arrays.test");
}

/**
 * Verify objects parse correctly.
 */
void JsonSchemaParserTest::testObjects() {
  RunTestsInFile("objects.test");
}

/**
 * Various other test cases.
 */
void JsonSchemaParserTest::testMisc() {
  RunTestsInFile("misc.test");
}

void JsonSchemaParserTest::testDefinitions() {
  string error;
  const string input = ReadTestFile("schema.json");
  auto_ptr<JsonSchema> schema(JsonSchema::FromString(input, &error));

  if (!error.empty()) {
    OLA_INFO << "Schema parse error: " << error;
  }
  OLA_ASSERT_NOT_NULL(schema.get());
  OLA_ASSERT_TRUE(error.empty());

  auto_ptr<const JsonObject> value(schema->AsJson());
  OLA_ASSERT_NOT_NULL(value.get());
  string actual = ola::web::JsonWriter::AsString(*value);
  actual.push_back('\n');
  OLA_INFO << actual;
  // OLA_ASSERT_EQ(input, actual);
}
