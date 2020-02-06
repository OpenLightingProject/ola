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
#include "ola/io/IOStack.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/LLRPProbeReplyPDU.h"

namespace ola {
namespace acn {

using ola::acn::LLRPProbeReplyPDU;
using ola::io::IOStack;
using ola::network::MACAddress;
using ola::rdm::UID;

class LLRPProbeReplyPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LLRPProbeReplyPDUTest);
  CPPUNIT_TEST(testPrepend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testPrepend();

 private:
  static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(LLRPProbeReplyPDUTest);

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
  delete[] buffer;
}
}  // namespace acn
}  // namespace ola
