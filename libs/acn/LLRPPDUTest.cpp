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
 * LLRPPDUTest.cpp
 * Test fixture for the LLRPPDU class
 * Copyright (C) 2020 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/LLRPPDU.h"
#include "libs/acn/PDUTestCommon.h"


namespace ola {
namespace acn {

using ola::io::IOQueue;
using ola::io::OutputStream;
using ola::network::HostToNetwork;
using std::string;

class LLRPPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LLRPPDUTest);
  CPPUNIT_TEST(testSimpleLLRPPDU);
  CPPUNIT_TEST(testSimpleLLRPPDUToOutputStream);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSimpleLLRPPDU();
  void testSimpleLLRPPDUToOutputStream();

  void setUp() {
    ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
  }

 private:
  static const unsigned int TEST_VECTOR;
  static const uint8_t TEST_DATA[];
};

const uint8_t LLRPPDUTest::TEST_DATA[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                          12, 13, 14, 15};

CPPUNIT_TEST_SUITE_REGISTRATION(LLRPPDUTest);

const unsigned int LLRPPDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a LLRPPDU without data works.
 */
void LLRPPDUTest::testSimpleLLRPPDU() {
  const CID destination_cid = CID::FromData(TEST_DATA);
  LLRPHeader header(destination_cid, 101);
  LLRPPDU pdu(TEST_VECTOR, header, NULL);

  OLA_ASSERT_EQ(20u, pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, pdu.DataSize());
  OLA_ASSERT_EQ(26u, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);

  // spot check the data
  OLA_ASSERT_EQ((uint8_t) 0x70, data[0]);
  OLA_ASSERT_EQ((uint8_t) bytes_used, data[1]);
  unsigned int actual_value;
  memcpy(&actual_value, data + 2, sizeof(actual_value));
  OLA_ASSERT_EQ(HostToNetwork(TEST_VECTOR), actual_value);

  uint8_t buffer[CID::CID_LENGTH];
  destination_cid.Pack(buffer);
  OLA_ASSERT_FALSE(memcmp(&data[6], buffer, CID::CID_LENGTH));
  // transaction number
  OLA_ASSERT_EQ((uint8_t) 0, data[6 + CID::CID_LENGTH]);
  OLA_ASSERT_EQ((uint8_t) 0, data[6 + CID::CID_LENGTH + 1]);
  OLA_ASSERT_EQ((uint8_t) 0, data[6 + CID::CID_LENGTH + 2]);
  OLA_ASSERT_EQ((uint8_t) 101, data[6 + CID::CID_LENGTH + 3]);

  // test undersized buffer
  bytes_used = size - 1;
  OLA_ASSERT_FALSE(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ(0u, bytes_used);

  // test oversized buffer
  bytes_used = size + 1;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);
  delete[] data;
}


/*
 * Test that writing to an output stream works.
 */
void LLRPPDUTest::testSimpleLLRPPDUToOutputStream() {
  const ola::acn::CID destination_cid = CID::FromData(TEST_DATA);
  LLRPHeader header(destination_cid, 101);
  LLRPPDU pdu(TEST_VECTOR, header, NULL);

  OLA_ASSERT_EQ(20u, pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, pdu.DataSize());
  OLA_ASSERT_EQ(26u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(26u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0xf0, 0x1a,
    0, 0, 0, 39,
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    0, 0, 0, 101,  // transaction number
  };
  OLA_ASSERT_DATA_EQUALS(EXPECTED, sizeof(EXPECTED), pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}
}  // namespace acn
}  // namespace ola
