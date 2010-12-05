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
#include "ola/network/Interface.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/artnet/ArtNetNode.h"
#include "plugins/artnet/MockUdpSocket.h"


using ola::plugin::artnet::ArtNetNode;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::string;


class ArtNetNodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ArtNetNodeTest);
  CPPUNIT_TEST(testBasicBehaviour);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testBasicBehaviour();

  private:
    static const uint8_t POLL_MESSAGE[];
    static const uint8_t POLL_REPLY_MESSAGE[];
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
  0, 2,  // subnet address
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
  0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0  // filler
};


void ArtNetNodeTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
}


/**
 * Check that the discovery sequence works correctly.
 */
void ArtNetNodeTest::testBasicBehaviour() {
  ola::network::Interface interface;
  uint8_t mac_address[] = {0x0a, 0x0b, 0x0c, 0x12, 0x34, 0x56};
  ola::network::StringToAddress("10.0.0.1", interface.ip_address);
  ola::network::StringToAddress("10.255.255.255", interface.bcast_address);
  memcpy(&interface.hw_address, mac_address, sizeof(mac_address));

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
  node.SetSubnetAddress(2);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, node.SubnetAddress());
  node.SetPortUniverse(ola::plugin::artnet::ARTNET_OUTPUT_PORT, 0, 3);
  CPPUNIT_ASSERT_EQUAL(
    (uint8_t) 0x23,
    node.GetPortUniverse(ola::plugin::artnet::ARTNET_OUTPUT_PORT, 0));
  CPPUNIT_ASSERT_EQUAL(
    (uint8_t) 0x20,
    node.GetPortUniverse(ola::plugin::artnet::ARTNET_OUTPUT_PORT, 1));

  struct sockaddr_in bcast_destination;
  ola::network::StringToAddress("10.255.255.255", bcast_destination.sin_addr);
  bcast_destination.sin_port = ola::network::HostToNetwork(
    static_cast<uint16_t>(6454));

  socket->AddExpectedData(
    reinterpret_cast<const uint8_t*>(POLL_REPLY_MESSAGE),
    sizeof(POLL_REPLY_MESSAGE),
    bcast_destination);

  CPPUNIT_ASSERT(node.Start());
  socket->Verify();
  CPPUNIT_ASSERT(socket->CheckNetworkParamsMatch(true, true, 6454, true));

  // now enabled an input port and check that we send a poll
  socket->AddExpectedData(
    reinterpret_cast<const uint8_t*>(POLL_MESSAGE),
    sizeof(POLL_MESSAGE),
    bcast_destination);

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
    reinterpret_cast<uint8_t*>(expected_poll_reply_packet),
    sizeof(expected_poll_reply_packet),
    bcast_destination);

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
    reinterpret_cast<const uint8_t*>(POLL_MESSAGE),
    sizeof(POLL_MESSAGE),
    bcast_destination);
  CPPUNIT_ASSERT(node.SendPoll());
  socket->Verify();

  CPPUNIT_ASSERT(node.Stop());
}
