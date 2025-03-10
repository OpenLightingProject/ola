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
 * ResponderHelperTest.cpp
 * Test fixture for the ResponderHelper class
 * Copyright (C) 2025 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <memory>
#include <string>

#include "common/rdm/TestHelper.h"
#include "ola/base/Array.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"

using ola::io::ByteString;
using ola::rdm::RDMGetRequest;
using ola::rdm::ResponderHelper;
using ola::rdm::UID;
using std::string;

class ResponderHelperTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ResponderHelperTest);
  CPPUNIT_TEST(testExtractString);
  CPPUNIT_TEST_SUITE_END();

 public:
  ResponderHelperTest()
    : m_source(1, 2),
      m_destination(3, 4) {
  }

  void setUp();

  void testExtractString();

 private:
  UID m_source;
  UID m_destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ResponderHelperTest);


void ResponderHelperTest::setUp() {
}


void ResponderHelperTest::testExtractString() {
  // No data
  RDMGetRequest request1(m_source,
                         m_destination,
                         0,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         NULL,  // data
                         0);  // data length

  string str1;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request1, &str1));
  OLA_ASSERT_TRUE(str1.empty());

  // Normal length
  uint8_t data2[3] = {'f', 'o', 'o'};
  RDMGetRequest request2(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data2,  // data
                         arraysize(data2));  // data length

  string str2;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request2, &str2));
  OLA_ASSERT_EQ((size_t) 3, str2.length());
  OLA_ASSERT_EQ(string("foo"), str2);

  // Normal length with null termination
  uint8_t data3[4] = {'f', 'o', 'o', 0};
  RDMGetRequest request3(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data3,  // data
                         arraysize(data3));  // data length

  string str3;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request3, &str3));
  OLA_ASSERT_EQ((size_t) 3, str3.length());  // Length doesn't include null
  OLA_ASSERT_EQ(string("foo"), str3);

  // Max length, no null
  uint8_t data4[32] = {'t', 'h', 'i', 's', ' ', 'i', 's', ' ', 'a', ' ',
                      's', 't', 'r', 'i', 'n', 'g', ' ', 'w', 'i', 't', 'h',
                      ' ', '3', '2', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't'};
  RDMGetRequest request4(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data4,  // data
                         arraysize(data4));  // data length

  string str4;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request4, &str4));
  OLA_ASSERT_EQ((size_t) 32, str4.length());
  OLA_ASSERT_EQ(string("this is a string with 32 charact"), str4);

  // Max data as nulls
  uint8_t data5[32];
  memset(data5, 0, arraysize(data5));
  RDMGetRequest request5(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data5,  // data
                         arraysize(data5));  // data length

  string str5;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request5, &str5));
  OLA_ASSERT_EQ((size_t) 0, str5.length());
  OLA_ASSERT_TRUE(str5.empty());

  // More than max data as nulls
  uint8_t data6[33];
  memset(data6, 0, arraysize(data6));
  RDMGetRequest request6(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data6,  // data
                         arraysize(data6));  // data length

  string str6;
  OLA_ASSERT_FALSE(ResponderHelper::ExtractString(&request6, &str6));
  OLA_ASSERT_TRUE(str6.empty());
}
