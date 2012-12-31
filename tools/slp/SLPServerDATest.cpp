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
using ola::slp::DA_ADVERTISEMENT;
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

    static const char DA_SCOPES[];
    static const char DA_SERVICE[];
};


const char SLPServerDATest::DA_SCOPES[] = "one,two";
const char SLPServerDATest::DA_SERVICE[] = "service:directory-agent";

CPPUNIT_TEST_SUITE_REGISTRATION(SLPServerDATest);


/**
 * Test the ConfiguredScopes() method.
 */
void SLPServerDATest::testConfiguredScopes() {
  {
    ScopeSet expected_scopes("DEFAULT");
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, expected_scopes);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(true, ""));
    OLA_ASSERT_EQ(expected_scopes, server->ConfiguredScopes());
    m_helper.ExpectMulticastDAAdvert(0, 0, expected_scopes);
  }

  {
    ScopeSet expected_scopes("one,two");
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, expected_scopes);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(true, "one,two"));
    OLA_ASSERT_EQ(expected_scopes, server->ConfiguredScopes());
    m_helper.ExpectMulticastDAAdvert(0, 0, expected_scopes);
  }

  {
    ScopeSet expected_scopes("rdmnet");
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, expected_scopes);
    auto_ptr<SLPServer> server(m_helper.CreateNewServer(true, "rdmnet"));
    OLA_ASSERT_EQ(expected_scopes, server->ConfiguredScopes());
    m_helper.ExpectMulticastDAAdvert(0, 0, expected_scopes);
  }
}


/**
 * Test the DumpStore() doesn't crash.
 */
void SLPServerDATest::testDumpStore() {
  const string foo_service = "service:foo";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

  // register a service
  ServiceEntry service("one", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  server->DumpStore();
  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Test that we send a DAAdvert on startup, and every CONFIG_DA_BEAT seconds.
 */
void SLPServerDATest::testDABeat() {
  auto_ptr<SLPServer> server;
  ScopeSet scopes(DA_SCOPES);
  xid_t xid = 0;

  // expect a DAAdvert on startup
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectMulticastDAAdvert(
        xid++, SLPServerTestHelper::INITIAL_BOOT_TIME, scopes);
    server.reset(m_helper.CreateNewServer(true, DA_SCOPES));
  }

  m_helper.HandleInitialActiveDADiscovery(DA_SCOPES);
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
    m_helper.HandleActiveDADiscovery(DA_SCOPES, xid++);
    m_helper.AdvanceTime(899, 0);
  }

  // 908 + 10 * 906 = 9968 so there is one more discovery to take place
  m_helper.HandleActiveDADiscovery(DA_SCOPES, xid++);

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

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Test that we respond to a SrvRqst for service:directory-agent
 */
void SLPServerDATest::testSrvRqstForDirectoryAgent() {
  auto_ptr<SLPServer> server;
  ScopeSet scopes(DA_SCOPES);

  // expect a DAAdvert on startup
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectMulticastDAAdvert(
        0, SLPServerTestHelper::INITIAL_BOOT_TIME, scopes);
    server.reset(m_helper.CreateNewServer(true, DA_SCOPES));
  }

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(peer, xid, false, SLP_OK,
        SLPServerTestHelper::INITIAL_BOOT_TIME, ScopeSet(DA_SCOPES));

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, DA_SERVICE,
                                  scopes);
  }

  // send a multicast SrvRqst, expect a DAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(peer, xid, false, SLP_OK,
        SLPServerTestHelper::INITIAL_BOOT_TIME, ScopeSet(DA_SCOPES));

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, DA_SERVICE, scopes);
  }

  // send a unicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(peer, xid, false, SLP_OK,
        SLPServerTestHelper::INITIAL_BOOT_TIME, ScopeSet(DA_SCOPES));

    ScopeSet scopes;
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, DA_SERVICE,
                                  scopes);
  }

  // send a multicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectDAAdvert(peer, xid, false, SLP_OK,
        SLPServerTestHelper::INITIAL_BOOT_TIME, ScopeSet(DA_SCOPES));

    ScopeSet scopes;
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, DA_SERVICE, scopes);
  }

  // send a multicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectError(peer, DA_ADVERTISEMENT, ++xid, SCOPE_NOT_SUPPORTED);

    ScopeSet scopes("three");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, DA_SERVICE,
                                  scopes);
  }

  // send a multicast SrvRqst with scopes that don't match, no response is
  // expected.
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("three");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, DA_SERVICE, scopes);
  }

  // Try a unicast request but with the DA's IP in the PR list
  // This shouldn't happen but check it anyway.
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    pr_list.insert(
        IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, DA_SERVICE,
                                  scopes);
  }

  // Try a multicast request but with the DA's IP in the PR list
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    pr_list.insert(
        IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, DA_SERVICE, scopes);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Test that DAs don't respond to SrvRqsts of the form service:service-agent
 */
void SLPServerDATest::testSrvRqstForServiceAgent() {
  const string sa_service = "service:service-agent";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

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
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, sa_service,
                                  scopes);
  }

  // send a multicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, sa_service, scopes);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Test the we respond correctly to SrvRqsts for locally registered services
 */
void SLPServerDATest::testSrvRqstForLocalService() {
  const string foo_service = "service:foo";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

  // register a service
  ServiceEntry service("one", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
  m_helper.AdvanceTime(0, 0);

  // now perform various SrvRqsts
  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, foo_service,
                                  scopes);
  }

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, foo_service,
                                  scopes);
  }

  // Try a multicast request but with the SA's IP in the PR list
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    pr_list.insert(
        IPV4Address::FromStringOrDie(SLPServerTestHelper::SERVER_IP));
    m_helper.InjectServiceRequest(peer, ++xid, true, pr_list, foo_service,
                                  scopes);
  }

  // test a multicast request for a scope that the DA doesn't support
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("three");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, ++xid, true, pr_list, foo_service,
                                  scopes);
  }

  // test a unicast request for a scope that the DA doesn't support
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(peer, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);

    ScopeSet scopes("three");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, foo_service,
                                  scopes);
  }

  // test a multicast request for a scope that the DA supports, but the service
  // wasn't registered with
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("two");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, ++xid, true, pr_list, foo_service,
                                  scopes);
  }

  // test a unicast request for a scope that the DA supports, but the service
  // wasn't registered with
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    ScopeSet scopes("two");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, foo_service,
                                  scopes);
  }

  // test a unicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectError(peer, SERVICE_REPLY, ++xid, SCOPE_NOT_SUPPORTED);

    ScopeSet scopes("");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, foo_service,
                                  scopes);
  }

  // test a multicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, ++xid, true, pr_list, foo_service,
                                  scopes);
  }

  // de-register the service
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(service));
  m_helper.AdvanceTime(0, 0);

  // send a unicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, foo_service,
                                  scopes);
  }

  // send a multicast SrvRqst, expect no response
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, foo_service,
                                  scopes);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Check that we expire local services when they timeout
 */
void SLPServerDATest::testLocalServiceTimeout() {
  const string foo_service = "service:foo";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

  // register a service
  ServiceEntry service("one", "service:foo://localhost", 10);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  // this should time the service out
  m_helper.AdvanceTime(11, 0);

  // now perform various SrvRqsts
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
    m_helper.InjectServiceRequest(peer, xid, false, pr_list, foo_service,
                                  scopes);
  }

  // send a multicast SrvRqst, expect no response
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, foo_service,
                                  scopes);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * test the SrvReg handling
 */
void SLPServerDATest::testRegistration() {
  const string foo_service = "service:foo";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // register a service, expect an Ack
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SLP_OK);

    ServiceEntry service("one", "service:foo://localhost", 300);
    ScopeSet scopes("one");
    m_helper.InjectServiceRegistration(peer, xid++, true, scopes, service);
  }

  // Try to register the same service with a different set of scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SCOPE_NOT_SUPPORTED);

    ServiceEntry service("one,two", "service:foo://localhost", 300);
    ScopeSet scopes("one,two");
    m_helper.InjectServiceRegistration(peer, xid++, true, scopes, service);
  }

  // same thing, but this time without the fresh flag
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SCOPE_NOT_SUPPORTED);

    ServiceEntry service("one,two", "service:foo://localhost", 300);
    ScopeSet scopes("one,two");
    m_helper.InjectServiceRegistration(peer, xid++, false, scopes, service);
  }

  // try a service that we don't know about, with the fresh flag unset
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, ola::slp::INVALID_UPDATE);

    ServiceEntry service("one", "service:bar://localhost", 300);
    ScopeSet scopes("one");
    m_helper.InjectServiceRegistration(peer, xid++, false, scopes, service);
  }

  // try a lifetime of 0, should return INVALID_REG
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, ola::slp::INVALID_REGISTRATION);

    ServiceEntry service("one", "service:foo://localhost", 0);
    ScopeSet scopes("one");
    m_helper.InjectServiceRegistration(peer, xid++, true, scopes, service);
  }

  // try a request that doesn't match the DA's scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, ola::slp::SCOPE_NOT_SUPPORTED);

    ServiceEntry service("one,three", "service:foo://localhost", 300);
    ScopeSet scopes("one,three");
    m_helper.InjectServiceRegistration(peer, xid++, true, scopes, service);
  }

  // try to re-register, with and without the fresh flag
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SLP_OK);
    ServiceEntry service("one", "service:foo://localhost", 300);
    ScopeSet scopes("one");
    m_helper.InjectServiceRegistration(peer, xid++, true, scopes, service);
  }

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SLP_OK);
    ServiceEntry service("one", "service:foo://localhost", 300);
    ScopeSet scopes("one");
    m_helper.InjectServiceRegistration(peer, xid++, false, scopes, service);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * test the SrvDeReg handling
 */
void SLPServerDATest::testDeRegistration() {
  const string foo_service = "service:foo";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  ServiceEntry service("one", "service:foo://localhost", 300);
  xid_t xid = 10;

  // try to de-reg a service that isn't registered
  // The RFC isn't clear what to return here, none of the error codes really
  // match so we return SLP_OK.
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SLP_OK);

    m_helper.InjectServiceDeRegistration(peer, xid, scopes, service);
  }

  // register a service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SLP_OK);

    m_helper.InjectServiceRegistration(peer, xid++, true, scopes, service);
  }

  // try to de-register the service without any scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SCOPE_NOT_SUPPORTED);

    ScopeSet empty_scopes;
    m_helper.InjectServiceDeRegistration(peer, xid++, empty_scopes, service);
  }

  // try to de-register the service with a subset of the scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SCOPE_NOT_SUPPORTED);

    ScopeSet scope_subset("one");
    m_helper.InjectServiceDeRegistration(peer, xid++, scope_subset, service);
  }

  // confirm we still reply to a SrvRqst
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, "service:foo",
                                  scopes);
  }

  // now actually de-register the service
  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SLP_OK);

    m_helper.InjectServiceDeRegistration(peer, xid++, scopes, service);
  }

  // confirm we no longer reply to a SrvRqst
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid, true, pr_list, "service:foo",
                                  scopes);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}



/**
 * Confirm that we respond to SrvRqst for registered services.
 */
void SLPServerDATest::testSrvRqstForRemoteService() {
  const string foo_service = "service:foo";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  ServiceEntry service("one", "service:foo://localhost", 300);
  xid_t xid = 10;

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SLP_OK);
    m_helper.InjectServiceRegistration(peer, xid, true, scopes, service);
  }

  // send a unicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid++, false, pr_list, foo_service,
                                  scopes);
  }

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);

    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid++, true, pr_list, foo_service,
                                  scopes);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Check that we expire remotely-registered services correctly.
 */
void SLPServerDATest::testRemoteServiceTimeout() {
  const string foo_service = "service:foo";
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup(DA_SCOPES));
  ScopeSet scopes(DA_SCOPES);

  // now perform various SrvRqsts
  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;
  ServiceEntry service("one", "service:foo://localhost", 10);

  {
    SocketVerifier verifier(&m_udp_socket);
    m_helper.ExpectServiceAck(peer, xid, SLP_OK);
    m_helper.InjectServiceRegistration(peer, xid, true, scopes, service);
  }

  // this should time the service out
  m_helper.AdvanceTime(11, 0);

  // send a unicast SrvRqst, expect an empty SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    m_helper.ExpectServiceReply(peer, xid, SLP_OK, urls);
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid++, false, pr_list, foo_service,
                                  scopes);
  }

  // send a multicast SrvRqst, expect no response
  {
    SocketVerifier verifier(&m_udp_socket);
    PRList pr_list;
    m_helper.InjectServiceRequest(peer, xid++, true, pr_list, foo_service,
                                  scopes);
  }

  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}
