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
 * StringUtilsTest.cpp
 * Unittest for String functions.
 * Copyright (C) 2005 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include <iostream>
#include <string>
#include <vector>

#include "ola/StringUtils.h"
#include "ola/testing/TestUtils.h"

using ola::CapitalizeLabel;
using ola::CustomCapitalizeLabel;
using ola::CapitalizeFirst;
using ola::EncodeString;
using ola::Escape;
using ola::EscapeString;
using ola::FormatData;
using ola::HexStringToInt;
using ola::IntToHexString;
using ola::IntToString;
using ola::PrefixedHexStringToInt;
using ola::ReplaceAll;
using ola::ShortenString;
using ola::StringBeginsWith;
using ola::StringEndsWith;
using ola::StringJoin;
using ola::StringSplit;
using ola::StringToBool;
using ola::StringToBoolTolerant;
using ola::StringToInt;
using ola::StringToIntOrDefault;
using ola::StringTrim;
using ola::StripPrefix;
using ola::StripSuffix;
using ola::ToLower;
using ola::ToUpper;
using ola::strings::ToHex;
using std::ostringstream;
using std::string;
using std::vector;

class StringUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StringUtilsTest);
  CPPUNIT_TEST(testSplit);
  CPPUNIT_TEST(testTrim);
  CPPUNIT_TEST(testShorten);
  CPPUNIT_TEST(testBeginsWith);
  CPPUNIT_TEST(testEndsWith);
  CPPUNIT_TEST(testStripPrefix);
  CPPUNIT_TEST(testStripSuffix);
  CPPUNIT_TEST(testIntToString);
  CPPUNIT_TEST(testIntToHexString);
  CPPUNIT_TEST(testEscape);
  CPPUNIT_TEST(testEncodeString);
  CPPUNIT_TEST(testStringToBool);
  CPPUNIT_TEST(testStringToBoolTolerant);
  CPPUNIT_TEST(testStringToUInt);
  CPPUNIT_TEST(testStringToUIntOrDefault);
  CPPUNIT_TEST(testStringToUInt16);
  CPPUNIT_TEST(testStringToUInt16OrDefault);
  CPPUNIT_TEST(testStringToUInt8);
  CPPUNIT_TEST(testStringToUInt8OrDefault);
  CPPUNIT_TEST(testStringToInt);
  CPPUNIT_TEST(testStringToIntOrDefault);
  CPPUNIT_TEST(testStringToInt16);
  CPPUNIT_TEST(testStringToInt16OrDefault);
  CPPUNIT_TEST(testStringToInt8);
  CPPUNIT_TEST(testStringToInt8OrDefault);
  CPPUNIT_TEST(testHexStringToInt);
  CPPUNIT_TEST(testPrefixedHexStringToInt);
  CPPUNIT_TEST(testToLower);
  CPPUNIT_TEST(testToUpper);
  CPPUNIT_TEST(testCapitalizeLabel);
  CPPUNIT_TEST(testCustomCapitalizeLabel);
  CPPUNIT_TEST(testCapitalizeFirst);
  CPPUNIT_TEST(testFormatData);
  CPPUNIT_TEST(testStringJoin);
  CPPUNIT_TEST(testReplaceAll);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testSplit();
    void testTrim();
    void testShorten();
    void testBeginsWith();
    void testEndsWith();
    void testStripPrefix();
    void testStripSuffix();
    void testIntToString();
    void testIntToHexString();
    void testEscape();
    void testEncodeString();
    void testStringToBool();
    void testStringToBoolTolerant();
    void testStringToUInt();
    void testStringToUIntOrDefault();
    void testStringToUInt16();
    void testStringToUInt16OrDefault();
    void testStringToUInt8();
    void testStringToUInt8OrDefault();
    void testStringToInt();
    void testStringToIntOrDefault();
    void testStringToInt16();
    void testStringToInt16OrDefault();
    void testStringToInt8();
    void testStringToInt8OrDefault();
    void testHexStringToInt();
    void testPrefixedHexStringToInt();
    void testToLower();
    void testToUpper();
    void testCapitalizeLabel();
    void testCustomCapitalizeLabel();
    void testCapitalizeFirst();
    void testFormatData();
    void testStringJoin();
    void testReplaceAll();
};


CPPUNIT_TEST_SUITE_REGISTRATION(StringUtilsTest);

/*
 * Test the split function
 */
void StringUtilsTest::testSplit() {
  vector<string> tokens;
  string input = "";
  StringSplit(input, &tokens);
  OLA_ASSERT_EQ((size_t) 1, tokens.size());
  OLA_ASSERT_EQ(string(""), tokens[0]);

  input = "1 2 345";
  tokens.clear();
  StringSplit(input, &tokens);

  OLA_ASSERT_EQ((size_t) 3, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
  OLA_ASSERT_EQ(string("2"), tokens[1]);
  OLA_ASSERT_EQ(string("345"), tokens[2]);

  input = "1,2,345";
  tokens.clear();
  StringSplit(input, &tokens, ",");

  OLA_ASSERT_EQ((size_t) 3, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
  OLA_ASSERT_EQ(string("2"), tokens[1]);
  OLA_ASSERT_EQ(string("345"), tokens[2]);

  input = ",1,2,345,,";
  tokens.clear();
  StringSplit(input, &tokens, ",");

  OLA_ASSERT_EQ((size_t) 6, tokens.size());
  OLA_ASSERT_EQ(string(""), tokens[0]);
  OLA_ASSERT_EQ(string("1"), tokens[1]);
  OLA_ASSERT_EQ(string("2"), tokens[2]);
  OLA_ASSERT_EQ(string("345"), tokens[3]);
  OLA_ASSERT_EQ(string(""), tokens[4]);
  OLA_ASSERT_EQ(string(""), tokens[5]);

  input = "1 2,345";
  tokens.clear();
  StringSplit(input, &tokens, " ,");

  OLA_ASSERT_EQ((size_t) 3, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
  OLA_ASSERT_EQ(string("2"), tokens[1]);
  OLA_ASSERT_EQ(string("345"), tokens[2]);

  input = "1, 2,345";
  tokens.clear();
  StringSplit(input, &tokens, " ,");

  OLA_ASSERT_EQ((size_t) 4, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);
  OLA_ASSERT_EQ(string(""), tokens[1]);
  OLA_ASSERT_EQ(string("2"), tokens[2]);
  OLA_ASSERT_EQ(string("345"), tokens[3]);

  input = "1";
  tokens.clear();
  StringSplit(input, &tokens, ".");

  OLA_ASSERT_EQ((size_t) 1, tokens.size());
  OLA_ASSERT_EQ(string("1"), tokens[0]);

  // And the old non-pointer version
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
 * Test the StringBeginsWith function.
 */
void StringUtilsTest::testBeginsWith() {
  string input = "foo bar baz";
  OLA_ASSERT_TRUE(StringBeginsWith(input, "foo"));
  OLA_ASSERT_TRUE(StringBeginsWith(input, "foo "));
  OLA_ASSERT_TRUE(StringBeginsWith(input, "foo bar"));
  OLA_ASSERT_TRUE(StringBeginsWith(input, ""));
  OLA_ASSERT_FALSE(StringBeginsWith(input, "baz"));
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
 * Test the StripPrefix function.
 */
void StringUtilsTest::testStripPrefix() {
  string input = "foo bar baz";
  OLA_ASSERT_TRUE(StripPrefix(&input, "foo"));
  OLA_ASSERT_EQ(string(" bar baz"), input);

  input = "foo bar baz";
  OLA_ASSERT_TRUE(StripPrefix(&input, "foo "));
  OLA_ASSERT_EQ(string("bar baz"), input);

  input = "foo bar baz";
  OLA_ASSERT_TRUE(StripPrefix(&input, "foo bar"));
  OLA_ASSERT_EQ(string(" baz"), input);

  input = "foo bar baz";
  OLA_ASSERT_TRUE(StripPrefix(&input, ""));
  OLA_ASSERT_EQ(string("foo bar baz"), input);

  input = "foo bar baz";
  OLA_ASSERT_FALSE(StripPrefix(&input, "baz"));
}


/*
 * Test the StripSuffix function.
 */
void StringUtilsTest::testStripSuffix() {
  string input = "foo bar baz";
  OLA_ASSERT_TRUE(StripSuffix(&input, "baz"));
  OLA_ASSERT_EQ(string("foo bar "), input);

  input = "foo bar baz";
  OLA_ASSERT_TRUE(StripSuffix(&input, " baz"));
  OLA_ASSERT_EQ(string("foo bar"), input);

  input = "foo bar baz";
  OLA_ASSERT_TRUE(StripSuffix(&input, "bar baz"));
  OLA_ASSERT_EQ(string("foo "), input);

  input = "foo bar baz";
  OLA_ASSERT_TRUE(StripSuffix(&input, ""));
  OLA_ASSERT_EQ(string("foo bar baz"), input);

  input = "foo bar baz";
  OLA_ASSERT_FALSE(StripSuffix(&input, "foo"));
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


/*
 * test the IntToHexString function.
 */
void StringUtilsTest::testIntToHexString() {
  // Using the old IntToHexString
  OLA_ASSERT_EQ(string("0x00"), IntToHexString((uint8_t)0));

  OLA_ASSERT_EQ(string("0x01"), IntToHexString((uint8_t)1));
  OLA_ASSERT_EQ(string("0x42"), IntToHexString((uint8_t)0x42));
  OLA_ASSERT_EQ(string("0x0001"), IntToHexString((uint16_t)0x0001));
  OLA_ASSERT_EQ(string("0xabcd"), IntToHexString((uint16_t)0xABCD));
  OLA_ASSERT_EQ(string("0xdeadbeef"), IntToHexString((uint32_t)0xDEADBEEF));
  // Deliberately no IntToHexString(uint64_t) or test as its deprecated

  unsigned int i = 0x42;
  OLA_ASSERT_EQ(string("0x00000042"), IntToHexString(i));

  // Using the inline string concatenation
  ostringstream str;
  str << ToHex((uint8_t)0);
  OLA_ASSERT_EQ(string("0x00"), str.str());
  str.str("");

  str << ToHex((uint8_t)1);
  OLA_ASSERT_EQ(string("0x01"), str.str());
  str.str("");

  str << ToHex((uint8_t)0x42);
  OLA_ASSERT_EQ(string("0x42"), str.str());
  str.str("");

  str << ToHex((uint16_t)0x0001);
  OLA_ASSERT_EQ(string("0x0001"), str.str());
  str.str("");

  str << ToHex((uint16_t)0xABCD);
  OLA_ASSERT_EQ(string("0xabcd"), str.str());
  str.str("");

  str << ToHex((uint32_t)0xDEADBEEF);
  OLA_ASSERT_EQ(string("0xdeadbeef"), str.str());
  str.str("");

  str << ToHex((uint64_t)0xDEADBEEFFEEDFACE);
  OLA_ASSERT_EQ(string("0xdeadbeeffeedface"), str.str());
  str.str("");

  str << ToHex(i);
  OLA_ASSERT_EQ(string("0x00000042"), str.str());
  str.str("");

  // Without prefix
  str << ToHex((uint8_t)0x42, false);
  OLA_ASSERT_EQ(string("42"), str.str());
  str.str("");

  str << ToHex((uint16_t)0xABCD, false);
  OLA_ASSERT_EQ(string("abcd"), str.str());
  str.str("");
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


/**
 * Test encoding string
 */
void StringUtilsTest::testEncodeString() {
  string s1 = "foo";
  OLA_ASSERT_EQ(string("foo"), EncodeString(s1));

  s1 = "newline\ntest";
  OLA_ASSERT_EQ(string("newline\\x0atest"), EncodeString(s1));

  s1 = "newline\n\ntest";
  OLA_ASSERT_EQ(string("newline\\x0a\\x0atest"), EncodeString(s1));

  s1 = "\x01newline\x02test";
  OLA_ASSERT_EQ(string("\\x01newline\\x02test"), EncodeString(s1));

  // Test a null in the middle of a string
  s1 = string("newline" "\x00" "test", 12);
  OLA_ASSERT_EQ(string("newline\\x00test"), EncodeString(s1));
}


void StringUtilsTest::testStringToBool() {
  bool value;
  OLA_ASSERT_FALSE(StringToBool("", &value));
  OLA_ASSERT_FALSE(StringToBool("-1", &value));
  OLA_ASSERT_FALSE(StringToBool("2", &value));
  OLA_ASSERT_FALSE(StringToBool("a", &value));
  OLA_ASSERT_TRUE(StringToBool("true", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBool("false", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBool("TrUE", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBool("FalSe", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBool("t", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBool("f", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBool("T", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBool("F", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBool("1", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBool("0", &value));
  OLA_ASSERT_EQ(value, false);
}


void StringUtilsTest::testStringToBoolTolerant() {
  bool value;
  OLA_ASSERT_FALSE(StringToBoolTolerant("", &value));
  OLA_ASSERT_FALSE(StringToBoolTolerant("-1", &value));
  OLA_ASSERT_FALSE(StringToBoolTolerant("2", &value));
  OLA_ASSERT_FALSE(StringToBoolTolerant("a", &value));
  OLA_ASSERT_TRUE(StringToBoolTolerant("true", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("false", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("TrUE", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("FalSe", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("t", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("f", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("T", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("F", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("1", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("0", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("on", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("off", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("On", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("oFf", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("enable", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("disable", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("EnAblE", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("dISaBle", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("enabled", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("disabled", &value));
  OLA_ASSERT_EQ(value, false);
  OLA_ASSERT_TRUE(StringToBoolTolerant("eNabLED", &value));
  OLA_ASSERT_EQ(value, true);
  OLA_ASSERT_TRUE(StringToBoolTolerant("DisaBLED", &value));
  OLA_ASSERT_EQ(value, false);
}


void StringUtilsTest::testStringToUInt() {
  unsigned int value;
  OLA_ASSERT_FALSE(StringToInt("", &value));
  OLA_ASSERT_FALSE(StringToInt("-1", &value));
  OLA_ASSERT_TRUE(StringToInt("0", &value));
  OLA_ASSERT_EQ(0u, value);
  OLA_ASSERT_TRUE(StringToInt("1", &value));
  OLA_ASSERT_EQ(1u, value);
  OLA_ASSERT_TRUE(StringToInt("    42050", &value));
  OLA_ASSERT_EQ(42050u, value);
  OLA_ASSERT_TRUE(StringToInt("65537", &value));
  OLA_ASSERT_EQ(65537u, value);
  OLA_ASSERT_TRUE(StringToInt("4294967295", &value));
  OLA_ASSERT_EQ(4294967295U, value);
  uint64_t value2;
  OLA_ASSERT_TRUE(StringToInt("77000000000", &value2));
  OLA_ASSERT_EQ((uint64_t) 77000000000, value2);
  OLA_ASSERT_FALSE(StringToInt("4294967296", &value));
  OLA_ASSERT_FALSE(StringToInt("foo", &value));

  // same tests with strict mode on
  OLA_ASSERT_FALSE(StringToInt("-1 foo", &value, true));
  OLA_ASSERT_FALSE(StringToInt("0 ", &value, true));
  OLA_ASSERT_FALSE(StringToInt("1 bar baz", &value, true));
  OLA_ASSERT_FALSE(StringToInt("65537cat", &value, true));
  OLA_ASSERT_FALSE(StringToInt("4294967295bat bar", &value, true));
}

void StringUtilsTest::testStringToUIntOrDefault() {
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("", 42u));
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("-1", 42u));
  OLA_ASSERT_EQ(0u, StringToIntOrDefault("0", 42u));
  OLA_ASSERT_EQ(1u, StringToIntOrDefault("1", 42u));
  OLA_ASSERT_EQ(65537u, StringToIntOrDefault("65537", 42u));
  OLA_ASSERT_EQ(4294967295U, StringToIntOrDefault("4294967295", 42U));
  OLA_ASSERT_EQ((uint64_t) 77000000000,
                StringToIntOrDefault("77000000000", (uint64_t) 42));
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("4294967296", 42u));
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("foo", 42u));

  // same tests with strict mode on
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("-1 foo", 42u, true));
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("0 ", 42u, true));
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("1 bar baz", 42u, true));
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("65537cat", 42u, true));
  OLA_ASSERT_EQ(42u, StringToIntOrDefault("4294967295bat bar", 42u, true));
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

  // uint64_t
  uint64_t value3;
  OLA_ASSERT_TRUE(HexStringToInt("11ed8ec200", &value3));
  OLA_ASSERT_EQ((uint64_t) 77000000000, value3);

  // int8_t
  int8_t value4;
  OLA_ASSERT_FALSE(HexStringToInt("", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("-1", &value4));

  OLA_ASSERT_TRUE(HexStringToInt("0", &value4));
  OLA_ASSERT_EQ((int8_t) 0, value4);
  OLA_ASSERT_TRUE(HexStringToInt("1", &value4));
  OLA_ASSERT_EQ((int8_t) 1, value4);
  OLA_ASSERT_TRUE(HexStringToInt("a", &value4));
  OLA_ASSERT_EQ((int8_t) 10, value4);
  OLA_ASSERT_TRUE(HexStringToInt("f", &value4));
  OLA_ASSERT_EQ((int8_t) 15, value4);
  OLA_ASSERT_TRUE(HexStringToInt("7f", &value4));
  OLA_ASSERT_EQ((int8_t) 127, value4);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value4));
  OLA_ASSERT_EQ((int8_t) -95, value4);
  OLA_ASSERT_TRUE(HexStringToInt("80", &value4));
  OLA_ASSERT_EQ((int8_t) -128, value4);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value4));
  OLA_ASSERT_EQ((int8_t) -1, value4);

  OLA_ASSERT_FALSE(HexStringToInt("ffff", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("fff0", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("ffffff", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("ffffffff", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("ef123456", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("fz", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("zfff", &value4));
  OLA_ASSERT_FALSE(HexStringToInt("0xf", &value4));

  // int16_t
  int16_t value5;
  OLA_ASSERT_FALSE(HexStringToInt("", &value5));
  OLA_ASSERT_FALSE(HexStringToInt("-1", &value5));

  OLA_ASSERT_TRUE(HexStringToInt("0", &value5));
  OLA_ASSERT_EQ((int16_t) 0, value5);
  OLA_ASSERT_TRUE(HexStringToInt("1", &value5));
  OLA_ASSERT_EQ((int16_t) 1, value5);
  OLA_ASSERT_TRUE(HexStringToInt("a", &value5));
  OLA_ASSERT_EQ((int16_t) 10, value5);
  OLA_ASSERT_TRUE(HexStringToInt("f", &value5));
  OLA_ASSERT_EQ((int16_t) 15, value5);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value5));
  OLA_ASSERT_EQ((int16_t) 161, value5);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value5));
  OLA_ASSERT_EQ((int16_t) 255, value5);
  OLA_ASSERT_TRUE(HexStringToInt("7fff", &value5));
  OLA_ASSERT_EQ((int16_t) 32767, value5);
  OLA_ASSERT_TRUE(HexStringToInt("ffff", &value5));
  OLA_ASSERT_EQ((int16_t) -1, value5);
  OLA_ASSERT_TRUE(HexStringToInt("fff0", &value5));
  OLA_ASSERT_EQ((int16_t) -16, value5);
  OLA_ASSERT_TRUE(HexStringToInt("8000", &value5));
  OLA_ASSERT_EQ((int16_t) -32768, value5);

  OLA_ASSERT_FALSE(HexStringToInt("ffffff", &value5));
  OLA_ASSERT_FALSE(HexStringToInt("ffffffff", &value5));
  OLA_ASSERT_FALSE(HexStringToInt("ef123456", &value5));
  OLA_ASSERT_FALSE(HexStringToInt("fz", &value5));
  OLA_ASSERT_FALSE(HexStringToInt("zfff", &value5));
  OLA_ASSERT_FALSE(HexStringToInt("0xf", &value5));

  // int32
  int32_t value6;
  OLA_ASSERT_FALSE(HexStringToInt("", &value6));
  OLA_ASSERT_FALSE(HexStringToInt("-1", &value6));

  OLA_ASSERT_TRUE(HexStringToInt("0", &value6));
  OLA_ASSERT_EQ((int32_t) 0, value6);
  OLA_ASSERT_TRUE(HexStringToInt("1", &value6));
  OLA_ASSERT_EQ((int32_t) 1, value6);
  OLA_ASSERT_TRUE(HexStringToInt("a", &value6));
  OLA_ASSERT_EQ((int32_t) 10, value6);
  OLA_ASSERT_TRUE(HexStringToInt("f", &value6));
  OLA_ASSERT_EQ((int32_t) 15, value6);
  OLA_ASSERT_TRUE(HexStringToInt("a1", &value6));
  OLA_ASSERT_EQ((int32_t) 161, value6);
  OLA_ASSERT_TRUE(HexStringToInt("ff", &value6));
  OLA_ASSERT_EQ((int32_t) 255, value6);
  OLA_ASSERT_TRUE(HexStringToInt("7fff", &value6));
  OLA_ASSERT_EQ((int32_t) 32767, value6);
  OLA_ASSERT_TRUE(HexStringToInt("ffff", &value6));
  OLA_ASSERT_EQ((int32_t) 65535, value6);
  OLA_ASSERT_TRUE(HexStringToInt("ffffffff", &value6));
  OLA_ASSERT_EQ((int32_t) -1, value6);
  OLA_ASSERT_TRUE(HexStringToInt("fffffff0", &value6));
  OLA_ASSERT_EQ((int32_t) -16, value6);
  OLA_ASSERT_TRUE(HexStringToInt("80000000", &value6));
  OLA_ASSERT_EQ((int32_t) -2147483647 - 1, value6);
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

  OLA_ASSERT_TRUE(PrefixedHexStringToInt("0X7f", &value));
  OLA_ASSERT_EQ(127, value);
  OLA_ASSERT_TRUE(PrefixedHexStringToInt("0X7F", &value));
  OLA_ASSERT_EQ(127, value);
  OLA_ASSERT_TRUE(PrefixedHexStringToInt("0x7F", &value));
  OLA_ASSERT_EQ(127, value);
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


void StringUtilsTest::testStringToUInt16OrDefault() {
  OLA_ASSERT_EQ((uint16_t) 42, StringToIntOrDefault("", (uint16_t) 42));
  OLA_ASSERT_EQ((uint16_t) 42, StringToIntOrDefault("-1", (uint16_t) 42));
  OLA_ASSERT_EQ((uint16_t) 42, StringToIntOrDefault("65536", (uint16_t) 42));

  OLA_ASSERT_EQ((uint16_t) 0, StringToIntOrDefault("0", (uint16_t) 42));
  OLA_ASSERT_EQ((uint16_t) 1, StringToIntOrDefault("1", (uint16_t) 42));
  OLA_ASSERT_EQ((uint16_t) 143, StringToIntOrDefault("143", (uint16_t) 42));
  OLA_ASSERT_EQ((uint16_t) 65535, StringToIntOrDefault("65535", (uint16_t) 42));
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


void StringUtilsTest::testStringToUInt8OrDefault() {
  OLA_ASSERT_EQ((uint8_t) 42, StringToIntOrDefault("", (uint8_t) 42));
  OLA_ASSERT_EQ((uint8_t) 42, StringToIntOrDefault("-1", (uint8_t) 42));
  OLA_ASSERT_EQ((uint8_t) 42, StringToIntOrDefault("256", (uint8_t) 42));

  OLA_ASSERT_EQ((uint8_t) 0, StringToIntOrDefault("0", (uint8_t) 42));
  OLA_ASSERT_EQ((uint8_t) 1, StringToIntOrDefault("1", (uint8_t) 42));
  OLA_ASSERT_EQ((uint8_t) 143, StringToIntOrDefault("143", (uint8_t) 42));
  OLA_ASSERT_EQ((uint8_t) 255, StringToIntOrDefault("255", (uint8_t) 42));
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


void StringUtilsTest::testStringToIntOrDefault() {
  OLA_ASSERT_EQ(42, StringToIntOrDefault("", 42));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("a", 42));

  OLA_ASSERT_EQ(42, StringToIntOrDefault("2147483649", 42));
  OLA_ASSERT_EQ(static_cast<int>(-2147483647 - 1),
                StringToIntOrDefault("-2147483648", 42));
  OLA_ASSERT_EQ(-2147483647, StringToIntOrDefault("-2147483647", 42));
  OLA_ASSERT_EQ(-1, StringToIntOrDefault("-1", 42));

  OLA_ASSERT_EQ(0, StringToIntOrDefault("0", 42));
  OLA_ASSERT_EQ(1, StringToIntOrDefault("1", 42));
  OLA_ASSERT_EQ(143, StringToIntOrDefault("143", 42));
  OLA_ASSERT_EQ(2147483647, StringToIntOrDefault("2147483647", 42));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("2147483648", 42));

  // strict mode on
  OLA_ASSERT_EQ(42, StringToIntOrDefault("2147483649 ", 42, true));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("-2147483648bar", 42, true));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("-2147483647 baz", 42, true));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("-1.", 42, true));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("0!", 42, true));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("1 this is a test", 42, true));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("143car", 42, true));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("2147483647 !@#", 42, true));
  OLA_ASSERT_EQ(42, StringToIntOrDefault("2147483648mm", 42, true));
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


void StringUtilsTest::testStringToInt16OrDefault() {
  OLA_ASSERT_EQ((int16_t) 42, StringToIntOrDefault("", (int16_t) 42));
  OLA_ASSERT_EQ((int16_t) 42, StringToIntOrDefault("a", (int16_t) 42));

  OLA_ASSERT_EQ((int16_t) 42, StringToIntOrDefault("-32769", (int16_t) 42));
  OLA_ASSERT_EQ((int16_t) -32768, StringToIntOrDefault("-32768", (int16_t) 42));
  OLA_ASSERT_EQ((int16_t) -32767, StringToIntOrDefault("-32767", (int16_t) 42));
  OLA_ASSERT_EQ((int16_t) -1, StringToIntOrDefault("-1", (int16_t) 42));

  OLA_ASSERT_EQ((int16_t) 0, StringToIntOrDefault("0", (int16_t) 42));
  OLA_ASSERT_EQ((int16_t) 1, StringToIntOrDefault("1", (int16_t) 42));
  OLA_ASSERT_EQ((int16_t) 143, StringToIntOrDefault("143", (int16_t) 42));
  OLA_ASSERT_EQ((int16_t) 32767, StringToIntOrDefault("32767", (int16_t) 42));
  OLA_ASSERT_EQ((int16_t) 42, StringToIntOrDefault("32768", (int16_t) 42));
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


void StringUtilsTest::testStringToInt8OrDefault() {
  OLA_ASSERT_EQ((int8_t) 42, StringToIntOrDefault("", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) 42, StringToIntOrDefault("a", (int8_t) 42));

  OLA_ASSERT_EQ((int8_t) 42, StringToIntOrDefault("-129", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) -128, StringToIntOrDefault("-128", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) -127, StringToIntOrDefault("-127", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) -127, StringToIntOrDefault("-127", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) -1, StringToIntOrDefault("-1", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) 0, StringToIntOrDefault("0", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) 1, StringToIntOrDefault("1", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) 127, StringToIntOrDefault("127", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) 42, StringToIntOrDefault("128", (int8_t) 42));
  OLA_ASSERT_EQ((int8_t) 42, StringToIntOrDefault("129", (int8_t) 42));
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
}


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

  string label7 = "dazzled_led_type";
  CustomCapitalizeLabel(&label7);
  OLA_ASSERT_EQ(string("Dazzled LED Type"), label7);

  string label8 = "device_rdm_uid";
  CustomCapitalizeLabel(&label8);
  OLA_ASSERT_EQ(string("Device RDM UID"), label8);

  string label9 = "dns_via_ipv4_dhcp";
  CustomCapitalizeLabel(&label9);
  OLA_ASSERT_EQ(string("DNS Via IPV4 DHCP"), label9);
}


void StringUtilsTest::testCapitalizeFirst() {
  string label1 = "foo bar";
  CapitalizeFirst(&label1);
  OLA_ASSERT_EQ(string("Foo bar"), label1);

  string label2 = "RDM bar";
  CapitalizeFirst(&label2);
  OLA_ASSERT_EQ(string("RDM bar"), label2);
}


void StringUtilsTest::testFormatData() {
  uint8_t data[] = {0, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd',
                    1, 2};
  std::ostringstream str;
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

  vector<char> chars;
  chars.push_back('a');
  chars.push_back('b');
  chars.push_back('c');
  OLA_ASSERT_EQ(string("a,b,c"), StringJoin(",", chars));

  vector<string> strings;
  strings.push_back("one");
  strings.push_back("two");
  strings.push_back("three");
  OLA_ASSERT_EQ(string("one,two,three"), StringJoin(",", strings));
}


void StringUtilsTest::testReplaceAll() {
  string input = "";
  ReplaceAll(&input, "", "");
  OLA_ASSERT_EQ(string(""), input);

  input = "abc";
  ReplaceAll(&input, "", "");
  OLA_ASSERT_EQ(string("abc"), input);
  ReplaceAll(&input, "", "def");
  OLA_ASSERT_EQ(string("abc"), input);

  input = "abc";
  ReplaceAll(&input, "b", "d");
  OLA_ASSERT_EQ(string("adc"), input);

  input = "aaa";
  ReplaceAll(&input, "a", "b");
  OLA_ASSERT_EQ(string("bbb"), input);

  input = "abcdef";
  ReplaceAll(&input, "cd", "cds");
  OLA_ASSERT_EQ(string("abcdsef"), input);

  input = "abcdefabcdef";
  ReplaceAll(&input, "cd", "gh");
  OLA_ASSERT_EQ(string("abghefabghef"), input);

  input = "abcde";
  ReplaceAll(&input, "c", "cdc");
  OLA_ASSERT_EQ(string("abcdcde"), input);

  input = "abcdcdce";
  ReplaceAll(&input, "cdc", "c");
  OLA_ASSERT_EQ(string("abce"), input);

  input = "abcdcdcdce";
  ReplaceAll(&input, "cdcdc", "cdc");
  OLA_ASSERT_EQ(string("abcdce"), input);
}
