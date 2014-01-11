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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
using std::string;

class UtilTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UtilTest);
  CPPUNIT_TEST(testFilenameFromPath);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testFilenameFromPath();
};


CPPUNIT_TEST_SUITE_REGISTRATION(UtilTest);

/*
 * Test the FilenameFromPath function
 */
void UtilTest::testFilenameFromPath() {
  // TODO(Peter): Make these tests work on Windows too
  OLA_ASSERT_EQ(string(""), FilenameFromPath(""));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/"));
  OLA_ASSERT_EQ(string("foo"), FilenameFromPath("/foo"));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/foo/"));
  OLA_ASSERT_EQ(string("bar"), FilenameFromPath("/foo/bar"));
  OLA_ASSERT_EQ(string(""), FilenameFromPath("/foo/bar/"));
  OLA_ASSERT_EQ(string("baz"), FilenameFromPath("/foo/bar/baz"));
}

