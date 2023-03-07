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
 * BrokerPDUTest.cpp
 * Test fixture for the BrokerPDU class
 * Copyright (C) 2023 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/BrokerPDU.h"
#include "libs/acn/PDUTestCommon.h"


namespace ola {
namespace acn {

using ola::io::IOQueue;
using ola::io::OutputStream;
using ola::network::HostToNetwork;

class BrokerPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BrokerPDUTest);
  CPPUNIT_TEST(testSimpleBrokerPDU);
  CPPUNIT_TEST(testSimpleBrokerPDUToOutputStream);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSimpleBrokerPDU();
  void testSimpleBrokerPDUToOutputStream();

  void setUp() {
    ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
  }

 private:
  static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(BrokerPDUTest);

const unsigned int BrokerPDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a BrokerPDU without data works.
 */
void BrokerPDUTest::testSimpleBrokerPDU() {
  BrokerPDU pdu(TEST_VECTOR, NULL);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, pdu.DataSize());
  OLA_ASSERT_EQ(7u, pdu.Size());

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
void BrokerPDUTest::testSimpleBrokerPDUToOutputStream() {
  BrokerPDU pdu(TEST_VECTOR, NULL);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, pdu.DataSize());
  OLA_ASSERT_EQ(7u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(7u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0xf0, 0x00, 0x07,
    0, 0, 0, 39
  };
  OLA_ASSERT_DATA_EQUALS(EXPECTED, sizeof(EXPECTED), pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}
}  // namespace acn
}  // namespace ola
