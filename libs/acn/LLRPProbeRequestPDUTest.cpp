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
#include "ola/io/IOStack.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/testing/TestUtils.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/LLRPProbeRequestPDU.h"

namespace ola {
namespace acn {

using ola::io::IOStack;
using ola::rdm::UID;
using ola::rdm::UIDSet;

class LLRPProbeRequestPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LLRPProbeRequestPDUTest);
  CPPUNIT_TEST(testPrepend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testPrepend();

 private:
  static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(LLRPProbeRequestPDUTest);

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
    0xf0, 0x23, 1,
    0, 0, 0, 0, 0, 0,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00,
    0x12, 0x34, 0, 0, 0, 1,
    0x43, 0x21, 0x12, 0x34, 0x56, 0x78,
    0x56, 0x78, 0, 0, 0, 2,
  };
  OLA_ASSERT_DATA_EQUALS(expected_data, sizeof(expected_data), buffer, length);
  delete[] buffer;
}
}  // namespace acn
}  // namespace ola
