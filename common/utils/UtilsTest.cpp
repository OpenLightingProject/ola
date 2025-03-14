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

#include "ola/util/SequenceNumber.h"
#include "ola/util/Utils.h"
#include "ola/testing/TestUtils.h"

using ola::SequenceNumber;
using ola::utils::SplitUInt16;
using ola::utils::JoinUInt8;
using ola::utils::TruncateUInt16High;
using ola::utils::TruncateUInt16Low;

class UtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UtilsTest);
  CPPUNIT_TEST(testSequenceNumber);
  CPPUNIT_TEST(testSplitUInt16);
  CPPUNIT_TEST(testJoinUInt8);
  CPPUNIT_TEST(testTruncateUInt16High);
  CPPUNIT_TEST(testTruncateUInt16Low);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testSequenceNumber();
    void testSplitUInt16();
    void testJoinUInt8();
    void testTruncateUInt16High();
    void testTruncateUInt16Low();
};


CPPUNIT_TEST_SUITE_REGISTRATION(UtilsTest);

/*
 * Test the SequenceNumber class
 */
void UtilsTest::testSequenceNumber() {
  // Test a basic uint8 sequence
  SequenceNumber<uint8_t> uint8_sequence;
  OLA_ASSERT_EQ(uint8_sequence.Next(), static_cast<uint8_t>(0));
  OLA_ASSERT_EQ(uint8_sequence.Next(), static_cast<uint8_t>(1));
  OLA_ASSERT_EQ(uint8_sequence.Next(), static_cast<uint8_t>(2));

  // Test a basic uint16 sequence
  SequenceNumber<uint16_t> uint16_sequence;
  OLA_ASSERT_EQ(uint16_sequence.Next(), static_cast<uint16_t>(0));
  OLA_ASSERT_EQ(uint16_sequence.Next(), static_cast<uint16_t>(1));
  OLA_ASSERT_EQ(uint16_sequence.Next(), static_cast<uint16_t>(2));

  // Test a basic uint32 sequence
  SequenceNumber<uint32_t> uint32_sequence;
  OLA_ASSERT_EQ(uint32_sequence.Next(), static_cast<uint32_t>(0));
  OLA_ASSERT_EQ(uint32_sequence.Next(), static_cast<uint32_t>(1));
  OLA_ASSERT_EQ(uint32_sequence.Next(), static_cast<uint32_t>(2));

  // Test a uint8 sequence starting at an offset
  SequenceNumber<uint8_t> uint8_offset_sequence(20);
  OLA_ASSERT_EQ(uint8_offset_sequence.Next(), static_cast<uint8_t>(20));
  OLA_ASSERT_EQ(uint8_offset_sequence.Next(), static_cast<uint8_t>(21));
  OLA_ASSERT_EQ(uint8_offset_sequence.Next(), static_cast<uint8_t>(22));

  // Test a uint8 sequence wraps around
  SequenceNumber<uint8_t> uint8_wrapping_sequence(254);
  OLA_ASSERT_EQ(uint8_wrapping_sequence.Next(), static_cast<uint8_t>(254));
  OLA_ASSERT_EQ(uint8_wrapping_sequence.Next(), static_cast<uint8_t>(255));
  OLA_ASSERT_EQ(uint8_wrapping_sequence.Next(), static_cast<uint8_t>(0));
  OLA_ASSERT_EQ(uint8_wrapping_sequence.Next(), static_cast<uint8_t>(1));
}

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

/*
 * Test the TruncateUInt16High function
 */
void UtilsTest::testTruncateUInt16High() {
  uint16_t input = 0xabcd;
  OLA_ASSERT_EQ(TruncateUInt16High(input), static_cast<uint8_t>(0xab));

  input = 0x0000;
  OLA_ASSERT_EQ(TruncateUInt16High(input), static_cast<uint8_t>(0x00));

  input = 0xffff;
  OLA_ASSERT_EQ(TruncateUInt16High(input), static_cast<uint8_t>(0xff));

  input = 0x0001;
  OLA_ASSERT_EQ(TruncateUInt16High(input), static_cast<uint8_t>(0x00));
}

/*
 * Test the TruncateUInt16Low function
 */
void UtilsTest::testTruncateUInt16Low() {
  uint16_t input = 0xabcd;
  OLA_ASSERT_EQ(TruncateUInt16Low(input), static_cast<uint8_t>(0xcd));

  input = 0x0000;
  OLA_ASSERT_EQ(TruncateUInt16Low(input), static_cast<uint8_t>(0x00));

  input = 0xffff;
  OLA_ASSERT_EQ(TruncateUInt16Low(input), static_cast<uint8_t>(0xff));

  input = 0x0001;
  OLA_ASSERT_EQ(TruncateUInt16Low(input), static_cast<uint8_t>(0x01));
}
