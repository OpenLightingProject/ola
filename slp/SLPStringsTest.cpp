/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SLPStringsTest.cpp
 * Test fixture for the SLPStrings functions.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "slp/SLPStrings.h"

using ola::slp::SLPCanonicalizeString;
using ola::slp::SLPGetCanonicalString;
using ola::slp::SLPServiceFromURL;
using ola::slp::SLPStringEscape;
using ola::slp::SLPStringUnescape;
using ola::slp::SLPStripServiceFromURL;
using std::string;


class SLPStringsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SLPStringsTest);
  CPPUNIT_TEST(testEscape);
  CPPUNIT_TEST(testUnescape);
  CPPUNIT_TEST(testCanonicalize);
  CPPUNIT_TEST(testSLPServiceFromURL);
  CPPUNIT_TEST(testSLPStripServiceFromURL);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testEscape();
    void testUnescape();
    void testCanonicalize();
    void testSLPServiceFromURL();
    void testSLPStripServiceFromURL();

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

  str = "no escaping here";
  SLPStringEscape(&str);
  OLA_ASSERT_EQ(string("no escaping here"), str);
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

  str = "\\80";
  SLPStringUnescape(&str);
  OLA_ASSERT_EQ(string("\\80"), str);

  str = "\\zz";
  SLPStringUnescape(&str);
  OLA_ASSERT_EQ(string("\\zz"), str);
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


/**
 * Test that SLPServiceFromURL() works.
 */
void SLPStringsTest::testSLPServiceFromURL() {
  OLA_ASSERT_EQ(string("service:foo"), SLPServiceFromURL("service:foo"));
  OLA_ASSERT_EQ(string("service:foo"), SLPServiceFromURL("service:FoO"));
  OLA_ASSERT_EQ(string("foo"), SLPServiceFromURL("foo"));
  OLA_ASSERT_EQ(string("foo"), SLPServiceFromURL("FoO"));
  OLA_ASSERT_EQ(string("service:foo"),
                SLPServiceFromURL("service:foo://localhost:9090"));
  OLA_ASSERT_EQ(string("service:printer"),
                SLPServiceFromURL("service:printer://foo"));
  OLA_ASSERT_EQ(string("service:printer:lpr"),
                SLPServiceFromURL("service:printer:lpr://foo"));
  OLA_ASSERT_EQ(string("service:foo.myorg"),
                SLPServiceFromURL("service:foo.myorg://bar"));
  OLA_ASSERT_EQ(string("service:foo.myorg:bar"),
                SLPServiceFromURL("service:foo.myorg:bar://baz"));
}


/**
 * Test that SLPStripServiceFromURL() works.
 */
void SLPStringsTest::testSLPStripServiceFromURL() {
  OLA_ASSERT_EQ(string(""), SLPStripServiceFromURL(""));
  OLA_ASSERT_EQ(string(""), SLPStripServiceFromURL("service:FoO"));
  OLA_ASSERT_EQ(string(""), SLPStripServiceFromURL("service:foo://"));
  OLA_ASSERT_EQ(string("localhost:9090"),
                SLPStripServiceFromURL("service:foo://localhost:9090"));
  OLA_ASSERT_EQ(string("foo"), SLPStripServiceFromURL("service:printer://foo"));
  OLA_ASSERT_EQ(string("foo"),
                SLPStripServiceFromURL("service:printer:lpr://foo"));
  OLA_ASSERT_EQ(
      string("10.0.0.1/7a7000000001"),
      SLPStripServiceFromURL("service:rdmnet-device://10.0.0.1/7a7000000001"));
}
