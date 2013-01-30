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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * StreamRpcChannelTest.cpp
 * Test fixture for the StreamRpcChannel class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>

#include "common/rpc/StreamRpcChannel.h"
#include "ola/testing/TestUtils.h"


using ola::rpc::StreamRpcHeader;
using ola::rpc::StreamRpcChannel;

class StreamRpcHeaderTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StreamRpcHeaderTest);
  CPPUNIT_TEST(testHeaderEncoding);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testHeaderEncoding();
};

CPPUNIT_TEST_SUITE_REGISTRATION(StreamRpcHeaderTest);

void StreamRpcHeaderTest::testHeaderEncoding() {
  /*
   * Test we can encode and decode headers correctly.
   */
  uint32_t header;
  unsigned int size, version, o_size, o_version;

  size = 0;
  version = 0;
  StreamRpcHeader::EncodeHeader(&header, version, size);
  StreamRpcHeader::DecodeHeader(header, &o_version, &o_size);
  OLA_ASSERT_EQ(version, o_version);

  version = StreamRpcChannel::PROTOCOL_VERSION;
  size = 24;
  StreamRpcHeader::EncodeHeader(&header, version, size);
  StreamRpcHeader::DecodeHeader(header, &o_version, &o_size);
  OLA_ASSERT_EQ(version, o_version);
}
