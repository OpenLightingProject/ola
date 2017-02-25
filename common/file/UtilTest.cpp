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
 * UtilTest.cpp
 * Unittest for file util functions.
 * Copyright (C) 2013 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/file/Util.h"
#include "ola/testing/TestUtils.h"


using ola::file::FilenameFromPath;
using ola::file::FilenameFromPathOrDefault;
using ola::file::FilenameFromPathOrPath;
using ola::file::JoinPaths;
using std::string;

class UtilTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UtilTest);
  CPPUNIT_TEST(testJoinPaths);
  CPPUNIT_TEST(testFilenameFromPath);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testFilenameFromPath();
  void testJoinPaths();
};


CPPUNIT_TEST_SUITE_REGISTRATION(UtilTest);

void UtilTest::testJoinPaths() {
  // Same behaviour as os.path.join()
  OLA_ASSERT_EQ(string("/tmp/1"), JoinPaths("/tmp", "1"));
  OLA_ASSERT_EQ(string("/tmp/1"), JoinPaths("/tmp/", "1"));
  OLA_ASSERT_EQ(string("1"), JoinPaths("", "1"));
  OLA_ASSERT_EQ(string("/tmp/"), JoinPaths("/tmp/", ""));
  OLA_ASSERT_EQ(string("/tmp"), JoinPaths("/tmp", ""));
  OLA_ASSERT_EQ(string("/foo"), JoinPaths("/tmp", "/foo"));
  OLA_ASSERT_EQ(string(""), JoinPaths("", ""));
}

/*
 * Test the FilenameFromPath function
 */
void UtilTest::testFilenameFromPath() {
  // TODO(Peter): Make these tests work on Windows too
  OLA_ASSERT_EQ(string(""), FilenameFromPath(""));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/"));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPath("/foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/foo/"));
  OLA_ASSERT_EQ(string("bar"), FilenameFromPath("/foo/bar"));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/foo/bar/"));
  OLA_ASSERT_EQ(string("baz"), FilenameFromPath("/foo/bar/baz"));

  OLA_ASSERT_EQ(string("bak"), FilenameFromPathOrDefault("", "bak"));
  OLA_ASSERT_EQ(string("bak"), FilenameFromPathOrDefault("foo", "bak"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrDefault("/", "bak"));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPathOrDefault("/foo", "bak"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrDefault("/foo/", "bak"));
  OLA_ASSERT_EQ(string("bar"), FilenameFromPathOrDefault("/foo/bar", "bak"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrDefault("/foo/bar/", "bak"));
  OLA_ASSERT_EQ(string("baz"),
                FilenameFromPathOrDefault("/foo/bar/baz", "bak"));

  OLA_ASSERT_EQ(string(""), FilenameFromPathOrPath(""));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPathOrPath("foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrPath("/"));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPathOrPath("/foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrPath("/foo/"));
  OLA_ASSERT_EQ(string("bar"), FilenameFromPathOrPath("/foo/bar"));
  OLA_ASSERT_EQ(string(""), FilenameFromPathOrPath("/foo/bar/"));
  OLA_ASSERT_EQ(string("baz"), FilenameFromPathOrPath("/foo/bar/baz"));
}

