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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * KiNetNodeTest.cpp
 * Test fixture for the KiNetNode class
 * Copyright (C) 2013 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "ola/testing/MockUDPSocket.h"
#include "plugins/kinet/KiNetNode.h"
#include "ola/testing/TestUtils.h"

using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::plugin::kinet::KiNetNode;
using ola::testing::MockUDPSocket;


class KiNetNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(KiNetNodeTest);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST_SUITE_END();

  public:
    KiNetNodeTest()
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

    static const uint16_t KINET_PORT = 6038;
};


CPPUNIT_TEST_SUITE_REGISTRATION(KiNetNodeTest);

void KiNetNodeTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  ola::network::IPV4Address::FromString("10.0.0.10", &target_ip);
}

/**
 * Check sending DMX works.
 */
void KiNetNodeTest::testSendDMX() {
  KiNetNode node(&ss, m_socket);
  OLA_ASSERT_TRUE(node.Start());

  const uint8_t expected_data[] = {
    0x04, 0x01, 0xdc, 0x4a, 0x01, 0x00,
    0x01, 0x01, 0, 0, 0, 0,
    0, 0, 0, 0, 0xff, 0xff, 0xff, 0xff,
    0, 1, 5, 8, 10, 14, 45, 100, 255
  };

  m_socket->AddExpectedData(expected_data, sizeof(expected_data), target_ip,
                            KINET_PORT);

  DmxBuffer buffer;
  buffer.SetFromString("1,5,8,10,14,45,100,255");
  OLA_ASSERT_TRUE(node.SendDMX(target_ip, buffer));
  m_socket->Verify();
  OLA_ASSERT(node.Stop());
}
