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
 * StringUtilsTest.cpp
 * Unittest for String functions.
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <iostream>
#include <string>
#include <vector>
#include "ola/StringUtils.h"

using std::string;
using std::vector;
using ola::Escape;
using ola::HexStringToUInt;
using ola::IntToString;
using ola::StringSplit;
using ola::StringToUInt;
using ola::StringToUInt16;
using ola::StringTrim;

class StringUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StringUtilsTest);
  CPPUNIT_TEST(testSplit);
  CPPUNIT_TEST(testTrim);
  CPPUNIT_TEST(testIntToString);
  CPPUNIT_TEST(testEscape);
  CPPUNIT_TEST(testStringToUInt);
  CPPUNIT_TEST(testStringToUInt16);
  CPPUNIT_TEST(testHexStringToUInt);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSplit();
    void testTrim();
    void testIntToString();
    void testEscape();
    void testStringToUInt();
    void testStringToUInt16();
    void testHexStringToUInt();
};


CPPUNIT_TEST_SUITE_REGISTRATION(StringUtilsTest);

/*
 * Test the split function
 */
void StringUtilsTest::testSplit() {
  vector<string> tokens;
  string input = "";
  StringSplit(input, tokens);
  CPPUNIT_ASSERT_EQUAL((size_t) 1, tokens.size());
  CPPUNIT_ASSERT_EQUAL(string(""), tokens[0]);

  input = "1 2 345";
  tokens.clear();
  StringSplit(input, tokens);

  CPPUNIT_ASSERT_EQUAL((size_t) 3, tokens.size());
  CPPUNIT_ASSERT_EQUAL(string("1"), tokens[0]);
  CPPUNIT_ASSERT_EQUAL(string("2"), tokens[1]);
  CPPUNIT_ASSERT_EQUAL(string("345"), tokens[2]);

  input = "1,2,345";
  tokens.clear();
  StringSplit(input, tokens, ",");

  CPPUNIT_ASSERT_EQUAL((size_t) 3, tokens.size());
  CPPUNIT_ASSERT_EQUAL(string("1"), tokens[0]);
  CPPUNIT_ASSERT_EQUAL(string("2"), tokens[1]);
  CPPUNIT_ASSERT_EQUAL(string("345"), tokens[2]);

  input = ",1,2,345,,";
  tokens.clear();
  StringSplit(input, tokens, ",");

  CPPUNIT_ASSERT_EQUAL((size_t) 6, tokens.size());
  CPPUNIT_ASSERT_EQUAL(string(""), tokens[0]);
  CPPUNIT_ASSERT_EQUAL(string("1"), tokens[1]);
  CPPUNIT_ASSERT_EQUAL(string("2"), tokens[2]);
  CPPUNIT_ASSERT_EQUAL(string("345"), tokens[3]);
  CPPUNIT_ASSERT_EQUAL(string(""), tokens[4]);
  CPPUNIT_ASSERT_EQUAL(string(""), tokens[5]);

  input = "1 2,345";
  tokens.clear();
  StringSplit(input, tokens, " ,");

  CPPUNIT_ASSERT_EQUAL((size_t) 3, tokens.size());
  CPPUNIT_ASSERT_EQUAL(string("1"), tokens[0]);
  CPPUNIT_ASSERT_EQUAL(string("2"), tokens[1]);
  CPPUNIT_ASSERT_EQUAL(string("345"), tokens[2]);

  input = "1, 2,345";
  tokens.clear();
  StringSplit(input, tokens, " ,");

  CPPUNIT_ASSERT_EQUAL((size_t) 4, tokens.size());
  CPPUNIT_ASSERT_EQUAL(string("1"), tokens[0]);
  CPPUNIT_ASSERT_EQUAL(string(""), tokens[1]);
  CPPUNIT_ASSERT_EQUAL(string("2"), tokens[2]);
  CPPUNIT_ASSERT_EQUAL(string("345"), tokens[3]);

  input = "1";
  tokens.clear();
  StringSplit(input, tokens, ".");

  CPPUNIT_ASSERT_EQUAL((size_t) 1, tokens.size());
  CPPUNIT_ASSERT_EQUAL(string("1"), tokens[0]);
}


/*
 * Test the trim function.
 */
void StringUtilsTest::testTrim() {
  string input = "foo bar baz";
  StringTrim(&input);
  CPPUNIT_ASSERT_EQUAL(input, input);
  input = "  \rfoo bar\t\n";
  StringTrim(&input);
  CPPUNIT_ASSERT_EQUAL(string("foo bar"), input);
  input = "  \r\t\n";
  StringTrim(&input);
  CPPUNIT_ASSERT_EQUAL(string(""), input);
}

/*
 * test the IntToString function.
 */
void StringUtilsTest::testIntToString() {
  CPPUNIT_ASSERT_EQUAL(string("0"), IntToString(0));
  CPPUNIT_ASSERT_EQUAL(string("1234"), IntToString(1234));
  CPPUNIT_ASSERT_EQUAL(string("-1234"), IntToString(-1234));
  unsigned int i = 42;
  CPPUNIT_ASSERT_EQUAL(string("42"), IntToString(i));
}

void StringUtilsTest::testEscape() {
  string s1 = "foo\"";
  Escape(&s1);
  CPPUNIT_ASSERT_EQUAL(string("foo\\\""), s1);

  string s2 = "he said \"foo\"";
  Escape(&s2);
  CPPUNIT_ASSERT_EQUAL(string("he said \\\"foo\\\""), s2);
}


void StringUtilsTest::testStringToUInt() {
  unsigned int value;
  CPPUNIT_ASSERT(!StringToUInt("", &value));
  CPPUNIT_ASSERT(!StringToUInt("-1", &value));
  CPPUNIT_ASSERT(StringToUInt("0", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, value);
  CPPUNIT_ASSERT(StringToUInt("1", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, value);
  CPPUNIT_ASSERT(StringToUInt("65537", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 65537, value);
  CPPUNIT_ASSERT(!StringToUInt("foo", &value));
}


void StringUtilsTest::testHexStringToUInt() {
  unsigned int value;
  CPPUNIT_ASSERT(!HexStringToUInt("", &value));
  CPPUNIT_ASSERT(!HexStringToUInt("-1", &value));

  CPPUNIT_ASSERT(HexStringToUInt("0", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, value);
  CPPUNIT_ASSERT(HexStringToUInt("1", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, value);
  CPPUNIT_ASSERT(HexStringToUInt("a", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 10, value);
  CPPUNIT_ASSERT(HexStringToUInt("f", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 15, value);
  CPPUNIT_ASSERT(HexStringToUInt("a1", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 161, value);
  CPPUNIT_ASSERT(HexStringToUInt("ff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 255, value);
  CPPUNIT_ASSERT(HexStringToUInt("a1", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 161, value);
  CPPUNIT_ASSERT(HexStringToUInt("ff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 255, value);
  CPPUNIT_ASSERT(HexStringToUInt("ffff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 65535, value);

  CPPUNIT_ASSERT(HexStringToUInt("ffffff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 16777215, value);
  CPPUNIT_ASSERT(HexStringToUInt("ffffffff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4294967295UL, value);
  CPPUNIT_ASSERT(HexStringToUInt("ef123456", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4010947670UL, value);
  CPPUNIT_ASSERT(!HexStringToUInt("fz", &value));
  CPPUNIT_ASSERT(!HexStringToUInt("zfff", &value));
  CPPUNIT_ASSERT(!HexStringToUInt("0xf", &value));
}


void StringUtilsTest::testStringToUInt16() {
  uint16_t value;

  CPPUNIT_ASSERT(!StringToUInt16("", &value));
  CPPUNIT_ASSERT(!StringToUInt16("-1", &value));
  CPPUNIT_ASSERT(!StringToUInt16("65536", &value));

  CPPUNIT_ASSERT(StringToUInt16("0", &value));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 0, value);
  CPPUNIT_ASSERT(StringToUInt16("1", &value));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 1, value);
  CPPUNIT_ASSERT(StringToUInt16("143", &value));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 143, value);
  CPPUNIT_ASSERT(StringToUInt16("65535", &value));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 65535, value);
}
