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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * BaseInflatorTest.cpp
 * Test fixture for the BaseInflator class
 * Copyright (C) 2005 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libs/acn/BaseInflator.h"
#include "libs/acn/HeaderSet.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {

uint8_t PDU_DATA[] = "this is some test data";

class BaseInflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BaseInflatorTest);
  CPPUNIT_TEST(testChildInflators);
  CPPUNIT_TEST(testDecodeLength);
  CPPUNIT_TEST(testDecodeVector);
  CPPUNIT_TEST(testInflatePDU);
  CPPUNIT_TEST(testInflatePDUBlock);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testChildInflators();
    void testDecodeLength();
    void testDecodeVector();
    void testInflatePDU();
    void testInflatePDUBlock();
 private:
};


class TestInflator: public ola::acn::BaseInflator {
 public:
    explicit TestInflator(unsigned int id = 0,
                          PDU::vector_size v_size = PDU::TWO_BYTES)
        : BaseInflator(v_size),
          m_id(id),
          m_blocks_handled(0) {}
    uint32_t Id() const { return m_id; }
    unsigned int BlocksHandled() const { return m_blocks_handled; }

 protected:
    void ResetHeaderField() {}
    bool DecodeHeader(HeaderSet*,
                     const uint8_t*,
                     unsigned int,
                     unsigned int *bytes_used) {
      *bytes_used = 0;
      return true;
    }

    bool HandlePDUData(uint32_t vector, OLA_UNUSED const HeaderSet &headers,
                       const uint8_t *data, unsigned int pdu_length) {
      OLA_ASSERT_EQ((uint32_t) 289, vector);
      OLA_ASSERT_EQ((unsigned int) sizeof(PDU_DATA), pdu_length);
      OLA_ASSERT_FALSE(memcmp(data, PDU_DATA, pdu_length));
      m_blocks_handled++;
      return true;
    }

 private:
    unsigned int m_id;
    unsigned int m_blocks_handled;
};

CPPUNIT_TEST_SUITE_REGISTRATION(BaseInflatorTest);


/*
 * Test that we can setup the child inflators correctly
 */
void BaseInflatorTest::testChildInflators() {
  TestInflator inflator;
  TestInflator inflator1(1);
  TestInflator inflator2(2);

  OLA_ASSERT_EQ((uint32_t) 1, inflator1.Id());
  OLA_ASSERT_EQ((uint32_t) 2, inflator2.Id());
  OLA_ASSERT(inflator.AddInflator(&inflator1));
  OLA_ASSERT(inflator.AddInflator(&inflator2));

  OLA_ASSERT(&inflator1 == inflator.GetInflator(inflator1.Id()));
  OLA_ASSERT(&inflator2 == inflator.GetInflator(inflator2.Id()));
  OLA_ASSERT(NULL == inflator.GetInflator(3));

  // Once an inflator is set it can't be changed.
  OLA_ASSERT_FALSE(inflator.AddInflator(&inflator1));
  OLA_ASSERT_FALSE(inflator.AddInflator(&inflator2));
}


/*
 * Test that DecodeLength works
 */
void BaseInflatorTest::testDecodeLength() {
  TestInflator inflator;
  uint8_t data[] = {0, 0, 0, 0};  // the test data
  unsigned int pdu_length;
  unsigned int bytes_used = 0;

  // with the length data set to 0, any length should fail.
  for (unsigned int i = 0; i <= sizeof(data); i++) {
    OLA_ASSERT_FALSE(inflator.DecodeLength(data, i, &pdu_length, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  // Set the length of the pdu to 1, note that as the length includes the
  // size of the length, vector & header fields, this is less than the number
  // of bytes required to determine the length and so it fails
  data[1] = 1;
  for (unsigned int i = 0; i <= sizeof(data); i++) {
    OLA_ASSERT_FALSE(inflator.DecodeLength(data, i, &pdu_length, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  // now set the length to 2, a data length of 0 or 1 should fail, but anything
  // more than that should return correctly.
  data[1] = 2;
  for (unsigned int i = 0; i <= 1; i++) {
    OLA_ASSERT_FALSE(inflator.DecodeLength(data, i, &pdu_length, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }
  for (unsigned int i = 2; i <= sizeof(data) ; i++) {
    OLA_ASSERT(inflator.DecodeLength(data, i, &pdu_length, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 2, pdu_length);
    OLA_ASSERT_EQ((unsigned int) 2, bytes_used);
  }

  // now check that both bytes are used
  data[0] = 1;  // total length of 258
  OLA_ASSERT(inflator.DecodeLength(data, sizeof(data), &pdu_length,
                                   &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 258, pdu_length);
  OLA_ASSERT_EQ((unsigned int) 2, bytes_used);

  // now check that the extend length format works
  data[0] = BaseInflator::LFLAG_MASK;

  // with the length data set to 0, any length should fail.
  data[1] = 0;
  for (unsigned int i = 0; i <= sizeof(data); i++) {
    OLA_ASSERT_FALSE(inflator.DecodeLength(data, i, &pdu_length, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  // Set the length of the pdu to 1, note that as the length includes the
  // size of the length, vector & header fields, this is less than the number
  // of bytes required to determine the length and so it fails
  data[2] = 1;
  for (unsigned int i = 0; i <= sizeof(data); i++) {
    OLA_ASSERT_FALSE(inflator.DecodeLength(data, i, &pdu_length, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  // now set the length to 3, a data length of 0, 1 or 2 should fail, but
  // anything more than that should return correctly.
  data[2] = 3;
  for (unsigned int i = 0; i <= 2; i++) {
    OLA_ASSERT_FALSE(inflator.DecodeLength(data, i, &pdu_length, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }
  for (unsigned int i = 3; i <= sizeof(data) ; i++) {
    OLA_ASSERT(inflator.DecodeLength(data, i, &pdu_length, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 3, pdu_length);
    OLA_ASSERT_EQ((unsigned int) 3, bytes_used);
  }

  // now check that all 3 bytes are used
  data[0] = BaseInflator::LFLAG_MASK + 1;
  data[1] = 0x01;
  OLA_ASSERT(inflator.DecodeLength(data, sizeof(data), &pdu_length,
                                   &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 65795, pdu_length);
  OLA_ASSERT_EQ((unsigned int) 3, bytes_used);
}


/*
 * test that DecodeVector works
 */
void BaseInflatorTest::testDecodeVector() {
  TestInflator inflator(0, PDU::ONE_BYTE);
  uint8_t data[] = {1, 2, 3, 4, 5, 6};  // the test data
  unsigned int vector = 1;
  unsigned int bytes_used = 0;
  uint8_t flags = PDU::VFLAG_MASK;

  OLA_ASSERT_FALSE(inflator.DecodeVector(flags, data, 0, &vector, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, vector);
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);

  data[0] = 42;
  for (unsigned int i = 1; i < sizeof(data); i++) {
    OLA_ASSERT(inflator.DecodeVector(flags, data, i, &vector, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 42, vector);
    OLA_ASSERT_EQ((unsigned int) 1, bytes_used);
  }

  // now make sure we can reuse the vector
  flags = 0;
  for (unsigned int i = 0; i < sizeof(data); i++) {
    OLA_ASSERT(inflator.DecodeVector(flags, data, i, &vector, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 42, vector);
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  // resetting doesn't allow us to reuse the vector
  inflator.ResetPDUFields();
  for (unsigned int i = 0; i < sizeof(data); i++) {
    OLA_ASSERT_FALSE(inflator.DecodeVector(flags, data, i, &vector,
                                           &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, vector);
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  // now try with a vector size of 2
  flags = PDU::VFLAG_MASK;
  TestInflator inflator2(0, PDU::TWO_BYTES);
  for (unsigned int i = 0; i < 2; i++) {
    OLA_ASSERT_FALSE(
        inflator2.DecodeVector(flags, data, i, &vector, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, vector);
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  data[0] = 0x80;
  data[1] = 0x21;
  for (unsigned int i = 2; i < sizeof(data); i++) {
    OLA_ASSERT(inflator2.DecodeVector(flags, data, i, &vector, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 32801, vector);
    OLA_ASSERT_EQ((unsigned int) 2, bytes_used);
  }

  // now make sure we can reuse the vector
  flags = 0;
  for (unsigned int i = 0; i < sizeof(data); i++) {
    OLA_ASSERT(inflator2.DecodeVector(flags, data, i, &vector, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 32801, vector);
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  // resetting doesn't allow us to reuse the vector
  inflator2.ResetPDUFields();
  for (unsigned int i = 0; i < sizeof(data); i++) {
    OLA_ASSERT_FALSE(
        inflator2.DecodeVector(flags, data, i, &vector, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, vector);
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  // now try with a vector size of 4
  flags = PDU::VFLAG_MASK;
  TestInflator inflator4(0, PDU::FOUR_BYTES);
  for (unsigned int i = 0; i < 4; i++) {
    OLA_ASSERT_FALSE(
        inflator4.DecodeVector(flags, data, i, &vector, &bytes_used));
    OLA_ASSERT_EQ((unsigned int) 0, vector);
    OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  }

  data[0] = 0x01;
  data[1] = 0x21;
  data[2] = 0x32;
  data[3] = 0x45;
  for (unsigned int i = 4; i < 8; i++) {
    OLA_ASSERT(inflator4.DecodeVector(flags, data, i, &vector, &bytes_used));
    OLA_ASSERT_EQ((uint32_t) 18952773, vector);
    OLA_ASSERT_EQ((unsigned int) 4, bytes_used);
  }
}


/*
 * Check that we can inflate a PDU
 */
void BaseInflatorTest::testInflatePDU() {
  TestInflator inflator;  // test with a vector size of 2
  HeaderSet header_set;
  uint8_t flags = PDU::VFLAG_MASK;
  unsigned int data_size = static_cast<unsigned int>(PDU::TWO_BYTES +
      sizeof(PDU_DATA));
  uint8_t *data = new uint8_t[data_size];
  // setup the vector
  data[0] = 0x01;
  data[1] = 0x21;
  memcpy(data + PDU::TWO_BYTES, PDU_DATA, sizeof(PDU_DATA));

  OLA_ASSERT(inflator.InflatePDU(&header_set, flags, data, data_size));
  delete[] data;
}


/*
 * Check that we can inflate a PDU block correctly.
 */
void BaseInflatorTest::testInflatePDUBlock() {
  TestInflator inflator;  // test with a vector size of 2
  HeaderSet header_set;
  const unsigned int length_size = 2;

  // inflate a single pdu block
  unsigned int data_size = static_cast<unsigned int>(
      length_size + PDU::TWO_BYTES + sizeof(PDU_DATA));
  uint8_t *data = new uint8_t[data_size];
  // setup the vector
  data[0] = PDU::VFLAG_MASK;
  data[1] = static_cast<uint8_t>(data_size);
  data[2] = 0x01;
  data[3] = 0x21;
  memcpy(data + length_size + PDU::TWO_BYTES, PDU_DATA,
         sizeof(PDU_DATA));
  OLA_ASSERT_EQ(data_size,
                inflator.InflatePDUBlock(&header_set, data, data_size));
  OLA_ASSERT_EQ(1u, inflator.BlocksHandled());
  delete[] data;

  // inflate a multi-pdu block
  data = new uint8_t[2 * data_size];
  data[0] = PDU::VFLAG_MASK;
  data[1] = static_cast<uint8_t>(data_size);
  data[2] = 0x01;
  data[3] = 0x21;
  memcpy(data + length_size + PDU::TWO_BYTES,
         PDU_DATA,
         sizeof(PDU_DATA));
  data[data_size] = PDU::VFLAG_MASK;
  data[data_size + 1] = static_cast<uint8_t>(data_size);
  data[data_size + 2] = 0x01;
  data[data_size + 3] = 0x21;
  memcpy(data + data_size + length_size + PDU::TWO_BYTES, PDU_DATA,
         sizeof(PDU_DATA));
  OLA_ASSERT_EQ(
      2 * data_size,
      inflator.InflatePDUBlock(&header_set, data, 2 * data_size));
  delete[] data;
  OLA_ASSERT_EQ(3u, inflator.BlocksHandled());

  // inflate with nested inflators
  TestInflator child_inflator(289);
  inflator.AddInflator(&child_inflator);
  unsigned int pdu_size = data_size + length_size + PDU::TWO_BYTES;
  data = new uint8_t[pdu_size];

  data[0] = PDU::VFLAG_MASK;
  data[1] = static_cast<uint8_t>(pdu_size);
  data[2] = 0x01;
  data[3] = 0x21;
  data[4] = PDU::VFLAG_MASK;
  data[5] = static_cast<uint8_t>(data_size);
  data[6] = 0x01;
  data[7] = 0x21;
  memcpy(data + 2 * (length_size + PDU::TWO_BYTES),
         PDU_DATA,
         sizeof(PDU_DATA));
  OLA_ASSERT_EQ(pdu_size,
                inflator.InflatePDUBlock(&header_set, data, pdu_size));
  OLA_ASSERT_EQ((unsigned int) 3, inflator.BlocksHandled());
  OLA_ASSERT_EQ((unsigned int) 1, child_inflator.BlocksHandled());
  delete[] data;
}
}  // namespace acn
}  // namespace ola
