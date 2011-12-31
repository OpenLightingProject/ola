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

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/Interface.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/timecode/TimeCode.h"
#include "ola/timecode/TimeCodeEnums.h"
#include "plugins/artnet/ArtNetNode.h"
#include "plugins/artnet/MockUdpSocket.h"


using ola::DmxBuffer;
using ola::network::IPV4Address;
using ola::network::Interface;
using ola::plugin::artnet::ArtNetNode;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using ola::timecode::TimeCode;
using std::string;


class ArtNetNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ArtNetNodeTest);
  CPPUNIT_TEST(testBasicBehaviour);
  CPPUNIT_TEST(testBroadcastSendDMX);
  CPPUNIT_TEST(testNonBroadcastSendDMX);
  CPPUNIT_TEST(testTimeCode);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testBasicBehaviour();
    void testBroadcastSendDMX();
    void testNonBroadcastSendDMX();
    void testTimeCode();

  private:
    static const uint8_t POLL_MESSAGE[];
    static const uint8_t POLL_REPLY_MESSAGE[];
    static const uint8_t TIMECODE_MESSAGE[];
    static const uint16_t ARTNET_PORT = 6454;

    Interface CreateInterface();
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


const uint8_t ArtNetNodeTest::TIMECODE_MESSAGE[] = {
  'A', 'r', 't', '-', 'N', 'e', 't', 0x00,
  0x00, 0x97,
  0x0, 14,
  0, 0,
  11, 30, 20, 10, 3
};


void ArtNetNodeTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
}


/**
 * Creates a mock interface for us to use.
 */
Interface ArtNetNodeTest::CreateInterface() {
  ola::network::InterfaceBuilder interface_builder;
  CPPUNIT_ASSERT(interface_builder.SetAddress("10.0.0.1"));
  CPPUNIT_ASSERT(interface_builder.SetSubnetMask("255.0.0.0"));
  CPPUNIT_ASSERT(interface_builder.SetBroadcast("10.255.255.255"));
  CPPUNIT_ASSERT(interface_builder.SetHardwareAddress("0a:0b:0c:12:34:56"));
  return interface_builder.Construct();
}

/**
 * Check that the discovery sequence works correctly.
 */
void ArtNetNodeTest::testBasicBehaviour() {
  ola::network::Interface interface = CreateInterface();

  ola::network::SelectServer ss;
  MockUdpSocket *socket = new MockUdpSocket();

  ArtNetNode node(interface,
                  &ss,
                  false,
                  20,
                  socket);

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
  socket->Verify();
  CPPUNIT_ASSERT(socket->CheckNetworkParamsMatch(true, true, 6454, true));

  // now enable an input port and check that we send a poll
  socket->AddExpectedData(
    static_cast<const uint8_t*>(POLL_MESSAGE),
    sizeof(POLL_MESSAGE),
    interface.bcast_address,
    ARTNET_PORT);

  // now we should see an unsolicted poll reply sent because conditions have
  // changed.
  uint8_t expected_poll_reply_packet[sizeof(POLL_REPLY_MESSAGE)];
  memcpy(expected_poll_reply_packet,
         POLL_REPLY_MESSAGE,
         sizeof(POLL_REPLY_MESSAGE));
  expected_poll_reply_packet[115] = '1';  // node report
  expected_poll_reply_packet[179] = 0;  // good input
  expected_poll_reply_packet[187] = 0x22;  // swin

  socket->AddExpectedData(
    static_cast<uint8_t*>(expected_poll_reply_packet),
    sizeof(expected_poll_reply_packet),
    interface.bcast_address,
    ARTNET_PORT);

  node.SetPortUniverse(ola::plugin::artnet::ARTNET_INPUT_PORT, 1, 2);
  CPPUNIT_ASSERT_EQUAL(
    (uint8_t) 0x20,
    node.GetPortUniverse(ola::plugin::artnet::ARTNET_INPUT_PORT, 0));
  CPPUNIT_ASSERT_EQUAL(
    (uint8_t) 0x22,
    node.GetPortUniverse(ola::plugin::artnet::ARTNET_INPUT_PORT, 1));
  socket->Verify();

  // check sending a poll works
  socket->AddExpectedData(
    static_cast<const uint8_t*>(POLL_MESSAGE),
    sizeof(POLL_MESSAGE),
    interface.bcast_address,
    ARTNET_PORT);
  CPPUNIT_ASSERT(node.SendPoll());
  socket->Verify();

  CPPUNIT_ASSERT(node.Stop());
}


/**
 * Check sending DMX using broadcast works.
 */
void ArtNetNodeTest::testBroadcastSendDMX() {
  ola::network::Interface interface = CreateInterface();
  ola::network::SelectServer ss;
  MockUdpSocket *socket = new MockUdpSocket();
  socket->SetDiscardMode(true);

  ArtNetNode node(interface,
                  &ss,
                  true,  // always broadcast dmx
                  20,
                  socket);

  uint8_t port_id = 1;
  node.SetNetAddress(4);
  node.SetSubnetAddress(2);
  node.SetPortUniverse(ola::plugin::artnet::ARTNET_INPUT_PORT, port_id, 3);

  CPPUNIT_ASSERT(node.Start());
  socket->Verify();
  socket->SetDiscardMode(false);

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
  socket->AddExpectedData(
    DMX_MESSAGE,
    sizeof(DMX_MESSAGE),
    interface.bcast_address,
    ARTNET_PORT);

  DmxBuffer dmx;
  dmx.SetFromString("0,1,2,3,4,5");
  CPPUNIT_ASSERT(node.SendDMX(port_id, dmx));
  socket->Verify();

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
  socket->AddExpectedData(
    DMX_MESSAGE2,
    sizeof(DMX_MESSAGE2),
    interface.bcast_address,
    ARTNET_PORT);
  dmx.SetFromString("0,1,2,3,4");
  CPPUNIT_ASSERT(node.SendDMX(port_id, dmx));
  socket->Verify();

  // attempt to send on a invalid port
  CPPUNIT_ASSERT(!node.SendDMX(4, dmx));
  socket->Verify();

  // attempt to send an empty fram
  DmxBuffer empty_buffer;
  CPPUNIT_ASSERT(node.SendDMX(port_id, empty_buffer));
  socket->Verify();
}


/**
 * Check sending DMX using unicast works.
 */
void ArtNetNodeTest::testNonBroadcastSendDMX() {
  ola::network::Interface interface = CreateInterface();
  ola::network::SelectServer ss;
  MockUdpSocket *socket = new MockUdpSocket();
  socket->SetDiscardMode(true);

  ArtNetNode node(interface,
                  &ss,
                  false,
                  20,
                  socket);

  uint8_t port_id = 1;
  node.SetNetAddress(4);
  node.SetSubnetAddress(2);
  node.SetPortUniverse(ola::plugin::artnet::ARTNET_INPUT_PORT, port_id, 3);

  CPPUNIT_ASSERT(node.Start());
  socket->Verify();
  socket->SetDiscardMode(false);

  DmxBuffer dmx;
  dmx.SetFromString("0,1,2,3,4,5");
  // we don't expect any data here because there are no nodes active
  CPPUNIT_ASSERT(node.SendDMX(port_id, dmx));
  socket->Verify();

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
  IPV4Address peer_ip;
  ola::network::IPV4Address::FromString("10.0.0.10", &peer_ip);

  // Fake an ArtPollReply
  socket->AddReceivedData(
      poll_reply_message,
      sizeof(poll_reply_message),
      peer_ip,
      6454);
  socket->PerformRead();

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
  socket->AddExpectedData(
    DMX_MESSAGE,
    sizeof(DMX_MESSAGE),
    peer_ip,
    ARTNET_PORT);
  CPPUNIT_ASSERT(node.SendDMX(port_id, dmx));
  socket->Verify();

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
  IPV4Address peer_ip2;
  ola::network::IPV4Address::FromString("10.0.0.11", &peer_ip2);

  // Fake an ArtPollReply
  socket->AddReceivedData(
      poll_reply_message2,
      sizeof(poll_reply_message2),
      peer_ip2,
      6454);
  socket->PerformRead();

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
  socket->AddExpectedData(
    DMX_MESSAGE2,
    sizeof(DMX_MESSAGE2),
    peer_ip,
    ARTNET_PORT);
  socket->AddExpectedData(
    DMX_MESSAGE2,
    sizeof(DMX_MESSAGE2),
    peer_ip2,
    ARTNET_PORT);
  CPPUNIT_ASSERT(node.SendDMX(port_id, dmx));
  socket->Verify();

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
  socket->AddExpectedData(
    DMX_MESSAGE3,
    sizeof(DMX_MESSAGE3),
    interface.bcast_address,
    ARTNET_PORT);
  CPPUNIT_ASSERT(node.SendDMX(port_id, dmx));
  socket->Verify();
}


/**
 * check Timecode sending works
 */
void ArtNetNodeTest::testTimeCode() {
  ola::network::Interface interface = CreateInterface();
  ola::network::SelectServer ss;
  MockUdpSocket *socket = new MockUdpSocket();
  socket->SetDiscardMode(true);

  ArtNetNode node(interface,
                  &ss,
                  false,
                  20,
                  socket);

  CPPUNIT_ASSERT(node.Start());
  socket->Verify();
  socket->SetDiscardMode(false);

  socket->AddExpectedData(
    TIMECODE_MESSAGE,
    sizeof(TIMECODE_MESSAGE),
    interface.bcast_address,
    ARTNET_PORT);

  TimeCode t1(ola::timecode::TIMECODE_SMPTE, 10, 20, 30, 11);
  CPPUNIT_ASSERT(node.SendTimeCode(t1));

  socket->Verify();
}
