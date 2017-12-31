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
 * NanoleafNodeTest.cpp
 * Test fixture for the NanoleafNode class
 * Copyright (C) 2017 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include <vector>

#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/MockUDPSocket.h"
#include "plugins/nanoleaf/NanoleafNode.h"
#include "ola/testing/TestUtils.h"

using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::plugin::nanoleaf::NanoleafNode;
using ola::testing::MockUDPSocket;
using std::vector;


class NanoleafNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(NanoleafNodeTest);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST_SUITE_END();

 public:
    NanoleafNodeTest()
        : CppUnit::TestFixture(),
          ss(NULL),
          m_socket(new MockUDPSocket()) {
    }
    void setUp();

    void testSendDMX();

 private:
    ola::io::SelectServer ss;
    IPV4Address target_ip;
    MockUDPSocket *m_socket;

    static const uint16_t NANOLEAF_PORT = 60221;
};


CPPUNIT_TEST_SUITE_REGISTRATION(NanoleafNodeTest);

void NanoleafNodeTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  ola::network::IPV4Address::FromString("10.0.0.10", &target_ip);
}

/**
 * Check sending DMX works.
 */
void NanoleafNodeTest::testSendDMX() {
  vector<uint8_t> panels;
  panels.push_back(0x10);
  panels.push_back(0x20);

  NanoleafNode node(&ss, panels, m_socket);
  OLA_ASSERT_TRUE(node.Start());

  const uint8_t expected_data[] = {
    0x02,
    0x10, 0x01, 1, 5, 8, 0x00, 0x01,
    0x20, 0x01, 10, 14, 45, 0x00, 0x01
  };

  m_socket->AddExpectedData(expected_data, sizeof(expected_data), target_ip,
                            NANOLEAF_PORT);

  DmxBuffer buffer;
  buffer.SetFromString("1,5,8,10,14,45");
  OLA_ASSERT_TRUE(node.SendDMX(IPV4SocketAddress(target_ip, NANOLEAF_PORT), buffer));
  m_socket->Verify();
  OLA_ASSERT(node.Stop());
}
