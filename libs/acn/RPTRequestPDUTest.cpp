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
 * RPTRequestPDUTest.cpp
 * Test fixture for the RPTRequestPDU class
 * Copyright (C) 2024 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/IOStack.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/RPTRequestPDU.h"

namespace ola {
namespace acn {

using ola::acn::RPTRequestPDU;
using ola::io::IOQueue;
using ola::io::IOStack;
using ola::io::OutputStream;
using ola::network::HostToNetwork;

class RPTRequestPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RPTRequestPDUTest);
  CPPUNIT_TEST(testSimpleRPTRequestPDU);
  CPPUNIT_TEST(testSimpleRPTRequestPDUToOutputStream);
  CPPUNIT_TEST(testPrepend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSimpleRPTRequestPDU();
  void testSimpleRPTRequestPDUToOutputStream();
  void testPrepend();

 private:
  static const uint32_t TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RPTRequestPDUTest);

const uint32_t RPTRequestPDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a RPTRequestPDU works.
 */
void RPTRequestPDUTest::testSimpleRPTRequestPDU() {
  RPTRequestPDU pdu(TEST_VECTOR);

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
  // bytes_used is technically part of data[0-2] if > 255
  OLA_ASSERT_EQ((uint8_t) bytes_used, data[2]);
  uint32_t actual_value;
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
void RPTRequestPDUTest::testSimpleRPTRequestPDUToOutputStream() {
  RPTRequestPDU pdu(TEST_VECTOR);

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


void RPTRequestPDUTest::testPrepend() {
  IOStack stack;
  RPTRequestPDU::PrependPDU(&stack);

  unsigned int length = stack.Size();
  uint8_t *buffer = new uint8_t[length];
  OLA_ASSERT(stack.Read(buffer, length));

  const uint8_t expected_data[] = {
    0xf0, 0x00, 0x07,
    0, 0, 0, 0x01
  };
  OLA_ASSERT_DATA_EQUALS(expected_data, sizeof(expected_data), buffer, length);
  delete[] buffer;
}
}  // namespace acn
}  // namespace ola
