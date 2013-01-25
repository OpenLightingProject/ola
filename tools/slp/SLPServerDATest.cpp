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
#include <vector>

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
using ola::slp::DA_ADVERTISEMENT;
using ola::slp::INVALID_REGISTRATION;
using ola::slp::PARSE_ERROR;
using ola::slp::SCOPE_NOT_SUPPORTED;
using ola::slp::SERVICE_REPLY;
using ola::slp::SERVICE_TYPE_REPLY;
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
    CPPUNIT_TEST(testConfiguredScopes);
    CPPUNIT_TEST(testDumpStore);
    CPPUNIT_TEST(testDABeat);
    CPPUNIT_TEST(testSrvRqstForDirectoryAgent);
    CPPUNIT_TEST(testSrvRqstForServiceAgent);
    CPPUNIT_TEST(testSrvRqstForLocalService);
    CPPUNIT_TEST(testLocalServiceTimeout);
    CPPUNIT_TEST(testRegistration);
    CPPUNIT_TEST(testDeRegistration);
    CPPUNIT_TEST(testSrvRqstForRemoteService);
    CPPUNIT_TEST(testRemoteServiceTimeout);
    CPPUNIT_TEST(testServiceTypeRequests);
    CPPUNIT_TEST_SUITE_END();

    void testConfiguredScopes();
    void testDumpStore();
    void testDABeat();
    void testSrvRqstForDirectoryAgent();
    void testSrvRqstForServiceAgent();
    void testSrvRqstForLocalService();
    void testLocalServiceTimeout();
    void testRegistration();
    void testDeRegistration();
    void testSrvRqstForRemoteService();
    void testRemoteServiceTimeout();
    void testServiceTypeRequests();

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

    static const char DA_SERVICE[];
    static const ScopeSet SCOPE1, SCOPE2, SCOPE3, SCOPE1_2, DA_SCOPES;
    static const ScopeSet EMPTY_SCOPES;
    static const char FOO_SERVICE[];
    static const char FOO_LOCALHOST_URL[], BAR_LOCALHOST_URL[];
    static const IPV4SocketAddress UA1;
};


const char SLPServerDATest::DA_SERVICE[] = "service:directory-agent";
const ScopeSet SLPServerDATest::DA_SCOPES("one,two");
const ScopeSet SLPServerDATest::SCOPE1("one");
const ScopeSet SLPServerDATest::SCOPE2("two");
const ScopeSet SLPServerDATest::SCOPE1_2("one,two");
const ScopeSet SLPServerDATest::SCOPE3("three");
const ScopeSet SLPServerDATest::EMPTY_SCOPES("");
const char SLPServerDATest::FOO_SERVICE[] = "service:foo";
const char SLPServerDATest::FOO_LOCALHOST_URL[] = "service:foo://localhost";
const char SLPServerDATest::BAR_LOCALHOST_URL[] = "service:bar://localhost";
const IPV4SocketAddress SLPServerDATest::UA1 =
    IPV4SocketAddress::FromStringOrDie("192.168.1.10:5570");

CPPUNIT_TEST_SUITE_REGISTRATION(SLPServerDATest);


/**
 * Test the ConfiguredScopes() method.
 */
void SLPServerDATest::testConfiguredScopes() {
  {
    ScopeSet expected_scopes("DEFAULT");
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, expected_scopes);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(true, EMPTY_SCOPES));
    OLA_ASSERT_EQ(expected_scopes, server->ConfiguredScopes());
    m_helper.ExpectMulticastDAAdvert(0, 0, expected_scopes);
  }

  {
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, SCOPE1_2);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(true, DA_SCOPES));
    OLA_ASSERT_EQ(SCOPE1_2, server->ConfiguredScopes());
    m_helper.ExpectMulticastDAAdvert(0, 0, SCOPE1_2);
  }

  {
    ScopeSet expected_scopes("rdmnet");
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, expected_scopes);
    auto_ptr<SLPServer> server(
        m_helper.CreateNewServer(true, ScopeSet("rdmnet")));
    OLA_ASSERT_EQ(expected_scopes, server->ConfiguredScopes());
    m_helper.ExpectMulticastDAAdvert(0, 0, expected_scopes);
  }
}


/**
 * Test the DumpStore() doesn't crash.
 */
void SLPServerDATest::testDumpStore() {
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));

  // register a service
  ServiceEntry service("one", FOO_LOCALHOST_URL, 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  server->DumpStore();
  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * Test that we send a DAAdvert on startup, and every CONFIG_DA_BEAT seconds.
 */
void SLPServerDATest::testDABeat() {
  auto_ptr<SLPServer> server;
  xid_t xid = 0;

  // expect a DAAdvert on startup
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectMulticastDAAdvert(
        xid++, SLPServerTestHelper::INITIAL_BOOT_TIME, DA_SCOPES);
    server.reset(m_helper.CreateNewServer(true, DA_SCOPES));
  }

  m_helper.HandleInitialActiveDADiscovery(DA_SCOPES);
  m_helper.AdvanceTime(899);

  /*
   * Each cycle takes 906 seconds. 899s of waiting, 1s tick, 6 seconds for the
   * discovery to timeout. The first cycle is different because it includes
   * CONFIG_START_WAIT, so that takes 908 seconds (3 + 6 + 899).
   * CONFIG_DA_BEAT is 10800 (3h).
   * So:
   *   (10800 - 908) / 906 = 10
   */
  for (unsigned int i = 0; i < 10; i++) {
    m_helper.HandleActiveDADiscovery(DA_SCOPES, xid++);
    m_helper.AdvanceTime(899);
  }

  // 908 + 10 * 906 = 9968 so there is one more discovery to take place
  m_helper.HandleActiveDADiscovery(DA_SCOPES, xid++);

  // We want to move to 10799 so: 10799 - 9968 - 7 = 824s
  m_helper.AdvanceTime(824);

  // verify we now send an unsolicited DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    // unsolicited DAAdverts have a xid of 0
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, DA_SCOPES);
    m_helper.AdvanceTime(1);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * Test that we respond to a SrvRqst for service:directory-agent
 */
void SLPServerDATest::testSrvRqstForDirectoryAgent() {
  auto_ptr<SLPServer> server;

  // expect a DAAdvert on startup
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectMulticastDAAdvert(0, SLPServerTestHelper::INITIAL_BOOT_TIME,
                                     DA_SCOPES);
    server.reset(m_helper.CreateNewServer(true, DA_SCOPES));
  }

  xid_t xid = 10;

  // send a unicast SrvRqst, expect a DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(UA1, xid, false, SLP_OK,
                            SLPServerTestHelper::INITIAL_BOOT_TIME, DA_SCOPES);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, DA_SERVICE, SCOPE1);
  }

  // send a multicast SrvRqst, expect a DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(UA1, xid, false, SLP_OK,
                            SLPServerTestHelper::INITIAL_BOOT_TIME, DA_SCOPES);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, DA_SERVICE, SCOPE1);
  }

  // send a unicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(UA1, xid, false, SLP_OK,
        SLPServerTestHelper::INITIAL_BOOT_TIME, DA_SCOPES);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, DA_SERVICE,
                                  EMPTY_SCOPES);
  }

  // send a multicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(UA1, xid, false, SLP_OK,
        SLPServerTestHelper::INITIAL_BOOT_TIME, DA_SCOPES);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, DA_SERVICE,
                                  EMPTY_SCOPES);
  }

  // send a multicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectError(UA1, DA_ADVERTISEMENT, ++xid, SCOPE_NOT_SUPPORTED);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, DA_SERVICE, SCOPE3);
  }

  // send a multicast SrvRqst with scopes that don't match, no response is
  // expected.
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, DA_SERVICE, SCOPE3);
  }

  // Try a unicast request but with the DA's IP in the PR list
  // This shouldn't happen but check it anyway.
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    pr_list.insert(
        IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, DA_SERVICE, SCOPE1);
  }

  // Try a multicast request but with the DA's IP in the PR list
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    pr_list.insert(
        IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, DA_SERVICE, SCOPE1);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * Test that DAs don't respond to SrvRqsts of the form service:service-agent
 */
void SLPServerDATest::testSrvRqstForServiceAgent() {
  const string sa_service = "service:service-agent";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

  xid_t xid = 10;

  // send a unicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, sa_service, SCOPE1);
  }

  // send a multicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, sa_service, SCOPE1);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * Test the we respond correctly to SrvRqsts for locally registered services
 */
void SLPServerDATest::testSrvRqstForLocalService() {
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));

  // register a service
  ServiceEntry service("one", FOO_LOCALHOST_URL, 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
  m_helper.AdvanceTime(0);

  // now perform various SrvRqsts
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, FOO_SERVICE,
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

  // test a multicast request for a scope that the DA doesn't support
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, ++xid, true, pr_list, FOO_SERVICE,
                                  SCOPE3);
  }

  // test a unicast request for a scope that the DA doesn't support
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(UA1, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  SCOPE3);
  }

  // test a multicast request for a scope that the DA supports, but the service
  // wasn't registered with
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, ++xid, true, pr_list, FOO_SERVICE,
                                  SCOPE2);
  }

  // test a unicast request for a scope that the DA supports, but the service
  // wasn't registered with
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  SCOPE2);
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

  // test a multicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, ++xid, true, pr_list, FOO_SERVICE,
                                  EMPTY_SCOPES);
  }

  // de-register the service
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(service));
  m_helper.AdvanceTime(0);

  // send a unicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }

  // send a multicast SrvRqst, expect no response
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * Check that we expire local services when they timeout
 */
void SLPServerDATest::testLocalServiceTimeout() {
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));

  // register a service
  ServiceEntry service("one", FOO_LOCALHOST_URL, 10);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  // this should time the service out
  m_helper.AdvanceTime(11);

  // now perform various SrvRqsts
  xid_t xid = 10;

  // send a unicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, false, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }

  // send a multicast SrvRqst, expect no response
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, FOO_SERVICE,
                                  SCOPE1);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * test the SrvReg handling
 */
void SLPServerDATest::testRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));

  xid_t xid = 10;

  // register a service, expect an Ack
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SLP_OK);

    ServiceEntry service("one", FOO_LOCALHOST_URL, 300);
    m_helper.InjectServiceRegistration(UA1, xid++, true, SCOPE1, service);
  }

  // Try to register the same service with a different set of scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SCOPE_NOT_SUPPORTED);

    ServiceEntry service("one,two", FOO_LOCALHOST_URL, 300);
    m_helper.InjectServiceRegistration(UA1, xid++, true, SCOPE1_2, service);
  }

  // same thing, but this time without the fresh flag
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SCOPE_NOT_SUPPORTED);

    ServiceEntry service("one,two", FOO_LOCALHOST_URL, 300);
    m_helper.InjectServiceRegistration(UA1, xid++, false, SCOPE1_2, service);
  }

  // try a service that we don't know about, with the fresh flag unset
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, ola::slp::INVALID_UPDATE);

    ServiceEntry service("one", "service:bar://localhost", 300);
    m_helper.InjectServiceRegistration(UA1, xid++, false, SCOPE1, service);
  }

  // try a lifetime of 0, should return INVALID_REG
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, ola::slp::INVALID_REGISTRATION);

    ServiceEntry service("one", FOO_LOCALHOST_URL, 0);
    m_helper.InjectServiceRegistration(UA1, xid++, true, SCOPE1, service);
  }

  // try a request that doesn't match the DA's scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, ola::slp::SCOPE_NOT_SUPPORTED);

    ServiceEntry service("one,three", FOO_LOCALHOST_URL, 300);
    ScopeSet scopes("one,three");
    m_helper.InjectServiceRegistration(UA1, xid++, true, scopes, service);
  }

  // try to re-register, with and without the fresh flag
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SLP_OK);
    ServiceEntry service("one", FOO_LOCALHOST_URL, 300);
    m_helper.InjectServiceRegistration(UA1, xid++, true, SCOPE1, service);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SLP_OK);
    ServiceEntry service("one", FOO_LOCALHOST_URL, 300);
    m_helper.InjectServiceRegistration(UA1, xid++, false, SCOPE1, service);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * test the SrvDeReg handling
 */
void SLPServerDATest::testDeRegistration() {
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));

  ServiceEntry service("one", FOO_LOCALHOST_URL, 300);
  xid_t xid = 10;

  // try to de-reg a service that isn't registered
  // The RFC isn't clear what to return here, none of the error codes really
  // match so we return SLP_OK.
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SLP_OK);

    m_helper.InjectServiceDeRegistration(UA1, xid, DA_SCOPES, service);
  }

  // register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SLP_OK);

    m_helper.InjectServiceRegistration(UA1, xid++, true, DA_SCOPES, service);
  }

  // try to de-register the service without any scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SCOPE_NOT_SUPPORTED);

    m_helper.InjectServiceDeRegistration(UA1, xid++, EMPTY_SCOPES, service);
  }

  // try to de-register the service with a subset of the scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SCOPE_NOT_SUPPORTED);

    m_helper.InjectServiceDeRegistration(UA1, xid++, SCOPE1, service);
  }

  // confirm we still reply to a SrvRqst
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, FOO_SERVICE,
                                  DA_SCOPES);
  }

  // now actually de-register the service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SLP_OK);

    m_helper.InjectServiceDeRegistration(UA1, xid++, DA_SCOPES, service);
  }

  // confirm we no longer reply to a SrvRqst
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid, true, pr_list, FOO_SERVICE,
                                  DA_SCOPES);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * Confirm that we respond to SrvRqst for registered services.
 */
void SLPServerDATest::testSrvRqstForRemoteService() {
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));

  ServiceEntry service("one", FOO_LOCALHOST_URL, 300);
  xid_t xid = 10;

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SLP_OK);
    m_helper.InjectServiceRegistration(UA1, xid, true, DA_SCOPES, service);
  }

  // send a unicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid++, false, pr_list, FOO_SERVICE,
                                  DA_SCOPES);
  }

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid++, true, pr_list, FOO_SERVICE,
                                  DA_SCOPES);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * Check that we expire remotely-registered services correctly.
 */
void SLPServerDATest::testRemoteServiceTimeout() {
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));

  xid_t xid = 10;
  ServiceEntry service("one", FOO_LOCALHOST_URL, 10);

  // register
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(UA1, xid, SLP_OK);
    m_helper.InjectServiceRegistration(UA1, xid, true, DA_SCOPES, service);
  }

  // this should time the service out
  m_helper.AdvanceTime(11);

  // send a unicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(UA1, xid, SLP_OK, urls);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid++, false, pr_list, FOO_SERVICE,
                                  DA_SCOPES);
  }

  // send a multicast SrvRqst, expect no response
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(UA1, xid++, true, pr_list, FOO_SERVICE,
                                  DA_SCOPES);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}


/**
 * Check that we respond to SrvTypeRqsts correctly.
 */
void SLPServerDATest::testServiceTypeRequests() {
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));

//  xid_t xid = 10;

  // register some services
  OLA_ASSERT_EQ(
      (uint16_t) SLP_OK,
      server->RegisterService(ServiceEntry("one,two", FOO_LOCALHOST_URL, 300)));
  OLA_ASSERT_EQ(
      (uint16_t) SLP_OK,
      server->RegisterService(ServiceEntry("one,two", BAR_LOCALHOST_URL, 300)));

  // a service with a naming authority
  ServiceEntry service_with_naming_auth(
      "one", "service:baz.auth://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK,
                server->RegisterService(service_with_naming_auth));

  xid_t xid = 10;
  // get all services for scope "one"
  {
    SocketVerifier verifier(&m_udp_socket);
    vector<string> service_types;
    service_types.push_back("service:bar");
    service_types.push_back("service:baz.auth");
    service_types.push_back("service:foo");
    PRList pr_list;

    m_helper.ExpectServiceTypeReply(UA1, xid, SLP_OK, service_types);
    m_helper.InjectAllServiceTypeRequest(UA1, xid++, pr_list, SCOPE1);
  }

  // limit to scope "two"
  {
    vector<string> service_types;
    service_types.push_back("service:bar");
    service_types.push_back("service:foo");
    PRList pr_list;

    m_helper.ExpectServiceTypeReply(UA1, xid, SLP_OK, service_types);
    m_helper.InjectAllServiceTypeRequest(UA1, xid++, pr_list, SCOPE2);
  }

  // test the PR list works
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    pr_list.insert(
        IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));

    m_helper.InjectAllServiceTypeRequest(UA1, xid++, pr_list, SCOPE1);
  }

  // test the IANA scopes
  {
    vector<string> service_types;
    service_types.push_back("service:bar");
    service_types.push_back("service:foo");
    PRList pr_list;

    m_helper.ExpectServiceTypeReply(UA1, xid, SLP_OK, service_types);
    m_helper.InjectServiceTypeRequest(UA1, xid++, pr_list, "", SCOPE1);
  }

  // test a specific naming authority
  {
    SocketVerifier verifier(&m_udp_socket);
    vector<string> service_types;
    service_types.push_back("service:baz.auth");
    PRList pr_list;
    m_helper.ExpectServiceTypeReply(UA1, xid, SLP_OK, service_types);
    m_helper.InjectServiceTypeRequest(UA1, xid++, pr_list, "auth", SCOPE1);
  }

  // test the SCOPE_NOT_SUPPORTED error, the request is multicast so there is
  // no response.
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectAllServiceTypeRequest(UA1, xid++, pr_list, ScopeSet("four"));
  }

  // test a naming auth that returns no results
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceTypeRequest(UA1, xid++, pr_list, "cat", SCOPE1);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, DA_SCOPES);
}
