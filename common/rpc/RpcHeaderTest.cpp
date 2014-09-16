/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * RpcHeaderTest.cpp
 * Test fixture for the RpcHeader class
 * Copyright (C) 2005 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>

#include "common/rpc/RpcChannel.h"
#include "common/rpc/RpcHeader.h"
#include "ola/testing/TestUtils.h"


using ola::rpc::RpcChannel;
using ola::rpc::RpcHeader;

class RpcHeaderTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RpcHeaderTest);
  CPPUNIT_TEST(testHeaderEncoding);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testHeaderEncoding();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RpcHeaderTest);

void RpcHeaderTest::testHeaderEncoding() {
  /*
   * Test we can encode and decode headers correctly.
   */
  uint32_t header;
  unsigned int size, version, o_size, o_version;

  size = 0;
  version = 0;
  RpcHeader::EncodeHeader(&header, version, size);
  RpcHeader::DecodeHeader(header, &o_version, &o_size);
  OLA_ASSERT_EQ(version, o_version);

  version = RpcChannel::PROTOCOL_VERSION;
  size = 24;
  RpcHeader::EncodeHeader(&header, version, size);
  RpcHeader::DecodeHeader(header, &o_version, &o_size);
  OLA_ASSERT_EQ(version, o_version);
}
