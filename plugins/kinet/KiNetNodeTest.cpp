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
  CPPUNIT_TEST(testSendPortOut);
  CPPUNIT_TEST_SUITE_END();

 public:
    KiNetNodeTest()
        : CppUnit::TestFixture(),
          ss(NULL),
          m_socket(new MockUDPSocket()) {
    }
    void setUp();

    void testSendDMX();

    void testSendPortOut();

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
    0x04, 0x01, 0xdc, 0x4a, 0x01, 0x00,  // magic number
    0x01, 0x01,  // packet type
    0, 0, 0, 0,  // packet counter
    0, 0, 0, 0,  // unknown flags
    0xff, 0xff, 0xff, 0xff,  // universe
    0,  // unknown start code
    1, 5, 8, 10, 14, 45, 100, 255  // data
  };

  m_socket->AddExpectedData(expected_data, sizeof(expected_data), target_ip,
                            KINET_PORT);

  DmxBuffer buffer;
  buffer.SetFromString("1,5,8,10,14,45,100,255");
  OLA_ASSERT_TRUE(node.SendDMX(target_ip, buffer));
  m_socket->Verify();
  OLA_ASSERT(node.Stop());
}

/**
 * Check sending PORTOUT works.
 */
void KiNetNodeTest::testSendPortOut() {
  KiNetNode node(&ss, m_socket);
  OLA_ASSERT_TRUE(node.Start());

  // Short frame, padded
  const uint8_t expected_data_1[] = {
    0x04, 0x01, 0xdc, 0x4a, 0x01, 0x00,  // magic number
    0x08, 0x01,  // packet type
    0, 0, 0, 0,  // packet counter
    0xff, 0xff, 0xff, 0xff,  // universe
    7,  // port number
    0, 0, 0,  // unknown flags
    24, 0,  // buffer size
    0, 0,  // unknown start code
    1, 5, 8, 10, 14, 45, 100, 255,  // data
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // data pad to 24 bytes
  };

  m_socket->AddExpectedData(expected_data_1, sizeof(expected_data_1), target_ip,
                            KINET_PORT);

  DmxBuffer buffer1;
  buffer1.SetFromString("1,5,8,10,14,45,100,255");
  OLA_ASSERT_TRUE(node.SendPortOut(target_ip, 7, buffer1));
  m_socket->Verify();

  // Correct minimum length frame, no padding, counter increments
  const uint8_t expected_data_2[] = {
    0x04, 0x01, 0xdc, 0x4a, 0x01, 0x00,  // magic number
    0x08, 0x01,  // packet type
    1, 0, 0, 0,  // packet counter
    0xff, 0xff, 0xff, 0xff,  // universe
    7,  // port number
    0, 0, 0,  // unknown flags
    24, 0,  // buffer size
    0, 0,  // unknown start code
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24  // data
  };

  m_socket->AddExpectedData(expected_data_2, sizeof(expected_data_2), target_ip,
                            KINET_PORT);

  DmxBuffer buffer2;
  buffer2.SetFromString("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24");
  OLA_ASSERT_TRUE(node.SendPortOut(target_ip, 7, buffer2));
  m_socket->Verify();

  // Above minimum length frame, no padding, counter increments, different port number
  const uint8_t expected_data_3[] = {
    0x04, 0x01, 0xdc, 0x4a, 0x01, 0x00,  // magic number
    0x08, 0x01,  // packet type
    2, 0, 0, 0,  // packet counter
    0xff, 0xff, 0xff, 0xff,  // universe
    0xff,  // port number
    0, 0, 0,  // unknown flags
    25, 0,  // buffer size
    0, 0,  // unknown start code
    255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    255  // data
  };

  m_socket->AddExpectedData(expected_data_3, sizeof(expected_data_3), target_ip,
                            KINET_PORT);

  DmxBuffer buffer3;
  buffer3.SetFromString(
      "255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255");
  OLA_ASSERT_TRUE(node.SendPortOut(target_ip, 255, buffer3));
  m_socket->Verify();
  OLA_ASSERT(node.Stop());
}
