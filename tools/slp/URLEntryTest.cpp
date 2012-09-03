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
 * URLEntryTest.cpp
 * Test fixture for the URLEntry class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/URLEntry.h"

using ola::io::IOQueue;
using ola::slp::URLEntry;
using ola::testing::ASSERT_DATA_EQUALS;


class URLEntryTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(URLEntryTest);
  CPPUNIT_TEST(testWrite);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testWrite();
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(URLEntryTest);


/*
 * Check that Write() works.
 */
void URLEntryTest::testWrite() {
  const unsigned int expected_size = 19;
  IOQueue output;
  URLEntry url_entry(1234, "service://foo");
  url_entry.Write(&output);

  CPPUNIT_ASSERT_EQUAL(expected_size, output.Size());

  uint8_t data[20];
  CPPUNIT_ASSERT_EQUAL(expected_size, output.Peek(data, expected_size));
  output.Pop(expected_size);

  const uint8_t expected_data[] = {
    0, 0x04, 0xd2, 0, 0x0d,
    's', 'e', 'r', 'v', 'i', 'c', 'e', ':', '/', '/', 'f', 'o', 'o',
    0
  };
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data), data,
                     expected_size);

  // now try a 0 length url
  URLEntry url_entry2(1234, "");
  url_entry2.Write(&output);
  CPPUNIT_ASSERT_EQUAL(6u, output.Size());
  CPPUNIT_ASSERT_EQUAL(6u, output.Peek(data, 6u));
  output.Pop(6u);

  const uint8_t expected_data2[] = {0, 0x04, 0xd2, 0, 0, 0};
  ASSERT_DATA_EQUALS(__LINE__, expected_data2, sizeof(expected_data2),
                     data, 6u);
}
