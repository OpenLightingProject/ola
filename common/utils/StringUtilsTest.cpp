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

#include <iostream>
#include <cppunit/extensions/HelperMacros.h>
#include <lla/StringUtils.h>

using namespace lla;
using namespace std;

class StringUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StringUtilsTest);
  CPPUNIT_TEST(testSplit);
  CPPUNIT_TEST(testTrim);
  CPPUNIT_TEST(testIntToString);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSplit();
    void testTrim();
    void testIntToString();
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
}


/*
 * Test the trim function.
 */
void StringUtilsTest::testTrim() {
  string input = "foo bar baz";
  StringTrim(input);
  CPPUNIT_ASSERT_EQUAL(input, input);
  input = "  \rfoo bar\t\n";
  StringTrim(input);
  CPPUNIT_ASSERT_EQUAL(string("foo bar"), input);
  input = "  \r\t\n";
  StringTrim(input);
  CPPUNIT_ASSERT_EQUAL(string(""), input);

}

/*
 * test the IntToString function.
 */
void StringUtilsTest::testIntToString() {
  CPPUNIT_ASSERT_EQUAL(string("0"), IntToString(0));
  CPPUNIT_ASSERT_EQUAL(string("1234"), IntToString(1234));
  CPPUNIT_ASSERT_EQUAL(string("-1234"), IntToString(-1234));
}
