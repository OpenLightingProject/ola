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
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/math/Random.h"
#include "ola/stl/STLUtils.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/MockUDPSocket.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/DATracker.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPServer.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/ServiceEntry.h"
#include "tools/slp/URLEntry.h"

using ola::io::SelectServer;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::slp::DirectoryAgent;
using ola::slp::INVALID_REGISTRATION;
using ola::slp::PARSE_ERROR;
using ola::slp::SCOPE_NOT_SUPPORTED;
using ola::slp::SLPPacketBuilder;
using ola::slp::SLPServer;
using ola::slp::SLP_OK;
using ola::slp::ScopeSet;
using ola::slp::ServiceEntry;
using ola::slp::URLEntries;
using ola::slp::URLEntry;
using ola::slp::xid_t;
using ola::testing::MockUDPSocket;
using ola::testing::SocketVerifier;
using std::auto_ptr;
using std::ostringstream;
using std::set;
using std::string;
using std::vector;


class SLPServerSATest: public CppUnit::TestFixture {
  public:
    SLPServerSATest()
        : m_ss(NULL, &m_clock) {
    }

  public:
    CPPUNIT_TEST_SUITE(SLPServerSATest);
    CPPUNIT_TEST(testSrvRqst);
    CPPUNIT_TEST(testInvalidRegistrations);
    CPPUNIT_TEST(testDeRegistration);
    CPPUNIT_TEST(testSrvRqstForServiceAgent);
    CPPUNIT_TEST(testExpiredService);
    CPPUNIT_TEST(testMissingServiceType);
    CPPUNIT_TEST(testMisconfiguredSA);
    CPPUNIT_TEST(testActiveDADiscovery);
    CPPUNIT_TEST(testPassiveDADiscovery);
    CPPUNIT_TEST(testPreDiscoveryRegistration);
    //CPPUNIT_TEST(testPostDiscoveryRegistration);
    CPPUNIT_TEST_SUITE_END();

    void testSrvRqst();
    void testInvalidRegistrations();
    void testDeRegistration();
    void testSrvRqstForServiceAgent();
    void testExpiredService();
    void testMissingServiceType();
    void testMisconfiguredSA();
    void testActiveDADiscovery();
    void testPassiveDADiscovery();
    void testPreDiscoveryRegistration();
    void testPostDiscoveryRegistration();

  public:
    void setUp() {
      ola::math::InitRandom();
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      m_udp_socket.Init();
      m_udp_socket.SetInterface(IPV4Address::FromStringOrDie(SERVER_IP));
      m_udp_socket.Bind(IPV4SocketAddress(IPV4Address::WildCard(),
                        SLP_TEST_PORT));
      // make sure WakeUpTime is populated
      m_ss.RunOnce(0, 0);
    }

    // Advance the time, which may trigger timeouts to run
    void AdvanceTime(int32_t sec, int32_t usec) {
      m_clock.AdvanceTime(sec, usec);
      // run any timeouts, and update the WakeUpTime
      m_ss.RunOnce(0, 0);
    }

  private:
    typedef set<IPV4Address> PRList;
    typedef set<IPV4Address> DAList;

    ola::MockClock m_clock;
    ola::io::SelectServer m_ss;
    MockUDPSocket m_udp_socket;

    SLPServer *CreateNewServer(bool enable_da, const string &scopes);
    void InjectServiceRequest(const IPV4SocketAddress &source,
                              xid_t xid,
                              bool multicast,
                              const set<IPV4Address> &pr_list,
                              const string &service_type,
                              const ScopeSet &scopes);
    void InjectSrvAck(const IPV4SocketAddress &source,
                      xid_t xid,
                      uint16_t error_code);
    void InjectDAAdvert(const IPV4SocketAddress &source,
                        xid_t xid,
                        bool multicast,
                        uint16_t error_code,
                        uint32_t boot_timestamp,
                        const ScopeSet &scopes);

    void ExpectServiceReply(const IPV4SocketAddress &dest,
                            xid_t xid,
                            uint16_t error_code,
                            const URLEntries &urls);
    void ExpectDAServiceRequest(xid_t xid,
                                const set<IPV4Address> &pr_list,
                                const ScopeSet &scopes);
    void ExpectServiceRegistration(const IPV4SocketAddress &dest,
                                   xid_t xid,
                                   bool fresh,
                                   const ScopeSet &scopes,
                                   const ServiceEntry &service);
    void ExpectSAAdvert(const IPV4SocketAddress &dest,
                        xid_t xid,
                        bool multicast,
                        const string &url,
                        const ScopeSet &scopes);

    void VerifyKnownDAs(unsigned int line,
                        SLPServer *server,
                        const set<IPV4Address> &expected_das);

    static const uint16_t SLP_TEST_PORT = 5570;
    static const char SERVER_IP[];
    static const char SLP_MULTICAST_IP[];
};


const char SLPServerSATest::SERVER_IP[] = "10.0.0.1";
const char SLPServerSATest::SLP_MULTICAST_IP[] = "239.255.255.253";

CPPUNIT_TEST_SUITE_REGISTRATION(SLPServerSATest);


SLPServer *SLPServerSATest::CreateNewServer(bool enable_da,
                                            const string &scopes) {
  ScopeSet scope_set(scopes);

  SLPServer::SLPServerOptions options;
  options.enable_da = enable_da;
  options.clock = &m_clock;
  options.ip_address = IPV4Address::FromStringOrDie(SERVER_IP);
  options.initial_xid = 0;  // don't randomize the xid for testing
  options.scopes = set<string>();
  // clamp the CONFIG_REG_ACTIVE times otherwise they can overlap with SrvRqsts
  // which makes the packet ordering non-deterministic.
  // This also ensures that we respect the values passed in.
  options.config_reg_active_min = 0;
  options.config_reg_active_max = 1;
  copy(scope_set.begin(), scope_set.end(),
       std::inserter(options.scopes, options.scopes.end()));
  options.slp_port = SLP_TEST_PORT;
  SLPServer *server = new SLPServer(&m_ss, &m_udp_socket, NULL, NULL, options);
  // TODO(simon): test without the Init here
  server->Init();
  return server;
}


/**
 * Inject a SrvRqst into the UDP socket.
 */
void SLPServerSATest::InjectServiceRequest(const IPV4SocketAddress &source,
                                           xid_t xid,
                                           bool multicast,
                                           const set<IPV4Address> &pr_list,
                                           const string &service_type,
                                           const ScopeSet &scopes) {
  ola::io::IOQueue output;
  ola::io::BigEndianOutputStream output_stream(&output);
  SLPPacketBuilder::BuildServiceRequest(
      &output_stream, xid, multicast, pr_list, service_type, scopes);
  m_udp_socket.InjectData(&output, source);
}


/**
 * Inject a SrvAck
 */
void SLPServerSATest::InjectSrvAck(const IPV4SocketAddress &source,
                                   xid_t xid,
                                   uint16_t error_code) {
  ola::io::IOQueue output;
  ola::io::BigEndianOutputStream output_stream(&output);
  SLPPacketBuilder::BuildServiceAck(
      &output_stream, xid, error_code);
  m_udp_socket.InjectData(&output, source);
}


/**
 * Inject a DAAdvert
 */
void SLPServerSATest::InjectDAAdvert(const IPV4SocketAddress &source,
                                     xid_t xid,
                                     bool multicast,
                                     uint16_t error_code,
                                     uint32_t boot_timestamp,
                                     const ScopeSet &scopes) {
  ostringstream str;
  str << "service:directory-agent://" << source.Host();
  ola::io::IOQueue output;
  ola::io::BigEndianOutputStream output_stream(&output);
  SLPPacketBuilder::BuildDAAdvert(
      &output_stream, xid, multicast, error_code, boot_timestamp, str.str(),
      scopes);
  m_udp_socket.InjectData(&output, source);
}


/**
 * Expect a SrvRepl
 */
void SLPServerSATest::ExpectServiceReply(const IPV4SocketAddress &dest,
                                         xid_t xid,
                                         uint16_t error_code,
                                         const URLEntries &urls) {
  ola::io::IOQueue output;
  ola::io::BigEndianOutputStream output_stream(&output);
  SLPPacketBuilder::BuildServiceReply(&output_stream, xid, error_code, urls);
  m_udp_socket.AddExpectedData(&output, dest);
  OLA_ASSERT_TRUE(output.Empty());
}


/**
 * Expect a SrvRqst for service:directory-agent
 */
void SLPServerSATest::ExpectDAServiceRequest(xid_t xid,
                                             const set<IPV4Address> &pr_list,
                                             const ScopeSet &scopes) {
  IPV4SocketAddress destination(IPV4Address::FromStringOrDie(SLP_MULTICAST_IP),
                                SLP_TEST_PORT);
  ola::io::IOQueue output;
  ola::io::BigEndianOutputStream output_stream(&output);
  SLPPacketBuilder::BuildServiceRequest(&output_stream, xid, true, pr_list,
                                        "service:directory-agent", scopes);
  m_udp_socket.AddExpectedData(&output, destination);
  OLA_ASSERT_TRUE(output.Empty());
}


/**
 * Expect a SrvReg.
 */
void SLPServerSATest::ExpectServiceRegistration(const IPV4SocketAddress &dest,
                                                xid_t xid,
                                                bool fresh,
                                                const ScopeSet &scopes,
                                                const ServiceEntry &service) {
  ola::io::IOQueue output;
  ola::io::BigEndianOutputStream output_stream(&output);
  SLPPacketBuilder::BuildServiceRegistration(&output_stream, xid, fresh,
                                             scopes, service);
  m_udp_socket.AddExpectedData(&output, dest);
  OLA_ASSERT_TRUE(output.Empty());
}


/**
 * Expect a SAAdvert
 */
void SLPServerSATest::ExpectSAAdvert(const IPV4SocketAddress &dest,
                                     xid_t xid,
                                     bool multicast,
                                     const string &url,
                                     const ScopeSet &scopes) {
  ola::io::IOQueue output;
  ola::io::BigEndianOutputStream output_stream(&output);
  SLPPacketBuilder::BuildSAAdvert(&output_stream, xid, multicast, url, scopes);
  m_udp_socket.AddExpectedData(&output, dest);
  OLA_ASSERT_TRUE(output.Empty());
}


/**
 * Verify the DAs that the server knows about matches the expected set
 */
void SLPServerSATest::VerifyKnownDAs(unsigned int line,
                                     SLPServer *server,
                                     const set<IPV4Address> &expected_das) {
  ostringstream str;
  str << "Line " << line;

  vector<DirectoryAgent> known_das;
  server->GetDirectoryAgents(&known_das);

  vector<DirectoryAgent>::const_iterator iter = known_das.begin();
  for (; iter != known_das.end(); ++iter) {
    ostringstream expected_url;
    expected_url << "service:directory-agent://" << iter->IPAddress();
    OLA_ASSERT_EQ_MSG(expected_url.str(), iter->URL(), str.str());

    OLA_ASSERT_TRUE_MSG(ola::STLContains(expected_das, iter->IPAddress()),
                        str.str());
  }
}


/**
 * Test the SA when no DAs are present.
 */
void SLPServerSATest::testSrvRqst() {
  auto_ptr<SLPServer> server(CreateNewServer(false, "one"));

  // register a service with the instance
  ServiceEntry service("one,two", "service:foo://localhost", 300);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
  AdvanceTime(0, 0);

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    ExpectServiceReply(peer, xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(peer, xid, true, pr_list, "service:foo", scopes);
  }

  // send a unicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(service.url());
    ExpectServiceReply(peer, ++xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "service:foo", scopes);
  }

  // Try a multicast request but with the SA's IP in the PR list
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    pr_list.insert(IPV4Address::FromStringOrDie(SERVER_IP));
    InjectServiceRequest(peer, ++xid, true, pr_list, "service:foo", scopes);
  }

  // test a multicast request for a scope that doesn't match the SAs scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("two");
    PRList pr_list;
    InjectServiceRequest(peer, ++xid, true, pr_list, "service:foo", scopes);
  }

  // test a unicast request for a scope that doesn't match the SAs scopes
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    ExpectServiceReply(peer, ++xid, SCOPE_NOT_SUPPORTED, urls);

    ScopeSet scopes("two");
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "service:foo", scopes);
  }

  // test a multicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("");
    PRList pr_list;
    InjectServiceRequest(peer, ++xid, true, pr_list, "service:foo", scopes);
  }

  // test a unicast request with no scope list
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    ExpectServiceReply(peer, ++xid, SCOPE_NOT_SUPPORTED, urls);

    ScopeSet scopes("");
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "service:foo", scopes);
  }

  // de-register, then we should receive no response to a multicast request
  {
    SocketVerifier verifier(&m_udp_socket);
    OLA_ASSERT_EQ((uint16_t) SLP_OK, server->DeRegisterService(service));
    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(peer, ++xid, true, pr_list, "service:foo", scopes);
  }

  // a unicast request should return a SrvRply with no URL entries
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    ExpectServiceReply(peer, ++xid, SLP_OK, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "service:foo", scopes);
  }
}


/*
 * Test that registering with mis-matched scopes fails.
 */
void SLPServerSATest::testInvalidRegistrations() {
  auto_ptr<SLPServer> server(CreateNewServer(false, "one"));

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
  auto_ptr<SLPServer> server(CreateNewServer(false, "one"));

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
  auto_ptr<SLPServer> server(CreateNewServer(false, "one,two"));

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    ExpectSAAdvert(peer, xid, false, "service:service-agent://10.0.0.1",
                   ScopeSet("one,two"));

    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "service:service-agent",
                         scopes);
  }

  // send a multicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    ExpectSAAdvert(peer, xid, false, "service:service-agent://10.0.0.1",
                   ScopeSet("one,two"));

    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(peer, xid, true, pr_list, "service:service-agent",
                         scopes);
  }

  // send a unicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    ExpectSAAdvert(peer, xid, false, "service:service-agent://10.0.0.1",
                   ScopeSet("one,two"));

    ScopeSet scopes;
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "service:service-agent",
                         scopes);
  }

  // send a multicast SrvRqst with no scopes, this should generate a response
  {
    SocketVerifier verifier(&m_udp_socket);
    ExpectSAAdvert(peer, xid, false, "service:service-agent://10.0.0.1",
                   ScopeSet("one,two"));

    ScopeSet scopes;
    PRList pr_list;
    InjectServiceRequest(peer, xid, true, pr_list, "service:service-agent",
                         scopes);
  }

  // send a unicast SrvRqst with scopes that don't match, expect an error
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    ExpectServiceReply(peer, ++xid, SCOPE_NOT_SUPPORTED, urls);
    ScopeSet scopes("three");
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "service:service-agent",
                         scopes);
  }

  // send a multicast SrvRqst with scopes that don't match, no response is
  // expected.
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("three");
    PRList pr_list;
    InjectServiceRequest(peer, xid, true, pr_list, "service:service-agent",
                         scopes);
  }
}


/**
 * Test that we don't return services that have expired
 */
void SLPServerSATest::testExpiredService() {
  auto_ptr<SLPServer> server(CreateNewServer(false, "one"));

  // TODO(simon): move this into a function that takes the IPs of the DAs to
  // inject.
  {
    ScopeSet scopes("one");
    PRList pr_list;
    DAList da_list;
    // The first request is somewhere between 0 and 3s (CONFIG_START_WAIT)
    // after we start
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(3, 0);

    // Then another one 2s later.
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(2, 0);
    m_udp_socket.Verify();
  }

  // register a service with the instance
  ServiceEntry service("one,two", "service:foo://localhost", 30);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
  AdvanceTime(0, 0);


  // expire the service
  AdvanceTime(31, 0);

  // send a multicast SrvRqst, expect a SrvRply
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(IPV4SocketAddress::FromStringOrDie("192.168.1.1:5570"),
                         10, true, pr_list, "service:foo", scopes);
  }
}


/**
 * Test for a missing service type
 */
void SLPServerSATest::testMissingServiceType() {
  auto_ptr<SLPServer> server(CreateNewServer(false, "one"));

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    URLEntries urls;
    ExpectServiceReply(peer, ++xid, PARSE_ERROR, urls);

    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "", scopes);
  }

  // send a multicast SrvRqst, this is silently dropped
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    InjectServiceRequest(peer, xid, true, pr_list, "", scopes);
  }
}


/**
 * Test that we can't configure an SA with no scopes.
 */
void SLPServerSATest::testMisconfiguredSA() {
  // this should switch to 'default'
  auto_ptr<SLPServer> server(CreateNewServer(false, ""));

  IPV4SocketAddress peer = IPV4SocketAddress::FromStringOrDie(
      "192.168.1.1:5570");
  xid_t xid = 10;

  // send a unicast SrvRqst, expect a SAAdvert
  {
    SocketVerifier verifier(&m_udp_socket);
    ExpectSAAdvert(peer, xid, false, "service:service-agent://10.0.0.1",
                   ScopeSet("default"));

    ScopeSet scopes("");
    PRList pr_list;
    InjectServiceRequest(peer, xid, false, pr_list, "service:service-agent",
                         scopes);
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
    auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    ExpectDAServiceRequest(0, pr_list, scopes);

    // The first request is somewhere between 0 and 3s (CONFIG_START_WAIT)
    // after we start
    AdvanceTime(3, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Then another one 2s later.
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(2, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    AdvanceTime(4, 0);
    m_udp_socket.Verify();
  }

  // A single DA that responds to the first SrvRqst
  {
    auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    ExpectDAServiceRequest(0, pr_list, scopes);

    AdvanceTime(3, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    InjectDAAdvert(da1, 0, false, SLP_OK, 1, scopes);
    da_list.insert(da1.Host());
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later, which includes the first DA in the
    // PRList. The XID changes since the request is different.
    pr_list.insert(da1.Host());
    ExpectDAServiceRequest(1, pr_list, scopes);
    AdvanceTime(2, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    AdvanceTime(4, 0);
    m_udp_socket.Verify();
  }

  // A single DA that responds to the second SrvRqst.
  // This simulates a dropped UDP multicast packet
  {
    auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    ExpectDAServiceRequest(0, pr_list, scopes);

    AdvanceTime(3, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Send another SrvRqst 2s later.
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(2, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Inject the DAAdvert
    InjectDAAdvert(da1, 0, false, SLP_OK, 1, scopes);
    da_list.insert(da1.Host());
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Since we got a response, we should send another SrvRqst
    pr_list.insert(da1.Host());
    ExpectDAServiceRequest(1, pr_list, scopes);
    AdvanceTime(4, 0);
    m_udp_socket.Verify();

    // No more after that
    AdvanceTime(8, 0);
    m_udp_socket.Verify();
  }

  // Two DAs that both respond to the first SrvRqst
  {
    auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    ExpectDAServiceRequest(0, pr_list, scopes);

    AdvanceTime(3, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    InjectDAAdvert(da1, 0, false, SLP_OK, 1, scopes);
    da_list.insert(da1.Host());
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    InjectDAAdvert(da2, 0, false, SLP_OK, 1, scopes);
    da_list.insert(da2.Host());
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later, which includes both DAs in the
    // PRList. The XID changes since the request is different.
    pr_list.insert(da1.Host());
    pr_list.insert(da2.Host());
    ExpectDAServiceRequest(1, pr_list, scopes);
    AdvanceTime(2, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    AdvanceTime(4, 0);
    m_udp_socket.Verify();
  }

  // A single DA that responds with an error. This isn't supposed to happen,
  // but let's make sure we handle it cleanly.
  {
    auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    ExpectDAServiceRequest(0, pr_list, scopes);

    AdvanceTime(3, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    InjectDAAdvert(da1, 0, false, SCOPE_NOT_SUPPORTED, 1, scopes);
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Now we send another SrvRqst 2s later. The bad DA should not be in the
    // list.
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(2, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    AdvanceTime(4, 0);
    m_udp_socket.Verify();
  }

  // test a shutdown while DA discovery is running
  {
    auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    ExpectDAServiceRequest(0, pr_list, scopes);

    AdvanceTime(3, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);
  }

  // test triggering  DA discovery while the process is already running
  {
    auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
    PRList pr_list;
    DAList da_list;
    ExpectDAServiceRequest(0, pr_list, scopes);

    AdvanceTime(3, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    server->TriggerActiveDADiscovery();

    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(2, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    AdvanceTime(4, 0);
    m_udp_socket.Verify();
  }
}


/**
 * Test Passive DA Discovery behaviour
 */
void SLPServerSATest::testPassiveDADiscovery() {
  auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
  ScopeSet scopes("one");

  // No DAs present
  // TODO: move this into a helper
  {
    PRList pr_list;
    DAList da_list;
    ExpectDAServiceRequest(0, pr_list, scopes);

    // The first request is somewhere between 0 and 3s (CONFIG_START_WAIT)
    // after we start
    AdvanceTime(3, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // Then another one 2s later.
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(2, 0);
    m_udp_socket.Verify();
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // No more after that
    AdvanceTime(4, 0);
    m_udp_socket.Verify();
  }

  // now inject an unsolicited DAAdvert
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  InjectDAAdvert(da1, 0, true, SLP_OK, 1, scopes);
  DAList da_list;
  da_list.insert(da1.Host());
  VerifyKnownDAs(__LINE__, server.get(), da_list);

  // now another DA appears..
  IPV4SocketAddress da2 = IPV4SocketAddress::FromStringOrDie("10.0.1.2:5570");
  scopes = ScopeSet("two");
  InjectDAAdvert(da2, 0, true, SLP_OK, 1, scopes);
  da_list.insert(da2.Host());
  VerifyKnownDAs(__LINE__, server.get(), da_list);
}


/**
 * Test that we register with a DA correctly. This checks the case when a
 * service is registered after we have discovered the DAs.
 */
void SLPServerSATest::testPreDiscoveryRegistration() {
  auto_ptr<SLPServer> server(CreateNewServer(false, "one"));
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
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(3, 0);
    m_udp_socket.Verify();

    // inject the DA, this causes a SrvReg to be sent
    InjectDAAdvert(da1, 0, false, SLP_OK, 1, scopes);
    DAList da_list;
    da_list.insert(da1.Host());
    VerifyKnownDAs(__LINE__, server.get(), da_list);

    // We'll register 0-1s after receiving the DAAdvert
    ExpectServiceRegistration(da1, 1, true, ScopeSet("one"), service);
    AdvanceTime(1, 0);
    m_udp_socket.Verify();

    pr_list.insert(da1.Host());
    ExpectDAServiceRequest(2, pr_list, scopes);
    AdvanceTime(1, 0);

    // Ack the SrvReg message
    InjectSrvAck(da1, 1, SLP_OK);

    // nothing further
    AdvanceTime(4, 0);
  }
}


/**
 * Test that we register with a DA correctly. This checks the case when a
 * service is registered before we have discovered the DAs
 */
void SLPServerSATest::testPostDiscoveryRegistration() {
  return;
  {
    SocketVerifier verifier(&m_udp_socket);
    ScopeSet scopes("one");
    PRList pr_list;
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(3, 0);

    m_udp_socket.Verify();
    ExpectDAServiceRequest(0, pr_list, scopes);
    AdvanceTime(2, 0);

    AdvanceTime(4, 0);
  }
}


/**
 * Test that we register with a DA when it starts after us (i.e. we receive an
 * unsolicited DAAdvert.
 */
/*

    // test a DAAdvert that does not match any of our scopes
 }
*/

/**
 * Test that we re-register with a DA when it reboots.
 */
