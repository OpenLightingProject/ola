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
 * RDMPDUTest.cpp
 * Test fixture for the RDMPDU class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/Logging.h"
#include "ola/io/ByteString.h"
#include "ola/io/IOQueue.h"
#include "ola/io/IOStack.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/RDMPDU.h"

namespace ola {
namespace acn {

using ola::io::ByteString;
using ola::io::IOQueue;
using ola::io::IOStack;
using ola::io::OutputStream;
using ola::network::HostToNetwork;

class RDMPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMPDUTest);
  CPPUNIT_TEST(testSimpleRDMPDU);
  CPPUNIT_TEST(testSimpleRDMPDUToOutputStream);
  CPPUNIT_TEST(testPrepend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSimpleRDMPDU();
  void testSimpleRDMPDUToOutputStream();
  void testPrepend();

 private:
  static const unsigned int TEST_VECTOR;
  static const uint8_t EXPECTED_GET_RESPONSE_BUFFER[];
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMPDUTest);

const unsigned int RDMPDUTest::TEST_VECTOR = 0xcc;

const uint8_t RDMPDUTest::EXPECTED_GET_RESPONSE_BUFFER[] = {
  1, 28,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 0, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x21, 1, 40, 4,  // command, param id, param data length
  0x5a, 0x5a, 0x5a, 0x5a,  // param data
  0x02, 0xb3  // checksum
};

/*
 * Test that packing an RDMPDU works.
 */
void RDMPDUTest::testSimpleRDMPDU() {
  ByteString empty;
  RDMPDU empty_pdu(empty);

  OLA_ASSERT_EQ(0u, empty_pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, empty_pdu.DataSize());
  OLA_ASSERT_EQ(4u, empty_pdu.Size());

  ByteString response(EXPECTED_GET_RESPONSE_BUFFER,
                      sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  RDMPDU pdu(response);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(29u, pdu.DataSize());
  OLA_ASSERT_EQ(33u, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);

  // spot check the data
  OLA_ASSERT_EQ((uint8_t) 0xf0, data[0]);
  // bytes_used is technically data[1] and data[2] if > 255
  OLA_ASSERT_EQ((uint8_t) bytes_used, data[2]);
  OLA_ASSERT_EQ(HostToNetwork((uint8_t) TEST_VECTOR), data[3]);

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
void RDMPDUTest::testSimpleRDMPDUToOutputStream() {
  ByteString response(EXPECTED_GET_RESPONSE_BUFFER,
                      sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  RDMPDU pdu(response);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(29u, pdu.DataSize());
  OLA_ASSERT_EQ(33u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(33u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0xf0, 0x00, 0x21,
    0xcc,
    1, 28,  // sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 0, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x21, 1, 40, 4,  // command, param id, param data length
    0x5a, 0x5a, 0x5a, 0x5a,  // param data
    0x02, 0xb3  // checksum
  };
  OLA_ASSERT_DATA_EQUALS(EXPECTED, sizeof(EXPECTED), pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}


void RDMPDUTest::testPrepend() {
  IOStack stack;
  RDMPDU::PrependPDU(&stack);

  unsigned int length = stack.Size();
  uint8_t *buffer = new uint8_t[length];
  OLA_ASSERT(stack.Read(buffer, length));

  const uint8_t expected_data[] = {0xf0, 0, 4, TEST_VECTOR};
  OLA_ASSERT_DATA_EQUALS(expected_data, sizeof(expected_data), buffer, length);
  delete[] buffer;
}
}  // namespace acn
}  // namespace ola
