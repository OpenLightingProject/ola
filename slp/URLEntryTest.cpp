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
 * URLEntryTest.cpp
 * Test fixture for the URLEntry classes
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <sstream>

#include "ola/Logging.h"
#include "ola/io/BigEndianStream.h"
#include "ola/io/IOQueue.h"
#include "ola/testing/TestUtils.h"
#include "ola/slp/URLEntry.h"

using ola::io::BigEndianOutputStream;
using ola::io::IOQueue;
using ola::slp::URLEntry;
using ola::testing::ASSERT_DATA_EQUALS;
using std::string;


class URLEntryTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(URLEntryTest);
  CPPUNIT_TEST(testURLEntry);
  CPPUNIT_TEST(testURLEntryWrite);
  CPPUNIT_TEST(testToString);
  CPPUNIT_TEST(testAging);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testURLEntry();
    void testURLEntryWrite();
    void testToString();
    void testAging();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

 private:
    IOQueue output;
};


CPPUNIT_TEST_SUITE_REGISTRATION(URLEntryTest);


/*
 * Check construction, getters and comparisons.
 */
void URLEntryTest::testURLEntry() {
  URLEntry url1("service:foo://192.168.1.1", 300);
  OLA_ASSERT_EQ(string("service:foo://192.168.1.1"), url1.url());
  OLA_ASSERT_EQ((uint16_t) 300, url1.lifetime());

  URLEntry url2("service:foo://192.168.1.1", 300);
  OLA_ASSERT_EQ(url1, url2);

  URLEntry url3(url1);
  OLA_ASSERT_EQ(url1, url3);

  URLEntry url4;
  OLA_ASSERT_NE(url1, url4);
  url4 = url1;
  OLA_ASSERT_EQ(url1, url4);

  // test case sensitivity
  URLEntry url5("SERVICE:fOo://192.168.1.1", 300);
  OLA_ASSERT_NE(url1, url5);

  // check the size of this entry
  OLA_ASSERT_EQ(31u, url1.PackedSize());

  // setters
  url1.set_lifetime(10);
  OLA_ASSERT_EQ((uint16_t) 10, url1.lifetime());
}


/*
 * Check that Write() works.
 */
void URLEntryTest::testURLEntryWrite() {
  const unsigned int expected_size = 31;
  BigEndianOutputStream stream(&output);

  URLEntry url_entry("service:foo://192.168.1.1", 1234);
  OLA_ASSERT_EQ(expected_size, url_entry.PackedSize());
  url_entry.Write(&stream);

  uint8_t data[expected_size + 10];
  OLA_ASSERT_EQ(expected_size, output.Peek(data, expected_size));
  output.Pop(expected_size);

  const uint8_t expected_data[] = {
    0, 0x04, 0xd2, 0, 0x19,
    's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
    '1', '9', '2', '.', '1', '6', '8', '.', '1', '.', '1',
    0
  };
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data), data,
                     expected_size);

  // now try a 0 length url
  URLEntry url_entry2("", 1234);
  url_entry2.Write(&stream);
  OLA_ASSERT_EQ(6u, output.Size());
  OLA_ASSERT_EQ(6u, output.Peek(data, 6u));
  output.Pop(6u);

  const uint8_t expected_data2[] = {0, 0x04, 0xd2, 0, 0, 0};
  ASSERT_DATA_EQUALS(__LINE__, expected_data2, sizeof(expected_data2),
                     data, 6u);
}


/**
 * Check converting a URLEntry & URLEntry to a string works
 */
void URLEntryTest::testToString() {
  URLEntry url1("service:foo://192.168.1.1", 300);
  OLA_ASSERT_EQ(string("service:foo://192.168.1.1(300)"), url1.ToString());
}


/**
 * Check that aging works.
 */
void URLEntryTest::testAging() {
  URLEntry url1("service:foo://192.168.1.1", 10);
  OLA_ASSERT_FALSE(url1.AgeLifetime(5));
  OLA_ASSERT_EQ((uint16_t) 5, url1.lifetime());

  OLA_ASSERT_TRUE(url1.AgeLifetime(5));
  OLA_ASSERT_EQ((uint16_t) 0, url1.lifetime());
}
