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
 * Unittest for util functions.
 * Copyright (C) 2013 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/util/Utils.h"
#include "ola/testing/TestUtils.h"

using ola::utils::SplitUInt16;
using ola::utils::JoinUInt8;

class UtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UtilsTest);
  CPPUNIT_TEST(testSplitUInt16);
  CPPUNIT_TEST(testJoinUInt8);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testSplitUInt16();
    void testJoinUInt8();
};


CPPUNIT_TEST_SUITE_REGISTRATION(UtilsTest);

/*
 * Test the SplitUInt16 function
 */
void UtilsTest::testSplitUInt16() {
  uint16_t input = 0xabcd;
  uint8_t high = 0x00;
  uint8_t low = 0x00;
  SplitUInt16(input, &high, &low);
  OLA_ASSERT_EQ(high, static_cast<uint8_t>(0xab));
  OLA_ASSERT_EQ(low, static_cast<uint8_t>(0xcd));

  input = 0x0000;
  SplitUInt16(input, &high, &low);
  OLA_ASSERT_EQ(high, static_cast<uint8_t>(0x00));
  OLA_ASSERT_EQ(low, static_cast<uint8_t>(0x00));

  input = 0xffff;
  SplitUInt16(input, &high, &low);
  OLA_ASSERT_EQ(high, static_cast<uint8_t>(0xff));
  OLA_ASSERT_EQ(low, static_cast<uint8_t>(0xff));

  input = 0x0001;
  SplitUInt16(input, &high, &low);
  OLA_ASSERT_EQ(high, static_cast<uint8_t>(0x00));
  OLA_ASSERT_EQ(low, static_cast<uint8_t>(0x01));
}

/*
 * Test the JoinUInt8 function
 */
void UtilsTest::testJoinUInt8() {
  uint8_t high = 0xab;
  uint8_t low = 0xcd;
  OLA_ASSERT_EQ(JoinUInt8(high, low), static_cast<uint16_t>(0xabcd));

  high = 0x00;
  low = 0x00;
  OLA_ASSERT_EQ(JoinUInt8(high, low), static_cast<uint16_t>(0x0000));

  high = 0xff;
  low = 0xff;
  OLA_ASSERT_EQ(JoinUInt8(high, low), static_cast<uint16_t>(0xffff));

  high = 0x00;
  low = 0x01;
  OLA_ASSERT_EQ(JoinUInt8(high, low), static_cast<uint16_t>(0x0001));
}
