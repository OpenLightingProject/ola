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
 * ServiceEntryTest.cpp
 * Test fixture for the URLEntry & ServiceEntry classes
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>

#include "ola/Logging.h"
#include "ola/io/BigEndianStream.h"
#include "ola/io/IOQueue.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/ServiceEntry.h"

using ola::io::BigEndianOutputStream;
using ola::io::IOQueue;
using ola::slp::ServiceEntry;
using ola::slp::URLEntry;
using ola::testing::ASSERT_DATA_EQUALS;
using std::set;
using std::string;


class ServiceEntryTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ServiceEntryTest);
  CPPUNIT_TEST(testURLEntry);
  CPPUNIT_TEST(testURLEntryWrite);
  CPPUNIT_TEST(testServiceEntry);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testURLEntry();
    void testURLEntryWrite();
    void testServiceEntry();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

  private:
    IOQueue output;
};


CPPUNIT_TEST_SUITE_REGISTRATION(ServiceEntryTest);


/*
 * Check construction, getters and comparisons.
 */
void ServiceEntryTest::testURLEntry() {
  URLEntry url1("service:foo://192.168.1.1", 300);
  OLA_ASSERT_EQ((uint16_t) 300, url1.Lifetime());
  OLA_ASSERT_EQ(string("service:foo://192.168.1.1"), url1.URL());

  URLEntry url2("service:foo://192.168.1.1", 300);
  OLA_ASSERT_EQ(url1, url2);

  URLEntry url3(url1);
  OLA_ASSERT_EQ(url1, url3);

  URLEntry url4;
  OLA_ASSERT_NE(url1, url4);
  url4 = url1;
  OLA_ASSERT_EQ(url1, url4);

  // check the size of this entry
  OLA_ASSERT_EQ(31u, url1.Size());
}


/*
 * Check that Write() works.
 */
void ServiceEntryTest::testURLEntryWrite() {
  const unsigned int expected_size = 31;
  BigEndianOutputStream stream(&output);

  URLEntry url_entry("service:foo://192.168.1.1", 1234);
  url_entry.Write(&stream);

  CPPUNIT_ASSERT_EQUAL(expected_size, output.Size());

  uint8_t data[40];
  CPPUNIT_ASSERT_EQUAL(expected_size, output.Peek(data, expected_size));
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
  CPPUNIT_ASSERT_EQUAL(6u, output.Size());
  CPPUNIT_ASSERT_EQUAL(6u, output.Peek(data, 6u));
  output.Pop(6u);

  const uint8_t expected_data2[] = {0, 0x04, 0xd2, 0, 0, 0};
  ASSERT_DATA_EQUALS(__LINE__, expected_data2, sizeof(expected_data2),
                     data, 6u);
}


/*
 * Check ServiceEntry.
 */
void ServiceEntryTest::testServiceEntry() {
  set<string> scopes, scopes2;
  scopes.insert("one");
  ServiceEntry service1(scopes, "service:foo://192.168.1.1", 1234);

  OLA_ASSERT_EQ(string("service:foo://192.168.1.1"), service1.URL());
  OLA_ASSERT_EQ((uint16_t) 1234, service1.Lifetime());
  OLA_ASSERT_SET_EQ(scopes, service1.Scopes());

  // check the setter works
  service1.SetLifetime(300);
  OLA_ASSERT_EQ((uint16_t) 300, service1.Lifetime());

  // check MatchesScopes
  OLA_ASSERT_TRUE(service1.MatchesScopes(scopes));

  // check IntersectsScopes
  OLA_ASSERT_TRUE(service1.IntersectsScopes(scopes));
  scopes2.insert("two");
  OLA_ASSERT_FALSE(service1.IntersectsScopes(scopes2));
  scopes2.insert("one");
  OLA_ASSERT_TRUE(service1.IntersectsScopes(scopes2));

  // test copy constructor
  ServiceEntry service2(service1);
  OLA_ASSERT_EQ((uint16_t) 300, service2.Lifetime());
  OLA_ASSERT_EQ(string("service:foo://192.168.1.1"), service2.URL());
  OLA_ASSERT_SET_EQ(scopes, service2.Scopes());

  // test assignment
  ServiceEntry service3;
  service3 = service1;
  OLA_ASSERT_EQ((uint16_t) 300, service3.Lifetime());
  OLA_ASSERT_EQ(string("service:foo://192.168.1.1"), service3.URL());
  OLA_ASSERT_SET_EQ(scopes, service3.Scopes());
}
