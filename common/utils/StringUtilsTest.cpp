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
using ola::CapitalizeLabel;
using ola::CustomCapitalizeLabel;
using ola::Escape;
using ola::EscapeString;
using ola::HexStringToInt;
using ola::IntToString;
using ola::PrefixedHexStringToInt;
using ola::ShortenString;
using ola::StringSplit;
using ola::StringToInt;
using ola::StringTrim;
using ola::ToLower;
using ola::ToUpper;

class StringUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StringUtilsTest);
  CPPUNIT_TEST(testSplit);
  CPPUNIT_TEST(testTrim);
  CPPUNIT_TEST(testShorten);
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
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSplit();
    void testTrim();
    void testShorten();
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
  CPPUNIT_ASSERT_EQUAL(string("foo bar baz"), input);
  input = "  \rfoo bar\t\n";
  StringTrim(&input);
  CPPUNIT_ASSERT_EQUAL(string("foo bar"), input);
  input = "  \r\t\n";
  StringTrim(&input);
  CPPUNIT_ASSERT_EQUAL(string(""), input);
}


/*
 * Test the shorten function.
 */
void StringUtilsTest::testShorten() {
  string input = "foo bar baz";
  ShortenString(&input);
  CPPUNIT_ASSERT_EQUAL(string("foo bar baz"), input);
  input = "foo \0bar";
  ShortenString(&input);
  CPPUNIT_ASSERT_EQUAL(string("foo "), input);
  input = "foo\0bar\0baz";
  StringTrim(&input);
  CPPUNIT_ASSERT_EQUAL(string("foo"), input);
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


/**
 * Test escaping.
 */
void StringUtilsTest::testEscape() {
  string s1 = "foo\"";
  Escape(&s1);
  CPPUNIT_ASSERT_EQUAL(string("foo\\\""), s1);

  s1 = "he said \"foo\"";
  Escape(&s1);
  CPPUNIT_ASSERT_EQUAL(string("he said \\\"foo\\\""), s1);

  s1 = "backslash\\test";
  Escape(&s1);
  CPPUNIT_ASSERT_EQUAL(string("backslash\\\\test"), s1);

  s1 = "newline\ntest";
  Escape(&s1);
  CPPUNIT_ASSERT_EQUAL(string("newline\\ntest"), s1);

  s1 = "tab\ttest";
  Escape(&s1);
  CPPUNIT_ASSERT_EQUAL(string("tab\\ttest"), s1);

  s1 = "one\"two\\three/four\bfive\fsix\nseven\reight\tnine";
  Escape(&s1);
  CPPUNIT_ASSERT_EQUAL(
      string("one\\\"two\\\\three\\/four\\bfive\\fsix\\nseven\\reight\\tnine"),
      s1);

  s1 = "one\"two\\three/four\bfive\fsix\nseven\reight\tnine";
  string result = EscapeString(s1);
  CPPUNIT_ASSERT_EQUAL(
      string("one\\\"two\\\\three\\/four\\bfive\\fsix\\nseven\\reight\\tnine"),
      result);
}


void StringUtilsTest::testStringToUInt() {
  unsigned int value;
  CPPUNIT_ASSERT(!StringToInt("", &value));
  CPPUNIT_ASSERT(!StringToInt("-1", &value));
  CPPUNIT_ASSERT(StringToInt("0", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, value);
  CPPUNIT_ASSERT(StringToInt("1", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, value);
  CPPUNIT_ASSERT(StringToInt("65537", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 65537, value);
  CPPUNIT_ASSERT(StringToInt("4294967295", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4294967295U, value);
  CPPUNIT_ASSERT(!StringToInt("4294967296", &value));
  CPPUNIT_ASSERT(!StringToInt("foo", &value));
}


void StringUtilsTest::testHexStringToInt() {
  unsigned int value;
  CPPUNIT_ASSERT(!HexStringToInt("", &value));
  CPPUNIT_ASSERT(!HexStringToInt("-1", &value));

  CPPUNIT_ASSERT(HexStringToInt("0", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, value);
  CPPUNIT_ASSERT(HexStringToInt("1", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, value);
  CPPUNIT_ASSERT(HexStringToInt("a", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 10, value);
  CPPUNIT_ASSERT(HexStringToInt("f", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 15, value);
  CPPUNIT_ASSERT(HexStringToInt("a1", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 161, value);
  CPPUNIT_ASSERT(HexStringToInt("ff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 255, value);
  CPPUNIT_ASSERT(HexStringToInt("a1", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 161, value);
  CPPUNIT_ASSERT(HexStringToInt("ff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 255, value);
  CPPUNIT_ASSERT(HexStringToInt("ffff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 65535, value);

  CPPUNIT_ASSERT(HexStringToInt("ffffff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 16777215, value);
  CPPUNIT_ASSERT(HexStringToInt("ffffffff", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4294967295UL, value);
  CPPUNIT_ASSERT(HexStringToInt("ef123456", &value));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4010947670UL, value);
  CPPUNIT_ASSERT(!HexStringToInt("fz", &value));
  CPPUNIT_ASSERT(!HexStringToInt("zfff", &value));
  CPPUNIT_ASSERT(!HexStringToInt("0xf", &value));

  // uint16_t
  uint16_t value2;
  CPPUNIT_ASSERT(!HexStringToInt("", &value2));
  CPPUNIT_ASSERT(!HexStringToInt("-1", &value2));

  CPPUNIT_ASSERT(HexStringToInt("0", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 0, value2);
  CPPUNIT_ASSERT(HexStringToInt("1", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 1, value2);
  CPPUNIT_ASSERT(HexStringToInt("a", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, value2);
  CPPUNIT_ASSERT(HexStringToInt("f", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 15, value2);
  CPPUNIT_ASSERT(HexStringToInt("a1", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 161, value2);
  CPPUNIT_ASSERT(HexStringToInt("ff", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 255, value2);
  CPPUNIT_ASSERT(HexStringToInt("a1", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 161, value2);
  CPPUNIT_ASSERT(HexStringToInt("ff", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 255, value2);
  CPPUNIT_ASSERT(HexStringToInt("400", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 1024, value2);
  CPPUNIT_ASSERT(HexStringToInt("ffff", &value2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 65535, value2);

  CPPUNIT_ASSERT(!HexStringToInt("ffffff", &value2));
  CPPUNIT_ASSERT(!HexStringToInt("ffffffff", &value2));
  CPPUNIT_ASSERT(!HexStringToInt("ef123456", &value2));
  CPPUNIT_ASSERT(!HexStringToInt("fz", &value2));
  CPPUNIT_ASSERT(!HexStringToInt("zfff", &value2));
  CPPUNIT_ASSERT(!HexStringToInt("0xf", &value2));

  // int8_t
  int8_t value3;
  CPPUNIT_ASSERT(!HexStringToInt("", &value3));
  CPPUNIT_ASSERT(!HexStringToInt("-1", &value3));

  CPPUNIT_ASSERT(HexStringToInt("0", &value3));
  CPPUNIT_ASSERT_EQUAL((int8_t) 0, value3);
  CPPUNIT_ASSERT(HexStringToInt("1", &value3));
  CPPUNIT_ASSERT_EQUAL((int8_t) 1, value3);
  CPPUNIT_ASSERT(HexStringToInt("a", &value3));
  CPPUNIT_ASSERT_EQUAL((int8_t) 10, value3);
  CPPUNIT_ASSERT(HexStringToInt("f", &value3));
  CPPUNIT_ASSERT_EQUAL((int8_t) 15, value3);
  CPPUNIT_ASSERT(HexStringToInt("7f", &value3));
  CPPUNIT_ASSERT_EQUAL((int8_t) 127, value3);
  CPPUNIT_ASSERT(HexStringToInt("a1", &value3));
  CPPUNIT_ASSERT_EQUAL((int8_t) -95, value3);
  CPPUNIT_ASSERT(HexStringToInt("80", &value3));
  CPPUNIT_ASSERT_EQUAL((int8_t) -128, value3);
  CPPUNIT_ASSERT(HexStringToInt("ff", &value3));
  CPPUNIT_ASSERT_EQUAL((int8_t) -1, value3);

  CPPUNIT_ASSERT(!HexStringToInt("ffff", &value3));
  CPPUNIT_ASSERT(!HexStringToInt("fff0", &value3));
  CPPUNIT_ASSERT(!HexStringToInt("ffffff", &value3));
  CPPUNIT_ASSERT(!HexStringToInt("ffffffff", &value3));
  CPPUNIT_ASSERT(!HexStringToInt("ef123456", &value3));
  CPPUNIT_ASSERT(!HexStringToInt("fz", &value3));
  CPPUNIT_ASSERT(!HexStringToInt("zfff", &value3));
  CPPUNIT_ASSERT(!HexStringToInt("0xf", &value3));

  // int16_t
  int16_t value4;
  CPPUNIT_ASSERT(!HexStringToInt("", &value4));
  CPPUNIT_ASSERT(!HexStringToInt("-1", &value4));

  CPPUNIT_ASSERT(HexStringToInt("0", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) 0, value4);
  CPPUNIT_ASSERT(HexStringToInt("1", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) 1, value4);
  CPPUNIT_ASSERT(HexStringToInt("a", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) 10, value4);
  CPPUNIT_ASSERT(HexStringToInt("f", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) 15, value4);
  CPPUNIT_ASSERT(HexStringToInt("a1", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) 161, value4);
  CPPUNIT_ASSERT(HexStringToInt("ff", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) 255, value4);
  CPPUNIT_ASSERT(HexStringToInt("7fff", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) 32767, value4);
  CPPUNIT_ASSERT(HexStringToInt("ffff", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) -1, value4);
  CPPUNIT_ASSERT(HexStringToInt("fff0", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) -16, value4);
  CPPUNIT_ASSERT(HexStringToInt("8000", &value4));
  CPPUNIT_ASSERT_EQUAL((int16_t) -32768, value4);

  CPPUNIT_ASSERT(!HexStringToInt("ffffff", &value4));
  CPPUNIT_ASSERT(!HexStringToInt("ffffffff", &value4));
  CPPUNIT_ASSERT(!HexStringToInt("ef123456", &value4));
  CPPUNIT_ASSERT(!HexStringToInt("fz", &value4));
  CPPUNIT_ASSERT(!HexStringToInt("zfff", &value4));
  CPPUNIT_ASSERT(!HexStringToInt("0xf", &value4));
}


void StringUtilsTest::testPrefixedHexStringToInt() {
  int value;
  CPPUNIT_ASSERT(!PrefixedHexStringToInt("", &value));
  CPPUNIT_ASSERT(!PrefixedHexStringToInt("-1", &value));
  CPPUNIT_ASSERT(!PrefixedHexStringToInt("0", &value));
  CPPUNIT_ASSERT(!PrefixedHexStringToInt("2000", &value));
  CPPUNIT_ASSERT(!PrefixedHexStringToInt("0x", &value));

  CPPUNIT_ASSERT(PrefixedHexStringToInt("0x1", &value));
  CPPUNIT_ASSERT_EQUAL(1, value);
  CPPUNIT_ASSERT(PrefixedHexStringToInt("0xff", &value));
  CPPUNIT_ASSERT_EQUAL(255, value);
  CPPUNIT_ASSERT(PrefixedHexStringToInt("0x70ff", &value));
  CPPUNIT_ASSERT_EQUAL(28927, value);
  CPPUNIT_ASSERT(PrefixedHexStringToInt("0xffffffff", &value));
  CPPUNIT_ASSERT_EQUAL(-1, value);
}


void StringUtilsTest::testStringToUInt16() {
  uint16_t value;

  CPPUNIT_ASSERT(!StringToInt("", &value));
  CPPUNIT_ASSERT(!StringToInt("-1", &value));
  CPPUNIT_ASSERT(!StringToInt("65536", &value));

  CPPUNIT_ASSERT(StringToInt("0", &value));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 0, value);
  CPPUNIT_ASSERT(StringToInt("1", &value));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 1, value);
  CPPUNIT_ASSERT(StringToInt("143", &value));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 143, value);
  CPPUNIT_ASSERT(StringToInt("65535", &value));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 65535, value);
}


void StringUtilsTest::testStringToUInt8() {
  uint8_t value;

  CPPUNIT_ASSERT(!StringToInt("", &value));
  CPPUNIT_ASSERT(!StringToInt("-1", &value));
  CPPUNIT_ASSERT(!StringToInt("256", &value));

  CPPUNIT_ASSERT(StringToInt("0", &value));
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, value);
  CPPUNIT_ASSERT(StringToInt("1", &value));
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, value);
  CPPUNIT_ASSERT(StringToInt("143", &value));
  CPPUNIT_ASSERT_EQUAL((uint8_t) 143, value);
  CPPUNIT_ASSERT(StringToInt("255", &value));
  CPPUNIT_ASSERT_EQUAL((uint8_t) 255, value);
}


void StringUtilsTest::testStringToInt() {
  int value;
  CPPUNIT_ASSERT(!StringToInt("", &value));
  CPPUNIT_ASSERT(!StringToInt("a", &value));

  CPPUNIT_ASSERT(!StringToInt("2147483649", &value));
  CPPUNIT_ASSERT(StringToInt("-2147483648", &value));
  CPPUNIT_ASSERT_EQUAL((int) (-2147483647 - 1), value);
  CPPUNIT_ASSERT(StringToInt("-2147483647", &value));
  CPPUNIT_ASSERT_EQUAL(-2147483647, value);
  CPPUNIT_ASSERT(StringToInt("-1", &value));
  CPPUNIT_ASSERT_EQUAL(-1, value);

  CPPUNIT_ASSERT(StringToInt("0", &value));
  CPPUNIT_ASSERT_EQUAL(0, value);
  CPPUNIT_ASSERT(StringToInt("1", &value));
  CPPUNIT_ASSERT_EQUAL(1, value);
  CPPUNIT_ASSERT(StringToInt("143", &value));
  CPPUNIT_ASSERT_EQUAL(143, value);
  CPPUNIT_ASSERT(StringToInt("2147483647", &value));
  CPPUNIT_ASSERT_EQUAL(2147483647, value);
  CPPUNIT_ASSERT(!StringToInt("2147483648", &value));
}


void StringUtilsTest::testStringToInt16() {
  int16_t value;

  CPPUNIT_ASSERT(!StringToInt("", &value));
  CPPUNIT_ASSERT(!StringToInt("a", &value));

  CPPUNIT_ASSERT(!StringToInt("-32769", &value));
  CPPUNIT_ASSERT(StringToInt("-32768", &value));
  CPPUNIT_ASSERT_EQUAL((int16_t) -32768, value);
  CPPUNIT_ASSERT(StringToInt("-32767", &value));
  CPPUNIT_ASSERT_EQUAL((int16_t) -32767, value);
  CPPUNIT_ASSERT(StringToInt("-1", &value));
  CPPUNIT_ASSERT_EQUAL((int16_t) -1, value);

  CPPUNIT_ASSERT(StringToInt("0", &value));
  CPPUNIT_ASSERT_EQUAL((int16_t) 0, value);
  CPPUNIT_ASSERT(StringToInt("1", &value));
  CPPUNIT_ASSERT_EQUAL((int16_t) 1, value);
  CPPUNIT_ASSERT(StringToInt("143", &value));
  CPPUNIT_ASSERT_EQUAL((int16_t) 143, value);
  CPPUNIT_ASSERT(StringToInt("32767", &value));
  CPPUNIT_ASSERT_EQUAL((int16_t) 32767, value);
  CPPUNIT_ASSERT(!StringToInt("32768", &value));
}


void StringUtilsTest::testStringToInt8() {
  int8_t value;

  CPPUNIT_ASSERT(!StringToInt("", &value));
  CPPUNIT_ASSERT(!StringToInt("a", &value));

  CPPUNIT_ASSERT(!StringToInt("-129", &value));
  CPPUNIT_ASSERT(StringToInt("-128", &value));
  CPPUNIT_ASSERT_EQUAL((int8_t) -128, value);
  CPPUNIT_ASSERT(StringToInt("-127", &value));
  CPPUNIT_ASSERT_EQUAL((int8_t) -127, value);
  CPPUNIT_ASSERT(StringToInt("-127", &value));
  CPPUNIT_ASSERT_EQUAL((int8_t) -127, value);
  CPPUNIT_ASSERT(StringToInt("-1", &value));
  CPPUNIT_ASSERT_EQUAL((int8_t) -1, value);
  CPPUNIT_ASSERT(StringToInt("0", &value));
  CPPUNIT_ASSERT_EQUAL((int8_t) 0, value);
  CPPUNIT_ASSERT(StringToInt("1", &value));
  CPPUNIT_ASSERT_EQUAL((int8_t) 1, value);
  CPPUNIT_ASSERT(StringToInt("127", &value));
  CPPUNIT_ASSERT_EQUAL((int8_t) 127, value);
  CPPUNIT_ASSERT(!StringToInt("128", &value));
  CPPUNIT_ASSERT(!StringToInt("129", &value));
}


void StringUtilsTest::testToLower() {
  string s = "HelLo There";
  ToLower(&s);
  CPPUNIT_ASSERT_EQUAL(string("hello there"), s);
}


void StringUtilsTest::testToUpper() {
  string s = "HelLo There";
  ToUpper(&s);
  CPPUNIT_ASSERT_EQUAL(string("HELLO THERE"), s);
}


void StringUtilsTest::testCapitalizeLabel() {
  string label = "this-is_a_test";
  CapitalizeLabel(&label);
  CPPUNIT_ASSERT_EQUAL(string("This Is A Test"), label);
};


void StringUtilsTest::testCustomCapitalizeLabel() {
  string label1 = "dmx_start_address";
  CustomCapitalizeLabel(&label1);
  CPPUNIT_ASSERT_EQUAL(string("DMX Start Address"), label1);

  string label2 = "foo-dmx";
  CustomCapitalizeLabel(&label2);
  CPPUNIT_ASSERT_EQUAL(string("Foo DMX"), label2);

  string label3 = "mini_dmxter_device";
  CustomCapitalizeLabel(&label3);
  CPPUNIT_ASSERT_EQUAL(string("Mini Dmxter Device"), label3);

  string label4 = "this-is_a_test";
  CapitalizeLabel(&label4);
  CPPUNIT_ASSERT_EQUAL(string("This Is A Test"), label4);
};
