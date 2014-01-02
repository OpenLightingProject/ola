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
 * MathUtilsTest.cpp
 * Unittest for math functions.
 * Copyright (C) 2013 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/math/MathUtils.h"
#include "ola/testing/TestUtils.h"

using ola::math::UInt16ToTwoUInt8;
using ola::math::TwoUInt8ToUInt16;

class MathUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(MathUtilsTest);
  CPPUNIT_TEST(testUInt16ToTwoUInt8);
  CPPUNIT_TEST(testTwoUInt8ToUInt16);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testUInt16ToTwoUInt8();
    void testTwoUInt8ToUInt16();
};


CPPUNIT_TEST_SUITE_REGISTRATION(MathUtilsTest);

/*
 * Test the UInt16ToTwoUInt8 function
 */
void MathUtilsTest::testUInt16ToTwoUInt8() {
  uint16_t input = 0xabcd;
  uint8_t high = 0x00;
  uint8_t low = 0x00;
  UInt16ToTwoUInt8(input, &high, &low);
  OLA_ASSERT_EQ(high, (uint8_t)0xab);
  OLA_ASSERT_EQ(low, (uint8_t)0xcd);

  input = 0x0000;
  UInt16ToTwoUInt8(input, &high, &low);
  OLA_ASSERT_EQ(high, (uint8_t)0x00);
  OLA_ASSERT_EQ(low, (uint8_t)0x00);

  input = 0xffff;
  UInt16ToTwoUInt8(input, &high, &low);
  OLA_ASSERT_EQ(high, (uint8_t)0xff);
  OLA_ASSERT_EQ(low, (uint8_t)0xff);

  input = 0x0001;
  UInt16ToTwoUInt8(input, &high, &low);
  OLA_ASSERT_EQ(high, (uint8_t)0x00);
  OLA_ASSERT_EQ(low, (uint8_t)0x01);
}

/*
 * Test the TwoUInt8ToUInt16 function
 */
void MathUtilsTest::testTwoUInt8ToUInt16() {
  uint8_t high = 0xab;
  uint8_t low = 0xcd;
  OLA_ASSERT_EQ(TwoUInt8ToUInt16(high, low), (uint16_t)0xabcd);

  high = 0x00;
  low = 0x00;
  OLA_ASSERT_EQ(TwoUInt8ToUInt16(high, low), (uint16_t)0x0000);

  high = 0xff;
  low = 0xff;
  OLA_ASSERT_EQ(TwoUInt8ToUInt16(high, low), (uint16_t)0xffff);

  high = 0x00;
  low = 0x01;
  OLA_ASSERT_EQ(TwoUInt8ToUInt16(high, low), (uint16_t)0x0001);
}
