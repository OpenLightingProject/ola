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
 * SLPServerDATest.cpp
 * Tests the DA functionality of the SLPServer class
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include "ola/Logging.h"
#include "ola/math/Random.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/MockUDPSocket.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/ServiceEntry.h"
#include "tools/slp/URLEntry.h"
#include "tools/slp/SLPServerTestHelper.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::slp::INVALID_REGISTRATION;
using ola::slp::PARSE_ERROR;
using ola::slp::SCOPE_NOT_SUPPORTED;
using ola::slp::SLPServer;
using ola::slp::SLP_OK;
using ola::slp::ScopeSet;
using ola::slp::ServiceEntry;
using ola::slp::URLEntries;
using ola::slp::xid_t;
using ola::testing::MockUDPSocket;
using ola::testing::SocketVerifier;
using std::auto_ptr;
using std::set;
using std::string;


class SLPServerDATest: public CppUnit::TestFixture {
  public:
    SLPServerDATest()
        : m_helper(&m_udp_socket) {
    }

  public:
    CPPUNIT_TEST_SUITE(SLPServerDATest);
    CPPUNIT_TEST(testDABeat);
    CPPUNIT_TEST(testSrvRqstForDirectoryAgent);
    CPPUNIT_TEST_SUITE_END();

    void testDABeat();
    void testSrvRqstForDirectoryAgent();

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
    typedef set<IPV4Address> PRList;

    MockUDPSocket m_udp_socket;
    SLPServerTestHelper m_helper;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SLPServerDATest);


/**
 * Test that we send a DAAdvert on startup, and every CONFIG_DA_BEAT seconds.
 */
void SLPServerDATest::testDABeat() {
  auto_ptr<SLPServer> server;
  ScopeSet scopes("one,two");
  xid_t xid = 0;

  // expect a DAAdvert on startup
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectMulticastDAAdvert(
        xid++, SLPServerTestHelper::INITIAL_BOOT_TIME, scopes);
    server.reset(m_helper.CreateNewServer(true, "one,two"));
  }

  m_helper.HandleInitialActiveDADiscovery("one,two");
  m_helper.AdvanceTime(899, 0);

  /*
   * Each cycle takes 906 seconds. 899s of waiting, 1s tick, 6 seconds for the
   * discovery to timeout. The first cycle is different because it includes
   * CONFIG_START_WAIT, so that takes 908 seconds (3 + 6 + 899).
   * CONFIG_DA_BEAT is 10800 (3h).
   * So:
   *   (10800 - 908) / 906 = 10
   */
  for (unsigned int i = 0; i < 10; i++) {
    m_helper.HandleActiveDADiscovery("one,two", xid++);
    m_helper.AdvanceTime(899, 0);
  }

  // 908 + 10 * 906 = 9968 so there is one more discovery to take place
  m_helper.HandleActiveDADiscovery("one,two", xid++);

  // We want to move to 10799 so: 10799 - 9968 - 7 = 824s
  m_helper.AdvanceTime(824, 0);

  // verify we now send an unsolicited DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    // unsolicited DAAdverts have a xid of 0
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, scopes);
    m_helper.AdvanceTime(1, 0);
  }
}

/**
 * Test that we respond to a SrvRqst for service:directory-agent
 */
void SLPServerDATest::testSrvRqstForDirectoryAgent() {
  auto_ptr<SLPServer> server;
  ScopeSet scopes("one,two");

  // expect a DAAdvert on startup
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, scopes);
    server.reset(m_helper.CreateNewServer(true, "one,two"));
  }

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(peer, xid, false, SLP_OK,
        SLPServerTestHelper::INITIAL_BOOT_TIME, ScopeSet("one,two"));

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list,
                                  "service:directory-agent", scopes);
  }

  // send a multicast SrvRqst, expect a DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(peer, xid, false, SLP_OK,
        SLPServerTestHelper::INITIAL_BOOT_TIME, ScopeSet("one,two"));

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list,
                                  "service:directory-agent", scopes);
  }
}


// Test for srvrqst to service:directory-agent

// test Reg

// test reg & lookup

// test local reg & lookup

// test timeout


// re-reg ?
