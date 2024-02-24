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
 * RPTPDUTest.cpp
 * Test fixture for the E1.33 RPTPDU class
 * Copyright (C) 2024 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/RPTPDU.h"
#include "libs/acn/PDUTestCommon.h"


namespace ola {
namespace acn {

using ola::io::IOQueue;
using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::rdm::UID;

class RPTPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RPTPDUTest);
  CPPUNIT_TEST(testSimpleRPTPDU);
  CPPUNIT_TEST(testSimpleRPTPDUToOutputStream);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSimpleRPTPDU();
  void testSimpleRPTPDUToOutputStream();

  void setUp() {
    ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
  }

 private:
  static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RPTPDUTest);

const unsigned int RPTPDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a RPTPDU without data works.
 */
void RPTPDUTest::testSimpleRPTPDU() {
  UID source_uid(1, 2);
  UID destination_uid(10, 20);
  RPTHeader header(source_uid, 42, destination_uid, 99, 2370);
  RPTPDU pdu(TEST_VECTOR, header, NULL);

  OLA_ASSERT_EQ(21u, pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, pdu.DataSize());
  OLA_ASSERT_EQ(28u, pdu.Size());

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

  uint8_t buffer[UID::LENGTH];
  source_uid.Pack(buffer, sizeof(buffer));
  OLA_ASSERT_DATA_EQUALS(&data[7], UID::LENGTH, buffer, sizeof(buffer));
  destination_uid.Pack(buffer, sizeof(buffer));
  OLA_ASSERT_DATA_EQUALS(&data[15], UID::LENGTH, buffer, sizeof(buffer));
  // sequence number
  OLA_ASSERT_EQ((uint8_t) 0, data[23]);
  OLA_ASSERT_EQ((uint8_t) 0, data[23 + 1]);
  OLA_ASSERT_EQ((unsigned int) 9, (unsigned int) data[23 + 2]);
  OLA_ASSERT_EQ((unsigned int) 66, (unsigned int) data[23 + 3]);

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
void RPTPDUTest::testSimpleRPTPDUToOutputStream() {
  RPTHeader header(UID(0x0102, 0x03040506), 3456,
                   UID(0x4050, 0x60708090), 7890,
                   2370);
  RPTPDU pdu(TEST_VECTOR, header, NULL);

  OLA_ASSERT_EQ(21u, pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, pdu.DataSize());
  OLA_ASSERT_EQ(28u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(28u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0xf0, 0x00, 0x1c,
    0, 0, 0, 39,
    1, 2, 3, 4, 5, 6,  // source UID
    13, 128,  // source endpoint
    64, 80, 96, 112, 128, 144,  // dest UID
    30, 210,  // dest endpoint
    0, 0, 9, 66,  // sequence number
    0  // reserved
  };
  OLA_ASSERT_DATA_EQUALS(EXPECTED, sizeof(EXPECTED), pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}
}  // namespace acn
}  // namespace ola
