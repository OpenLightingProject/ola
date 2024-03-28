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
 * BrokerClientEntryRPTPDUTest.cpp
 * Test fixture for the BrokerClientEntryRPTPDU class
 * Copyright (C) 2023 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/IOStack.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/BrokerClientEntryRPTPDU.h"
#include "libs/acn/PDUTestCommon.h"


namespace ola {
namespace acn {

using ola::io::IOQueue;
using ola::io::IOStack;
using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::rdm::UID;

class BrokerClientEntryRPTPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BrokerClientEntryRPTPDUTest);
  CPPUNIT_TEST(testSimpleBrokerClientEntryRPTPDU);
  CPPUNIT_TEST(testSimpleBrokerClientEntryRPTPDUToOutputStream);
  CPPUNIT_TEST(testPrepend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSimpleBrokerClientEntryRPTPDU();
  void testSimpleBrokerClientEntryRPTPDUToOutputStream();
  void testPrepend();

  void setUp() {
    ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
  }

 private:
  static const unsigned int TEST_VECTOR;
  static const uint8_t TEST_DATA_1[];
  static const uint8_t TEST_DATA_2[];
};

const uint8_t BrokerClientEntryRPTPDUTest::TEST_DATA_1[] = {0, 1, 2, 3, 4, 5,
                                                            6, 7, 8, 9, 10, 11,
                                                            12, 13, 14, 15};

const uint8_t BrokerClientEntryRPTPDUTest::TEST_DATA_2[] = {16, 17, 18, 19, 20,
                                                            21, 22, 23, 24, 25,
                                                            26, 27, 28, 29,
                                                            30, 31};

CPPUNIT_TEST_SUITE_REGISTRATION(BrokerClientEntryRPTPDUTest);

const unsigned int BrokerClientEntryRPTPDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a BrokerClientEntryRPTPDU without data works.
 */
void BrokerClientEntryRPTPDUTest::testSimpleBrokerClientEntryRPTPDU() {
  const CID client_cid = CID::FromData(TEST_DATA_1);
  const UID client_uid = UID(0x4321, 0x12345678);
  const CID bounding_cid = CID::FromData(TEST_DATA_2);
  BrokerClientEntryHeader header(client_cid);
  BrokerClientEntryRPTPDU pdu(TEST_VECTOR, header, client_uid, 3, bounding_cid);

  OLA_ASSERT_EQ(16u, pdu.HeaderSize());
  OLA_ASSERT_EQ(23u, pdu.DataSize());
  OLA_ASSERT_EQ(46u, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);

  // spot check the data
  OLA_ASSERT_EQ((uint8_t) 0xf0, data[0]);
  // bytes_used is technically data[1] and data[2] if > 255
  OLA_ASSERT_EQ((uint8_t) bytes_used, data[2]);
  unsigned int actual_value;
  memcpy(&actual_value, data + 3, sizeof(actual_value));
  OLA_ASSERT_EQ(HostToNetwork(TEST_VECTOR), actual_value);

  uint8_t buffer[CID::CID_LENGTH];
  client_cid.Pack(buffer);
  OLA_ASSERT_DATA_EQUALS(&data[7], CID::CID_LENGTH, buffer, sizeof(buffer));

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
void BrokerClientEntryRPTPDUTest::
    testSimpleBrokerClientEntryRPTPDUToOutputStream() {
  const ola::acn::CID client_cid = CID::FromData(TEST_DATA_1);
  const UID client_uid = UID(0x4321, 0x12345678);
  const CID bounding_cid = CID::FromData(TEST_DATA_2);
  BrokerClientEntryHeader header(client_cid);
  BrokerClientEntryRPTPDU pdu(TEST_VECTOR, header, client_uid, 3, bounding_cid);

  OLA_ASSERT_EQ(16u, pdu.HeaderSize());
  OLA_ASSERT_EQ(23u, pdu.DataSize());
  OLA_ASSERT_EQ(46u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(46u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0xf0, 0x00, 0x2e,
    0, 0, 0, 39,
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    0x43, 0x21, 0x12, 0x34, 0x56, 0x78,
    3,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31
  };
  OLA_ASSERT_DATA_EQUALS(EXPECTED, sizeof(EXPECTED), pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}

void BrokerClientEntryRPTPDUTest::testPrepend() {
  const CID client_cid = CID::FromData(TEST_DATA_1);
  const UID client_uid = UID(0x4321, 0x12345678);
  const CID bounding_cid = CID::FromData(TEST_DATA_2);
  IOStack stack;
  BrokerClientEntryRPTPDU::PrependPDU(&stack,
                                      TEST_VECTOR,
                                      client_cid,
                                      client_uid,
                                      3,
                                      bounding_cid);

  unsigned int length = stack.Size();
  uint8_t *buffer = new uint8_t[length];
  OLA_ASSERT(stack.Read(buffer, length));

  const uint8_t expected_data[] = {
    0xf0, 0x00, 0x2e,
    0, 0, 0, 39,
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    0x43, 0x21, 0x12, 0x34, 0x56, 0x78,
    3,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31
  };
  OLA_ASSERT_DATA_EQUALS(expected_data, sizeof(expected_data), buffer, length);
  delete[] buffer;
}
}  // namespace acn
}  // namespace ola
