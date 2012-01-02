/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMResponseCodes.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/timecode/TimeCode.h"
#include "ola/timecode/TimeCodeEnums.h"
#include "plugins/artnet/ArtNetNode.h"
#include "plugins/artnet/MockUdpSocket.h"


using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::network::Interface;
using ola::plugin::artnet::ArtNetNode;
using ola::rdm::RDMCallback;
using ola::rdm::RDMCommand;
using ola::rdm::RDMGetRequest;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::timecode::TimeCode;
using std::string;


class ArtNetNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ArtNetNodeTest);
  CPPUNIT_TEST(testBasicBehaviour);
  CPPUNIT_TEST(testBroadcastSendDMX);
  CPPUNIT_TEST(testNonBroadcastSendDMX);
  CPPUNIT_TEST(testReceiveDMX);
  CPPUNIT_TEST(testHTPMerge);
  CPPUNIT_TEST(testLTPMerge);
  CPPUNIT_TEST(testControllerDiscovery);
  CPPUNIT_TEST(testControllerIncrementalDiscovery);
  CPPUNIT_TEST(testUnsolicitedTod);
  CPPUNIT_TEST(testResponderDiscovery);
  CPPUNIT_TEST(testRDMResponder);
  CPPUNIT_TEST(testRDMController);
  CPPUNIT_TEST(testTimeCode);
  CPPUNIT_TEST_SUITE_END();

  public:
    ArtNetNodeTest()
        : CppUnit::TestFixture(),
          ss(NULL, &m_clock),
          m_got_dmx(false),
          m_discovery_done(false),
          m_tod_flush(false),
          m_tod_request(false),
          m_rdm_request(NULL),
          m_rdm_callback(NULL),
          m_rdm_response(NULL),
          m_port_id(1),
          m_socket(new MockUdpSocket()) {
    }
    void setUp();

    void testBasicBehaviour();
    void testBroadcastSendDMX();
    void testNonBroadcastSendDMX();
    void testReceiveDMX();
    void testHTPMerge();
    void testLTPMerge();
    void testControllerDiscovery();
    void testControllerIncrementalDiscovery();
    void testUnsolicitedTod();
    void testResponderDiscovery();
    void testRDMResponder();
    void testRDMController();
    void testTimeCode();

  private:
    ola::MockClock m_clock;
    ola::network::SelectServer ss;
    bool m_got_dmx;
    bool m_discovery_done;
    bool m_tod_flush;
    bool m_tod_request;
    UIDSet m_uids;
    const RDMRequest *m_rdm_request;
    RDMCallback *m_rdm_callback;
    const RDMResponse *m_rdm_response;
    uint8_t m_port_id;
    Interface interface;
    IPV4Address peer_ip, peer_ip2, peer_ip3;
    MockUdpSocket *m_socket;

    /**
     * Called when new DMX arrives
     */
    void NewDmx() {
      m_got_dmx = true;
    }

    void DiscoveryComplete(const UIDSet &uids) {
      m_uids = uids;
      m_discovery_done = true;
    }

    void TodRequest() {
      m_tod_request = true;
    }

    void Flush() {
      m_tod_flush = true;
    }

    void HandleRDM(const RDMRequest *request, RDMCallback *callback) {
      m_rdm_request = request;
      m_rdm_callback = callback;
    }

    void FinalizeRDM(ola::rdm::rdm_response_code status,
                     const RDMResponse *response,
                     const std::vector<string> &packets) {
      CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_COMPLETED_OK, status);
      m_rdm_response = response;
      (void) packets;
    }

    void ExpectedSend(const uint8_t *data,
                      unsigned int data_size,
                      const IPV4Address &address) {
      m_socket->AddExpectedData(data, data_size, address, ARTNET_PORT);
    }

    void ExpectedBroadcast(const uint8_t *data, unsigned int data_size) {
      ExpectedSend(data, data_size, interface.bcast_address);
    }

    void ReceiveFromPeer(const uint8_t *data,
                         unsigned int data_size,
                         const IPV4Address &address) {
      ss.RunOnce(0, 0);  // update the wake up time
      m_socket->ReceiveData(data, data_size, address, ARTNET_PORT);
    }


    void SetupInputPort(ArtNetNode *node) {
      node->SetNetAddress(4);
      node->SetSubnetAddress(2);
      node->SetPortUniverse(
          ola::plugin::artnet::ARTNET_INPUT_PORT,
          m_port_id,
          3);
    }

    void SetupOutputPort(ArtNetNode *node) {
      node->SetNetAddress(4);
      node->SetSubnetAddress(2);
      node->SetPortUniverse(
          ola::plugin::artnet::ARTNET_OUTPUT_PORT,
          m_port_id,
          3);
    }

    static const uint8_t POLL_MESSAGE[];
    static const uint8_t POLL_REPLY_MESSAGE[];
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


void ArtNetNodeTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  ola::network::InterfaceBuilder interface_builder;
  CPPUNIT_ASSERT(interface_builder.SetAddress("10.0.0.1"));
  CPPUNIT_ASSERT(interface_builder.SetSubnetMask("255.0.0.0"));
  CPPUNIT_ASSERT(interface_builder.SetBroadcast("10.255.255.255"));
  CPPUNIT_ASSERT(interface_builder.SetHardwareAddress("0a:0b:0c:12:34:56"));
  interface = interface_builder.Construct();

  ola::network::IPV4Address::FromString("10.0.0.10", &peer_ip);
  ola::network::IPV4Address::FromString("10.0.0.11", &peer_ip2);
  ola::network::IPV4Address::FromString("10.0.0.12", &peer_ip3);
}


/**
 * Check that the discovery sequence works correctly.
 */
void ArtNetNodeTest::testBasicBehaviour() {
  ArtNetNode node(interface, &ss, false, 20, m_socket);

  node.SetShortName("Short Name");
  CPPUNIT_ASSERT_EQUAL(string("Short Name"), node.ShortName());
  node.SetLongName("This is the very long name");
  CPPUNIT_ASSERT_EQUAL(
    string("This is the very long name"),
    node.LongName());
  node.SetNetAddress(4);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 4, node.NetAddress());
  node.SetSubnetAddress(2);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, node.SubnetAddress());

  node.SetPortUniverse(ola::plugin::artnet::ARTNET_OUTPUT_PORT, 0, 3);
  CPPUNIT_ASSERT(
      !node.SetPortUniverse(ola::plugin::artnet::ARTNET_OUTPUT_PORT, 4, 3));
  CPPUNIT_ASSERT_EQUAL(
    (uint8_t) 0x23,
    node.GetPortUniverse(ola::plugin::artnet::ARTNET_OUTPUT_PORT, 0));
  CPPUNIT_ASSERT_EQUAL(
    (uint8_t) 0x20,
    node.GetPortUniverse(ola::plugin::artnet::ARTNET_OUTPUT_PORT, 1));

  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  CPPUNIT_ASSERT(m_socket->CheckNetworkParamsMatch(true, true, 6454, true));

  // now enable an input port and check that we send a poll
  ExpectedBroadcast(POLL_MESSAGE, sizeof(POLL_MESSAGE));

  // now we should see an unsolicted poll reply sent because conditions have
  // changed.
  uint8_t expected_poll_reply_packet[sizeof(POLL_REPLY_MESSAGE)];
  memcpy(expected_poll_reply_packet,
         POLL_REPLY_MESSAGE,
         sizeof(POLL_REPLY_MESSAGE));
  expected_poll_reply_packet[115] = '1';  // node report
  expected_poll_reply_packet[179] = 0;  // good input
  expected_poll_reply_packet[187] = 0x22;  // swin

  ExpectedBroadcast(expected_poll_reply_packet,
                    sizeof(expected_poll_reply_packet));

  node.SetPortUniverse(ola::plugin::artnet::ARTNET_INPUT_PORT, 1, 2);
  CPPUNIT_ASSERT_EQUAL(
    (uint8_t) 0x20,
    node.GetPortUniverse(ola::plugin::artnet::ARTNET_INPUT_PORT, 0));
  CPPUNIT_ASSERT_EQUAL(
    (uint8_t) 0x22,
    node.GetPortUniverse(ola::plugin::artnet::ARTNET_INPUT_PORT, 1));
  m_socket->Verify();

  // check sending a poll works
  ExpectedBroadcast(POLL_MESSAGE, sizeof(POLL_MESSAGE));
  CPPUNIT_ASSERT(node.SendPoll());
  m_socket->Verify();

  CPPUNIT_ASSERT(node.Stop());
}


/**
 * Check sending DMX using broadcast works.
 */
void ArtNetNodeTest::testBroadcastSendDMX() {
  m_socket->SetDiscardMode(true);

  ArtNetNode node(interface,
                  &ss,
                  true,  // always broadcast dmx
                  20,
                  m_socket);
  SetupInputPort(&node);

  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

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
  CPPUNIT_ASSERT(node.SendDMX(m_port_id, dmx));
  m_socket->Verify();

  // now send an odd sized dmx frame, we should pad this to a multiple of two
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
  dmx.SetFromString("0,1,2,3,4");
  CPPUNIT_ASSERT(node.SendDMX(m_port_id, dmx));
  m_socket->Verify();

  // attempt to send on a invalid port
  CPPUNIT_ASSERT(!node.SendDMX(4, dmx));
  m_socket->Verify();

  // attempt to send an empty fram
  DmxBuffer empty_buffer;
  CPPUNIT_ASSERT(node.SendDMX(m_port_id, empty_buffer));
  m_socket->Verify();
}


/**
 * Check sending DMX using unicast works.
 */
void ArtNetNodeTest::testNonBroadcastSendDMX() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, false, 20, m_socket);
  SetupInputPort(&node);
  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  DmxBuffer dmx;
  dmx.SetFromString("0,1,2,3,4,5");
  // we don't expect any data here because there are no nodes active
  CPPUNIT_ASSERT(node.SendDMX(m_port_id, dmx));
  m_socket->Verify();

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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
    '#', '0', '0', '0', '1', ' ', '[', '0', ']', ' ', 'O', 'L', 'A',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0  // filler
  };

  // Fake an ArtPollReply
  ReceiveFromPeer(poll_reply_message, sizeof(poll_reply_message), peer_ip);

  // now send a DMX frame, this should get unicast
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
  CPPUNIT_ASSERT(node.SendDMX(m_port_id, dmx));
  m_socket->Verify();

  // add another peer
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // long name
    '#', '0', '0', '0', '1', ' ', '[', '0', ']', ' ', 'O', 'L', 'A',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0  // filler
  };

  // Fake an ArtPollReply
  ReceiveFromPeer(poll_reply_message2, sizeof(poll_reply_message2), peer_ip2);

  // now send another DMX frame, this should get unicast twice
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
  CPPUNIT_ASSERT(node.SendDMX(m_port_id, dmx));
  m_socket->Verify();

  // now adjust the broadcast threshold
  node.SetBroadcastThreshold(2);

  // now send another DMX frame, this should get broadcast
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
  CPPUNIT_ASSERT(node.SendDMX(m_port_id, dmx));
  m_socket->Verify();
}


/**
 * Check that receiving DMX works
 */
void ArtNetNodeTest::testReceiveDMX() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, false, 20, m_socket);
  SetupOutputPort(&node);
  DmxBuffer input_buffer;
  node.SetDMXHandler(m_port_id,
                     &input_buffer,
                     ola::NewCallback(this, &ArtNetNodeTest::NewDmx));

  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // 'receive' a DMX message
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

  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(DMX_MESSAGE, sizeof(DMX_MESSAGE), peer_ip);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("0,1,2,3,4,5"), input_buffer.ToString());

  // now send a second frame
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
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("5,4,3,2,1,0"), input_buffer.ToString());

  // now advance the clock by more than the merge timeout (10s)
  m_clock.AdvanceTime(11, 0);

  // send another message, but first update the seq #
  DMX_MESSAGE[12] = 2;

  m_got_dmx = false;
  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(DMX_MESSAGE, sizeof(DMX_MESSAGE), peer_ip);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("0,1,2,3,4,5"), input_buffer.ToString());
}


/**
 * Check that merging works
 */
void ArtNetNodeTest::testHTPMerge() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, false, 20, m_socket);
  SetupOutputPort(&node);
  DmxBuffer input_buffer;
  node.SetDMXHandler(m_port_id,
                     &input_buffer,
                     ola::NewCallback(this, &ArtNetNodeTest::NewDmx));

  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // 'receive' a DMX message from the first peer
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

  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(source1_message1, sizeof(source1_message1), peer_ip);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("0,1,2,3,4,5"), input_buffer.ToString());

  // receive a message from a second peer
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
  CPPUNIT_ASSERT(!m_got_dmx);

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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0  // filler
  };

  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));

  ReceiveFromPeer(source2_message1, sizeof(source2_message1), peer_ip2);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("5,4,3,3,4,5"), input_buffer.ToString());

  // send a packet from a third source, this shouldn't result in any new dmx
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
  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(source3_message1, sizeof(source3_message1), peer_ip3);
  CPPUNIT_ASSERT(!m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("5,4,3,3,4,5"), input_buffer.ToString());

  // now send another packet from the first source
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
  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(source1_message2, sizeof(source1_message2), peer_ip);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("10,11,12,2,2,1,0,0"),
                       input_buffer.ToString());


  // now advance the clock by half the merge timeout
  m_clock.AdvanceTime(5, 0);

  // now send another packet from the first source
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
  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(source1_message3, sizeof(source1_message3), peer_ip);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("5,4,3,3,4,5,7,9"),
                       input_buffer.ToString());

  // now advance the clock so the second source times out
  m_clock.AdvanceTime(6, 0);

  // now send another packet from the first source
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
  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(source1_message4, sizeof(source1_message4), peer_ip);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("0,1,2,3,4,5,7,9"),
                       input_buffer.ToString());
}


/**
 * Check that LTP merging works
 */
void ArtNetNodeTest::testLTPMerge() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, false, 20, m_socket);
  SetupOutputPort(&node);
  DmxBuffer input_buffer;
  node.SetDMXHandler(m_port_id,
                     &input_buffer,
                     ola::NewCallback(this, &ArtNetNodeTest::NewDmx));

  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // switch to LTP merge mode, this will trigger an art poll reply
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0  // filler
  };

  ExpectedBroadcast(poll_reply_message, sizeof(poll_reply_message));
  node.SetMergeMode(m_port_id, ola::plugin::artnet::ARTNET_MERGE_LTP);
  m_socket->Verify();

  // 'receive' a DMX message from the first peer
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

  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(source1_message1, sizeof(source1_message1), peer_ip);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("0,1,2,3,4,5"), input_buffer.ToString());

  // receive a message from a second peer
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
  CPPUNIT_ASSERT(!m_got_dmx);

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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0  // filler
  };

  ExpectedBroadcast(poll_reply_message2, sizeof(poll_reply_message2));

  ReceiveFromPeer(source2_message1, sizeof(source2_message1), peer_ip2);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("5,4,3,2,1,0"), input_buffer.ToString());

  // now advance the clock so the second source times out
  m_clock.AdvanceTime(11, 0);

  // now send another packet from the first source
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
  CPPUNIT_ASSERT(!m_got_dmx);
  ReceiveFromPeer(source1_message2, sizeof(source1_message2), peer_ip);
  CPPUNIT_ASSERT(m_got_dmx);
  CPPUNIT_ASSERT_EQUAL(string("0,1,2,3,4,5,7,9"),
                       input_buffer.ToString());
}


/**
 * Check the node can act as an RDM controller.
 */
void ArtNetNodeTest::testControllerDiscovery() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, true, 20, m_socket);
  SetupInputPort(&node);
  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

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

  ExpectedBroadcast(tod_control, sizeof(tod_control));

  node.RunFullDiscovery(
      m_port_id,
      ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
  CPPUNIT_ASSERT(!m_discovery_done);

  // now advance the clock and run the select server
  m_clock.AdvanceTime(5, 0);  // tod timeout is 4s
  ss.RunOnce(0, 0);  // update the wake up time
  CPPUNIT_ASSERT(m_discovery_done);

  UIDSet uids;
  CPPUNIT_ASSERT_EQUAL(uids, m_uids);

  // now run discovery again, this time returning a ArtTod from a peer
  m_discovery_done = false;

  ExpectedBroadcast(tod_control, sizeof(tod_control));

  node.RunFullDiscovery(
      m_port_id,
      ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
  CPPUNIT_ASSERT(!m_discovery_done);

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
  CPPUNIT_ASSERT(!m_discovery_done);

  // now advance the clock and run the select server
  m_clock.AdvanceTime(5, 0);  // tod timeout is 4s
  ss.RunOnce(0, 0);  // update the wake up time
  CPPUNIT_ASSERT(m_discovery_done);

  UID uid1(0x7a70, 0);
  UID uid2(0x7a70, 1);
  UID uid3(0x7a70, 2);
  uids.AddUID(uid1);
  uids.AddUID(uid2);
  uids.AddUID(uid3);
  CPPUNIT_ASSERT_EQUAL(uids, m_uids);

  // now run discovery again, removing one UID, and moving another from peer1
  // to peer2
  m_discovery_done = false;

  ExpectedBroadcast(tod_control, sizeof(tod_control));

  node.RunFullDiscovery(
      m_port_id,
      ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
  CPPUNIT_ASSERT(!m_discovery_done);

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
  CPPUNIT_ASSERT(!m_discovery_done);

  // now advance the clock and run the select server
  m_clock.AdvanceTime(5, 0);  // tod timeout is 4s
  ss.RunOnce(0, 0);  // update the wake up time
  CPPUNIT_ASSERT(m_discovery_done);

  uids.Clear();
  uids.AddUID(uid1);
  uids.AddUID(uid2);
  CPPUNIT_ASSERT_EQUAL(uids, m_uids);

  // finally try discovery for a invalid port id
  m_discovery_done = false;
  node.RunFullDiscovery(
      4,
      ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
  CPPUNIT_ASSERT(m_discovery_done);
  uids.Clear();
  CPPUNIT_ASSERT_EQUAL(uids, m_uids);
}


/**
 * Check that incremental discovery works
 */
void ArtNetNodeTest::testControllerIncrementalDiscovery() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, true, 20, m_socket);
  SetupInputPort(&node);
  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

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
  CPPUNIT_ASSERT(!m_discovery_done);

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
    0, 1,  // uid count
    0,  // block count
    1,  // uid count
    0x7a, 0x70, 0, 0, 0, 0,
  };

  ReceiveFromPeer(art_tod1, sizeof(art_tod1), peer_ip);
  CPPUNIT_ASSERT(!m_discovery_done);

  // now advance the clock and run the select server
  m_clock.AdvanceTime(5, 0);  // tod timeout is 4s
  ss.RunOnce(0, 0);  // update the wake up time
  CPPUNIT_ASSERT(m_discovery_done);

  UIDSet uids;
  UID uid1(0x7a70, 0);
  uids.AddUID(uid1);
  CPPUNIT_ASSERT_EQUAL(uids, m_uids);

  // finally try discovery for a invalid port id
  m_discovery_done = false;
  node.RunIncrementalDiscovery(
      4,
      ola::NewSingleCallback(this, &ArtNetNodeTest::DiscoveryComplete));
  CPPUNIT_ASSERT(m_discovery_done);
  uids.Clear();
  CPPUNIT_ASSERT_EQUAL(uids, m_uids);
}


/**
 * Check that unsolicated TOD messages work
 */
void ArtNetNodeTest::testUnsolicitedTod() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, true, 20, m_socket);
  SetupInputPort(&node);
  CPPUNIT_ASSERT(node.SetUnsolicatedUIDSetHandler(
      m_port_id,
      ola::NewCallback(this, &ArtNetNodeTest::DiscoveryComplete)));

  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  CPPUNIT_ASSERT(!m_discovery_done);

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

  CPPUNIT_ASSERT(m_discovery_done);
  UIDSet uids;
  UID uid1(0x7a70, 0);
  uids.AddUID(uid1);
}


/**
 * Check that we respond to Tod messages
 */
void ArtNetNodeTest::testResponderDiscovery() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, false, 20, m_socket);
  SetupOutputPort(&node);
  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  CPPUNIT_ASSERT(node.SetOutputPortRDMHandlers(
      m_port_id,
      ola::NewCallback(this, &ArtNetNodeTest::TodRequest),
      ola::NewCallback(this, &ArtNetNodeTest::Flush),
      NULL));

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

  CPPUNIT_ASSERT(!m_tod_request);
  ReceiveFromPeer(tod_request, sizeof(tod_request), peer_ip);

  CPPUNIT_ASSERT(m_tod_request);

  // now respond with a Tod
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
  CPPUNIT_ASSERT(node.SendTod(m_port_id, uids));

  // try a tod request a universe that doesn't match ours
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

  CPPUNIT_ASSERT(!m_tod_request);
  ReceiveFromPeer(tod_request2, sizeof(tod_request2), peer_ip);
  CPPUNIT_ASSERT(!m_tod_request);

  // now check TodControl
  CPPUNIT_ASSERT(!m_tod_flush);

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

  CPPUNIT_ASSERT(m_tod_flush);

  // try a tod control a universe that doesn't match ours
  m_tod_flush = false;
  CPPUNIT_ASSERT(!m_tod_flush);
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
  CPPUNIT_ASSERT(!m_tod_flush);
}


/**
 * Check that we respond to Tod messages
 */
void ArtNetNodeTest::testRDMResponder() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, false, 20, m_socket);
  SetupOutputPort(&node);
  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  CPPUNIT_ASSERT(node.SetOutputPortRDMHandlers(
      m_port_id,
      NULL,
      NULL,
      ola::NewCallback(this, &ArtNetNodeTest::HandleRDM)));

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

  CPPUNIT_ASSERT(!m_rdm_request);
  CPPUNIT_ASSERT(!m_rdm_callback);
  ReceiveFromPeer(rdm_request, sizeof(rdm_request), peer_ip);

  CPPUNIT_ASSERT(m_rdm_request);
  CPPUNIT_ASSERT(m_rdm_callback);

  // check the request
  UID source(1, 2);
  UID destination(3, 4);

  CPPUNIT_ASSERT_EQUAL(source, m_rdm_request->SourceUID());
  CPPUNIT_ASSERT_EQUAL(destination, m_rdm_request->DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, m_rdm_request->TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, m_rdm_request->PortId());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, m_rdm_request->MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, m_rdm_request->SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND, m_rdm_request->CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, m_rdm_request->ParamId());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t*>(NULL), m_rdm_request->ParamData());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, m_rdm_request->ParamDataSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 25, m_rdm_request->Size());
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_REQUEST, m_rdm_request->CommandType());

  // now run the callback
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

  // now send back a response
  RDMResponse *response = GetResponseFromData(m_rdm_request,
                                              param_data,
                                              sizeof(param_data));
  std::vector<string> packets;
  m_rdm_callback->Run(ola::rdm::RDM_COMPLETED_OK, response, packets);

  // clean up
  delete m_rdm_request;
  m_rdm_request = NULL;
}


/**
 * Check that the node works as a RDM controller.
 */
void ArtNetNodeTest::testRDMController() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, true, 20, m_socket);
  SetupInputPort(&node);
  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  // We need to send a TodData so we populate the node's UID map
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

  // create a new RDM request
  UID source(1, 2);
  UID destination(0x7a70, 0);

  const RDMGetRequest *request = new RDMGetRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // message count
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

  node.SendRDMRequest(
    m_port_id,
    request,
    ola::NewSingleCallback(this, &ArtNetNodeTest::FinalizeRDM));

  // now send a response
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

  CPPUNIT_ASSERT(!m_rdm_response);
  ReceiveFromPeer(rdm_response, sizeof(rdm_response), peer_ip);

  CPPUNIT_ASSERT(m_rdm_response);
  delete m_rdm_response;
}


/**
 * Check Timecode sending works
 */
void ArtNetNodeTest::testTimeCode() {
  m_socket->SetDiscardMode(true);
  ArtNetNode node(interface, &ss, false, 20, m_socket);

  CPPUNIT_ASSERT(node.Start());
  m_socket->Verify();
  m_socket->SetDiscardMode(false);

  const uint8_t timecode_message[] = {
    'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
    0x00, 0x97,
    0x0, 14,
    0, 0,
    11, 30, 20, 10, 3
  };

  ExpectedBroadcast(timecode_message, sizeof(timecode_message));

  TimeCode t1(ola::timecode::TIMECODE_SMPTE, 10, 20, 30, 11);
  CPPUNIT_ASSERT(node.SendTimeCode(t1));

  m_socket->Verify();
}
