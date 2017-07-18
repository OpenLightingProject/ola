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
 * OpcServerTest.cpp
 * Test fixture for the OpcServer class
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include <memory>
#include "ola/base/Array.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/network/TCPSocket.h"
#include "ola/testing/TestUtils.h"
#include "ola/util/Utils.h"
#include "plugins/openpixelcontrol/OpcServer.h"


using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::TCPSocket;
using ola::plugin::openpixelcontrol::OpcServer;
using std::auto_ptr;

class OpcServerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OpcServerTest);
  CPPUNIT_TEST(testReceiveDmx);
  CPPUNIT_TEST(testUnknownCommand);
  CPPUNIT_TEST(testLargeFrame);
  CPPUNIT_TEST(testHangingFrame);
  CPPUNIT_TEST_SUITE_END();

 public:
  OpcServerTest()
      : CppUnit::TestFixture(),
        m_ss(NULL) {
  }
  void setUp();

  void testReceiveDmx();
  void testUnknownCommand();
  void testLargeFrame();
  void testHangingFrame();

 private:
  ola::io::SelectServer m_ss;
  auto_ptr<OpcServer> m_server;
  auto_ptr<TCPSocket> m_client_socket;
  DmxBuffer m_received_data;
  uint8_t m_command;

  void SendDataAndCheck(uint8_t channel,
                        const DmxBuffer &data);

  void CaptureData(uint8_t command, const uint8_t *data, unsigned int length) {
    m_received_data.Set(data, length);
    m_command = command;
    m_ss.Terminate();
  }

  static const uint8_t CHANNEL = 1;
  static const uint8_t SET_PIXELS_COMMAND = 0;
};

CPPUNIT_TEST_SUITE_REGISTRATION(OpcServerTest);

void OpcServerTest::setUp() {
  IPV4SocketAddress listen_addr(IPV4Address::Loopback(), 0);
  m_server.reset(new OpcServer(&m_ss, listen_addr));
  m_server->SetCallback(
      CHANNEL,
      ola::NewCallback(this, &OpcServerTest::CaptureData));

  OLA_ASSERT_TRUE(m_server->Init());

  m_client_socket.reset(TCPSocket::Connect(m_server->ListenAddress()));
}

void OpcServerTest::SendDataAndCheck(uint8_t channel,
                                     const DmxBuffer &buffer) {
  unsigned int dmx_size = buffer.Size();
  uint8_t data[dmx_size + 4];
  data[0] = channel;
  data[1] = SET_PIXELS_COMMAND;
  ola::utils::SplitUInt16(static_cast<uint16_t>(dmx_size), &data[2], &data[3]);
  buffer.Get(data + 4, &dmx_size);
  m_client_socket->Send(data, dmx_size + 4);

  m_ss.Run();
  OLA_ASSERT_EQ(m_received_data, buffer);
}

void OpcServerTest::testReceiveDmx() {
  DmxBuffer buffer;
  buffer.SetFromString("1,2,3,4");
  SendDataAndCheck(CHANNEL, buffer);
  buffer.SetFromString("5,6");
  SendDataAndCheck(CHANNEL, buffer);
  buffer.SetFromString("5,6,7,8,89,9,5,6,7,8,3");
  SendDataAndCheck(CHANNEL, buffer);
}

void OpcServerTest::testUnknownCommand() {
  uint8_t data[] = {1, 1, 0, 2, 3, 4};
  m_client_socket->Send(data, arraysize(data));
  m_ss.Run();

  DmxBuffer buffer;
  buffer.SetFromString("3,4");
  OLA_ASSERT_EQ(static_cast<uint8_t>(1), m_command);
  OLA_ASSERT_EQ(m_received_data, buffer);
}

void OpcServerTest::testLargeFrame() {
  uint8_t data[1028];
  data[0] = 1;
  data[1] = 0;
  data[2] = 4;
  data[3] = 0;
  for (unsigned int i = 0; i < 1024; i++) {
    data[i + 4] = i;
  }

  m_client_socket->Send(data, arraysize(data));
  m_ss.Run();

  DmxBuffer buffer(data + 4, ola::DMX_UNIVERSE_SIZE);
  OLA_ASSERT_EQ(m_received_data, buffer);
}

void OpcServerTest::testHangingFrame() {
  uint8_t data[] = {1, 0};
  m_client_socket->Send(data, arraysize(data));
}
