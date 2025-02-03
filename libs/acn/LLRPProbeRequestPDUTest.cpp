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
 * LLRPProbeRequestPDUTest.cpp
 * Test fixture for the LLRPProbeRequestPDU class
 * Copyright (C) 2020 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/IOStack.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/LLRPProbeRequestPDU.h"

namespace ola {
namespace acn {

using ola::acn::LLRPProbeRequestPDU;
using ola::io::IOQueue;
using ola::io::IOStack;
using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::rdm::UID;
using ola::rdm::UIDSet;

class LLRPProbeRequestPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LLRPProbeRequestPDUTest);
  CPPUNIT_TEST(testSimpleLLRPProbeRequestPDU);
  CPPUNIT_TEST(testSimpleLLRPProbeRequestPDUToOutputStream);
  CPPUNIT_TEST(testPrepend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSimpleLLRPProbeRequestPDU();
  void testSimpleLLRPProbeRequestPDUToOutputStream();
  void testPrepend();

 private:
  static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(LLRPProbeRequestPDUTest);

const unsigned int LLRPProbeRequestPDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a LLRPProbeRequestPDU works.
 */
void LLRPProbeRequestPDUTest::testSimpleLLRPProbeRequestPDU() {
  UID lower_uid = UID(0x4321, 0x12345678);
  UID upper_uid = UID(0x5678, 0x00abcdef);
  UIDSet known_uids;
  known_uids.AddUID(UID(0x1234, 0x00000001));
  known_uids.AddUID(UID(0x5678, 0x00000002));
  known_uids.AddUID(UID(0x4321, 0x56789abc));
  LLRPProbeRequestPDU pdu(
      TEST_VECTOR,
      lower_uid,
      upper_uid,
      false,
      false,
      known_uids);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(32u, pdu.DataSize());
  OLA_ASSERT_EQ(36u, pdu.Size());

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

  uint8_t buffer[UID::LENGTH];
  lower_uid.Pack(buffer, sizeof(buffer));
  OLA_ASSERT_DATA_EQUALS(&data[4], UID::LENGTH, buffer, sizeof(buffer));
  upper_uid.Pack(buffer, sizeof(buffer));
  OLA_ASSERT_DATA_EQUALS(&data[10], UID::LENGTH, buffer, sizeof(buffer));

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
void LLRPProbeRequestPDUTest::testSimpleLLRPProbeRequestPDUToOutputStream() {
  UID lower_uid = UID(0x4321, 0x12345678);
  UID upper_uid = UID(0x5678, 0x00abcdef);
  UIDSet known_uids;
  known_uids.AddUID(UID(0x1234, 0x00000001));
  known_uids.AddUID(UID(0x5678, 0x00000002));
  known_uids.AddUID(UID(0x4321, 0x56789abc));
  LLRPProbeRequestPDU pdu(
      TEST_VECTOR,
      lower_uid,
      upper_uid,
      false,
      false,
      known_uids);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(32u, pdu.DataSize());
  OLA_ASSERT_EQ(36u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(36u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0xf0, 0x00, 0x24,
    39,
    0x43, 0x21, 0x12, 0x34, 0x56, 0x78,
    0x56, 0x78, 0x00, 0xab, 0xcd, 0xef,
    0x00, 0x00,
    0x12, 0x34, 0, 0, 0, 1,
    0x43, 0x21, 0x56, 0x78, 0x9a, 0xbc,
    0x56, 0x78, 0, 0, 0, 2
  };
  OLA_ASSERT_DATA_EQUALS(EXPECTED, sizeof(EXPECTED), pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}


void LLRPProbeRequestPDUTest::testPrepend() {
  IOStack stack;
  UIDSet known_uids;
  known_uids.AddUID(UID(0x1234, 0x00000001));
  known_uids.AddUID(UID(0x5678, 0x00000002));
  known_uids.AddUID(UID(0x4321, 0x12345678));
  LLRPProbeRequestPDU::PrependPDU(&stack,
                                  UID(0x0000, 0x00000000),
                                  UID(0xffff, 0xffffffff),
                                  false,
                                  false,
                                  known_uids);

  unsigned int length = stack.Size();
  uint8_t *buffer = new uint8_t[length];
  OLA_ASSERT(stack.Read(buffer, length));

  const uint8_t expected_data[] = {
    0xf0, 0x00, 0x24, 1,
    0, 0, 0, 0, 0, 0,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00,
    0x12, 0x34, 0, 0, 0, 1,
    0x43, 0x21, 0x12, 0x34, 0x56, 0x78,
    0x56, 0x78, 0, 0, 0, 2,
  };
  OLA_ASSERT_DATA_EQUALS(expected_data, sizeof(expected_data), buffer, length);

  // test null stack
  LLRPProbeRequestPDU::PrependPDU(NULL,
                                  UID(0x0000, 0x00000000),
                                  UID(0xffff, 0xffffffff),
                                  false,
                                  false,
                                  known_uids);

  delete[] buffer;
}
}  // namespace acn
}  // namespace ola
