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
#include "ola/testing/TestUtils.h"


using ola::CapitalizeLabel;
using ola::CustomCapitalizeLabel;
using ola::Escape;
using ola::EscapeString;
using ola::FormatData;
using ola::HexStringToInt;
using ola::IntToString;
using ola::PrefixedHexStringToInt;
using ola::ShortenString;
using ola::StringEndsWith;
using ola::StringJoin;
using ola::StringSplit;
using ola::StringToInt;
using ola::StringTrim;
using ola::ToLower;
using ola::ToUpper;
using std::string;
using std::vector;

class StringUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StringUtilsTest);
  CPPUNIT_TEST(testSplit);
  CPPUNIT_TEST(testTrim);
  CPPUNIT_TEST(testShorten);
  CPPUNIT_TEST(testEndsWith);
  CPPUNIT_TEST(testIntToString);
  CPPUNIT_TEST(testEscape);
  CPPUNIT_TEST(testStringToUInt);
  CPPUNIT_TEST(testStringToUInt16);
  CPPUNIT_TEST(testStringToUInt8);
  CPPUNIT_TEST(testStringToInt);
  CPPUNIT_TEST(testStringToInt16);
  CPPUNIT_TEST(testStringToInt8);
  CPPUNIT_TEST(testHexStringToInt);
  CPPUNIT_TEST(testPrefixedHexStringToInt);
  CPPUNIT_TEST(testToLower);
  CPPUNIT_TEST(testToUpper);
  CPPUNIT_TEST(testCapitalizeLabel);
  CPPUNIT_TEST(testCustomCapitalizeLabel);
  CPPUNIT_TEST(testFormatData);
  CPPUNIT_TEST(testStringJoin);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSplit();
    void testTrim();
    void testShorten();
    void testEndsWith();
    void testIntToString();
    void testEscape();
    void testStringToUInt();
    void testStringToUInt16();
    void testStringToUInt8();
    void testStringToInt();
    void testStringToInt16();
    void testStringToInt8();
    void testHexStringToInt();
    void testPrefixedHexStringToInt();
    void testToLower();
    void testToUpper();
    void testCapitalizeLabel();
    void testCustomCapitalizeLabel();
    void testFormatData();
    void testStringJoin();
};


CPPUNIT_TEST_SUITE_REGISTRATION(StringUtilsTest);

/*
 * Test the split function
 */
void StringUtilsTest::testSplit() {
  vector<string> tokens;
  string input = "";
  StringSplit(input, tokens);
  OLA_ASSERT_EQ((size_t) 1, tokens.size());
  OLA_ASSERT_EQ(string(""), tokens[0]);

  input = "1 2 345";
  tokens.clear();
  StringSplit(input, tokens);

  OLA_ASSERT_EQ((size_t) 3, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
  OLA_ASSERT_EQ(string("2"), tokens[1]);
  OLA_ASSERT_EQ(string("345"), tokens[2]);

  input = "1,2,345";
  tokens.clear();
  StringSplit(input, tokens, ",");

  OLA_ASSERT_EQ((size_t) 3, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
  OLA_ASSERT_EQ(string("2"), tokens[1]);
  OLA_ASSERT_EQ(string("345"), tokens[2]);

  input = ",1,2,345,,";
  tokens.clear();
  StringSplit(input, tokens, ",");

  OLA_ASSERT_EQ((size_t) 6, tokens.size());
  OLA_ASSERT_EQ(string(""), tokens[0]);
  OLA_ASSERT_EQ(string("1"), tokens[1]);
  OLA_ASSERT_EQ(string("2"), tokens[2]);
  OLA_ASSERT_EQ(string("345"), tokens[3]);
  OLA_ASSERT_EQ(string(""), tokens[4]);
  OLA_ASSERT_EQ(string(""), tokens[5]);

  input = "1 2,345";
  tokens.clear();
  StringSplit(input, tokens, " ,");

  OLA_ASSERT_EQ((size_t) 3, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
  OLA_ASSERT_EQ(string("2"), tokens[1]);
  OLA_ASSERT_EQ(string("345"), tokens[2]);

  input = "1, 2,345";
  tokens.clear();
  StringSplit(input, tokens, " ,");

  OLA_ASSERT_EQ((size_t) 4, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
  OLA_ASSERT_EQ(string(""), tokens[1]);
  OLA_ASSERT_EQ(string("2"), tokens[2]);
  OLA_ASSERT_EQ(string("345"), tokens[3]);

  input = "1";
  tokens.clear();
  StringSplit(input, tokens, ".");

  OLA_ASSERT_EQ((size_t) 1, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
}


/*
 * Test the trim function.
 */
void StringUtilsTest::testTrim() {
  string input = "foo bar baz";
  StringTrim(&input);
  OLA_ASSERT_EQ(string("foo bar baz"), input);
  input = "  \rfoo bar\t\n";
  StringTrim(&input);
  OLA_ASSERT_EQ(string("foo bar"), input);
  input = "  \r\t\n";
  StringTrim(&input);
  OLA_ASSERT_EQ(string(""), input);
}


/*
 * Test the shorten function.
 */
void StringUtilsTest::testShorten() {
  string input = "foo bar baz";
  ShortenString(&input);
  OLA_ASSERT_EQ(string("foo bar baz"), input);
  input = "foo \0bar";
  ShortenString(&input);
  OLA_ASSERT_EQ(string("foo "), input);
  input = "foo\0bar\0baz";
  StringTrim(&input);
  OLA_ASSERT_EQ(string("foo"), input);
}


/*
 * Test the StringEndsWith function.
 */
void StringUtilsTest::testEndsWith() {
  string input = "foo bar baz";
  OLA_ASSERT_TRUE(StringEndsWith(input, "baz"));
  OLA_ASSERT_TRUE(StringEndsWith(input, " baz"));
  OLA_ASSERT_TRUE(StringEndsWith(input, "bar baz"));
  OLA_ASSERT_TRUE(StringEndsWith(input, ""));
  OLA_ASSERT_FALSE(StringEndsWith(input, "foo"));
}


/*
 * test the IntToString function.
 */
void StringUtilsTest::testIntToString() {
  OLA_ASSERT_EQ(string("0"), IntToString(0));
  OLA_ASSERT_EQ(string("1234"), IntToString(1234));
  OLA_ASSERT_EQ(string("-1234"), IntToString(-1234));
  unsigned int i = 42;
  OLA_ASSERT_EQ(string("42"), IntToString(i));
}


/**
 * Test escaping.
 */
void StringUtilsTest::testEscape() {
  string s1 = "foo\"";
  Escape(&s1);
  OLA_ASSERT_EQ(string("foo\\\""), s1);

  s1 = "he said \"foo\"";
  Escape(&s1);
  OLA_ASSERT_EQ(string("he said \\\"foo\\\""), s1);

  s1 = "backslash\\test";
  Escape(&s1);
  OLA_ASSERT_EQ(string("backslash\\\\test"), s1);

  s1 = "newline\ntest";
  Escape(&s1);
  OLA_ASSERT_EQ(string("newline\\ntest"), s1);

  s1 = "tab\ttest";
  Escape(&s1);
  OLA_ASSERT_EQ(string("tab\\ttest"), s1);

  s1 = "one\"two\\three/four\bfive\fsix\nseven\reight\tnine";
  Escape(&s1);
  OLA_ASSERT_EQ(
      string("one\\\"two\\\\three\\/four\\bfive\\fsix\\nseven\\reight\\tnine"),
      s1);

  s1 = "one\"two\\three/four\bfive\fsix\nseven\reight\tnine";
  string result = EscapeString(s1);
  OLA_ASSERT_EQ(
      string("one\\\"two\\\\three\\/four\\bfive\\fsix\\nseven\\reight\\tnine"),
      result);
}


void StringUtilsTest::testStringToUInt() {
  unsigned int value;
  OLA_ASSERT_FALSE(StringToInt("", &value));
  OLA_ASSERT_FALSE(StringToInt("-1", &value));
  OLA_ASSERT_TRUE(StringToInt("0", &value));
  OLA_ASSERT_EQ(0u, value);
  OLA_ASSERT_TRUE(StringToInt("1", &value));
  OLA_ASSERT_EQ(1u, value);
  OLA_ASSERT_TRUE(StringToInt("65537", &value));
  OLA_ASSERT_EQ(65537u, value);
  OLA_ASSERT_TRUE(StringToInt("4294967295", &value));
  OLA_ASSERT_EQ(4294967295U, value);
  OLA_ASSERT_FALSE(StringToInt("4294967296", &value));
  OLA_ASSERT_FALSE(StringToInt("foo", &value));

  // same tests with strict mode on
  OLA_ASSERT_FALSE(StringToInt("-1 foo", &value, true));
  OLA_ASSERT_FALSE(StringToInt("0 ", &value, true));
  OLA_ASSERT_FALSE(StringToInt("1 bar baz", &value, true));
  OLA_ASSERT_FALSE(StringToInt("65537cat", &value, true));
  OLA_ASSERT_FALSE(StringToInt("4294967295bat bar", &value, true));
}


void StringUtilsTest::testHexStringToInt() {
  unsigned int value;
  OLA_ASSERT_FALSE(HexStringToInt("", &value));
  OLA_ASSERT_FALSE(HexStringToInt("-1", &value));

  OLA_ASSERT_TRUE(HexStringToInt("0", &value));
  OLA_ASSERT_EQ(0u, value);
  OLA_ASSERT_TRUE(HexStringToInt("1", &value));
  OLA_ASSERT_EQ(1u, value);
  OLA_ASSERT_TRUE(HexStringToInt("a", &value));
  OLA_ASSERT_EQ(10u, value);
  OLA_ASSERT_TRUE(HexStringToInt("f", &value));
  OLA_ASSERT_EQ(15u, value);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value));
  OLA_ASSERT_EQ(161u, value);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value));
  OLA_ASSERT_EQ(255u, value);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value));
  OLA_ASSERT_EQ(161u, value);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value));
  OLA_ASSERT_EQ(255u, value);
  OLA_ASSERT_TRUE(HexStringToInt("ffff", &value));
  OLA_ASSERT_EQ(65535u, value);

  OLA_ASSERT_TRUE(HexStringToInt("ffffff", &value));
  OLA_ASSERT_EQ((unsigned int) 16777215, value);
  OLA_ASSERT_TRUE(HexStringToInt("ffffffff", &value));
  OLA_ASSERT_EQ((unsigned int) 4294967295UL, value);
  OLA_ASSERT_TRUE(HexStringToInt("ef123456", &value));
  OLA_ASSERT_EQ((unsigned int) 4010947670UL, value);
  OLA_ASSERT_FALSE(HexStringToInt("fz", &value));
  OLA_ASSERT_FALSE(HexStringToInt("zfff", &value));
  OLA_ASSERT_FALSE(HexStringToInt("0xf", &value));

  // uint16_t
  uint16_t value2;
  OLA_ASSERT_FALSE(HexStringToInt("", &value2));
  OLA_ASSERT_FALSE(HexStringToInt("-1", &value2));

  OLA_ASSERT_TRUE(HexStringToInt("0", &value2));
  OLA_ASSERT_EQ((uint16_t) 0, value2);
  OLA_ASSERT_TRUE(HexStringToInt("1", &value2));
  OLA_ASSERT_EQ((uint16_t) 1, value2);
  OLA_ASSERT_TRUE(HexStringToInt("a", &value2));
  OLA_ASSERT_EQ((uint16_t) 10, value2);
  OLA_ASSERT_TRUE(HexStringToInt("f", &value2));
  OLA_ASSERT_EQ((uint16_t) 15, value2);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value2));
  OLA_ASSERT_EQ((uint16_t) 161, value2);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value2));
  OLA_ASSERT_EQ((uint16_t) 255, value2);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value2));
  OLA_ASSERT_EQ((uint16_t) 161, value2);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value2));
  OLA_ASSERT_EQ((uint16_t) 255, value2);
  OLA_ASSERT_TRUE(HexStringToInt("400", &value2));
  OLA_ASSERT_EQ((uint16_t) 1024, value2);
  OLA_ASSERT_TRUE(HexStringToInt("ffff", &value2));
  OLA_ASSERT_EQ((uint16_t) 65535, value2);

  OLA_ASSERT_FALSE(HexStringToInt("ffffff", &value2));
  OLA_ASSERT_FALSE(HexStringToInt("ffffffff", &value2));
  OLA_ASSERT_FALSE(HexStringToInt("ef123456", &value2));
  OLA_ASSERT_FALSE(HexStringToInt("fz", &value2));
  OLA_ASSERT_FALSE(HexStringToInt("zfff", &value2));
  OLA_ASSERT_FALSE(HexStringToInt("0xf", &value2));

  // int8_t
  int8_t value3;
  OLA_ASSERT_FALSE(HexStringToInt("", &value3));
  OLA_ASSERT_FALSE(HexStringToInt("-1", &value3));

  OLA_ASSERT_TRUE(HexStringToInt("0", &value3));
  OLA_ASSERT_EQ((int8_t) 0, value3);
  OLA_ASSERT_TRUE(HexStringToInt("1", &value3));
  OLA_ASSERT_EQ((int8_t) 1, value3);
  OLA_ASSERT_TRUE(HexStringToInt("a", &value3));
  OLA_ASSERT_EQ((int8_t) 10, value3);
  OLA_ASSERT_TRUE(HexStringToInt("f", &value3));
  OLA_ASSERT_EQ((int8_t) 15, value3);
  OLA_ASSERT_TRUE(HexStringToInt("7f", &value3));
  OLA_ASSERT_EQ((int8_t) 127, value3);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value3));
  OLA_ASSERT_EQ((int8_t) -95, value3);
  OLA_ASSERT_TRUE(HexStringToInt("80", &value3));
  OLA_ASSERT_EQ((int8_t) -128, value3);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value3));
  OLA_ASSERT_EQ((int8_t) -1, value3);

  OLA_ASSERT_FALSE(HexStringToInt("ffff", &value3));
  OLA_ASSERT_FALSE(HexStringToInt("fff0", &value3));
  OLA_ASSERT_FALSE(HexStringToInt("ffffff", &value3));
  OLA_ASSERT_FALSE(HexStringToInt("ffffffff", &value3));
  OLA_ASSERT_FALSE(HexStringToInt("ef123456", &value3));
  OLA_ASSERT_FALSE(HexStringToInt("fz", &value3));
  OLA_ASSERT_FALSE(HexStringToInt("zfff", &value3));
  OLA_ASSERT_FALSE(HexStringToInt("0xf", &value3));

  // int16_t
  int16_t value4;
  OLA_ASSERT_FALSE(HexStringToInt("", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("-1", &value4));

  OLA_ASSERT_TRUE(HexStringToInt("0", &value4));
  OLA_ASSERT_EQ((int16_t) 0, value4);
  OLA_ASSERT_TRUE(HexStringToInt("1", &value4));
  OLA_ASSERT_EQ((int16_t) 1, value4);
  OLA_ASSERT_TRUE(HexStringToInt("a", &value4));
  OLA_ASSERT_EQ((int16_t) 10, value4);
  OLA_ASSERT_TRUE(HexStringToInt("f", &value4));
  OLA_ASSERT_EQ((int16_t) 15, value4);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value4));
  OLA_ASSERT_EQ((int16_t) 161, value4);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value4));
  OLA_ASSERT_EQ((int16_t) 255, value4);
  OLA_ASSERT_TRUE(HexStringToInt("7fff", &value4));
  OLA_ASSERT_EQ((int16_t) 32767, value4);
  OLA_ASSERT_TRUE(HexStringToInt("ffff", &value4));
  OLA_ASSERT_EQ((int16_t) -1, value4);
  OLA_ASSERT_TRUE(HexStringToInt("fff0", &value4));
  OLA_ASSERT_EQ((int16_t) -16, value4);
  OLA_ASSERT_TRUE(HexStringToInt("8000", &value4));
  OLA_ASSERT_EQ((int16_t) -32768, value4);

  OLA_ASSERT_FALSE(HexStringToInt("ffffff", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("ffffffff", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("ef123456", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("fz", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("zfff", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("0xf", &value4));

  // int32
  int32_t value5;
  OLA_ASSERT_FALSE(HexStringToInt("", &value5));
  OLA_ASSERT_FALSE(HexStringToInt("-1", &value5));

  OLA_ASSERT_TRUE(HexStringToInt("0", &value5));
  OLA_ASSERT_EQ((int32_t) 0, value5);
  OLA_ASSERT_TRUE(HexStringToInt("1", &value5));
  OLA_ASSERT_EQ((int32_t) 1, value5);
  OLA_ASSERT_TRUE(HexStringToInt("a", &value5));
  OLA_ASSERT_EQ((int32_t) 10, value5);
  OLA_ASSERT_TRUE(HexStringToInt("f", &value5));
  OLA_ASSERT_EQ((int32_t) 15, value5);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value5));
  OLA_ASSERT_EQ((int32_t) 161, value5);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value5));
  OLA_ASSERT_EQ((int32_t) 255, value5);
  OLA_ASSERT_TRUE(HexStringToInt("7fff", &value5));
  OLA_ASSERT_EQ((int32_t) 32767, value5);
  OLA_ASSERT_TRUE(HexStringToInt("ffff", &value5));
  OLA_ASSERT_EQ((int32_t) 65535, value5);
  OLA_ASSERT_TRUE(HexStringToInt("ffffffff", &value5));
  OLA_ASSERT_EQ((int32_t) -1, value5);
  OLA_ASSERT_TRUE(HexStringToInt("fffffff0", &value5));
  OLA_ASSERT_EQ((int32_t) -16, value5);
  OLA_ASSERT_TRUE(HexStringToInt("80000000", &value5));
  OLA_ASSERT_EQ((int32_t) -2147483647 - 1, value5);
}


void StringUtilsTest::testPrefixedHexStringToInt() {
  int value;
  OLA_ASSERT_FALSE(PrefixedHexStringToInt("", &value));
  OLA_ASSERT_FALSE(PrefixedHexStringToInt("-1", &value));
  OLA_ASSERT_FALSE(PrefixedHexStringToInt("0", &value));
  OLA_ASSERT_FALSE(PrefixedHexStringToInt("2000", &value));
  OLA_ASSERT_FALSE(PrefixedHexStringToInt("0x", &value));

  OLA_ASSERT_TRUE(PrefixedHexStringToInt("0x1", &value));
  OLA_ASSERT_EQ(1, value);
  OLA_ASSERT_TRUE(PrefixedHexStringToInt("0xff", &value));
  OLA_ASSERT_EQ(255, value);
  OLA_ASSERT_TRUE(PrefixedHexStringToInt("0x70ff", &value));
  OLA_ASSERT_EQ(28927, value);
  OLA_ASSERT_TRUE(PrefixedHexStringToInt("0xffffffff", &value));
  OLA_ASSERT_EQ(-1, value);
}


void StringUtilsTest::testStringToUInt16() {
  uint16_t value;

  OLA_ASSERT_FALSE(StringToInt("", &value));
  OLA_ASSERT_FALSE(StringToInt("-1", &value));
  OLA_ASSERT_FALSE(StringToInt("65536", &value));

  OLA_ASSERT_TRUE(StringToInt("0", &value));
  OLA_ASSERT_EQ((uint16_t) 0, value);
  OLA_ASSERT_TRUE(StringToInt("1", &value));
  OLA_ASSERT_EQ((uint16_t) 1, value);
  OLA_ASSERT_TRUE(StringToInt("143", &value));
  OLA_ASSERT_EQ((uint16_t) 143, value);
  OLA_ASSERT_TRUE(StringToInt("65535", &value));
  OLA_ASSERT_EQ((uint16_t) 65535, value);
}


void StringUtilsTest::testStringToUInt8() {
  uint8_t value;

  OLA_ASSERT_FALSE(StringToInt("", &value));
  OLA_ASSERT_FALSE(StringToInt("-1", &value));
  OLA_ASSERT_FALSE(StringToInt("256", &value));

  OLA_ASSERT_TRUE(StringToInt("0", &value));
  OLA_ASSERT_EQ((uint8_t) 0, value);
  OLA_ASSERT_TRUE(StringToInt("1", &value));
  OLA_ASSERT_EQ((uint8_t) 1, value);
  OLA_ASSERT_TRUE(StringToInt("143", &value));
  OLA_ASSERT_EQ((uint8_t) 143, value);
  OLA_ASSERT_TRUE(StringToInt("255", &value));
  OLA_ASSERT_EQ((uint8_t) 255, value);
}


void StringUtilsTest::testStringToInt() {
  int value;
  OLA_ASSERT_FALSE(StringToInt("", &value));
  OLA_ASSERT_FALSE(StringToInt("a", &value));

  OLA_ASSERT_FALSE(StringToInt("2147483649", &value));
  OLA_ASSERT_TRUE(StringToInt("-2147483648", &value));
  OLA_ASSERT_EQ(static_cast<int>(-2147483647 - 1), value);
  OLA_ASSERT_TRUE(StringToInt("-2147483647", &value));
  OLA_ASSERT_EQ(-2147483647, value);
  OLA_ASSERT_TRUE(StringToInt("-1", &value));
  OLA_ASSERT_EQ(-1, value);

  OLA_ASSERT_TRUE(StringToInt("0", &value));
  OLA_ASSERT_EQ(0, value);
  OLA_ASSERT_TRUE(StringToInt("1", &value));
  OLA_ASSERT_EQ(1, value);
  OLA_ASSERT_TRUE(StringToInt("143", &value));
  OLA_ASSERT_EQ(143, value);
  OLA_ASSERT_TRUE(StringToInt("2147483647", &value));
  OLA_ASSERT_EQ(2147483647, value);
  OLA_ASSERT_FALSE(StringToInt("2147483648", &value));

  // strict mode on
  OLA_ASSERT_FALSE(StringToInt("2147483649 ", &value, true));
  OLA_ASSERT_FALSE(StringToInt("-2147483648bar", &value, true));
  OLA_ASSERT_FALSE(StringToInt("-2147483647 baz", &value, true));
  OLA_ASSERT_FALSE(StringToInt("-1.", &value, true));
  OLA_ASSERT_FALSE(StringToInt("0!", &value, true));
  OLA_ASSERT_FALSE(StringToInt("1 this is a test", &value, true));
  OLA_ASSERT_FALSE(StringToInt("143car", &value, true));
  OLA_ASSERT_FALSE(StringToInt("2147483647 !@#", &value, true));
  OLA_ASSERT_FALSE(StringToInt("2147483648mm", &value, true));
}


void StringUtilsTest::testStringToInt16() {
  int16_t value;

  OLA_ASSERT_FALSE(StringToInt("", &value));
  OLA_ASSERT_FALSE(StringToInt("a", &value));

  OLA_ASSERT_FALSE(StringToInt("-32769", &value));
  OLA_ASSERT_TRUE(StringToInt("-32768", &value));
  OLA_ASSERT_EQ((int16_t) -32768, value);
  OLA_ASSERT_TRUE(StringToInt("-32767", &value));
  OLA_ASSERT_EQ((int16_t) -32767, value);
  OLA_ASSERT_TRUE(StringToInt("-1", &value));
  OLA_ASSERT_EQ((int16_t) -1, value);

  OLA_ASSERT_TRUE(StringToInt("0", &value));
  OLA_ASSERT_EQ((int16_t) 0, value);
  OLA_ASSERT_TRUE(StringToInt("1", &value));
  OLA_ASSERT_EQ((int16_t) 1, value);
  OLA_ASSERT_TRUE(StringToInt("143", &value));
  OLA_ASSERT_EQ((int16_t) 143, value);
  OLA_ASSERT_TRUE(StringToInt("32767", &value));
  OLA_ASSERT_EQ((int16_t) 32767, value);
  OLA_ASSERT_FALSE(StringToInt("32768", &value));
}


void StringUtilsTest::testStringToInt8() {
  int8_t value;

  OLA_ASSERT_FALSE(StringToInt("", &value));
  OLA_ASSERT_FALSE(StringToInt("a", &value));

  OLA_ASSERT_FALSE(StringToInt("-129", &value));
  OLA_ASSERT_TRUE(StringToInt("-128", &value));
  OLA_ASSERT_EQ((int8_t) -128, value);
  OLA_ASSERT_TRUE(StringToInt("-127", &value));
  OLA_ASSERT_EQ((int8_t) -127, value);
  OLA_ASSERT_TRUE(StringToInt("-127", &value));
  OLA_ASSERT_EQ((int8_t) -127, value);
  OLA_ASSERT_TRUE(StringToInt("-1", &value));
  OLA_ASSERT_EQ((int8_t) -1, value);
  OLA_ASSERT_TRUE(StringToInt("0", &value));
  OLA_ASSERT_EQ((int8_t) 0, value);
  OLA_ASSERT_TRUE(StringToInt("1", &value));
  OLA_ASSERT_EQ((int8_t) 1, value);
  OLA_ASSERT_TRUE(StringToInt("127", &value));
  OLA_ASSERT_EQ((int8_t) 127, value);
  OLA_ASSERT_FALSE(StringToInt("128", &value));
  OLA_ASSERT_FALSE(StringToInt("129", &value));
}


void StringUtilsTest::testToLower() {
  string s = "HelLo There";
  ToLower(&s);
  OLA_ASSERT_EQ(string("hello there"), s);
}


void StringUtilsTest::testToUpper() {
  string s = "HelLo There";
  ToUpper(&s);
  OLA_ASSERT_EQ(string("HELLO THERE"), s);
}


void StringUtilsTest::testCapitalizeLabel() {
  string label = "this-is_a_test";
  CapitalizeLabel(&label);
  OLA_ASSERT_EQ(string("This Is A Test"), label);
};


void StringUtilsTest::testCustomCapitalizeLabel() {
  string label1 = "dmx_start_address";
  CustomCapitalizeLabel(&label1);
  OLA_ASSERT_EQ(string("DMX Start Address"), label1);

  string label2 = "foo-dmx";
  CustomCapitalizeLabel(&label2);
  OLA_ASSERT_EQ(string("Foo DMX"), label2);

  string label3 = "mini_dmxter_device";
  CustomCapitalizeLabel(&label3);
  OLA_ASSERT_EQ(string("Mini Dmxter Device"), label3);

  string label4 = "this-is_a_test";
  CustomCapitalizeLabel(&label4);
  OLA_ASSERT_EQ(string("This Is A Test"), label4);

  string label5 = "ip_address";
  CustomCapitalizeLabel(&label5);
  OLA_ASSERT_EQ(string("IP Address"), label5);

  string label6 = "controller_ip_address";
  CustomCapitalizeLabel(&label6);
  OLA_ASSERT_EQ(string("Controller IP Address"), label6);
};


void StringUtilsTest::testFormatData() {
  uint8_t data[] = {0, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd',
                    1, 2};
  std::stringstream str;
  FormatData(&str, data, sizeof(data));
  OLA_ASSERT_EQ(
      string("00 48 65 6c 6c 6f 20 57  .Hello W\n"
             "6f 72 6c 64 01 02        orld..\n"),
      str.str());

  // try 4 bytes per line with a 2 space indent
  str.str("");
  FormatData(&str, data, sizeof(data), 2, 4);
  OLA_ASSERT_EQ(
      string("  00 48 65 6c  .Hel\n"
             "  6c 6f 20 57  lo W\n"
             "  6f 72 6c 64  orld\n"
             "  01 02        ..\n"),
      str.str());

  str.str("");
  // try ending on the block boundary
  uint8_t data1[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o'};
  FormatData(&str, data1, sizeof(data1), 0, 4);
  OLA_ASSERT_EQ(
      string("48 65 6c 6c  Hell\n"
             "6f 20 57 6f  o Wo\n"),
      str.str());
}

void StringUtilsTest::testStringJoin() {
  vector<int> ints;
  ints.push_back(1);
  ints.push_back(2);
  ints.push_back(3);
  OLA_ASSERT_EQ(string("1,2,3"), StringJoin(",", ints));

  vector<string> strings;
  strings.push_back("one");
  strings.push_back("two");
  strings.push_back("three");
  OLA_ASSERT_EQ(string("one,two,three"), StringJoin(",", strings));
}
