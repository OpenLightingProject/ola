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
 * OPCClientTest.cpp
 * Test fixture for the OPCClient class
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
#include "ola/testing/TestUtils.h"
#include "ola/util/Utils.h"
#include "plugins/openpixelcontrol/OPCClient.h"
#include "plugins/openpixelcontrol/OPCServer.h"


using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::plugin::openpixelcontrol::OPCClient;
using ola::plugin::openpixelcontrol::OPCServer;
using std::auto_ptr;

class OPCClientTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OPCClientTest);
  CPPUNIT_TEST(testTransmit);
  CPPUNIT_TEST_SUITE_END();

 public:
  OPCClientTest()
      : CppUnit::TestFixture(),
        m_ss(NULL) {
  }
  void setUp();

  void testTransmit();

 private:
  ola::io::SelectServer m_ss;
  auto_ptr<OPCServer> m_server;
  DmxBuffer m_received_data;
  uint8_t m_command;

  void CaptureData(uint8_t command, const uint8_t *data, unsigned int length) {
    m_received_data.Set(data, length);
    m_command = command;
    m_ss.Terminate();
  }

  void SendDMX(OPCClient *client, DmxBuffer *buffer, bool connected) {
    if (connected) {
      OLA_ASSERT_TRUE(client->SendDmx(CHANNEL, *buffer));
    } else {
      m_ss.Terminate();
    }
  }

  static const uint8_t CHANNEL = 1;
};

CPPUNIT_TEST_SUITE_REGISTRATION(OPCClientTest);

void OPCClientTest::setUp() {
  IPV4SocketAddress listen_addr(IPV4Address::Loopback(), 0);
  m_server.reset(new OPCServer(&m_ss, listen_addr));
  m_server->SetCallback(
      CHANNEL,
      ola::NewCallback(this, &OPCClientTest::CaptureData));

  OLA_ASSERT_TRUE(m_server->Init());
}

void OPCClientTest::testTransmit() {
  OPCClient client(&m_ss, m_server->ListenAddress());
  OLA_ASSERT_EQ(m_server->ListenAddress().ToString(),
                client.GetRemoteAddress());

  DmxBuffer buffer;
  buffer.SetFromString("1,2,3,4");

  client.SetSocketCallback(
      ola::NewCallback(this, &OPCClientTest::SendDMX, &client, &buffer));

  m_ss.Run();
  OLA_ASSERT_EQ(m_received_data, buffer);

  // Now shut down the server.
  m_server.reset();
  m_ss.Run();

  // Now sends should fail since there is no connection
  OLA_ASSERT_FALSE(client.SendDmx(CHANNEL, buffer));
}
