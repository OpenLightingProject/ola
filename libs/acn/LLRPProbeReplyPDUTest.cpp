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
 * LLRPProbeReplyPDUTest.cpp
 * Test fixture for the LLRPProbeReplyPDU class
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
#include "libs/acn/LLRPProbeReplyPDU.h"

namespace ola {
namespace acn {

using ola::acn::LLRPProbeReplyPDU;
using ola::io::IOQueue;
using ola::io::IOStack;
using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::network::MACAddress;
using ola::rdm::UID;

class LLRPProbeReplyPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LLRPProbeReplyPDUTest);
  CPPUNIT_TEST(testSimpleLLRPProbeReplyPDU);
  CPPUNIT_TEST(testSimpleLLRPProbeReplyPDUToOutputStream);
  CPPUNIT_TEST(testPrepend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSimpleLLRPProbeReplyPDU();
  void testSimpleLLRPProbeReplyPDUToOutputStream();
  void testPrepend();

 private:
  static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(LLRPProbeReplyPDUTest);

const unsigned int LLRPProbeReplyPDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a LLRPProbeReplyPDU works.
 */
void LLRPProbeReplyPDUTest::testSimpleLLRPProbeReplyPDU() {
  UID target_uid = UID(0x4321, 0x12345678);
  MACAddress hardware_address;
  MACAddress::FromString("01:23:45:67:89:ab", &hardware_address);
  LLRPProbeReplyPDU pdu(
      TEST_VECTOR,
      target_uid,
      hardware_address,
      LLRPProbeReplyPDU::LLRP_COMPONENT_TYPE_NON_RDMNET);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(13u, pdu.DataSize());
  OLA_ASSERT_EQ(17u, pdu.Size());

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
  target_uid.Pack(buffer, sizeof(buffer));
  OLA_ASSERT_DATA_EQUALS(&data[4], UID::LENGTH, buffer, sizeof(buffer));
  uint8_t buffer2[MACAddress::LENGTH];
  hardware_address.Pack(buffer2, sizeof(buffer2));
  OLA_ASSERT_DATA_EQUALS(&data[10], MACAddress::LENGTH,
                         buffer2, sizeof(buffer2));

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
void LLRPProbeReplyPDUTest::testSimpleLLRPProbeReplyPDUToOutputStream() {
  UID target_uid = UID(0x4321, 0x12345678);
  MACAddress hardware_address;
  MACAddress::FromString("01:23:45:67:89:ab", &hardware_address);
  LLRPProbeReplyPDU pdu(
      TEST_VECTOR,
      target_uid,
      hardware_address,
      LLRPProbeReplyPDU::LLRP_COMPONENT_TYPE_NON_RDMNET);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(13u, pdu.DataSize());
  OLA_ASSERT_EQ(17u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(17u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0xf0, 0x00, 0x11,
    39,
    0x43, 0x21, 0x12, 0x34, 0x56, 0x78,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
    0xff
  };
  OLA_ASSERT_DATA_EQUALS(EXPECTED, sizeof(EXPECTED), pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}


void LLRPProbeReplyPDUTest::testPrepend() {
  IOStack stack;
  UID target_uid = UID(0x4321, 0x12345678);
  MACAddress hardware_address;
  MACAddress::FromString("01:23:45:67:89:ab", &hardware_address);
  LLRPProbeReplyPDU::PrependPDU(
      &stack,
      target_uid,
      hardware_address,
      LLRPProbeReplyPDU::LLRP_COMPONENT_TYPE_NON_RDMNET);

  unsigned int length = stack.Size();
  uint8_t *buffer = new uint8_t[length];
  OLA_ASSERT(stack.Read(buffer, length));

  const uint8_t expected_data[] = {
    0xf0, 0x00, 0x11, 1,
    0x43, 0x21, 0x12, 0x34, 0x56, 0x78,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
    0xff
  };
  OLA_ASSERT_DATA_EQUALS(expected_data, sizeof(expected_data), buffer, length);

  // test null stack
  LLRPProbeReplyPDU::PrependPDU(
      NULL,
      target_uid,
      hardware_address,
      LLRPProbeReplyPDU::LLRP_COMPONENT_TYPE_NON_RDMNET);

  delete[] buffer;
}
}  // namespace acn
}  // namespace ola
