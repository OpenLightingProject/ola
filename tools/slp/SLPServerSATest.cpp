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
 * SLPServerSATest.cpp
 * Tests the SA functionality of the SLPServer class
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
using ola::slp::SERVICE_REPLY;
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


class SLPServerSATest: public CppUnit::TestFixture {
  public:
    SLPServerSATest()
        : m_helper(&m_udp_socket) {
    }

  public:
    CPPUNIT_TEST_SUITE(SLPServerSATest);
    CPPUNIT_TEST(testSrvRqst);
    CPPUNIT_TEST(testInvalidRegistrations);
    CPPUNIT_TEST(testDeRegistration);
    CPPUNIT_TEST(testSrvRqstForServiceAgent);
    CPPUNIT_TEST(testSrvRqstForDirectoryAgent);
    CPPUNIT_TEST(testExpiredService);
    CPPUNIT_TEST(testMissingServiceType);
    CPPUNIT_TEST(testMisconfiguredSA);
    CPPUNIT_TEST(testActiveDADiscovery);
    CPPUNIT_TEST(testPassiveDADiscovery);
    CPPUNIT_TEST(testActiveDiscoveryRegistration);
    CPPUNIT_TEST(testPassiveDiscoveryRegistration);
    CPPUNIT_TEST(testDAShutdown);
    CPPUNIT_TEST(testDADeRegistration);
    CPPUNIT_TEST_SUITE_END();

    void testSrvRqst();
    void testInvalidRegistrations();
    void testDeRegistration();
    void testSrvRqstForServiceAgent();
    void testSrvRqstForDirectoryAgent();
    void testExpiredService();
    void testMissingServiceType();
    void testMisconfiguredSA();
    void testActiveDADiscovery();
    void testPassiveDADiscovery();
    void testActiveDiscoveryRegistration();
    void testPassiveDiscoveryRegistration();
    void testDAShutdown();
    void testDADeRegistration();

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
    typedef set<IPV4Address> DAList;

    MockUDPSocket m_udp_socket;
    SLPServerTestHelper m_helper;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SLPServerSATest);


/**
 * Test the SA when no DAs are present.
 */
void SLPServerSATest::testSrvRqst() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));

  // register a service
  ServiceEntry service("one,two", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
  m_helper.AdvanceTime(0, 0);

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, "service:foo",
                                  scopes);
  }

  // send a unicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(peer, ++xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, "service:foo",
                                  scopes);
  }

  // Try a multicast request but with the SA's IP in the PR list
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    pr_list.insert(
        IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));
    m_helper.InjectServiceRequest(peer, ++xid, true, pr_list, "service:foo",
                                  scopes);
  }

  // test a multicast request for a scope that doesn't match the SAs scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("two");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, ++xid, true, pr_list, "service:foo",
                                  scopes);
  }

  // test a unicast request for a scope that doesn't match the SAs scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(peer, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);

    ScopeSet scopes("two");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, "service:foo",
                                  scopes);
  }

  // test a multicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, ++xid, true, pr_list, "service:foo",
                                  scopes);
  }

  // test a unicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(peer, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);

    ScopeSet scopes("");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, "service:foo",
                                  scopes);
  }

  // de-register, then we should receive no response to a multicast request
  {
    SocketVerifier verifier(&m_udp_socket);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(service));
    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, ++xid, true, pr_list, "service:foo",
                                  scopes);
  }

  // a unicast request should return a SrvRply with no URL entries
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(peer, ++xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, "service:foo",
                                  scopes);
  }
}


/*
 * Test that registering with mis-matched scopes fails.
 */
void SLPServerSATest::testInvalidRegistrations() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));

  // register a service with a lifetime of 0
  ServiceEntry bad_service("one", "service:foo://localhost", 0);
  OLA_ASSERT_EQ((uint16_t) INVALID_REGISTRATION,
                server->RegisterService(bad_service));

  ServiceEntry service("one", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK,
                server->RegisterService(service));
  // try to register the same service but with a different set of scopes
  ServiceEntry service2("one,two", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SCOPE_NOT_SUPPORTED,
                server->RegisterService(service2));
}


/*
 * Test that various error conditions while de-registering are handled.
 */
void SLPServerSATest::testDeRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));

  // de-register a non-existent service
  ServiceEntry missing_service("one", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(missing_service));

  // register a service
  ServiceEntry service("one", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK,
                server->RegisterService(service));

  // try to de-register the same service but with a different set of scopes
  ServiceEntry service2("one,two", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SCOPE_NOT_SUPPORTED,
                server->DeRegisterService(service2));
}


/**
 * Test for SrvRqsts of the form service:service-agent
 */
void SLPServerSATest::testSrvRqstForServiceAgent() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one,two"));

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(peer, xid, ScopeSet("one,two"));

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list,
                                  "service:service-agent", scopes);
  }

  // send a multicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(peer, xid, ScopeSet("one,two"));

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list,
                                  "service:service-agent", scopes);
  }

  // send a unicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(peer, xid, ScopeSet("one,two"));

    ScopeSet scopes;
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list,
                                  "service:service-agent", scopes);
  }

  // send a multicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(peer, xid, ScopeSet("one,two"));

    ScopeSet scopes;
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list,
                                  "service:service-agent", scopes);
  }

  // send a unicast SrvRqst with scopes that don't match, expect an error
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(peer, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);
    ScopeSet scopes("three");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list,
                                  "service:service-agent", scopes);
  }

  // send a multicast SrvRqst with scopes that don't match, no response is
  // expected.
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("three");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list,
                                  "service:service-agent", scopes);
  }
}


/**
 * Test that SAs don't respond to SrvRqsts of the form service:directory-agent
 */
void SLPServerSATest::testSrvRqstForDirectoryAgent() {
  const string da_service = "service:directory-agent";
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one,two"));

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, da_service,
                                  scopes);
  }

  // send a multicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, da_service, scopes);
  }
}


/**
 * Test that we don't return services that have expired
 */
void SLPServerSATest::testExpiredService() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  m_helper.HandleInitialActiveDADiscovery("one");

  // register a service
  ServiceEntry service("one,two", "service:foo://localhost", 30);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
  m_helper.AdvanceTime(0, 0);

  // expire the service
  m_helper.AdvanceTime(31, 0);

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(
        IPV4SocketAddress::FromStringOrDie("192.168.1.1:5570"),
        10, true, pr_list, "service:foo", scopes);
  }
}


/**
 * Test for a missing service type
 */
void SLPServerSATest::testMissingServiceType() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(peer, SERVICE_REPLY, ++xid, PARSE_ERROR);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, "", scopes);
  }

  // send a multicast SrvRqst, this is silently dropped
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, "", scopes);
  }
}


/**
 * Test that we can't configure an SA with no scopes.
 */
void SLPServerSATest::testMisconfiguredSA() {
  // this should switch to 'default'
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, ""));

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(peer, xid, ScopeSet("default"));

    ScopeSet scopes("");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list,
                                  "service:service-agent", scopes);
  }
}


/**
 * Test Active DA Discovery behaviour
 */
void SLPServerSATest::testActiveDADiscovery() {
  ScopeSet scopes("one");
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  IPV4SocketAddress da2 = IPV4SocketAddress::FromStringOrDie("10.0.1.2:5570");

  // No DAs present
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);

    // The first request is somewhere between 0 and 3s (CONFIG_START_WAIT)
    // after we start
    m_helper.AdvanceTime(3, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Then another one 2s later.
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);
    m_helper.AdvanceTime(2, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4, 0);
  }

  // A single DA that responds to the first SrvRqst
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);

    m_helper.AdvanceTime(3, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    m_helper.InjectDAAdvert(da1, 0, false, SLP_OK, 1, scopes);
    da_list.insert(da1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later, which includes the first DA in the
    // PRList. The XID changes since the request is different.
    pr_list.insert(da1.Host());
    m_helper.ExpectDAServiceRequest(1, pr_list, scopes);
    m_helper.AdvanceTime(2, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4, 0);
  }

  // A single DA that responds to the second SrvRqst.
  // This simulates a dropped UDP multicast packet
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);

    m_helper.AdvanceTime(3, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Send another SrvRqst 2s later.
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);
    m_helper.AdvanceTime(2, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Inject the DAAdvert
    m_helper.InjectDAAdvert(da1, 0, false, SLP_OK, 1, scopes);
    da_list.insert(da1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Since we got a response, we should send another SrvRqst
    pr_list.insert(da1.Host());
    m_helper.ExpectDAServiceRequest(1, pr_list, scopes);
    m_helper.AdvanceTime(4, 0);
    m_udp_socket.Verify();

    // No more after that
    m_helper.AdvanceTime(8, 0);
  }

  // Two DAs that both respond to the first SrvRqst
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);

    m_helper.AdvanceTime(3, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    m_helper.InjectDAAdvert(da1, 0, false, SLP_OK, 1, scopes);
    da_list.insert(da1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    m_helper.InjectDAAdvert(da2, 0, false, SLP_OK, 1, scopes);
    da_list.insert(da2.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later, which includes both DAs in the
    // PRList. The XID changes since the request is different.
    pr_list.insert(da1.Host());
    pr_list.insert(da2.Host());
    m_helper.ExpectDAServiceRequest(1, pr_list, scopes);
    m_helper.AdvanceTime(2, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4, 0);
  }

  // A single DA that responds with an error. This isn't supposed to happen,
  // but let's make sure we handle it cleanly.
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);

    m_helper.AdvanceTime(3, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    m_helper.InjectDAAdvert(da1, 0, false, SCOPE_NOT_SUPPORTED, 1, scopes);
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later. The bad DA should not be in the
    // list.
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);
    m_helper.AdvanceTime(2, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4, 0);
  }

  // test a shutdown while DA discovery is running
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);

    m_helper.AdvanceTime(3, 0);
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // test triggering DA discovery while the process is already running
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);

    m_helper.AdvanceTime(3, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    server->TriggerActiveDADiscovery();

    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);
    m_helper.AdvanceTime(2, 0);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4, 0);
  }

  // Now make sure we send a SrvRqst for DAs each CONFIG_DA_FIND seconds
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
    m_helper.HandleInitialActiveDADiscovery("one");

    // advancing CONFIG_DA_FIND (900)s
    m_helper.ExpectDAServiceRequest(1, pr_list, scopes);
    m_helper.AdvanceTime(900, 0);

    m_udp_socket.Verify();

    // Then another one 2s later.
    m_helper.ExpectDAServiceRequest(1, pr_list, scopes);
    m_helper.AdvanceTime(2, 0);
    m_udp_socket.Verify();

    // And let that one time out
    m_helper.AdvanceTime(4, 0);
  }
}


/**
 * Test Passive DA Discovery behaviour
 */
void SLPServerSATest::testPassiveDADiscovery() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  ScopeSet scopes("one");

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery("one");

  // now inject an unsolicited DAAdvert
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 1, scopes);
  DAList da_list;
  da_list.insert(da1.Host());
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

  // now another DA appears..
  IPV4SocketAddress da2 = IPV4SocketAddress::FromStringOrDie("10.0.1.2:5570");
  scopes = ScopeSet("two");
  m_helper.InjectDAAdvert(da2, 0, true, SLP_OK, 1, scopes);
  da_list.insert(da2.Host());
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

  // Send a truncated DAAdvert with an error code, this shouldn't happen but
  // just check we don't crash. As far as I can see the only way we should get
  // DAAdverts with errors is if we unicast SrvRqsts to DAs, which we don't do
  IPV4SocketAddress da3 = IPV4SocketAddress::FromStringOrDie("10.0.1.3:5570");
  m_helper.InjectError(da3, ola::slp::DA_ADVERTISEMENT, 0, SCOPE_NOT_SUPPORTED);
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
}


/**
 * Test that we register with a DA correctly. This checks the case when a
 * service is registered before we have discovered the DAs.
 */
void SLPServerSATest::testActiveDiscoveryRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");

  // pre-register a service
  ServiceEntry service("one", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK,
                server->RegisterService(service));

  // the initial DASrvRqst is sent up to 3 seconds (CONFIG_START_WAIT) after
  // startup.
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, scopes);
    m_helper.AdvanceTime(3, 0);
    m_udp_socket.Verify();

    // inject the DA, this causes a SrvReg to be sent
    m_helper.InjectDAAdvert(da1, 0, false, SLP_OK, 1, scopes);
    DAList da_list;
    da_list.insert(da1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // We'll register 0-1s after receiving the DAAdvert
    ServiceEntry updated_service("one", "service:foo://localhost", 297);
    m_helper.ExpectServiceRegistration(da1, 1, true, ScopeSet("one"),
                                       updated_service);
    m_helper.AdvanceTime(1, 0);
    m_udp_socket.Verify();

    pr_list.insert(da1.Host());
    m_helper.ExpectDAServiceRequest(2, pr_list, scopes);
    m_helper.AdvanceTime(1, 0);

    // Ack the SrvReg message
    m_helper.InjectSrvAck(da1, 1, SLP_OK);

    // nothing further
    m_helper.AdvanceTime(4, 0);
  }
}


/**
 * Test that we register with a DA correctly during passive discovery.
 */
void SLPServerSATest::testPassiveDiscoveryRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  ScopeSet scopes("one");

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery("one");

  // register a service
  ServiceEntry service("one", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  // one seconds later, a DA appears
  m_helper.AdvanceTime(1, 0);
  DAList da_list;
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 1, scopes);
    da_list.insert(da1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we register with the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", "service:foo://localhost", 299);
    m_helper.ExpectServiceRegistration(da1, 1, true, scopes, updated_service);
    m_helper.AdvanceTime(1, 0);
  }

  // And the DA responds...
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectSrvAck(da1, 1, SLP_OK);
  }

  // now another DA appears, but this one doesn't match our scopes
  IPV4SocketAddress da2 = IPV4SocketAddress::FromStringOrDie("10.0.1.2:5570");
  ScopeSet scopes2("two");
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(da2, 0, true, SLP_OK, 1, scopes2);
    da_list.insert(da2.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // But nothing should happen
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(3, 0);
  }

  // now the first DA sends another DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(da1, 1, true, SLP_OK, 1, scopes);
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // Nothing should happen
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(3, 0);
  }

  // Now the first DA reboots, this causes us to re-register
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(da1, 2, true, SLP_OK, 1000, scopes);
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we register with the DA, note that 7 seconds have been
  // removed from the service's lifetime.
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", "service:foo://localhost", 292);
    m_helper.ExpectServiceRegistration(da1, 2, true, scopes, updated_service);
    m_helper.AdvanceTime(1, 0);
  }

  // And the DA responds...
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectSrvAck(da1, 2, SLP_OK);
  }
}


/**
 * Confirm that we don't send SrvRev messages to DAs that have shutdown
 */
void SLPServerSATest::testDAShutdown() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  ScopeSet scopes("one");

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery("one");

  // register a service
  ServiceEntry service("one", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  // one seconds later, a DA appears
  m_helper.AdvanceTime(1, 0);
  DAList da_list;
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 1, scopes);
    da_list.insert(da1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we register with the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", "service:foo://localhost", 299);
    m_helper.ExpectServiceRegistration(da1, 1, true, scopes, updated_service);
    m_helper.AdvanceTime(1, 0);
  }

  // And the DA responds...
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectSrvAck(da1, 1, SLP_OK);
  }

  // now the DA tells us it's shutting down
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 0, scopes);
    da_list.insert(da1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // register another service, this shouldn't cause any messages to the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry service("one", "service:bar://localhost", 300);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
    m_helper.AdvanceTime(4, 0);
  }
}


/**
 * Test that we de-register with a DA correctly.
 */
void SLPServerSATest::testDADeRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  ScopeSet scopes("one");
  xid_t xid = 0;

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery("one");

  // A DA appears
  DAList da_list;
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  m_helper.InjectDAAdvert(da1, xid++, true, SLP_OK, 1, scopes);
  m_udp_socket.Verify();

  OLA_INFO << "registering service";
  // Register a service
  ServiceEntry service("one", "service:foo://localhost", 300);
  m_helper.RegisterWithDA(server.get(), da1, service, xid++);

  // now de-register the service
  ServiceEntry dereg_service("one", "service:foo://localhost", 0);
  m_helper.ExpectServiceDeRegistration(da1, xid, scopes, dereg_service);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(dereg_service));
  m_helper.InjectSrvAck(da1, xid++, SLP_OK);

  // register the service again
  m_helper.RegisterWithDA(server.get(), da1, service, xid++);

  // try to de-register, this time the DA doesn't respond.
  OLA_INFO << "DeReg";
  m_helper.ExpectServiceDeRegistration(da1, xid, scopes, dereg_service);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(dereg_service));

  m_helper.ExpectServiceDeRegistration(da1, xid, scopes, dereg_service);
  m_helper.AdvanceTime(2, 0);

  m_helper.ExpectServiceDeRegistration(da1, xid, scopes, dereg_service);
  m_helper.AdvanceTime(4, 0);

  m_helper.ExpectServiceDeRegistration(da1, xid, scopes, dereg_service);
  m_helper.AdvanceTime(8, 0);

  // what about an outstanding request when we exit
}


// test DA failure

// test what happens when the DA sends us a failure message
// all sorts of failures to test here
