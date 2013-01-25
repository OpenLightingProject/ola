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
 * OSCNodeTest.cpp
 * Test fixture for the OSCNode class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Socket.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/TestUtils.h"
#include "plugins/osc/OSCNode.h"
#include "plugins/osc/OSCTarget.h"

using ola::DmxBuffer;
using ola::NewCallback;
using ola::io::SelectServer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::UDPSocket;
using ola::plugin::osc::OSCNode;
using ola::plugin::osc::OSCTarget;
using ola::testing::ASSERT_DATA_EQUALS;


/**
 * The test fixture for the OSCNode.
 */
class OSCNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OSCNodeTest);
  CPPUNIT_TEST(testSend);
  CPPUNIT_TEST(testReceive);
  CPPUNIT_TEST_SUITE_END();

  public:
    /**
     * Initilize the OSCNode and the timeout_id
     */
    OSCNodeTest()
        : CppUnit::TestFixture(),
          m_osc_node(&m_ss, NULL, OSCNode::OSCNodeOptions()),
          m_timeout_id(ola::thread::INVALID_TIMEOUT) {
    }

    // The setUp and tearDown methods. These are run before and after each test
    // case.
    void setUp();
    void tearDown() { m_osc_node.Stop(); }

    // our two tests
    void testSend();
    void testReceive();

    // Called if we don't receive data in ABORT_TIMEOUT_IN_MS
    void Timeout() { OLA_FAIL("timeout"); }

  private:
    ola::io::SelectServer m_ss;
    OSCNode m_osc_node;
    UDPSocket m_udp_socket;
    ola::thread::timeout_id m_timeout_id;
    DmxBuffer m_dmx_data;

    void UDPSocketReady();
    void DMXHandler(const DmxBuffer &dmx);

    static const unsigned int TEST_GROUP = 10;  // the group to use for testing
    // The number of mseconds to wait before failing the test.
    static const int ABORT_TIMEOUT_IN_MS = 2000;
    // The expected OSC packet on the wire
    static const uint8_t EXPECTED_OSC_PACKET[];
    // The OSC address to use for testing
    static const char TEST_OSC_ADDRESS[];
};

CPPUNIT_TEST_SUITE_REGISTRATION(OSCNodeTest);

// The expected OSC packet. See http://opensoundcontrol.org/ for what each
// byte means.
const uint8_t OSCNodeTest::EXPECTED_OSC_PACKET[] = {
  // osc address
  '/', 'd', 'm', 'x', '/', 'u', 'n', 'i',
  'v', 'e', 'r', 's', 'e', '/', '1', '0',
  0, 0, 0, 0,
  // tag type
  ',', 'b', 0, 0, 0, 0, 0, 0x0b,
  // data
  0, 1, 2, 3, 4, 5, 6, 7,
  8, 9, 0xa, 0
};

// An OSC Address used for testing.
const char OSCNodeTest::TEST_OSC_ADDRESS[] = "/dmx/universe/10";


void OSCNodeTest::setUp() {
  // Init logging
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  // Setup and register the Timeout.
  m_timeout_id = m_ss.RegisterSingleTimeout(
        ABORT_TIMEOUT_IN_MS,
        ola::NewSingleCallback(this, &OSCNodeTest::Timeout));
  OLA_ASSERT_TRUE(m_timeout_id);

  // Init our UDP socket.
  OLA_ASSERT_TRUE(m_udp_socket.Init());
  // Put some data into the DMXBuffer
  m_dmx_data.SetFromString("0,1,2,3,4,5,6,7,8,9,10");

  // Initialize the OSCNode
  OLA_ASSERT_TRUE(m_osc_node.Init());
}

/**
 * Called when data arrives on our UDP socket. We check that this data matches
 * the expected OSC packet
 */
void OSCNodeTest::UDPSocketReady() {
  uint8_t data[500];  // 500 bytes is more than enough
  ssize_t data_read = sizeof(data);
  // Read the received packet into 'data'.
  OLA_ASSERT_TRUE(m_udp_socket.RecvFrom(data, &data_read));
  // Verify it matches the expected packet
  ASSERT_DATA_EQUALS(__LINE__, EXPECTED_OSC_PACKET,
                     sizeof(EXPECTED_OSC_PACKET), data, data_read);
  // Stop the SelectServer
  m_ss.Terminate();
}

/**
 * Called when we receive DMX data via OSC. We check this matches what we
 * expect, and then stop the SelectServer.
 */
void OSCNodeTest::DMXHandler(const DmxBuffer &dmx) {
  OLA_ASSERT_EQ(11u, dmx.Size());
  OLA_ASSERT_EQ(m_dmx_data, dmx);
  m_ss.Terminate();
}


/**
 * Check that we send OSC messages correctly.
 */
void OSCNodeTest::testSend() {
  // First up create a UDP socket to receive the messages on.
  // Port 0 means 'ANY'
  IPV4SocketAddress socket_address(IPV4Address::Loopback(), 0);
  // Bind the socket, set the callback, and register with the select server.
  OLA_ASSERT_TRUE(m_udp_socket.Bind(socket_address));
  m_udp_socket.SetOnData(NewCallback(this, &OSCNodeTest::UDPSocketReady));
  OLA_ASSERT_TRUE(m_ss.AddReadDescriptor(&m_udp_socket));
  // Store the local address of the UDP socket so we know where to tell the
  // OSCNode to send to.
  OLA_ASSERT_TRUE(m_udp_socket.GetSocketAddress(&socket_address));

  // Setup the OSCTarget pointing to the local socket address
  OSCTarget target(socket_address, TEST_OSC_ADDRESS);
  // Add the target to the node.
  m_osc_node.AddTarget(TEST_GROUP, target);
  // Send the data
  OLA_ASSERT_TRUE(m_osc_node.SendData(TEST_GROUP, m_dmx_data));

  // Run the SelectServer this will return either when UDPSocketReady
  // completes, or the abort timeout triggers.
  m_ss.Run();

  // Remove target
  OLA_ASSERT_TRUE(m_osc_node.RemoveTarget(TEST_GROUP, target));
  // Try to remove it a second time
  OLA_ASSERT_FALSE(m_osc_node.RemoveTarget(TEST_GROUP, target));

  // Try to remove the target from a group that doesn't exist
  OLA_ASSERT_FALSE(m_osc_node.RemoveTarget(TEST_GROUP + 1, target));
}


/**
 * Check that we receive OSC messages correctly.
 */
void OSCNodeTest::testReceive() {
  // Register the test OSC Address with the OSCNode using the DMXHandler as the
  // callback.
  OLA_ASSERT_TRUE(m_osc_node.RegisterAddress(
      TEST_OSC_ADDRESS, NewCallback(this, &OSCNodeTest::DMXHandler)));

  // Attempt to register the same address with a different path, this should
  // return false
  OLA_ASSERT_FALSE(m_osc_node.RegisterAddress(
      TEST_OSC_ADDRESS,
      NewCallback(this, &OSCNodeTest::DMXHandler)));

  // Using our test UDP socket, send the EXPECTED_OSC_PACKET to the default OSC
  // port. The OSCNode should receive the packet and call DMXHandler.
  IPV4SocketAddress dest_address(IPV4Address::Loopback(), 7770);
  m_udp_socket.SendTo(EXPECTED_OSC_PACKET, sizeof(EXPECTED_OSC_PACKET),
                      dest_address);
  // Run the SelectServer, this will return either when DMXHandler
  // completes, or the abort timeout triggers.
  m_ss.Run();

  // De-regsiter
  OLA_ASSERT_TRUE(m_osc_node.RegisterAddress(TEST_OSC_ADDRESS, NULL));
  // De-register a second time
  OLA_ASSERT_TRUE(m_osc_node.RegisterAddress(TEST_OSC_ADDRESS, NULL));
}
