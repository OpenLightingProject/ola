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
 * SLPStringsTest.cpp
 * Test fixture for the SLPStrings functions.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPStrings.h"

using ola::slp::SLPCanonicalizeString;
using ola::slp::SLPGetCanonicalString;
using ola::slp::SLPReduceList;
using ola::slp::SLPScopesMatch;
using ola::slp::SLPSetIntersect;
using ola::slp::SLPStringCanonicalizeAndCompare;
using ola::slp::SLPStringEscape;
using ola::slp::SLPStringUnescape;
using std::set;
using std::string;
using std::vector;


class SLPStringsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SLPStringsTest);
  CPPUNIT_TEST(testEscape);
  CPPUNIT_TEST(testUnescape);
  CPPUNIT_TEST(testCanonicalize);
  CPPUNIT_TEST(testComparison);
  CPPUNIT_TEST(testIntersection);
  CPPUNIT_TEST(testReduceList);
  CPPUNIT_TEST(testScopesMatch);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testEscape();
    void testUnescape();
    void testCanonicalize();
    void testComparison();
    void testIntersection();
    void testReduceList();
    void testScopesMatch();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(SLPStringsTest);

/**
 * Test that SLP escaping works
 */
void SLPStringsTest::testEscape() {
  string str = "this is a \\ string, with commas";
  SLPStringEscape(&str);
  OLA_ASSERT_EQ(string("this is a \\5c string\\2c with commas"), str);

  str = "ends in a ,";
  SLPStringEscape(&str);
  OLA_ASSERT_EQ(string("ends in a \\2c"), str);
}


/**
 * test that SLP un-escaping works
 */
void SLPStringsTest::testUnescape() {
  string str = "an \\5cescaped\\2c string";
  SLPStringUnescape(&str);
  OLA_ASSERT_EQ(string("an \\escaped, string"), str);

  str = "\\z";
  SLPStringUnescape(&str);
  OLA_ASSERT_EQ(string(""), str);

  // These should generate warnings and just discard the remaining characters.
  str = "\\";
  SLPStringUnescape(&str);
  OLA_ASSERT_EQ(string(""), str);
  str = "\\2";
  SLPStringUnescape(&str);
  OLA_ASSERT_EQ(string(""), str);
}


/*
 * Check that we can convert strings to their canonical form
 */
void SLPStringsTest::testCanonicalize() {
  string one = "  Some String  ";
  SLPCanonicalizeString(&one);
  OLA_ASSERT_EQ(string("some string"), one);

  string two = "SOME   STRING";
  SLPCanonicalizeString(&two);
  OLA_ASSERT_EQ(string("some string"), two);

  // now try the const version
  const string three = "  Some String";
  string output = SLPGetCanonicalString(three);
  OLA_ASSERT_EQ(string("some string"), output);
}


/*
 * Check that comparisons fold whitesare and are case insensitive.
 */
void SLPStringsTest::testComparison() {
  const string one = "  Some String  ";
  const string two = "SOME   STRING";

  OLA_ASSERT_TRUE(SLPStringCanonicalizeAndCompare(one, two));
  OLA_ASSERT_TRUE(SLPStringCanonicalizeAndCompare("", "  "));
  OLA_ASSERT_TRUE(SLPStringCanonicalizeAndCompare("", "\t\r"));
  OLA_ASSERT_TRUE(SLPStringCanonicalizeAndCompare("Foo Bar", "Foo\tBar"));
  OLA_ASSERT_TRUE(SLPStringCanonicalizeAndCompare("  foo", "Foo  \r"));
}


/*
 * Check that SLPSetIntersect works.
 */
void SLPStringsTest::testIntersection() {
  set<string> one, two;
  OLA_ASSERT_FALSE(SLPSetIntersect(one, two));

  one.insert("default");
  OLA_ASSERT_FALSE(SLPSetIntersect(one, two));

  two.insert("default");
  OLA_ASSERT_TRUE(SLPSetIntersect(one, two));
}


/*
 * Check that SLPReduceList works.
 */
void SLPStringsTest::testReduceList() {
  set<string> output;
  vector<string> input;
  input.push_back("default");
  input.push_back("DEFAULT");
  input.push_back("  DEFAULT  ");
  input.push_back("  Some String  ");
  input.push_back("SOME   STRING");

  SLPReduceList(input, &output);
  set<string> expected;
  expected.insert("default");
  expected.insert("some string");
  OLA_ASSERT_SET_EQ(expected, output);
}


/**
 * Test the SLPScopesMatch function
 */
void SLPStringsTest::testScopesMatch() {
  vector<string> input;
  set<string> output;

  OLA_ASSERT_FALSE(SLPScopesMatch(input, output));
  input.push_back("DEFAULT");
  OLA_ASSERT_FALSE(SLPScopesMatch(input, output));
  output.insert("default");
  OLA_ASSERT_TRUE(SLPScopesMatch(input, output));
}
