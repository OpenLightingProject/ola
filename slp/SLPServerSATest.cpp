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
#include "ola/slp/URLEntry.h"
#include "slp/SLPPacketConstants.h"
#include "slp/SLPServer.h"
#include "slp/SLPServerTestHelper.h"
#include "slp/ScopeSet.h"
#include "slp/ServiceEntry.h"
#include "slp/URLListVerifier.h"

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
    CPPUNIT_TEST(testConfiguredScopes);
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
    CPPUNIT_TEST(testMultipleDARegistation);
    CPPUNIT_TEST(testDARegistrationFailure);
    CPPUNIT_TEST(testDARegistrationTimeout);
    CPPUNIT_TEST(testExpiryDuringRegistration);
    CPPUNIT_TEST(testDAShutdownDuringRegistration);
    CPPUNIT_TEST(testDAShutdown);
    CPPUNIT_TEST(testDADeRegistration);
    CPPUNIT_TEST(testDADeRegistrationFailure);
    CPPUNIT_TEST(testDeRegistrationWhileRegistering);
    CPPUNIT_TEST(testDAShutdownDuringDeRegistration);
    CPPUNIT_TEST(testRegistrationWhileDeRegistering);
    CPPUNIT_TEST(testFindLocalServices);
    CPPUNIT_TEST_SUITE_END();

    void testConfiguredScopes();
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
    void testMultipleDARegistation();
    void testDARegistrationFailure();
    void testDARegistrationTimeout();
    void testExpiryDuringRegistration();
    void testDAShutdownDuringRegistration();
    void testDAShutdown();
    void testDADeRegistration();
    void testDADeRegistrationFailure();
    void testDeRegistrationWhileRegistering();
    void testDAShutdownDuringDeRegistration();
    void testRegistrationWhileDeRegistering();
    void testFindLocalServices();

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

    static const IPV4SocketAddress DA1;
    static const IPV4SocketAddress DA2;
    static const IPV4SocketAddress DA3;
    static const IPV4SocketAddress UA1;
    static const ScopeSet SCOPE1, SCOPE2, SCOPE3, SCOPE1_2, EMPTY_SCOPES;
    static const char FOO_SERVICE[], SA_SERVICE[], DA_SERVICE[];
    static const char FOO_LOCALHOST_URL[], BAR_LOCALHOST_URL[];
    static const ServiceEntry SERVICE1, SERVICE2, SERVICE1_2, SERVICE1_EXPIRED;
};


const IPV4SocketAddress SLPServerSATest::DA1 =
    IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
const IPV4SocketAddress SLPServerSATest::DA2 =
    IPV4SocketAddress::FromStringOrDie("10.0.1.2:5570");
const IPV4SocketAddress SLPServerSATest::DA3 =
    IPV4SocketAddress::FromStringOrDie("10.0.1.3:5570");
const IPV4SocketAddress SLPServerSATest::UA1 =
    IPV4SocketAddress::FromStringOrDie("192.168.1.10:5570");
const ScopeSet SLPServerSATest::SCOPE1("one");
const ScopeSet SLPServerSATest::SCOPE2("two");
const ScopeSet SLPServerSATest::SCOPE1_2("one,two");
const ScopeSet SLPServerSATest::EMPTY_SCOPES("");
const ScopeSet SLPServerSATest::SCOPE3("three");
const char SLPServerSATest::FOO_SERVICE[] = "service:foo";
const char SLPServerSATest::SA_SERVICE[] = "service:service-agent";
const char SLPServerSATest::DA_SERVICE[] = "service:directory-agent";
const char SLPServerSATest::FOO_LOCALHOST_URL[] = "service:foo://localhost";
const char SLPServerSATest::BAR_LOCALHOST_URL[] = "service:bar://localhost";
const ServiceEntry SLPServerSATest::SERVICE1(SCOPE1, FOO_LOCALHOST_URL, 300);
const ServiceEntry SLPServerSATest::SERVICE2(SCOPE2, FOO_LOCALHOST_URL, 300);
const ServiceEntry SLPServerSATest::SERVICE1_2(
    SCOPE1_2, FOO_LOCALHOST_URL, 300);
const ServiceEntry SLPServerSATest::SERVICE1_EXPIRED(
    SCOPE1, FOO_LOCALHOST_URL, 0);

CPPUNIT_TEST_SUITE_REGISTRATION(SLPServerSATest);


/**
 * Test the ConfiguredScopes() method.
 */
void SLPServerSATest::testConfiguredScopes() {
  {
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, EMPTY_SCOPES));
    ScopeSet expected_scopes("DEFAULT");
    OLA_ASSERT_EQ(expected_scopes, server->ConfiguredScopes());
  }

  {
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1_2));
    ScopeSet expected_scopes("one,two");
    OLA_ASSERT_EQ(expected_scopes, server->ConfiguredScopes());
  }

  {
    auto_ptr<SLPServer> server(
        m_helper.CreateNewServer(false, ScopeSet("rdmnet")));
    ScopeSet expected_scopes("rdmnet");
    OLA_ASSERT_EQ(expected_scopes, server->ConfiguredScopes());
  }
}


/**
 * Test the SA when no DAs are present.
 */
void SLPServerSATest::testSrvRqst() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // register a service
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1_2));
  m_helper.AdvanceTime(0);

  xid_t xid = 10;

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(SERVICE1_2.url());
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, FOO_SERVICE, SCOPE1);
  }

  // send a unicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(SERVICE1_2.url());
    m_helper.ExpectServiceReply(UA1, ++xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }

  // Try a multicast request but with the SA's IP in the PR list
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    pr_list.insert(
        IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));
    m_helper.InjectServiceRequest(UA1, ++xid, true, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }

  // test a multicast request for a scope that doesn't match the SAs scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, ++xid, true, pr_list, FOO_SERVICE,
                                  SCOPE2);
  }

  // test a unicast request for a scope that doesn't match the SAs scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(UA1, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  SCOPE2);
  }

  // test a multicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, ++xid, true, pr_list, FOO_SERVICE,
                                  EMPTY_SCOPES);
  }

  // test a unicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(UA1, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  EMPTY_SCOPES);
  }

  // de-register, then we should receive no response to a multicast request
  {
    SocketVerifier verifier(&m_udp_socket);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(SERVICE1_2));
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, ++xid, true, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }

  // a unicast request should return a SrvRply with no URL entries
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(UA1, ++xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }
}


/*
 * Test that registering with mis-matched scopes fails.
 */
void SLPServerSATest::testInvalidRegistrations() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // register a service with a lifetime of 0
  ServiceEntry bad_service("one", FOO_LOCALHOST_URL, 0);
  OLA_ASSERT_EQ((uint16_t) INVALID_REGISTRATION,
                server->RegisterService(bad_service));

  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));
  // try to register the same service but with a different set of scopes
  OLA_ASSERT_EQ((uint16_t) SCOPE_NOT_SUPPORTED,
                server->RegisterService(SERVICE1_2));
}


/*
 * Test that various error conditions while de-registering are handled.
 */
void SLPServerSATest::testDeRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // de-register a non-existent service
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(SERVICE1));

  // register a service
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));

  // try to de-register the same service but with a different set of scopes
  OLA_ASSERT_EQ((uint16_t) SCOPE_NOT_SUPPORTED,
                server->DeRegisterService(SERVICE1_2));
}


/**
 * Test for SrvRqsts of the form service:service-agent
 */
void SLPServerSATest::testSrvRqstForServiceAgent() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1_2));

  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(UA1, xid, SCOPE1_2);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, SA_SERVICE, SCOPE1);
  }

  // send a multicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(UA1, xid, SCOPE1_2);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, SA_SERVICE, SCOPE1);
  }

  // send a unicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(UA1, xid, SCOPE1_2);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, SA_SERVICE,
                                  EMPTY_SCOPES);
  }

  // send a multicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(UA1, xid, SCOPE1_2);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, SA_SERVICE,
                                  EMPTY_SCOPES);
  }

  // send a unicast SrvRqst with scopes that don't match, expect an error
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(UA1, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, SA_SERVICE, SCOPE3);
  }

  // send a multicast SrvRqst with scopes that don't match, no response is
  // expected.
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, SA_SERVICE, SCOPE3);
  }
}


/**
 * Test that SAs don't respond to SrvRqsts of the form service:directory-agent
 */
void SLPServerSATest::testSrvRqstForDirectoryAgent() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1_2));


  // send a unicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    xid_t xid = 10;
    URLEntries urls;
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, DA_SERVICE, SCOPE1);
  }

  // send a multicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, 11, true, pr_list, DA_SERVICE, SCOPE1);
  }
}


/**
 * Test that we don't return services that have expired
 */
void SLPServerSATest::testExpiredService() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // register a service
  ServiceEntry service("one,two", FOO_LOCALHOST_URL, 30);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
  m_helper.AdvanceTime(0);

  // expire the service
  m_helper.AdvanceTime(31);

  // send a multicast SrvRqst, expect no response.
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, 10, true, pr_list, FOO_SERVICE, SCOPE1);
  }
}


/**
 * Test for a missing service type
 */
void SLPServerSATest::testMissingServiceType() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(UA1, SERVICE_REPLY, ++xid, PARSE_ERROR);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, "", SCOPE1);
  }

  // send a multicast SrvRqst, this is silently dropped
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, "", SCOPE1);
  }
}


/**
 * Test that we can't configure an SA with no scopes.
 */
void SLPServerSATest::testMisconfiguredSA() {
  // this should switch to 'default'
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, EMPTY_SCOPES));


  // send a unicast SrvRqst, expect a SAAdvert
  {
    xid_t xid = 10;
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectSAAdvert(UA1, xid, ScopeSet("default"));

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, SA_SERVICE,
                                  EMPTY_SCOPES);
  }
}


/**
 * Test Active DA Discovery behaviour
 */
void SLPServerSATest::testActiveDADiscovery() {
  // No DAs present
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);

    // The first request is somewhere between 0 and 3s (CONFIG_START_WAIT)
    // after we start
    m_helper.AdvanceTime(3);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Then another one 2s later.
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);
    m_helper.AdvanceTime(2);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4);
  }

  // A single DA that responds to the first SrvRqst
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);

    m_helper.AdvanceTime(3);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    m_helper.InjectDAAdvert(DA1, 0, false, SLP_OK, 1, SCOPE1);
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later, which includes the first DA in the
    // PRList. The XID changes since the request is different.
    pr_list.insert(DA1.Host());
    m_helper.ExpectDAServiceRequest(1, pr_list, SCOPE1);
    m_helper.AdvanceTime(2);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4);
  }

  // A single DA that responds to the second SrvRqst.
  // This simulates a dropped UDP multicast packet
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);

    m_helper.AdvanceTime(3);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Send another SrvRqst 2s later.
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);
    m_helper.AdvanceTime(2);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Inject the DAAdvert
    m_helper.InjectDAAdvert(DA1, 0, false, SLP_OK, 1, SCOPE1);
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Since we got a response, we should send another SrvRqst
    pr_list.insert(DA1.Host());
    m_helper.ExpectDAServiceRequest(1, pr_list, SCOPE1);
    m_helper.AdvanceTime(4);
    m_udp_socket.Verify();

    // No more after that
    m_helper.AdvanceTime(8);
  }

  // Two DAs that both respond to the first SrvRqst
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);

    m_helper.AdvanceTime(3);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    m_helper.InjectDAAdvert(DA1, 0, false, SLP_OK, 1, SCOPE1);
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    m_helper.InjectDAAdvert(DA2, 0, false, SLP_OK, 1, SCOPE1);
    da_list.insert(DA2.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later, which includes both DAs in the
    // PRList. The XID changes since the request is different.
    pr_list.insert(DA1.Host());
    pr_list.insert(DA2.Host());
    m_helper.ExpectDAServiceRequest(1, pr_list, SCOPE1);
    m_helper.AdvanceTime(2);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4);
  }

  // A single DA that responds with an error. This isn't supposed to happen,
  // but let's make sure we handle it cleanly.
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);

    m_helper.AdvanceTime(3);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    m_helper.InjectDAAdvert(DA1, 0, false, SCOPE_NOT_SUPPORTED, 1, SCOPE1);
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later. The bad DA should not be in the
    // list.
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);
    m_helper.AdvanceTime(2);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4);
  }

  // test a shutdown while DA discovery is running
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);

    m_helper.AdvanceTime(3);
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // test triggering DA discovery while the process is already running
  {
    SocketVerifier verifier(&m_udp_socket);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
    PRList pr_list;
    DAList da_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);

    m_helper.AdvanceTime(3);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    server->TriggerActiveDADiscovery();

    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);
    m_helper.AdvanceTime(2);
    m_udp_socket.Verify();
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    m_helper.AdvanceTime(4);
  }

  // Now make sure we send a SrvRqst for DAs each CONFIG_DA_FIND seconds
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
    m_helper.HandleInitialActiveDADiscovery(SCOPE1);

    // advancing CONFIG_DA_FIND (900)s
    m_helper.ExpectDAServiceRequest(1, pr_list, SCOPE1);
    m_helper.AdvanceTime(900);

    m_udp_socket.Verify();

    // Then another one 2s later.
    m_helper.ExpectDAServiceRequest(1, pr_list, SCOPE1);
    m_helper.AdvanceTime(2);
    m_udp_socket.Verify();

    // And let that one time out
    m_helper.AdvanceTime(4);
  }
}


/**
 * Test Passive DA Discovery behaviour
 */
void SLPServerSATest::testPassiveDADiscovery() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // now inject an unsolicited DAAdvert
  m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 1, SCOPE1);
  DAList da_list;
  da_list.insert(DA1.Host());
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

  // now another DA appears..
  m_helper.InjectDAAdvert(DA2, 0, true, SLP_OK, 1, SCOPE2);
  da_list.insert(DA2.Host());
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

  // Send a truncated DAAdvert with an error code, this shouldn't happen but
  // just check we don't crash. As far as I can see the only way we should get
  // DAAdverts with errors is if we unicast SrvRqsts to DAs, which we don't do
  m_helper.InjectError(DA3, ola::slp::DA_ADVERTISEMENT, 0, SCOPE_NOT_SUPPORTED);
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
}


/**
 * Test that we register with a DA correctly. This checks the case when a
 * service is registered before we have discovered the DAs.
 */
void SLPServerSATest::testActiveDiscoveryRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // pre-register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));
  }

  // the initial DASrvRqst is sent up to 3 seconds (CONFIG_START_WAIT) after
  // startup.
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.ExpectDAServiceRequest(0, pr_list, SCOPE1);
    m_helper.AdvanceTime(3);
  }

    // inject the DA, this causes a SrvReg to be sent soon after
  {
    SocketVerifier verifier(&m_udp_socket);

    m_helper.InjectDAAdvert(DA1, 0, false, SLP_OK, 1, SCOPE1);
    DAList da_list;
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    // We'll register 0-1s after receiving the DAAdvert
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 297);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(1);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    pr_list.insert(DA1.Host());
    m_helper.ExpectDAServiceRequest(2, pr_list, SCOPE1);
    m_helper.AdvanceTime(1);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    // Ack the SrvReg message
    m_helper.InjectSrvAck(DA1, 1, SLP_OK);

    // nothing further
    m_helper.AdvanceTime(4);
  }
}


/**
 * Test that we register with a DA correctly during passive discovery.
 */
void SLPServerSATest::testPassiveDiscoveryRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // register a service
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));

  // one seconds later, a DA appears
  m_helper.AdvanceTime(1);
  DAList da_list;
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 1, SCOPE1);
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we register with the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 299);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(1);
  }

  // And the DA responds...
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectSrvAck(DA1, 1, SLP_OK);
  }

  // now another DA appears, but this one doesn't match our scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA2, 0, true, SLP_OK, 1, SCOPE2);
    da_list.insert(DA2.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // But nothing should happen
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(3);
  }

  // now the first DA sends another DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 1, true, SLP_OK, 1, SCOPE1);
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // Nothing should happen
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(3);
  }

  // Now the first DA reboots, this causes us to re-register
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 2, true, SLP_OK, 1000, SCOPE1);
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we register with the DA, note that 7 seconds have been
  // removed from the service's lifetime.
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 292);
    m_helper.ExpectServiceRegistration(DA1, 2, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(1);
  }

  // And the DA responds...
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectSrvAck(DA1, 2, SLP_OK);
  }
}


/**
 * Check we handle registering with multiple DAs simultaneously
 */
void SLPServerSATest::testMultipleDARegistation() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // 2 DAs appear
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 1, SCOPE1);
    m_helper.InjectDAAdvert(DA2, 0, true, SLP_OK, 1, SCOPE1);
    DAList da_list;
    da_list.insert(DA1.Host());
    da_list.insert(DA2.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // advance the time so the new DA reg doesn't start in the middle of the
  // tests.
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(6);
  }

  xid_t xid = 1;

  // now register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceRegistration(DA1, xid++, true, SCOPE1, SERVICE1);
    m_helper.ExpectServiceRegistration(DA2, xid, true, SCOPE1, SERVICE1);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));
  }

  // the first DA responds
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectSrvAck(DA1, 1, SLP_OK);
  }

  // Let the second SrvReg timeout, this also tests the destructor while a
  // Reg operation is pending.
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry service("one", FOO_LOCALHOST_URL, 298);
    m_helper.ExpectServiceRegistration(DA2, xid, true, SCOPE1, service);
    m_helper.AdvanceTime(2);
  }
}


/**
 * Test that we handle registration failures
 */
void SLPServerSATest::testDARegistrationFailure() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // register a service
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));

  // one seconds later, a DA appears
  m_helper.AdvanceTime(1);
  DAList da_list;
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 1, SCOPE1);
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we register with the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 299);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(1);
  }

  // And the DA responds with an error
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectSrvAck(DA1, 1, SCOPE_NOT_SUPPORTED);
  }
}


/**
 * Test that we handle registration timeouts
 */
void SLPServerSATest::testDARegistrationTimeout() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // register a service
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));

  // one seconds later, a DA appears
  m_helper.AdvanceTime(1);
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 1, SCOPE1);
    DAList da_list;
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we try register with the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 299);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(1);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 297);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(2);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 293);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(4);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(8);

    // confirm the DA is now bad
    DAList da_list;
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }
}


/**
 * Test that we handle the case where a service expiries while we're trying to
 * register it.
 */
void SLPServerSATest::testExpiryDuringRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // register a service
  ServiceEntry service("one", FOO_LOCALHOST_URL, 7);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  // one seconds later, a DA appears
  m_helper.AdvanceTime(1);
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 1, SCOPE1);
    DAList da_list;
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we try register with the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 6);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(1);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 4);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(2);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(4);

    // confirm the DA is still ok
    DAList da_list;
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }
}


/**
 * Test that we handle the case where a DA is shutdown during registration.
 */
void SLPServerSATest::testDAShutdownDuringRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // A DA appears
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 10, true, SLP_OK, 1, SCOPE1);
  }

  // register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, SERVICE1);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));
  }

  // the DA sends a shutdown message
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 0, SCOPE1);
    DAList da_list;
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // and then the timeout expires
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(2);
  }
}


/**
 * Confirm that we don't send SrvRev messages to DAs that have shutdown
 */
void SLPServerSATest::testDAShutdown() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));
  }

  // one seconds later, a DA appears
  m_helper.AdvanceTime(1);
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 1, SCOPE1);
    DAList da_list;
    da_list.insert(DA1.Host());
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // A bit later, we register with the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    ServiceEntry updated_service("one", FOO_LOCALHOST_URL, 299);
    m_helper.ExpectServiceRegistration(DA1, 1, true, SCOPE1, updated_service);
    m_helper.AdvanceTime(1);
  }

  // And the DA responds...
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectSrvAck(DA1, 1, SLP_OK);
  }

  // now the DA tells us it's shutting down
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 0, SCOPE1);
    DAList da_list;
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // register another service, this shouldn't cause any messages to the DA
  {
    SocketVerifier verifier(&m_udp_socket);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));
    m_helper.AdvanceTime(4);
  }
}


/**
 * Test that we de-register with a DA correctly.
 */
void SLPServerSATest::testDADeRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
  xid_t xid = 0;

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // A DA appears
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, xid++, true, SLP_OK, 1, SCOPE1);
  }

  // Register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.RegisterWithDA(server.get(), DA1, SERVICE1, xid++);
  }

  // now de-register the service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceDeRegistration(DA1, xid, SCOPE1, SERVICE1_EXPIRED);
    OLA_ASSERT_EQ((uint16_t) SLP_OK,
                  server->DeRegisterService(SERVICE1_EXPIRED));
    m_helper.InjectSrvAck(DA1, xid++, SLP_OK);
  }

  // register the service again
  m_helper.RegisterWithDA(server.get(), DA1, SERVICE1, xid++);

  // try to de-register, this time the DA doesn't respond.
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceDeRegistration(DA1, xid, SCOPE1, SERVICE1_EXPIRED);
    OLA_ASSERT_EQ((uint16_t) SLP_OK,
                  server->DeRegisterService(SERVICE1_EXPIRED));
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceDeRegistration(DA1, xid, SCOPE1, SERVICE1_EXPIRED);
    m_helper.AdvanceTime(2);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceDeRegistration(DA1, xid, SCOPE1, SERVICE1_EXPIRED);
    m_helper.AdvanceTime(4);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(8);
  }

  DAList da_list;
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
}


/**
 * Test that we handle failures during de-registration.
 */
void SLPServerSATest::testDADeRegistrationFailure() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
  xid_t xid = 0;

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // A DA appears
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, xid++, true, SLP_OK, 1, SCOPE1);
  }

  // Register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.RegisterWithDA(server.get(), DA1, SERVICE1, xid++);
  }

  // now de-register the service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceDeRegistration(DA1, xid, SCOPE1, SERVICE1_EXPIRED);
    OLA_ASSERT_EQ((uint16_t) SLP_OK,
                  server->DeRegisterService(SERVICE1_EXPIRED));
    m_helper.InjectSrvAck(DA1, xid++, SCOPE_NOT_SUPPORTED);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(8);  // check nothing else happens
  }

  // confirm the DA is still marked as healthy
  DAList da_list;
  da_list.insert(DA1.Host());
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
}


/**
 * Check the case where we deregister while the registration process is
 * happening.
 */
void SLPServerSATest::testDeRegistrationWhileRegistering() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
  xid_t xid = 0;

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // A DA appears
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, xid++, true, SLP_OK, 1, SCOPE1);
  }

  // Register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceRegistration(DA1, xid++, true, SCOPE1, SERVICE1);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));
  }

  // Now before the DA can ack, de-register it
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceDeRegistration(DA1, xid++, SCOPE1, SERVICE1);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(SERVICE1));
  }
}


/**
 * Check the case where a DA shuts down during the DeReg process.
 */
void SLPServerSATest::testDAShutdownDuringDeRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
  xid_t xid = 0;

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // A DA appears
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, xid++, true, SLP_OK, 1, SCOPE1);
  }

  // Register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceRegistration(DA1, xid++, true, SCOPE1, SERVICE1);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1));
  }

  // the DA sends a shutdown message
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, 0, true, SLP_OK, 0, SCOPE1);
    DAList da_list;
    m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // and then the timeout expires
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.AdvanceTime(2);
  }
}


/**
 * Check the case where we register while the de-registration process is
 * happening.
 */
void SLPServerSATest::testRegistrationWhileDeRegistering() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));
  xid_t xid = 0;

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // A DA appears
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.InjectDAAdvert(DA1, xid++, true, SLP_OK, 1, SCOPE1);
  }

  // Register a service
  {
    m_helper.RegisterWithDA(server.get(), DA1, SERVICE1, xid++);
  }

  // Now de-register it
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceDeRegistration(DA1, xid++, SCOPE1, SERVICE1);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(SERVICE1));
  }

  // Before the DA can ack. try to register it again
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.RegisterWithDA(server.get(), DA1, SERVICE1, xid++);
  }
}


/**
 * Confirm that we return locally-registered services when running in non-DA
 * node.
 */
void SLPServerSATest::testFindLocalServices() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, SCOPE1));

  // No DAs present
  m_helper.HandleInitialActiveDADiscovery(SCOPE1);

  // Register a service locally
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(SERVICE1_2));

  xid_t xid = 1;
  set<string> search_scopes;
  search_scopes.insert("one");

  // send a multicast SrvRqst, nothing responds
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(SERVICE1_2.url());
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectMulticastServiceRequest(xid, FOO_SERVICE, SCOPE1, pr_list);

    server->FindService(search_scopes, FOO_SERVICE, url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    m_helper.ExpectMulticastServiceRequest(xid, FOO_SERVICE, SCOPE1, pr_list);
    m_helper.AdvanceTime(2);  // first timeout

    m_helper.AdvanceTime(4);  // second timeout
    OLA_ASSERT_TRUE(url_verifier.CallbackRan());
  }
}
