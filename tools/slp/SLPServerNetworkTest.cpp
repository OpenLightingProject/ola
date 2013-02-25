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
 * SLPServerNetworkTest.cpp
 * Tests the common network functionality of the SLPServer class.
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include "ola/Logging.h"
#include "ola/math/Random.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/MockUDPSocket.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/SLPServerTestHelper.h"
#include "tools/slp/URLEntry.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::slp::SLPServer;
using ola::slp::URLEntry;
using ola::testing::MockUDPSocket;
using ola::testing::SocketVerifier;
using std::auto_ptr;


class SLPServerNetworkTest: public CppUnit::TestFixture {
  public:
    SLPServerNetworkTest()
        : m_helper(&m_udp_socket) {
    }

  public:
    CPPUNIT_TEST_SUITE(SLPServerNetworkTest);
    CPPUNIT_TEST(testMalformedPackets);
    CPPUNIT_TEST(testUnmatchedAcks);
    CPPUNIT_TEST(testUnmatchedSrvRply);
    CPPUNIT_TEST(testShortPackets);
    CPPUNIT_TEST_SUITE_END();

    void testMalformedPackets();
    void testUnmatchedAcks();
    void testUnmatchedSrvRply();
    void testShortPackets();

  public:
    void setUp() {
      ola::math::InitRandom();
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      m_udp_socket.Init();
      m_udp_socket.SetInterface(
          IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));
      m_udp_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                        SLPServerTestHelper::SLP_TEST_PORT));
      // make sure WakeUpTime is populated
      m_helper.RunOnce();
    }

  private:
    MockUDPSocket m_udp_socket;
    SLPServerTestHelper m_helper;

    static const IPV4SocketAddress peer;
    static const ScopeSet TEST_SCOPE;
};


const IPV4SocketAddress SLPServerNetworkTest::peer =
    IPV4SocketAddress::FromStringOrDie("192.168.1.1:5570");
const ScopeSet SLPServerNetworkTest::TEST_SCOPE("one");

CPPUNIT_TEST_SUITE_REGISTRATION(SLPServerNetworkTest);


/**
 * Send various malformed packets to make sure we don't crash the server.
 */
void SLPServerNetworkTest::testMalformedPackets() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, TEST_SCOPE));
  SocketVerifier verifier(&m_udp_socket);

  uint8_t very_short_packet[] = {2};
  m_udp_socket.InjectData(very_short_packet, sizeof(very_short_packet), peer);

  uint8_t attribute_request_packet[] = {2, 6};
  m_udp_socket.InjectData(attribute_request_packet,
                          sizeof(attribute_request_packet), peer);

  uint8_t attribute_reply_packet[] = {2, 7};
  m_udp_socket.InjectData(attribute_reply_packet,
                          sizeof(attribute_reply_packet), peer);

  uint8_t service_type_request_packet[] = {2, 9};
  m_udp_socket.InjectData(service_type_request_packet,
                          sizeof(service_type_request_packet), peer);

  uint8_t service_type_reply_packet[] = {2, 10};
  m_udp_socket.InjectData(service_type_reply_packet,
                          sizeof(service_type_reply_packet), peer);

  uint8_t sa_advert_packet[] = {2, 11};
  m_udp_socket.InjectData(sa_advert_packet, sizeof(sa_advert_packet), peer);

  uint8_t unknown_packet[] = {2, 16};
  m_udp_socket.InjectData(unknown_packet, sizeof(unknown_packet), peer);
}


/**
 * Test that we can't crash the server by sending un-matched SrvAcks.
 */
void SLPServerNetworkTest::testUnmatchedAcks() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, TEST_SCOPE));
  SocketVerifier verifier(&m_udp_socket);
  m_helper.InjectSrvAck(peer, 1234, 0x5678);
}


/**
 * Test that we can't crash the server by sending un-matched SrvRply.
 */
void SLPServerNetworkTest::testUnmatchedSrvRply() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, TEST_SCOPE));
  SocketVerifier verifier(&m_udp_socket);

  URLEntries urls;
  urls.push_back(URLEntry("service:foo://1.1.1.1", 300));
  urls.push_back(URLEntry("service:foo://1.1.1.10", 600));
  m_helper.InjectServiceReply(peer, 0x1234, ola::slp::OPTION_NOT_UNDERSTOOD,
                              urls);
}


/**
 * Send various short packets.
 */
void SLPServerNetworkTest::testShortPackets() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, TEST_SCOPE));
  SocketVerifier verifier(&m_udp_socket);

  uint8_t srv_request_packet[] = {2, 1};
  m_udp_socket.InjectData(srv_request_packet, sizeof(srv_request_packet), peer);

  uint8_t srv_reply_packet[] = {2, 2};
  m_udp_socket.InjectData(srv_reply_packet, sizeof(srv_reply_packet), peer);

  uint8_t srv_reg_packet[] = {2, 3};
  m_udp_socket.InjectData(srv_reg_packet, sizeof(srv_reg_packet), peer);

  uint8_t srv_dereg_packet[] = {2, 4};
  m_udp_socket.InjectData(srv_dereg_packet, sizeof(srv_request_packet), peer);

  uint8_t srv_ack_packet[] = {2, 5};
  m_udp_socket.InjectData(srv_ack_packet, sizeof(srv_request_packet), peer);

  uint8_t daadvert_packet[] = {2, 8};
  m_udp_socket.InjectData(daadvert_packet, sizeof(daadvert_packet), peer);

  uint8_t srv_type_packet[] = {2, 9};
  m_udp_socket.InjectData(srv_type_packet, sizeof(srv_type_packet), peer);

  uint8_t saadvert_packet[] = {2, 11};
  m_udp_socket.InjectData(saadvert_packet, sizeof(saadvert_packet), peer);
}
