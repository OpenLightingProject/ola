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
 * SectionsTest.cpp
 * Unittest for JsonSections classes.
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/web/Json.h"
#include "ola/web/JsonWriter.h"
#include "ola/web/JsonSections.h"
#include "ola/testing/TestUtils.h"


using std::string;
using std::vector;
using ola::web::BoolItem;
using ola::web::GenericItem;
using ola::web::HiddenItem;
using ola::web::JsonObject;
using ola::web::JsonSection;
using ola::web::JsonWriter;
using ola::web::SelectItem;
using ola::web::StringItem;
using ola::web::UIntItem;

class JsonSectionsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonSectionsTest);
  CPPUNIT_TEST(testStringItem);
  CPPUNIT_TEST(testUIntItem);
  CPPUNIT_TEST(testSelectItem);
  CPPUNIT_TEST(testBoolItem);
  CPPUNIT_TEST(testHiddenItem);
  CPPUNIT_TEST(testSection);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testStringItem();
    void testUIntItem();
    void testSelectItem();
    void testBoolItem();
    void testHiddenItem();
    void testSection();

 private:
    string ConvertToString(const GenericItem &item);
};


CPPUNIT_TEST_SUITE_REGISTRATION(JsonSectionsTest);


string JsonSectionsTest::ConvertToString(const GenericItem &item) {
  JsonObject object;
  item.PopulateItem(&object);
  return JsonWriter::AsString(object);
}


/*
 * Test the string item
 */
void JsonSectionsTest::testStringItem() {
  StringItem item("Foo", "bar");
  string expected =
    "{\n"
    "  \"description\": \"Foo\",\n"
    "  \"type\": \"string\",\n"
    "  \"value\": \"bar\"\n"
    "}";
  OLA_ASSERT_EQ(expected, ConvertToString(item));

  StringItem item2("Foo", "bar", "baz");
  item2.SetButtonText("Action");
  string expected2 =
    "{\n"
    "  \"button\": \"Action\",\n"
    "  \"description\": \"Foo\",\n"
    "  \"id\": \"baz\",\n"
    "  \"type\": \"string\",\n"
    "  \"value\": \"bar\"\n"
    "}";
  OLA_ASSERT_EQ(expected2, ConvertToString(item2));

  StringItem item3("Foo\" bar", "baz\\");
  item3.SetButtonText("Action\n");
  string expected3 =
    "{\n"
    "  \"button\": \"Action\\\\x0a\",\n"
    "  \"description\": \"Foo\\\" bar\",\n"
    "  \"type\": \"string\",\n"
    "  \"value\": \"baz\\\\\"\n"
    "}";
  OLA_ASSERT_EQ(expected3, ConvertToString(item3));
}


/*
 * Test the uint item
 */
void JsonSectionsTest::testUIntItem() {
  UIntItem item("Foo", 10);
  string expected =
    "{\n"
    "  \"description\": \"Foo\",\n"
    "  \"type\": \"uint\",\n"
    "  \"value\": 10\n"
    "}";
  OLA_ASSERT_EQ(expected, ConvertToString(item));

  UIntItem item2("Foo", 20, "baz");
  item2.SetButtonText("Action");
  item2.SetMin(10);
  string expected2 =
    "{\n"
    "  \"button\": \"Action\",\n"
    "  \"description\": \"Foo\",\n"
    "  \"id\": \"baz\",\n"
    "  \"min\": 10,\n"
    "  \"type\": \"uint\",\n"
    "  \"value\": 20\n"
    "}";
  OLA_ASSERT_EQ(expected2, ConvertToString(item2));

  UIntItem item3("Foo", 20);
  item3.SetMax(30);
  string expected3 =
    "{\n"
    "  \"description\": \"Foo\",\n"
    "  \"max\": 30,\n"
    "  \"type\": \"uint\",\n"
    "  \"value\": 20\n"
    "}";
  OLA_ASSERT_EQ(expected3, ConvertToString(item3));

  UIntItem item4("Foo", 20);
  item4.SetMin(10);
  item4.SetMax(30);
  string expected4 =
    "{\n"
    "  \"description\": \"Foo\",\n"
    "  \"max\": 30,\n"
    "  \"min\": 10,\n"
    "  \"type\": \"uint\",\n"
    "  \"value\": 20\n"
    "}";
  OLA_ASSERT_EQ(expected4, ConvertToString(item4));
}


/*
 * Test the select item
 */
void JsonSectionsTest::testSelectItem() {
  SelectItem item("Language", "lang");
  item.AddItem("English", "EN");
  item.AddItem("German", 2);
  item.SetSelectedOffset(1);
  string expected =
    "{\n"
    "  \"description\": \"Language\",\n"
    "  \"id\": \"lang\",\n"
    "  \"selected_offset\": 1,\n"
    "  \"type\": \"select\",\n"
    "  \"value\": [\n"
    "    {\n"
    "      \"label\": \"English\",\n"
    "      \"value\": \"EN\"\n"
    "    },\n"
    "    {\n"
    "      \"label\": \"German\",\n"
    "      \"value\": \"2\"\n"
    "    }\n"
    "  ]\n"
    "}";
  OLA_ASSERT_EQ(expected, ConvertToString(item));
}


/*
 * Test the bool item
 */
void JsonSectionsTest::testBoolItem() {
  BoolItem item("Foo", true, "baz");
  string expected =
    "{\n"
    "  \"description\": \"Foo\",\n"
    "  \"id\": \"baz\",\n"
    "  \"type\": \"bool\",\n"
    "  \"value\": true\n"
    "}";
  OLA_ASSERT_EQ(expected, ConvertToString(item));

  BoolItem item2("Foo", false, "baz");
  string expected2 =
    "{\n"
    "  \"description\": \"Foo\",\n"
    "  \"id\": \"baz\",\n"
    "  \"type\": \"bool\",\n"
    "  \"value\": false\n"
    "}";
  OLA_ASSERT_EQ(expected2, ConvertToString(item2));
}

/*
 * Test the hidden item
 */
void JsonSectionsTest::testHiddenItem() {
  HiddenItem item("bar", "baz");
  item.SetButtonText("Action");
  string expected =
    "{\n"
    "  \"button\": \"Action\",\n"
    "  \"description\": \"\",\n"
    "  \"id\": \"baz\",\n"
    "  \"type\": \"hidden\",\n"
    "  \"value\": \"bar\"\n"
    "}";
  OLA_ASSERT_EQ(expected, ConvertToString(item));
}


/*
 * Test the entire section
 */
void JsonSectionsTest::testSection() {
  JsonSection section(false);
  HiddenItem *item = new HiddenItem("bar\r", "baz");

  section.AddItem(item);
  section.SetSaveButton("Action\\");

  string expected =
    "{\n"
    "  \"error\": \"\",\n"
    "  \"items\": [\n"
    "    {\n"
    "      \"description\": \"\",\n"
    "      \"id\": \"baz\",\n"
    "      \"type\": \"hidden\",\n"
    "      \"value\": \"bar\\\\x0d\"\n"
    "    }\n"
    "  ],\n"
    "  \"refresh\": false,\n"
    "  \"save_button\": \"Action\\\\\"\n"
    "}";
  OLA_ASSERT_EQ(expected, section.AsString());
}
