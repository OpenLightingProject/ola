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
 * RunLengthEncoderTest.cpp
 * Test fixture for the RunLengthEncoder class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string.h>
#include <stdlib.h>
#include <cppunit/extensions/HelperMacros.h>

#include <ola/BaseTypes.h>
#include <ola/DmxBuffer.h>
#include <ola/RunLengthEncoder.h>

using namespace ola;

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
    ola::RunLengthEncoder m_encoder;
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
  m_dst = (uint8_t*) malloc(DMX_UNIVERSE_SIZE);
}


/*
 * clean up
 */
void RunLengthEncoderTest::tearDown() {
  free(m_dst);
}


/*
 * Check that for a given buffer and dst buffer size, we get the expected data
 */
void RunLengthEncoderTest::checkEncode(const DmxBuffer &buffer,
                                       unsigned int dst_size,
                                       bool is_complete,
                                       const uint8_t *expected_data,
                                       unsigned int expected_length) {
  memset(m_dst, 0, DMX_UNIVERSE_SIZE);
  CPPUNIT_ASSERT_EQUAL(is_complete,
                       m_encoder.Encode(buffer, m_dst, dst_size));
  CPPUNIT_ASSERT_EQUAL(expected_length, dst_size);
  CPPUNIT_ASSERT(!memcmp(expected_data, m_dst, dst_size));
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

  checkEncode(buffer, DMX_UNIVERSE_SIZE, true, EXPECTED_DATA,
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

  checkEncode(buffer, DMX_UNIVERSE_SIZE, true, EXPECTED_DATA,
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

  unsigned int dst_size = DMX_UNIVERSE_SIZE;
  memset(m_dst, 0, dst_size);
  CPPUNIT_ASSERT(m_encoder.Encode(src, m_dst, dst_size));

  CPPUNIT_ASSERT(m_encoder.Decode(&dst, 0, m_dst, dst_size));
  CPPUNIT_ASSERT(src == dst);
  CPPUNIT_ASSERT_EQUAL(dst.Size(), data_size);
  CPPUNIT_ASSERT(!memcmp(data, dst.GetRaw(), dst.Size()));
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

