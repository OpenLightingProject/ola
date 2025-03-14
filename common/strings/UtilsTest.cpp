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
 * UtilsTest.cpp
 * Unittest for strings Util functions.
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/base/Array.h"
#include "ola/strings/Utils.h"
#include "ola/testing/TestUtils.h"

using ola::strings::CopyToFixedLengthBuffer;
using ola::strings::StrNLength;
using std::string;

class UtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UtilsTest);
  CPPUNIT_TEST(testCopyToFixedLengthBuffer);
  CPPUNIT_TEST(testStrNLength);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testCopyToFixedLengthBuffer();
    void testStrNLength();
};


CPPUNIT_TEST_SUITE_REGISTRATION(UtilsTest);

/*
 * Test the CopyToFixedLengthBuffer function
 */
void UtilsTest::testCopyToFixedLengthBuffer() {
  char buffer[6];
  const string short_input("foo");
  const string input("foobar");
  const string oversized_input("foobarbaz");

  const char short_output[] = {'f', 'o', 'o', 0, 0, 0};
  CopyToFixedLengthBuffer(short_input, buffer, arraysize(buffer));
  OLA_ASSERT_DATA_EQUALS(short_output, arraysize(short_output),
                         buffer, arraysize(buffer));

  const char output[] = {'f', 'o', 'o', 'b', 'a', 'r'};
  CopyToFixedLengthBuffer(input, buffer, arraysize(buffer));
  OLA_ASSERT_DATA_EQUALS(output, arraysize(output),
                         buffer, arraysize(buffer));

  const char oversized_output[] = {'f', 'o', 'o', 'b', 'a', 'r'};
  CopyToFixedLengthBuffer(oversized_input, buffer, arraysize(buffer));
  OLA_ASSERT_DATA_EQUALS(oversized_output, arraysize(oversized_output),
                         buffer, arraysize(buffer));
}

/*
 * Test the StrNLength function
 */
void UtilsTest::testStrNLength() {
  const string short_input("foo");
  const string input("foobar");
  const string oversized_input("foobarbaz");
  const string terminated_input("foo\0bar");

  OLA_ASSERT_EQ((size_t) 3, StrNLength(short_input.c_str(), 6));
  OLA_ASSERT_EQ((size_t) 6, StrNLength(input.c_str(), 6));
  OLA_ASSERT_EQ((size_t) 6, StrNLength(oversized_input.c_str(), 6));
  OLA_ASSERT_EQ((size_t) 3, StrNLength(terminated_input.c_str(), 6));

  OLA_ASSERT_EQ((size_t) 3, StrNLength(short_input, 6));
  OLA_ASSERT_EQ((size_t) 6, StrNLength(input, 6));
  OLA_ASSERT_EQ((size_t) 6, StrNLength(oversized_input, 6));
  OLA_ASSERT_EQ((size_t) 3, StrNLength(terminated_input, 6));

  const char short_input_array[] = {'f', 'o', 'o', 0, 0, 0};
  OLA_ASSERT_EQ((size_t) 3, StrNLength(short_input_array, 6));

  const char oversized_input_array[] = {'f', 'o', 'o', 'b', 'a', 'r',
                                        'b', 'a', 'z'};
  OLA_ASSERT_EQ((size_t) 6, StrNLength(oversized_input_array, 6));
}
