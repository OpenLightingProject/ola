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
 * ArtNetNodeTest.cpp
 * Test fixture for the ArtNetNode class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <queue>
#include <vector>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/Socket.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMReply.h"
#include "ola/rdm/RDMResponseCodes.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/testing/MockUDPSocket.h"
#include "ola/timecode/TimeCodeEnums.h"
#include "ola/timecode/TimeCode.h"
#include "plugins/artnet/ArtNetNode.h"
#include "ola/testing/TestUtils.h"


using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::network::Interface;
using ola::network::MACAddress;
using ola::plugin::artnet::ArtNetNode;
using ola::plugin::artnet::ArtNetNodeOptions;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMGetRequest;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::timecode::TimeCode;
using ola::testing::MockUDPSocket;
using ola::testing::SocketVerifier;
using std::string;
using std::vector;


class ArtNetNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ArtNetNodeTest);
  CPPUNIT_TEST(testBasicBehaviour);
  CPPUNIT_TEST(testConfigurationMode);
  CPPUNIT_TEST(testExtendedInputPorts);
  CPPUNIT_TEST(testBroadcastSendDMX);
  CPPUNIT_TEST(testBroadcastSendDMXZeroUniverse);
  CPPUNIT_TEST(testLimitedBroadcastDMX);
  CPPUNIT_TEST(testNonBroadcastSendDMX);
  CPPUNIT_TEST(testReceiveDMX);
  CPPUNIT_TEST(testReceiveDMXZeroUniverse);
  CPPUNIT_TEST(testHTPMerge);
  CPPUNIT_TEST(testLTPMerge);
  CPPUNIT_TEST(testControllerDiscovery);
  CPPUNIT_TEST(testControllerIncrementalDiscovery);
  CPPUNIT_TEST(testUnsolicitedTod);
  CPPUNIT_TEST(testResponderDiscovery);
  CPPUNIT_TEST(testRDMResponder);
  CPPUNIT_TEST(testRDMRequest);
  CPPUNIT_TEST(testRDMRequestTimeout);
  CPPUNIT_TEST(testRDMRequestIPMismatch);
  CPPUNIT_TEST(testRDMRequestUIDMismatch);
  CPPUNIT_TEST(testTimeCode);
  CPPUNIT_TEST_SUITE_END();

 public:
  ArtNetNodeTest()
      : CppUnit::TestFixture(),
        ss(NULL, &m_clock),
        m_got_dmx(false),
        m_got_rdm_timeout(false),
        m_discovery_done(false),
        m_tod_flush(false),
        m_tod_request(false),
        m_rdm_request(NULL),
        m_rdm_callback(NULL),
        m_rdm_response(NULL),
        m_port_id(1),
        broadcast_ip(IPV4Address::Broadcast()),
        m_socket(new MockUDPSocket()) {
  }
  void setUp();

  void testBasicBehaviour();
  void testConfigurationMode();
  void testExtendedInputPorts();
  void testBroadcastSendDMX();
  void testBroadcastSendDMXZeroUniverse();
  void testLimitedBroadcastDMX();
  void testNonBroadcastSendDMX();
  void testReceiveDMX();
  void testReceiveDMXZeroUniverse();
  void testHTPMerge();
  void testLTPMerge();
  void testControllerDiscovery();
  void testControllerIncrementalDiscovery();
  void testUnsolicitedTod();
  void testResponderDiscovery();
  void testRDMResponder();
  void testRDMRequest();
  void testRDMRequestTimeout();
  void testRDMRequestIPMismatch();
  void testRDMRequestUIDMismatch();
  void testTimeCode();

 private:
  ola::MockClock m_clock;
  ola::io::SelectServer ss;
  bool m_got_dmx;
  bool m_got_rdm_timeout;
  bool m_discovery_done;
  bool m_tod_flush;
  bool m_tod_request;
  UIDSet m_uids;
  const RDMRequest *m_rdm_request;
  RDMCallback *m_rdm_callback;
  const RDMResponse *m_rdm_response;
  uint8_t m_port_id;
  Interface iface;
  IPV4Address peer_ip, peer_ip2, peer_ip3;
  IPV4Address broadcast_ip;
  MockUDPSocket *m_socket;

  /**
   * Called when new DMX arrives
   */
  void NewDmx() { m_got_dmx = true; }

  void DiscoveryComplete(const UIDSet &uids) {
    m_uids = uids;
    m_discovery_done = true;
  }

  void TodRequest() { m_tod_request = true; }
  void Flush() { m_tod_flush = true; }

  void HandleRDM(RDMRequest *request, RDMCallback *callback) {
    m_rdm_request = request;
    m_rdm_callback = callback;
  }

  void FinalizeRDM(RDMReply *reply) {
    OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, reply->StatusCode());
    m_rdm_response = reply->Response()->Duplicate();
  }

  void ExpectTimeout(RDMReply *reply) {
    OLA_ASSERT_EQ(ola::rdm::RDM_TIMEOUT, reply->StatusCode());
    OLA_ASSERT_NULL(reply->Response());
    m_got_rdm_timeout = true;
  }

  void ExpectedSend(const uint8_t *data,
                    unsigned int data_size,
                    const IPV4Address &address) {
    m_socket->AddExpectedData(data, data_size, address, ARTNET_PORT);
  }

  void ExpectedBroadcast(const uint8_t *data, unsigned int data_size) {
    ExpectedSend(data, data_size, iface.bcast_address);
  }

  void ReceiveFromPeer(const uint8_t *data,
                       unsigned int data_size,
                       const IPV4Address &address) {
    ss.RunOnce();  // update the wake up time
    m_socket->InjectData(data, data_size, address, ARTNET_PORT);
  }

  void SetupInputPort(ArtNetNode *node) {
    node->SetNetAddress(4);
    node->SetSubnetAddress(2);
    node->SetInputPortUniverse(m_port_id, 3);
  }

  void SetupOutputPort(ArtNetNode *node) {
    node->SetNetAddress(4);
    node->SetSubnetAddress(2);
    node->SetOutputPortUniverse(m_port_id, 3);
  }

  // This sends a tod data so 7s70:00000000 is insert into the tod
  void PopulateTod() {
    SocketVerifier verifer(m_socket);
    const uint8_t art_tod[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x81,
      0x0, 14,
      1,  // rdm standard
      1,  // first port
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full tod
      0x23,  // universe address
      0, 1,  // uid count
      0,  // block count
      1,  // uid count
      0x7a, 0x70, 0, 0, 0, 0,
    };

    ReceiveFromPeer(art_tod, sizeof(art_tod), peer_ip);
  }

  void SendRDMRequest(ArtNetNode *node,
                      RDMCallback *callback) {
    UID source(1, 2);
    UID destination(0x7a70, 0);

    RDMGetRequest *request = new RDMGetRequest(
        source,
        destination,
        0,  // transaction #
        1,  // port id
        10,  // sub device
        296,  // param id
        NULL,  // data
        0);  // data length

    const uint8_t rdm_request[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x83,
      0x0, 14,
      1, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // process
      0x23,
      // rdm data
      1, 24,  // sub code & length
      0x7a, 0x70, 0, 0, 0, 0,   // dst uid
      0, 1, 0, 0, 0, 2,   // src uid
      0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
      0x20, 0x1, 0x28, 0,  // command, param id, param data length
      0x02, 0x26
    };

    ExpectedSend(rdm_request, sizeof(rdm_request), peer_ip);
    node->SendRDMRequest(m_port_id, request, callback);
  }

  static const uint8_t POLL_MESSAGE[];
  static const uint8_t POLL_REPLY_MESSAGE[];
  static const uint8_t TOD_CONTROL[];
  static const uint16_t ARTNET_PORT = 6454;
};


CPPUNIT_TEST_SUITE_REGISTRATION(ArtNetNodeTest);


const uint8_t ArtNetNodeTest::POLL_MESSAGE[] = {
  'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
  0x00, 0x20,
  0x0, 14,
  0x2, 0
};


const uint8_t ArtNetNodeTest::POLL_REPLY_MESSAGE[] = {
  'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
  0x00, 0x21,
  10, 0, 0, 1,
  0x36, 0x19,
  0, 0,
  4, 2,  // subnet address
  0x4, 0x31,  // oem
  0,
  0xd2,
  0x70, 0x7a,  // esta
  'S', 'h', 'o', 'r', 't', ' ', 'N', 'a', 'm', 'e',
  0, 0, 0, 0, 0, 0, 0, 0,  // short name
  'T', 'h', 'i', 's', ' ', 'i', 's', ' ', 't', 'h', 'e', ' ',
  'v', 'e', 'r', 'y', ' ', 'l', 'o', 'n', 'g', ' ',
  'n', 'a', 'm', 'e',
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
  '#', '0', '0', '0', '1', ' ', '[', '0', ']', ' ', 'O', 'L', 'A',
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0,  // node report
  0, 4,  // num ports
  0xc0, 0xc0, 0xc0, 0xc0,
  8, 8, 8, 8,
  0x80, 0, 0, 0,
  0x20, 0x20, 0x20, 0x20,  // swin
  0x23, 0x20, 0x20, 0x20,  // swout
  0, 0, 0, 0, 0, 0, 0,  // video, macro, remote, spare, style
  0xa, 0xb, 0xc, 0x12, 0x34, 0x56,  // mac address
  0xa, 0x0, 0x0, 0x1,
  0,
  8,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0  // filler
};


const uint8_t ArtNetNodeTest::TOD_CONTROL[] = {
  'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
  0x00, 0x82,
  0x0, 14,
  0, 0,
  0, 0, 0, 0, 0, 0, 0,
  4,  // net
  1,  // flush
  0x23
};

void ArtNetNodeTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  ola::network::InterfaceBuilder interface_builder;
  OLA_ASSERT(interface_builder.SetAddress("10.0.0.1"));
  OLA_ASSERT(interface_builder.SetSubnetMask("255.0.0.0"));
  OLA_ASSERT(interface_builder.SetBroadcast("10.255.255.255"));
  interface_builder.SetHardwareAddress(
      MACAddress::FromStringOrDie("0a:0b:0c:12:34:56"));
  iface = interface_builder.Construct();

  ola::network::IPV4Address::FromString("10.0.0.10", &peer_ip);
  ola::network::IPV4Address::FromString("10.0.0.11", &peer_ip2);
  ola::network::IPV4Address::FromString("10.0.0.12", &peer_ip3);
}


/**
 * Check that the discovery sequence works correctly.
 */
void ArtNetNodeTest::testBasicBehaviour() {
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);

  node.SetShortName("Short Name");
  OLA_ASSERT_EQ(string("Short Name"), node.ShortName());
  node.SetLongName("This is the very long name");
  OLA_ASSERT_EQ(string("This is the very long name"), node.LongName());
  node.SetNetAddress(4);
  OLA_ASSERT_EQ((uint8_t) 4, node.NetAddress());
  node.SetSubnetAddress(2);
  OLA_ASSERT_EQ((uint8_t) 2, node.SubnetAddress());

  node.SetOutputPortUniverse(0, 3);
  OLA_ASSERT(!node.SetOutputPortUniverse(4, 3));
  OLA_ASSERT_EQ((uint8_t) 0x23, node.GetOutputPortUniverse(0));
  OLA_ASSERT_EQ((uint8_t) 0x20, node.GetOutputPortUniverse(1));

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  OLA_ASSERT(m_socket->CheckNetworkParamsMatch(true, true, 6454, true));

  // check port states
  OLA_ASSERT_EQ((uint8_t) 4, node.InputPortCount());
  OLA_ASSERT_FALSE(node.InputPortState(0));
  OLA_ASSERT_FALSE(node.InputPortState(1));
  OLA_ASSERT_FALSE(node.InputPortState(2));
  OLA_ASSERT_FALSE(node.InputPortState(3));
  OLA_ASSERT(node.OutputPortState(0));
  OLA_ASSERT_FALSE(node.OutputPortState(1));
  OLA_ASSERT_FALSE(node.OutputPortState(2));
  OLA_ASSERT_FALSE(node.OutputPortState(3));

  // enable an input port and check that we send a poll
  ExpectedBroadcast(POLL_MESSAGE, sizeof(POLL_MESSAGE));

  // we should see an unsolicited poll reply sent because conditions have
  // changed.
  uint8_t expected_poll_reply_packet[sizeof(POLL_REPLY_MESSAGE)];
  memcpy(expected_poll_reply_packet, POLL_REPLY_MESSAGE,
         sizeof(POLL_REPLY_MESSAGE));
  expected_poll_reply_packet[115] = '1';  // node report
  expected_poll_reply_packet[179] = 0;  // good input
  expected_poll_reply_packet[187] = 0x22;  // swin

  ExpectedBroadcast(expected_poll_reply_packet,
                    sizeof(expected_poll_reply_packet));

  node.SetInputPortUniverse(1, 2);
  OLA_ASSERT_EQ((uint8_t) 0x20, node.GetInputPortUniverse(0));
  OLA_ASSERT_EQ((uint8_t) 0x22, node.GetInputPortUniverse(1));
  m_socket->Verify();

  // check sending a poll works
  ExpectedBroadcast(POLL_MESSAGE, sizeof(POLL_MESSAGE));
  OLA_ASSERT(node.SendPoll());
  m_socket->Verify();

  OLA_ASSERT(node.Stop());
}


/**
 * Check that configuration mode works correctly.
 */
void ArtNetNodeTest::testConfigurationMode() {
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();

  // no changes should cause no messages
  node.EnterConfigurationMode();
  node.EnterConfigurationMode();  // enter a second time
  node.ExitConfigurationMode();
  m_socket->Verify();

  // exit again just to make sure
  node.ExitConfigurationMode();
  m_socket->Verify();

  uint8_t poll_reply_message[] = {
    'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
    0x00, 0x21,
    10, 0, 0, 1,
    0x36, 0x19,
    0, 0,
    0, 0,  // subnet address
    0x4, 0x31,  // oem
    0,
    0xd2,
    0x70, 0x7a,  // esta
    'S', 'h', 'o', 'r', 't', ' ', 'N', 'a', 'm', 'e',
    0, 0, 0, 0, 0, 0, 0, 0,  // short name
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
    '#', '0', '0', '0', '1', ' ', '[', '1', ']', ' ', 'O', 'L', 'A',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  // node report
    0, 4,  // num ports
    0xc0, 0xc0, 0xc0, 0xc0,  // port types
    8, 8, 8, 8,  // good input
    0, 0, 0, 0,  // good output
    0x0, 0x0, 0x0, 0x0,  // swin
    0x0, 0x0, 0x0, 0x0,  // swout
    0, 0, 0, 0, 0, 0, 0,  // video, macro, remote, spare, style
    0xa, 0xb, 0xc, 0x12, 0x34, 0x56,  // mac address
    0xa, 0x0, 0x0, 0x1,
    0,
    8,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0  // filler
  };

  node.EnterConfigurationMode();
  node.SetShortName("Short Name");
  m_socket->Verify();
  OLA_ASSERT_EQ(string("Short Name"), node.ShortName());
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  node.EnterConfigurationMode();
  const string long_name("This is a long name");
  node.SetLongName(long_name);
  m_socket->Verify();
  OLA_ASSERT_EQ(long_name, node.LongName());
  strncpy(reinterpret_cast<char*>(&poll_reply_message[44]), long_name.c_str(),
          long_name.size());
  poll_reply_message[115] = '2';
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  node.EnterConfigurationMode();
  node.SetNetAddress(4);
  m_socket->Verify();
  OLA_ASSERT_EQ((uint8_t) 4, node.NetAddress());
  poll_reply_message[18] = 4;
  poll_reply_message[115] = '3';
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  node.EnterConfigurationMode();
  node.SetSubnetAddress(2);
  OLA_ASSERT_EQ((uint8_t) 2, node.SubnetAddress());
  poll_reply_message[19] = 2;
  poll_reply_message[115] = '4';
  for (unsigned int i = 186; i <= 193; i++) {
    poll_reply_message[i] = 0x20;
  }
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  node.EnterConfigurationMode();
  node.SetOutputPortUniverse(0, 3);
  OLA_ASSERT(!node.SetOutputPortUniverse(4, 3));
  OLA_ASSERT(node.OutputPortState(0));
  OLA_ASSERT_EQ((uint8_t) 0x23, node.GetOutputPortUniverse(0));
  poll_reply_message[182] = 0x80;
  poll_reply_message[190] = 0x23;
  poll_reply_message[115] = '5';
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  // now try an input port, this should trigger a poll
  node.EnterConfigurationMode();
  node.SetInputPortUniverse(0, 2);
  OLA_ASSERT(node.OutputPortState(0));
  OLA_ASSERT_EQ((uint8_t) 0x22, node.GetInputPortUniverse(0));
  poll_reply_message[178] = 0;
  poll_reply_message[186] = 0x22;
  poll_reply_message[115] = '6';
  ExpectedBroadcast(POLL_MESSAGE, sizeof(POLL_MESSAGE));
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  // change the subnet, which should trigger another poll
  node.EnterConfigurationMode();
  node.SetSubnetAddress(4);
  poll_reply_message[19] = 4;
  poll_reply_message[186] = 0x42;
  poll_reply_message[187] = 0x40;
  poll_reply_message[188] = 0x40;
  poll_reply_message[189] = 0x40;
  poll_reply_message[190] = 0x43;
  poll_reply_message[191] = 0x40;
  poll_reply_message[192] = 0x40;
  poll_reply_message[193] = 0x40;
  poll_reply_message[115] = '7';
  ExpectedBroadcast(POLL_MESSAGE, sizeof(POLL_MESSAGE));
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  // if nothing changes, no messages are sent
  node.EnterConfigurationMode();
  node.SetShortName("Short Name");
  node.SetLongName(long_name);
  node.SetNetAddress(4);
  node.SetSubnetAddress(4);
  node.SetOutputPortUniverse(0, 3);
  node.SetInputPortUniverse(0, 2);
  node.ExitConfigurationMode();
  m_socket->Verify();

  // disable input port
  node.EnterConfigurationMode();
  node.DisableInputPort(0);
  poll_reply_message[115] = '8';
  poll_reply_message[178] = 0x8;
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  // disable output port
  node.EnterConfigurationMode();
  node.DisableOutputPort(0);
  poll_reply_message[115] = '9';
  poll_reply_message[182] = 0;
  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.ExitConfigurationMode();
  m_socket->Verify();

  OLA_ASSERT(node.Stop());
}


/**
 * Check a node with more than the default number of input ports.
 */
void ArtNetNodeTest::testExtendedInputPorts() {
  ArtNetNodeOptions node_options;
  node_options.input_port_count = 8;
  ArtNetNode node(iface, &ss, node_options, m_socket);

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();

  OLA_ASSERT_EQ((uint8_t) 8, node.InputPortCount());
  OLA_ASSERT_FALSE(node.InputPortState(0));
  OLA_ASSERT_FALSE(node.InputPortState(1));
  OLA_ASSERT_FALSE(node.InputPortState(2));
  OLA_ASSERT_FALSE(node.InputPortState(3));
  OLA_ASSERT_FALSE(node.InputPortState(4));
  OLA_ASSERT_FALSE(node.InputPortState(5));
  OLA_ASSERT_FALSE(node.InputPortState(6));
  OLA_ASSERT_FALSE(node.InputPortState(6));
  OLA_ASSERT_FALSE(node.InputPortState(7));
  OLA_ASSERT_FALSE(node.OutputPortState(0));
  OLA_ASSERT_FALSE(node.OutputPortState(1));
  OLA_ASSERT_FALSE(node.OutputPortState(2));
  OLA_ASSERT_FALSE(node.OutputPortState(3));
  OLA_ASSERT_FALSE(node.OutputPortState(4));
  OLA_ASSERT_FALSE(node.OutputPortState(5));
  OLA_ASSERT_FALSE(node.OutputPortState(6));
  OLA_ASSERT_FALSE(node.OutputPortState(7));

  // no changes should cause no messages
  node.EnterConfigurationMode();
  node.ExitConfigurationMode();
  m_socket->Verify();
}


/**
 * Check sending DMX using broadcast works.
 */
void ArtNetNodeTest::testBroadcastSendDMX() {
  m_socket->SetDiscardMode(true);

  ArtNetNodeOptions node_options;
  node_options.always_broadcast = true;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  {
    SocketVerifier verifer(m_socket);
    const uint8_t DMX_MESSAGE[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      0, 1, 2, 3, 4, 5
    };
    ExpectedBroadcast(DMX_MESSAGE, sizeof(DMX_MESSAGE));

    DmxBuffer dmx;
    dmx.SetFromString("0,1,2,3,4,5");
    OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  }

  // send an odd sized dmx frame, we should pad this to a multiple of two
  {
    SocketVerifier verifer(m_socket);
    const uint8_t DMX_MESSAGE2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      1,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      0, 1, 2, 3, 4, 0
    };
    ExpectedBroadcast(DMX_MESSAGE2, sizeof(DMX_MESSAGE2));
    DmxBuffer dmx;
    dmx.SetFromString("0,1,2,3,4");
    OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  }

  {  // attempt to send on a invalid port
    SocketVerifier verifer(m_socket);
    DmxBuffer dmx;
    dmx.SetFromString("0,1,2,3,4");
    OLA_ASSERT_FALSE(node.SendDMX(4, dmx));
  }

  {  // attempt to send an empty frame
    SocketVerifier verifer(m_socket);
    DmxBuffer empty_buffer;
    OLA_ASSERT(node.SendDMX(m_port_id, empty_buffer));
  }
}

/**
 * Check sending DMX using broadcast works to Art-Net universe 0.
 */
void ArtNetNodeTest::testBroadcastSendDMXZeroUniverse() {
  m_socket->SetDiscardMode(true);

  ArtNetNodeOptions node_options;
  node_options.always_broadcast = true;
  ArtNetNode node(iface, &ss, node_options, m_socket);

  node.SetNetAddress(0);
  node.SetSubnetAddress(0);
  node.SetInputPortUniverse(m_port_id, 0);

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  {
    SocketVerifier verifer(m_socket);
    const uint8_t DMX_MESSAGE[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0, 0,  // subnet & net address
      0, 6,  // dmx length
      0, 1, 2, 3, 4, 5
    };
    ExpectedBroadcast(DMX_MESSAGE, sizeof(DMX_MESSAGE));

    DmxBuffer dmx;
    dmx.SetFromString("0,1,2,3,4,5");
    OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  }

  // Now disable, and set to a different universe.
  {
    uint8_t poll_reply_message[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x21,
      10, 0, 0, 1,
      0x36, 0x19,
      0, 0,
      0, 0,  // subnet address
      0x4, 0x31,  // oem
      0,
      0xd2,
      0x70, 0x7a,  // esta
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,  // short name
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
      '#', '0', '0', '0', '1', ' ', '[', '1', ']', ' ', 'O', 'L', 'A',
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0,  // node report
      0, 4,  // num ports
      0xc0, 0xc0, 0xc0, 0xc0,  // port types
      8, 0, 8, 8,  // good input
      0, 0, 0, 0,  // good output
      0x0, 0x0, 0x0, 0x0,  // swin
      0x0, 0x0, 0x0, 0x0,  // swout
      0, 0, 0, 0, 0, 0, 0,  // video, macro, remote, spare, style
      0xa, 0xb, 0xc, 0x12, 0x34, 0x56,  // mac address
      0xa, 0x0, 0x0, 0x1,
      0,
      8,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0  // filler
    };
    ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));

    node.EnterConfigurationMode();
    node.DisableInputPort(m_port_id);
    node.SetInputPortUniverse(m_port_id, 0);
    node.ExitConfigurationMode();
    m_socket->Verify();
  }

  {
    SocketVerifier verifer(m_socket);
    const uint8_t DMX_MESSAGE[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      1,  // seq #
      1,  // physical port
      0, 0,  // subnet & net address
      0, 6,  // dmx length
      10, 11, 12, 13, 14, 15
    };
    ExpectedBroadcast(DMX_MESSAGE, sizeof(DMX_MESSAGE));

    DmxBuffer dmx;
    dmx.SetFromString("10,11,12,13,14,15");
    OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  }
}

/*
 * Check sending DMX using the limited broadcast address.
 */
void ArtNetNodeTest::testLimitedBroadcastDMX() {
  m_socket->SetDiscardMode(true);

  ArtNetNodeOptions node_options;
  node_options.always_broadcast = true;
  node_options.use_limited_broadcast_address = true;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  {
    SocketVerifier verifer(m_socket);
    const uint8_t DMX_MESSAGE[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      0, 1, 2, 3, 4, 5
    };
    ExpectedSend(DMX_MESSAGE, sizeof(DMX_MESSAGE), broadcast_ip);

    DmxBuffer dmx;
    dmx.SetFromString("0,1,2,3,4,5");
    OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  }
}


/**
 * Check sending DMX using unicast works.
 */
void ArtNetNodeTest::testNonBroadcastSendDMX() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  DmxBuffer dmx;
  dmx.SetFromString("0,1,2,3,4,5");
  // we don't expect any data here because there are no nodes active
  OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  m_socket->Verify();

  // used to check GetSubscribedNodes()
  vector<IPV4Address> node_addresses;

  {
    SocketVerifier verifer(m_socket);
    const uint8_t poll_reply_message[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x21,
      10, 0, 0, 10,
      0x36, 0x19,
      0, 0,
      4, 2,  // subnet address
      0x4, 0x31,  // oem
      0,
      0xd2,
      0x70, 0x7a,  // esta
      'P', 'e', 'e', 'r', ' ', '1', 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,  // short name
      'T', 'h', 'i', 's', ' ', 'i', 's', ' ', 't', 'h', 'e', ' ',
      'v', 'e', 'r', 'y', ' ', 'l', 'o', 'n', 'g', ' ',
      'n', 'a', 'm', 'e',
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
      '#', '0', '0', '0', '1', ' ', '[', '0', ']', ' ', 'O', 'L', 'A',
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,
      0,  // node report
      0, 4,  // num ports
      0x80, 0x80, 0x80, 0x80,  // 4 output ports
      8, 8, 8, 8,
      0, 0, 0, 0,
      0x0, 0x0, 0x0, 0x0,  // swin
      0x23, 0x0, 0x0, 0x0,  // swout
      0, 0, 0, 0, 0, 0, 0,  // video, macro, remote, spare, style
      0x12, 0x34, 0x56, 0x12, 0x34, 0x56,  // mac address
      0xa, 0x0, 0x0, 0xa,
      0,
      8,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0  // filler
    };

    // Fake an ArtPollReply
    ReceiveFromPeer(poll_reply_message, sizeof(poll_reply_message), peer_ip);

    // check the node list is up to date
    node_addresses.clear();
    node.GetSubscribedNodes(m_port_id, &node_addresses);
    OLA_ASSERT_EQ(static_cast<size_t>(1), node_addresses.size());
    OLA_ASSERT_EQ(peer_ip, node_addresses[0]);

    // send a DMX frame, this should get unicast
    const uint8_t DMX_MESSAGE[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      0, 1, 2, 3, 4, 5
    };
    ExpectedSend(DMX_MESSAGE, sizeof(DMX_MESSAGE), peer_ip);
    OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  }

  // add another peer
  {
    SocketVerifier verifer(m_socket);
    const uint8_t poll_reply_message2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x21,
      10, 0, 0, 11,
      0x36, 0x19,
      0, 0,
      4, 2,  // subnet address
      0x4, 0x31,  // oem
      0,
      0xd2,
      0x70, 0x7a,  // esta
      'P', 'e', 'e', 'r', ' ', '2', 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,  // short name
      'T', 'h', 'i', 's', ' ', 'i', 's', ' ', 't', 'h', 'e', ' ',
      'v', 'e', 'r', 'y', ' ', 'l', 'o', 'n', 'g', ' ',
      'n', 'a', 'm', 'e',
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
      '#', '0', '0', '0', '1', ' ', '[', '0', ']', ' ', 'O', 'L', 'A',
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,
      0,  // node report
      0, 4,  // num ports
      0x80, 0x80, 0x80, 0x80,  // 4 output ports
      8, 8, 8, 8,
      0, 0, 0, 0,
      0x0, 0x0, 0x0, 0x0,  // swin
      0x23, 0x0, 0x0, 0x0,  // swout
      0, 0, 0, 0, 0, 0, 0,  // video, macro, remote, spare, style
      0x12, 0x34, 0x56, 0x12, 0x34, 0x56,  // mac address
      0xa, 0x0, 0x0, 0xb,
      0,
      8,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0  // filler
    };

    // Fake an ArtPollReply
    ReceiveFromPeer(poll_reply_message2,
                    sizeof(poll_reply_message2),
                    peer_ip2);

    // check the node list is up to date
    node_addresses.clear();
    node.GetSubscribedNodes(m_port_id, &node_addresses);
    OLA_ASSERT_EQ(static_cast<size_t>(2), node_addresses.size());
    OLA_ASSERT_EQ(peer_ip, node_addresses[0]);
    OLA_ASSERT_EQ(peer_ip2, node_addresses[1]);
  }

  // send another DMX frame, this should get unicast twice
  {
    SocketVerifier verifer(m_socket);
    const uint8_t DMX_MESSAGE2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      1,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      10, 11, 12, 0, 1, 2
    };
    dmx.SetFromString("10,11,12,0,1,2");
    ExpectedSend(DMX_MESSAGE2, sizeof(DMX_MESSAGE2), peer_ip);
    ExpectedSend(DMX_MESSAGE2, sizeof(DMX_MESSAGE2), peer_ip2);
    OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  }

  // adjust the broadcast threshold
  {
    SocketVerifier verifer(m_socket);
    node.SetBroadcastThreshold(2);

    // send another DMX frame, this should get broadcast
    const uint8_t DMX_MESSAGE3[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      2,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      11, 13, 14, 7, 8, 9
    };
    dmx.SetFromString("11,13,14,7,8,9");
    ExpectedBroadcast(DMX_MESSAGE3, sizeof(DMX_MESSAGE3));
    OLA_ASSERT(node.SendDMX(m_port_id, dmx));
  }
}

/**
 * Check that receiving DMX works
 */
void ArtNetNodeTest::testReceiveDMX() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupOutputPort(&node);
  DmxBuffer input_buffer;
  node.SetDMXHandler(m_port_id,
                     &input_buffer,
                     ola::NewCallback(this, &ArtNetNodeTest::NewDmx));

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  uint8_t DMX_MESSAGE[] = {
    'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
    0x00, 0x50,
    0x0, 14,
    0,  // seq #
    1,  // physical port
    0x23, 4,  // subnet & net address
    0, 6,  // dmx length
    0, 1, 2, 3, 4, 5
  };

  // 'receive' a DMX message
  {
    SocketVerifier verifer(m_socket);
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(DMX_MESSAGE, sizeof(DMX_MESSAGE), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("0,1,2,3,4,5"), input_buffer.ToString());
  }

  // send a second frame
  {
    SocketVerifier verifer(m_socket);
    uint8_t DMX_MESSAGE2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      1,  // different seq # this time
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      5, 4, 3, 2, 1, 0
    };

    m_got_dmx = false;
    ReceiveFromPeer(DMX_MESSAGE2, sizeof(DMX_MESSAGE2), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("5,4,3,2,1,0"), input_buffer.ToString());
  }

  // advance the clock by more than the merge timeout (10s)
  {
    SocketVerifier verifer(m_socket);
    m_clock.AdvanceTime(11, 0);

    // send another message, but first update the seq #
    DMX_MESSAGE[12] = 2;

    m_got_dmx = false;
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(DMX_MESSAGE, sizeof(DMX_MESSAGE), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("0,1,2,3,4,5"), input_buffer.ToString());
  }
}

/**
 * Check that receiving DMX for universe 0 works.
 */
void ArtNetNodeTest::testReceiveDMXZeroUniverse() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);

  node.SetNetAddress(0);
  node.SetSubnetAddress(0);
  node.SetOutputPortUniverse(m_port_id, 0);

  DmxBuffer input_buffer;
  node.SetDMXHandler(m_port_id,
                     &input_buffer,
                     ola::NewCallback(this, &ArtNetNodeTest::NewDmx));

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);


  // 'receive' a DMX message
  {
    uint8_t DMX_MESSAGE[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0, 0,  // subnet & net address
      0, 6,  // dmx length
      0, 1, 2, 3, 4, 5
    };
    SocketVerifier verifer(m_socket);
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(DMX_MESSAGE, sizeof(DMX_MESSAGE), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("0,1,2,3,4,5"), input_buffer.ToString());
  }

  // Now disable, and set to a different universe.
  {
    node.EnterConfigurationMode();
    node.DisableInputPort(m_port_id);
    node.SetInputPortUniverse(m_port_id, 0);
    node.ExitConfigurationMode();
    m_socket->Verify();
  }

  m_got_dmx = false;

  // 'receive' another DMX message
  {
    uint8_t DMX_MESSAGE[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      1,  // seq #
      1,  // physical port
      0, 0,  // subnet & net address
      0, 4,  // dmx length
      10, 11, 12, 13
    };
    SocketVerifier verifer(m_socket);
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(DMX_MESSAGE, sizeof(DMX_MESSAGE), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("10,11,12,13"), input_buffer.ToString());
  }
}

/**
 * Check that merging works
 */
void ArtNetNodeTest::testHTPMerge() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupOutputPort(&node);
  DmxBuffer input_buffer;
  node.SetDMXHandler(m_port_id,
                     &input_buffer,
                     ola::NewCallback(this, &ArtNetNodeTest::NewDmx));

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // 'receive' a DMX message from the first peer
  {
    SocketVerifier verifer(m_socket);
    uint8_t source1_message1[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      0, 1, 2, 3, 4, 5
    };

    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(source1_message1, sizeof(source1_message1), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("0,1,2,3,4,5"), input_buffer.ToString());
  }

  // receive a message from a second peer
  {
    SocketVerifier verifer(m_socket);
    uint8_t source2_message1[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      5, 4, 3, 2, 1, 0
    };

    m_got_dmx = false;
    OLA_ASSERT_FALSE(m_got_dmx);

    // this will engage merge mode, so the node will send an ArtPolReply
    uint8_t poll_reply_message[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x21,
      10, 0, 0, 1,
      0x36, 0x19,
      0, 0,
      4, 2,  // subnet address
      0x4, 0x31,  // oem
      0,
      0xd2,
      0x70, 0x7a,  // esta
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // short name
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
      '#', '0', '0', '0', '1', ' ', '[', '1', ']', ' ', 'O', 'L', 'A',
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,
      0,  // node report
      0, 4,  // num ports
      0xc0, 0xc0, 0xc0, 0xc0,
      8, 8, 8, 8,
      0, 0x88, 0, 0,  // 0x88 indicates we're merging data
      0x20, 0x20, 0x20, 0x20,  // swin
      0x20, 0x23, 0x20, 0x20,  // swout
      0, 0, 0, 0, 0, 0, 0,  // video, macro, remote, spare, style
      0xa, 0xb, 0xc, 0x12, 0x34, 0x56,  // mac address
      0xa, 0x0, 0x0, 0x1,
      0,
      8,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0  // filler
    };

    ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));

    ReceiveFromPeer(source2_message1, sizeof(source2_message1), peer_ip2);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("5,4,3,3,4,5"), input_buffer.ToString());
  }

  // send a packet from a third source, this shouldn't result in any new dmx
  {
    SocketVerifier verifer(m_socket);
    const uint8_t source3_message1[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 4,  // dmx length
      255, 255, 255, 0
    };
    m_got_dmx = false;
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(source3_message1, sizeof(source3_message1), peer_ip3);
    OLA_ASSERT_FALSE(m_got_dmx);
    OLA_ASSERT_EQ(string("5,4,3,3,4,5"), input_buffer.ToString());
  }

  // send another packet from the first source
  {
    SocketVerifier verifer(m_socket);
    uint8_t source1_message2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      1,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 8,  // dmx length
      10, 11, 12, 1, 2, 1, 0, 0
    };

    m_got_dmx = false;
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(source1_message2, sizeof(source1_message2), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("10,11,12,2,2,1,0,0"),
                  input_buffer.ToString());
  }

  // advance the clock by half the merge timeout
  m_clock.AdvanceTime(5, 0);

  // send another packet from the first source
  {
    SocketVerifier verifer(m_socket);
    uint8_t source1_message3[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      2,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 8,  // dmx length
      0, 1, 2, 3, 4, 5, 7, 9
    };

    m_got_dmx = false;
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(source1_message3, sizeof(source1_message3), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("5,4,3,3,4,5,7,9"),
                  input_buffer.ToString());
  }

  // advance the clock so the second source times out
  m_clock.AdvanceTime(6, 0);

  // send another packet from the first source
  {
    SocketVerifier verifer(m_socket);
    uint8_t source1_message4[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      3,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 8,  // dmx length
      0, 1, 2, 3, 4, 5, 7, 9
    };

    m_got_dmx = false;
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(source1_message4, sizeof(source1_message4), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("0,1,2,3,4,5,7,9"),
                  input_buffer.ToString());
  }
}


/**
 * Check that LTP merging works
 */
void ArtNetNodeTest::testLTPMerge() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupOutputPort(&node);
  DmxBuffer input_buffer;
  node.SetDMXHandler(m_port_id,
                     &input_buffer,
                     ola::NewCallback(this, &ArtNetNodeTest::NewDmx));

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // switch to LTP merge mode, this will trigger an art poll reply
  {
    SocketVerifier verifer(m_socket);
    uint8_t poll_reply_message[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x21,
      10, 0, 0, 1,
      0x36, 0x19,
      0, 0,
      4, 2,  // subnet address
      0x4, 0x31,  // oem
      0,
      0xd2,
      0x70, 0x7a,  // esta
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // short name
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
      '#', '0', '0', '0', '1', ' ', '[', '1', ']', ' ', 'O', 'L', 'A',
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,
      0,  // node report
      0, 4,  // num ports
      0xc0, 0xc0, 0xc0, 0xc0,
      8, 8, 8, 8,
      0, 0x82, 0, 0,  // 0x82 indicates we're configured for LTP merge
      0x20, 0x20, 0x20, 0x20,  // swin
      0x20, 0x23, 0x20, 0x20,  // swout
      0, 0, 0, 0, 0, 0, 0,  // video, macro, remote, spare, style
      0xa, 0xb, 0xc, 0x12, 0x34, 0x56,  // mac address
      0xa, 0x0, 0x0, 0x1,
      0,
      8,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0  // filler
    };

    ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
    node.SetMergeMode(m_port_id, ola::plugin::artnet::ARTNET_MERGE_LTP);
  }

  // 'receive' a DMX message from the first peer
  {
    SocketVerifier verifer(m_socket);
    uint8_t source1_message1[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      0, 1, 2, 3, 4, 5
    };

    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(source1_message1, sizeof(source1_message1), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("0,1,2,3,4,5"), input_buffer.ToString());
  }

  // receive a message from a second peer
  {
    SocketVerifier verifer(m_socket);
    uint8_t source2_message1[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      0,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 6,  // dmx length
      5, 4, 3, 2, 1, 0
    };

    m_got_dmx = false;
    OLA_ASSERT_FALSE(m_got_dmx);

    // this will engage merge mode, so the node will send another ArtPolReply
    uint8_t poll_reply_message2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x21,
      10, 0, 0, 1,
      0x36, 0x19,
      0, 0,
      4, 2,  // subnet address
      0x4, 0x31,  // oem
      0,
      0xd2,
      0x70, 0x7a,  // esta
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // short name
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
      '#', '0', '0', '0', '1', ' ', '[', '2', ']', ' ', 'O', 'L', 'A',
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,
      0,  // node report
      0, 4,  // num ports
      0xc0, 0xc0, 0xc0, 0xc0,
      8, 8, 8, 8,
      0, 0x8a, 0, 0,  // 0x8a indicates we're LTP merging
      0x20, 0x20, 0x20, 0x20,  // swin
      0x20, 0x23, 0x20, 0x20,  // swout
      0, 0, 0, 0, 0, 0, 0,  // video, macro, remote, spare, style
      0xa, 0xb, 0xc, 0x12, 0x34, 0x56,  // mac address
      0xa, 0x0, 0x0, 0x1,
      0,
      8,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0  // filler
    };

    ExpectedBroadcast(poll_reply_message2, sizeof(poll_reply_message2));

    ReceiveFromPeer(source2_message1, sizeof(source2_message1), peer_ip2);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("5,4,3,2,1,0"), input_buffer.ToString());
  }

  // advance the clock so the second source times out
  m_clock.AdvanceTime(11, 0);

  // send another packet from the first source
  {
    SocketVerifier verifer(m_socket);
    uint8_t source1_message2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x50,
      0x0, 14,
      1,  // seq #
      1,  // physical port
      0x23, 4,  // subnet & net address
      0, 8,  // dmx length
      0, 1, 2, 3, 4, 5, 7, 9
    };

    m_got_dmx = false;
    OLA_ASSERT_FALSE(m_got_dmx);
    ReceiveFromPeer(source1_message2, sizeof(source1_message2), peer_ip);
    OLA_ASSERT(m_got_dmx);
    OLA_ASSERT_EQ(string("0,1,2,3,4,5,7,9"),
                  input_buffer.ToString());
  }
}


/**
 * Check the node can act as an RDM controller.
 */
void ArtNetNodeTest::testControllerDiscovery() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  UID uid1(0x7a70, 0);
  UID uid2(0x7a70, 1);
  UID uid3(0x7a70, 2);

  // send a tod control
  {
    SocketVerifier verifer(m_socket);
    ExpectedBroadcast(TOD_CONTROL, sizeof(TOD_CONTROL));
    node.RunFullDiscovery(
        m_port_id,
        ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
    OLA_ASSERT_FALSE(m_discovery_done);
  }

  // advance the clock and run the select server
  {
    SocketVerifier verifer(m_socket);
    m_clock.AdvanceTime(5, 0);  // tod timeout is 4s
    ss.RunOnce();  // update the wake up time
    OLA_ASSERT(m_discovery_done);

    UIDSet uids;
    OLA_ASSERT_EQ(uids, m_uids);
  }

  // run discovery again, this time returning a ArtTod from a peer
  {
    SocketVerifier verifer(m_socket);
    m_discovery_done = false;
    ExpectedBroadcast(TOD_CONTROL, sizeof(TOD_CONTROL));

    node.RunFullDiscovery(
        m_port_id,
        ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
    OLA_ASSERT_FALSE(m_discovery_done);

    // send a ArtTod
    const uint8_t art_tod1[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x81,
      0x0, 14,
      1,  // rdm standard
      1,  // first port
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full tod
      0x23,  // universe address
      0, 3,  // uid count
      0,  // block count
      3,  // uid count
      0x7a, 0x70, 0, 0, 0, 0,
      0x7a, 0x70, 0, 0, 0, 1,
      0x7a, 0x70, 0, 0, 0, 2,
    };

    ReceiveFromPeer(art_tod1, sizeof(art_tod1), peer_ip);
    OLA_ASSERT_FALSE(m_discovery_done);
  }

  // advance the clock and run the select server
  {
    SocketVerifier verifer(m_socket);
    m_clock.AdvanceTime(5, 0);  // tod timeout is 4s
    ss.RunOnce();  // update the wake up time
    OLA_ASSERT(m_discovery_done);

    UIDSet uids;
    uids.AddUID(uid1);
    uids.AddUID(uid2);
    uids.AddUID(uid3);
    OLA_ASSERT_EQ(uids, m_uids);
  }

  // run discovery again, removing one UID, and moving another from peer1
  // to peer2
  {
    SocketVerifier verifer(m_socket);
    m_discovery_done = false;

    ExpectedBroadcast(TOD_CONTROL, sizeof(TOD_CONTROL));

    node.RunFullDiscovery(
        m_port_id,
        ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
    OLA_ASSERT_FALSE(m_discovery_done);

    // send a ArtTod
    const uint8_t art_tod2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x81,
      0x0, 14,
      1,  // rdm standard
      1,  // first port
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full tod
      0x23,  // universe address
      0, 1,  // uid count
      0,  // block count
      1,  // uid count
      0x7a, 0x70, 0, 0, 0, 0,
    };

    ReceiveFromPeer(art_tod2, sizeof(art_tod2), peer_ip);

    const uint8_t art_tod3[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x81,
      0x0, 14,
      1,  // rdm standard
      1,  // first port
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full tod
      0x23,  // universe address
      0, 1,  // uid count
      0,  // block count
      1,  // uid count
      0x7a, 0x70, 0, 0, 0, 1,
    };

    ReceiveFromPeer(art_tod3, sizeof(art_tod3), peer_ip2);
    OLA_ASSERT_FALSE(m_discovery_done);
  }

  // advance the clock and run the select server
  {
    SocketVerifier verifer(m_socket);
    m_clock.AdvanceTime(5, 0);  // tod timeout is 4s
    ss.RunOnce();  // update the wake up time
    OLA_ASSERT(m_discovery_done);

    UIDSet uids;
    uids.AddUID(uid1);
    uids.AddUID(uid2);
    OLA_ASSERT_EQ(uids, m_uids);
  }

  // try running discovery for a invalid port id
  {
    SocketVerifier verifer(m_socket);
    m_discovery_done = false;
    node.RunFullDiscovery(
        4,
        ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
    OLA_ASSERT(m_discovery_done);
    UIDSet uids;
    OLA_ASSERT_EQ(uids, m_uids);
  }
}


/**
 * Check that incremental discovery works
 */
void ArtNetNodeTest::testControllerIncrementalDiscovery() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // send a tod request
  {
    SocketVerifier verifer(m_socket);
    const uint8_t tod_request[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x80,
      0x0, 14,
      0, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full
      1,  // universe array size
      0x23,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    ExpectedBroadcast(tod_request, sizeof(tod_request));

    node.RunIncrementalDiscovery(
        m_port_id,
        ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
    OLA_ASSERT_FALSE(m_discovery_done);
  }

  // respond with a tod
  {
    SocketVerifier verifer(m_socket);
    const uint8_t art_tod1[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x81,
      0x0, 14,
      1,  // rdm standard
      1,  // first port
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full tod
      0x23,  // universe address
      0, 1,  // uid count
      0,  // block count
      1,  // uid count
      0x7a, 0x70, 0, 0, 0, 0,
    };

    ReceiveFromPeer(art_tod1, sizeof(art_tod1), peer_ip);
    OLA_ASSERT_FALSE(m_discovery_done);

    // advance the clock and run the select server
    m_clock.AdvanceTime(5, 0);  // tod timeout is 4s
    ss.RunOnce();  // update the wake up time
    OLA_ASSERT(m_discovery_done);

    UIDSet uids;
    UID uid1(0x7a70, 0);
    uids.AddUID(uid1);
    OLA_ASSERT_EQ(uids, m_uids);
  }

  // try running discovery for a invalid port id
  {
    SocketVerifier verifer(m_socket);
    m_discovery_done = false;
    node.RunIncrementalDiscovery(
        4,
        ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
    OLA_ASSERT(m_discovery_done);
    UIDSet uids;
    OLA_ASSERT_EQ(uids, m_uids);
  }
}


/**
 * Check that unsolicated TOD messages work
 */
void ArtNetNodeTest::testUnsolicitedTod() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);
  OLA_ASSERT(node.SetUnsolicitedUIDSetHandler(
      m_port_id,
      ola::NewCallback(this, &ArtNetNodeTest::DiscoveryComplete)));

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // receive a tod
  {
    SocketVerifier verifer(m_socket);
    OLA_ASSERT_FALSE(m_discovery_done);

    // receive a ArtTod
    const uint8_t art_tod[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x81,
      0x0, 14,
      1,  // rdm standard
      1,  // first port
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full tod
      0x23,  // universe address
      0, 1,  // uid count
      0,  // block count
      1,  // uid count
      0x7a, 0x70, 0, 0, 0, 0,
    };

    ReceiveFromPeer(art_tod, sizeof(art_tod), peer_ip);

    OLA_ASSERT(m_discovery_done);
    UIDSet uids;
    UID uid1(0x7a70, 0);
    uids.AddUID(uid1);
  }
}


/**
 * Check that we respond to Tod messages
 */
void ArtNetNodeTest::testResponderDiscovery() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupOutputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  OLA_ASSERT(node.SetOutputPortRDMHandlers(
      m_port_id,
      ola::NewCallback(this, &ArtNetNodeTest::TodRequest),
      ola::NewCallback(this, &ArtNetNodeTest::Flush),
      NULL));

  // receive a tod request
  {
    SocketVerifier verifer(m_socket);
    const uint8_t tod_request[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x80,
      0x0, 14,
      0, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full
      1,  // universe array size
      0x23,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    OLA_ASSERT_FALSE(m_tod_request);
    ReceiveFromPeer(tod_request, sizeof(tod_request), peer_ip);
    OLA_ASSERT(m_tod_request);
  }

  // respond with a Tod
  {
    SocketVerifier verifer(m_socket);
    const uint8_t art_tod1[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x81,
      0x0, 14,
      1,  // rdm standard
      2,  // first port
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full tod
      0x23,  // universe address
      0, 1,  // uid count
      0,  // block count
      1,  // uid count
      0x7a, 0x70, 0, 0, 0, 0,
    };

    ExpectedBroadcast(art_tod1, sizeof(art_tod1));

    UIDSet uids;
    UID uid1(0x7a70, 0);
    uids.AddUID(uid1);
    OLA_ASSERT(node.SendTod(m_port_id, uids));
  }

  // try a tod request a universe that doesn't match ours
  {
    SocketVerifier verifer(m_socket);
    m_tod_request = false;
    const uint8_t tod_request2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x80,
      0x0, 14,
      0, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // full
      2,  // universe array size
      0x13, 0x24,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    OLA_ASSERT_FALSE(m_tod_request);
    ReceiveFromPeer(tod_request2, sizeof(tod_request2), peer_ip);
    OLA_ASSERT_FALSE(m_tod_request);
  }

  // check TodControl
  {
    SocketVerifier verifer(m_socket);
    OLA_ASSERT_FALSE(m_tod_flush);

    const uint8_t tod_control[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x82,
      0x0, 14,
      0, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      1,  // flush
      0x23
    };

    ReceiveFromPeer(tod_control, sizeof(tod_control), peer_ip);
    OLA_ASSERT(m_tod_flush);
  }

  // try a tod control a universe that doesn't match ours
  {
    SocketVerifier verifer(m_socket);
    m_tod_flush = false;
    OLA_ASSERT_FALSE(m_tod_flush);
    const uint8_t tod_control2[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x82,
      0x0, 14,
      0, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      1,  // flush
      0x13
    };

    ReceiveFromPeer(tod_control2, sizeof(tod_control2), peer_ip);
    OLA_ASSERT_FALSE(m_tod_flush);
  }
}


/**
 * Check that we respond to Tod messages
 */
void ArtNetNodeTest::testRDMResponder() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupOutputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  OLA_ASSERT(node.SetOutputPortRDMHandlers(
      m_port_id,
      NULL,
      NULL,
      ola::NewCallback(this, &ArtNetNodeTest::HandleRDM)));

  {
    SocketVerifier verifer(m_socket);
    const uint8_t rdm_request[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x83,
      0x0, 14,
      1, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // process
      0x23,
      // rdm data
      1, 24,  // sub code & length
      0, 3, 0, 0, 0, 4,   // dst uid
      0, 1, 0, 0, 0, 2,   // src uid
      0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
      0x20, 1, 40, 0,  // command, param id, param data length
      0x01, 0x43
    };

    OLA_ASSERT_FALSE(m_rdm_request);
    OLA_ASSERT_FALSE(m_rdm_callback);
    ReceiveFromPeer(rdm_request, sizeof(rdm_request), peer_ip);
    OLA_ASSERT(m_rdm_request);
    OLA_ASSERT(m_rdm_callback);

    // check the request
    UID source(1, 2);
    UID destination(3, 4);

    OLA_ASSERT_EQ(source, m_rdm_request->SourceUID());
    OLA_ASSERT_EQ(destination, m_rdm_request->DestinationUID());
    OLA_ASSERT_EQ((uint8_t) 0, m_rdm_request->TransactionNumber());
    OLA_ASSERT_EQ((uint8_t) 1, m_rdm_request->PortId());
    OLA_ASSERT_EQ((uint8_t) 0, m_rdm_request->MessageCount());
    OLA_ASSERT_EQ((uint16_t) 10, m_rdm_request->SubDevice());
    OLA_ASSERT_EQ(RDMCommand::GET_COMMAND,
                  m_rdm_request->CommandClass());
    OLA_ASSERT_EQ((uint16_t) 296, m_rdm_request->ParamId());
    OLA_ASSERT_EQ(static_cast<const uint8_t*>(NULL),
                  m_rdm_request->ParamData());
    OLA_ASSERT_EQ(0u, m_rdm_request->ParamDataSize());
    OLA_ASSERT_EQ(25u, RDMCommandSerializer::RequiredSize(*m_rdm_request));
  }

  // run the RDM callback, triggering the response
  {
    SocketVerifier verifer(m_socket);
    const uint8_t rdm_response[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x83,
      0x0, 14,
      1, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // process
      0x23,
      // rdm data
      1, 28,  // sub code & length
      0, 1, 0, 0, 0, 2,   // dst uid
      0, 3, 0, 0, 0, 4,   // src uid
      0, 0, 0, 0, 10,  // transaction, port id, msg count & sub device
      0x21, 1, 40, 4,  // command, param id, param data length
      0x5a, 0xa5, 0x5a, 0xa5,  // param data
      0x3, 0x49  // checksum, filled in below
    };
    ExpectedSend(rdm_response, sizeof(rdm_response), peer_ip);

    uint8_t param_data[] = {0x5a, 0xa5, 0x5a, 0xa5};
    RDMResponse *response = GetResponseFromData(m_rdm_request,
                                                param_data,
                                                sizeof(param_data));
    RDMReply reply(ola::rdm::RDM_COMPLETED_OK, response);
    m_rdm_callback->Run(&reply);

    // clean up
    delete m_rdm_request;
    m_rdm_request = NULL;
  }
}


/**
 * Check that the node works as a RDM controller.
 */
void ArtNetNodeTest::testRDMRequest() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // We need to send a TodData so we populate the node's UID map
  PopulateTod();

  // create a new RDM request
  {
    SocketVerifier verifer(m_socket);
    SendRDMRequest(
        &node,
        ola::NewSingleCallback(this, &ArtNetNodeTest::FinalizeRDM));
  }

  // send a response
  {
    SocketVerifier verifer(m_socket);
    const uint8_t rdm_response[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x83,
      0x0, 14,
      1, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // process
      0x23,
      // rdm data
      1, 28,  // sub code & length
      0, 1, 0, 0, 0, 2,   // dst uid
      0x7a, 0x70, 0, 0, 0, 0,   // dst uid
      0, 0, 0, 0, 10,  // transaction, port id, msg count & sub device
      0x21, 1, 40, 4,  // command, param id, param data length
      0x5a, 0xa5, 0x5a, 0xa5,  // param data
      0x4, 0x2c  // checksum, filled in below
    };

    OLA_ASSERT_FALSE(m_rdm_response);
    ReceiveFromPeer(rdm_response, sizeof(rdm_response), peer_ip);
    OLA_ASSERT(m_rdm_response);
    delete m_rdm_response;
  }
}


/**
 * Check that request times out if we don't get a response.
 */
void ArtNetNodeTest::testRDMRequestTimeout() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // We need to send a TodData so we populate the node's UID map
  PopulateTod();

  // create a new RDM request
  {
    SocketVerifier verifer(m_socket);
    SendRDMRequest(
        &node,
        ola::NewSingleCallback(this, &ArtNetNodeTest::ExpectTimeout));
  }

  m_clock.AdvanceTime(3, 0);  // timeout is 2s
  ss.RunOnce();
  OLA_ASSERT(m_got_rdm_timeout);
}


/**
 * Check we don't accept responses from a different src IP
 */
void ArtNetNodeTest::testRDMRequestIPMismatch() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // We need to send a TodData so we populate the node's UID map
  PopulateTod();

  // create a new RDM request
  {
    SocketVerifier verifer(m_socket);
    SendRDMRequest(
        &node,
        ola::NewSingleCallback(this, &ArtNetNodeTest::ExpectTimeout));
  }

  // send a response from a different IP
  {
    SocketVerifier verifer(m_socket);
    const uint8_t rdm_response[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x83,
      0x0, 14,
      1, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // process
      0x23,
      // rdm data
      1, 28,  // sub code & length
      0, 1, 0, 0, 0, 2,   // dst uid
      0x7a, 0x70, 0, 0, 0, 0,   // src uid
      0, 0, 0, 0, 10,  // transaction, port id, msg count & sub device
      0x21, 1, 40, 4,  // command, param id, param data length
      0x5a, 0xa5, 0x5a, 0xa5,  // param data
      0x4, 0x2c  // checksum, filled in below
    };

    OLA_ASSERT_FALSE(m_rdm_response);
    ReceiveFromPeer(rdm_response, sizeof(rdm_response), peer_ip2);
    OLA_ASSERT_FALSE(m_rdm_response);
  }

  m_clock.AdvanceTime(3, 0);  // timeout is 2s
  ss.RunOnce();
  OLA_ASSERT(m_got_rdm_timeout);
}


/**
 * Check we don't accept responses with a different UID
 */
void ArtNetNodeTest::testRDMRequestUIDMismatch() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);
  SetupInputPort(&node);
  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // We need to send a TodData so we populate the node's UID map
  PopulateTod();

  // create a new RDM request
  {
    SocketVerifier verifer(m_socket);
    SendRDMRequest(
        &node,
        ola::NewSingleCallback(this, &ArtNetNodeTest::ExpectTimeout));
  }

  // send a response from a different IP
  {
    SocketVerifier verifer(m_socket);
    const uint8_t rdm_response[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x83,
      0x0, 14,
      1, 0,
      0, 0, 0, 0, 0, 0, 0,
      4,  // net
      0,  // process
      0x23,
      // rdm data
      1, 28,  // sub code & length
      0, 1, 0, 0, 0, 2,   // dst uid
      0x7a, 0x70, 0, 0, 0, 1,   // src uid
      0, 0, 0, 0, 10,  // transaction, port id, msg count & sub device
      0x21, 1, 40, 4,  // command, param id, param data length
      0x5a, 0xa5, 0x5a, 0xa5,  // param data
      0x4, 0x2d  // checksum, filled in below
    };

    OLA_ASSERT_FALSE(m_rdm_response);
    ReceiveFromPeer(rdm_response, sizeof(rdm_response), peer_ip);
    OLA_ASSERT_FALSE(m_rdm_response);
  }

  m_clock.AdvanceTime(3, 0);  // timeout is 2s
  ss.RunOnce();
  OLA_ASSERT(m_got_rdm_timeout);
}


/**
 * Check Timecode sending works
 */
void ArtNetNodeTest::testTimeCode() {
  m_socket->SetDiscardMode(true);
  ArtNetNodeOptions node_options;
  ArtNetNode node(iface, &ss, node_options, m_socket);

  OLA_ASSERT(node.Start());
  ss.RemoveReadDescriptor(m_socket);
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  {
    SocketVerifier verifer(m_socket);
    const uint8_t timecode_message[] = {
      'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
      0x00, 0x97,
      0x0, 14,
      0, 0,
      11, 30, 20, 10, 3
    };

    ExpectedBroadcast(timecode_message, sizeof(timecode_message));
    TimeCode t1(ola::timecode::TIMECODE_SMPTE, 10, 20, 30, 11);
    OLA_ASSERT(node.SendTimeCode(t1));
  }
}
