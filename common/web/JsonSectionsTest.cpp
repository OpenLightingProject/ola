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
 * JsonSectionsTest.cpp
 * Unittest for JsonSections classses.
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include "ola/web/JsonSections.h"

using std::string;
using std::vector;
using ola::web::BoolItem;
using ola::web::HiddenItem;
using ola::web::JsonSection;
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
};


CPPUNIT_TEST_SUITE_REGISTRATION(JsonSectionsTest);

/*
 * Test the string item
 */
void JsonSectionsTest::testStringItem() {
  StringItem item("Foo", "bar");
  string expected =
    "    {\n"
    "    \"description\": \"Foo\",\n"
    "    \"type\": \"string\",\n"
    "    \"value\": \"bar\",\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected, item.AsString());

  StringItem item2("Foo", "bar", "baz");
  item2.SetButtonText("Action");
  string expected2 =
    "    {\n"
    "    \"button\": \"Action\",\n"
    "    \"description\": \"Foo\",\n"
    "    \"id\": \"baz\",\n"
    "    \"type\": \"string\",\n"
    "    \"value\": \"bar\",\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected2, item2.AsString());

  StringItem item3("Foo\" bar", "baz\\");
  item3.SetButtonText("Action\n");
  string expected3 =
    "    {\n"
    "    \"button\": \"Action\\n\",\n"
    "    \"description\": \"Foo\\\" bar\",\n"
    "    \"type\": \"string\",\n"
    "    \"value\": \"baz\\\\\",\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected3, item3.AsString());
}


/*
 * Test the uint item
 */
void JsonSectionsTest::testUIntItem() {
  UIntItem item("Foo", 10);
  string expected =
    "    {\n"
    "    \"description\": \"Foo\",\n"
    "    \"type\": \"uint\",\n"
    "    \"value\": 10,\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected, item.AsString());

  UIntItem item2("Foo", 20, "baz");
  item2.SetButtonText("Action");
  item2.SetMin(10);
  string expected2 =
    "    {\n"
    "    \"button\": \"Action\",\n"
    "    \"description\": \"Foo\",\n"
    "    \"id\": \"baz\",\n"
    "    \"type\": \"uint\",\n"
    "    \"value\": 20,\n"
    "    \"min\": 10,\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected2, item2.AsString());

  UIntItem item3("Foo", 20);
  item3.SetMax(30);
  string expected3 =
    "    {\n"
    "    \"description\": \"Foo\",\n"
    "    \"type\": \"uint\",\n"
    "    \"value\": 20,\n"
    "    \"max\": 30,\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected3, item3.AsString());

  UIntItem item4("Foo", 20);
  item4.SetMin(10);
  item4.SetMax(30);
  string expected4 =
    "    {\n"
    "    \"description\": \"Foo\",\n"
    "    \"type\": \"uint\",\n"
    "    \"value\": 20,\n"
    "    \"min\": 10,\n"
    "    \"max\": 30,\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected4, item4.AsString());
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
    "    {\n"
    "    \"description\": \"Language\",\n"
    "    \"id\": \"lang\",\n"
    "    \"type\": \"select\",\n"
    "    \"value\": [\n"
    "      {\n"
    "        \"label\": \"English\",\n"
    "        \"value\": \"EN\",\n"
    "      },\n"
    "      {\n"
    "        \"label\": \"German\",\n"
    "        \"value\": \"2\",\n"
    "      }\n"
    "    ],\n"
    "    \"selected_offset\": 1,\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected, item.AsString());
}


/*
 * Test the bool item
 */
void JsonSectionsTest::testBoolItem() {
  BoolItem item("Foo", true, "baz");
  string expected =
    "    {\n"
    "    \"description\": \"Foo\",\n"
    "    \"id\": \"baz\",\n"
    "    \"type\": \"bool\",\n"
    "    \"value\": 1,\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected, item.AsString());

  BoolItem item2("Foo", false, "baz");
  string expected2 =
    "    {\n"
    "    \"description\": \"Foo\",\n"
    "    \"id\": \"baz\",\n"
    "    \"type\": \"bool\",\n"
    "    \"value\": 0,\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected2, item2.AsString());
}

/*
 * Test the hidden item
 */
void JsonSectionsTest::testHiddenItem() {
  HiddenItem item("bar", "baz");
  item.SetButtonText("Action");
  string expected =
    "    {\n"
    "    \"button\": \"Action\",\n"
    "    \"description\": \"\",\n"
    "    \"id\": \"baz\",\n"
    "    \"type\": \"hidden\",\n"
    "    \"value\": \"bar\",\n"
    "    }";
  CPPUNIT_ASSERT_EQUAL(expected, item.AsString());
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
    "  \"refresh\": 0,\n"
    "  \"error\": \"\",\n"
    "  \"save_button\": \"Action\\\\\",\n"
    "  \"items\": [\n"
    "    {\n"
    "    \"description\": \"\",\n"
    "    \"id\": \"baz\",\n"
    "    \"type\": \"hidden\",\n"
    "    \"value\": \"bar\\r\",\n"
    "    }\n"
    "  ],\n"
    "}\n";
  CPPUNIT_ASSERT_EQUAL(expected, section.AsString());
}
