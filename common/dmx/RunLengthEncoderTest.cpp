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
 * RunLengthEncoderTest.cpp
 * Test fixture for the RunLengthEncoder class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <stdlib.h>

#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/dmx/RunLengthEncoder.h"
#include "ola/testing/TestUtils.h"


using ola::dmx::RunLengthEncoder;
using ola::DmxBuffer;

class RunLengthEncoderTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RunLengthEncoderTest);
  CPPUNIT_TEST(testEncode);
  CPPUNIT_TEST(testEncode2);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testEncode();
    void testEncode2();
    void testEncodeDecode();
    void setUp();
    void tearDown();
 private:
    RunLengthEncoder m_encoder;
    uint8_t *m_dst;

    void checkEncode(const DmxBuffer &buffer,
                     unsigned int dst_size,
                     bool is_incomplete,
                     const uint8_t *expected_data,
                     unsigned int expected_length);
    void checkEncodeDecode(const uint8_t *data, unsigned int data_size);
};


CPPUNIT_TEST_SUITE_REGISTRATION(RunLengthEncoderTest);


/*
 * allocate a scratch pad
 */
void RunLengthEncoderTest::setUp() {
  m_dst = new uint8_t[ola::DMX_UNIVERSE_SIZE];
}


/*
 * clean up
 */
void RunLengthEncoderTest::tearDown() {
  delete[] m_dst;
}


/*
 * Check that for a given buffer and dst buffer size, we get the expected data
 */
void RunLengthEncoderTest::checkEncode(const DmxBuffer &buffer,
                                       unsigned int dst_size,
                                       bool is_complete,
                                       const uint8_t *expected_data,
                                       unsigned int expected_length) {
  memset(m_dst, 0, ola::DMX_UNIVERSE_SIZE);
  OLA_ASSERT_EQ(is_complete,
                m_encoder.Encode(buffer, m_dst, &dst_size));
  OLA_ASSERT_EQ(expected_length, dst_size);
  OLA_ASSERT_EQ(0, memcmp(expected_data, m_dst, dst_size));
}


/*
 * Check that encoding works.
 */
void RunLengthEncoderTest::testEncode() {
  const uint8_t TEST_DATA[] = {1, 2, 2, 3, 0, 0, 0, 1, 3, 3, 3, 1, 2};
  const uint8_t EXPECTED_DATA[] = {4, 1, 2, 2, 3, 0x83, 0, 1, 1, 0x83, 3, 2,
                                   1, 2};
  const uint8_t EXPECTED_DATA2[] = {3, 1, 2, 2};
  const uint8_t EXPECTED_DATA3[] = {4, 1, 2, 2, 3, 0x83, 0, 1, 1, 0x83, 3, 1,
                                    1};
  DmxBuffer buffer(TEST_DATA, sizeof(TEST_DATA));

  checkEncode(buffer, ola::DMX_UNIVERSE_SIZE, true, EXPECTED_DATA,
              sizeof(EXPECTED_DATA));

  checkEncode(buffer, 4, false, EXPECTED_DATA2, sizeof(EXPECTED_DATA2));
  checkEncode(buffer, 5, false, EXPECTED_DATA, 5);
  checkEncode(buffer, 6, false, EXPECTED_DATA, 5);
  checkEncode(buffer, 7, false, EXPECTED_DATA, 7);
  checkEncode(buffer, 8, false, EXPECTED_DATA, 7);
  checkEncode(buffer, 9, false, EXPECTED_DATA, 9);
  checkEncode(buffer, 10, false, EXPECTED_DATA, 9);
  checkEncode(buffer, 11, false, EXPECTED_DATA, 11);
  checkEncode(buffer, 12, false, EXPECTED_DATA, 11);
  checkEncode(buffer, 13, false, EXPECTED_DATA3, sizeof(EXPECTED_DATA3));
}


/*
 * Check that encoding works.
 */
void RunLengthEncoderTest::testEncode2() {
  const uint8_t TEST_DATA[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  const uint8_t EXPECTED_DATA[] = {0x8A, 0};
  DmxBuffer buffer(TEST_DATA, sizeof(TEST_DATA));

  checkEncode(buffer, ola::DMX_UNIVERSE_SIZE, true, EXPECTED_DATA,
              sizeof(EXPECTED_DATA));

  checkEncode(buffer, 2, true, EXPECTED_DATA, sizeof(EXPECTED_DATA));
  checkEncode(buffer, 1, false, EXPECTED_DATA, 0);
  checkEncode(buffer, 0, false, EXPECTED_DATA, 0);
}


/*
 * Call Encode then Decode and check the results
 */
void RunLengthEncoderTest::checkEncodeDecode(const uint8_t *data,
                                             unsigned int data_size) {
  DmxBuffer src(data, data_size);
  DmxBuffer dst;

  unsigned int dst_size = ola::DMX_UNIVERSE_SIZE;
  memset(m_dst, 0, dst_size);
  OLA_ASSERT_TRUE(m_encoder.Encode(src, m_dst, &dst_size));

  OLA_ASSERT_TRUE(m_encoder.Decode(0, m_dst, dst_size, &dst));
  OLA_ASSERT_TRUE(src == dst);
  OLA_ASSERT_EQ(dst.Size(), data_size);
  OLA_ASSERT_NE(0, memcmp(data, dst.GetRaw(), dst.Size()));
}


/*
 * Check that an Encode/Decode pair works
 */
void RunLengthEncoderTest::testEncodeDecode() {
  const uint8_t TEST_DATA[] = {1, 2, 2, 3, 0, 0, 0, 1, 3, 3, 3, 1, 2};
  const uint8_t TEST_DATA2[] = {0, 0, 0, 0, 6, 5, 4, 3, 3, 3};
  const uint8_t TEST_DATA3[] = {0, 0, 0};

  checkEncodeDecode(TEST_DATA, sizeof(TEST_DATA));
  checkEncodeDecode(TEST_DATA2, sizeof(TEST_DATA2));
  checkEncodeDecode(TEST_DATA3, sizeof(TEST_DATA3));
}
