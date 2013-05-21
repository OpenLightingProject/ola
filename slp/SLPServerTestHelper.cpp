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
 * SLPServerTestHelper.cpp
 * Tests the SA functionality of the SLPServer class
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/MockUDPSocket.h"
#include "ola/testing/TestUtils.h"
#include "ola/slp/URLEntry.h"
#include "slp/DATracker.h"
#include "slp/SLPPacketBuilder.h"
#include "slp/SLPServer.h"
#include "slp/SLPServerTestHelper.h"
#include "slp/ScopeSet.h"
#include "slp/ServiceEntry.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::slp::DirectoryAgent;
using ola::slp::EN_LANGUAGE_TAG;
using ola::slp::SLPPacketBuilder;
using ola::slp::SLPServer;
using ola::slp::SLP_OK;
using ola::slp::ScopeSet;
using ola::slp::ServiceEntry;
using ola::slp::URLEntries;
using ola::slp::xid_t;
using ola::testing::MockUDPSocket;
using std::ostringstream;
using std::set;
using std::string;
using std::vector;


const char SLPServerTestHelper::SERVER_IP[] = "10.0.0.1";
const char SLPServerTestHelper::SLP_MULTICAST_IP[] = "239.255.255.253";


/**
 * Create a new SLPServer
 */
SLPServer *SLPServerTestHelper::CreateNewServer(bool enable_da,
                                                const ScopeSet &scopes) {
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
  // set the boot timestamp to something we know
  options.boot_time = INITIAL_BOOT_TIME;
  copy(scopes.begin(), scopes.end(),
       std::inserter(options.scopes, options.scopes.end()));
  options.slp_port = SLP_TEST_PORT;
  m_server_start_time = *(m_ss.WakeUpTime());
  SLPServer *server = new SLPServer(&m_ss, m_udp_socket, NULL, &m_export_map,
                                    options);
  // TODO(simon): test without the Init here
  server->Init();
  return server;
}


/**
 * Create a new SLPServer (with DA enabled) and handle the initial DAAdvert &
 * SrvRqsts it sends.
 */
SLPServer *SLPServerTestHelper::CreateDAAndHandleStartup(
    const ScopeSet &scopes) {
  ExpectMulticastDAAdvert(0, INITIAL_BOOT_TIME, scopes);
  SLPServer *server = CreateNewServer(true, scopes);
  HandleInitialActiveDADiscovery(scopes);
  m_udp_socket->Verify();
  return server;
}


/**
 * Handle the Initial active DA discovery.
 * This assumes the timing options are using the default values and causes 9
 * seconds to pass
 */
void SLPServerTestHelper::HandleInitialActiveDADiscovery(
    const ScopeSet &scopes) {
  set<IPV4Address> pr_list;
  // The first request is somewhere between 0 and 3s (CONFIG_START_WAIT)
  // after we start
  ExpectDAServiceRequest(0, pr_list, scopes);
  AdvanceTime(3);
  m_udp_socket->Verify();

  // Then another one 2s later.
  ExpectDAServiceRequest(0, pr_list, scopes);
  AdvanceTime(2);
  m_udp_socket->Verify();

  // And let that one time out
  AdvanceTime(4);
  m_udp_socket->Verify();
}

/**
 * Perform the Active DA discovery dance.
 * This assumes the timing options are using the default values.
 * Each call takes 7 seconds on the clock.
 */
void SLPServerTestHelper::HandleActiveDADiscovery(const ScopeSet &scopes,
                                                  xid_t xid) {
  set<IPV4Address> pr_list;
  // The first request is somewhere between 0 and 3s (CONFIG_START_WAIT)
  // after we start
  ExpectDAServiceRequest(xid, pr_list, scopes);
  AdvanceTime(1);
  m_udp_socket->Verify();

  // Then another one 2s later.
  ExpectDAServiceRequest(xid, pr_list, scopes);
  AdvanceTime(2);
  m_udp_socket->Verify();

  // And let that one time out
  AdvanceTime(4);
  m_udp_socket->Verify();
}


/**
 * A Helper method to go through the steps of registering a service with a DA
 */
void SLPServerTestHelper::RegisterWithDA(SLPServer *server,
                                         const IPV4SocketAddress &da_addr,
                                         const ServiceEntry &service,
                                         xid_t xid) {
  ExpectServiceRegistration(da_addr, xid, true, service.scopes(), service);
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));
  InjectSrvAck(da_addr, xid, SLP_OK);
  m_udp_socket->Verify();
}


/**
 * Inject a SrvRqst into the UDP socket.
 */
void SLPServerTestHelper::InjectServiceRequest(const IPV4SocketAddress &source,
                                               xid_t xid,
                                               bool multicast,
                                               const set<IPV4Address> &pr_list,
                                               const string &service_type,
                                               const ScopeSet &scopes) {
  SLPPacketBuilder::BuildServiceRequest(
      &m_output_stream, xid, multicast, EN_LANGUAGE_TAG, pr_list, service_type,
      scopes);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Inject a SrvRepl.
 */
void SLPServerTestHelper::InjectServiceReply(const IPV4SocketAddress &source,
                                             xid_t xid,
                                             uint16_t error_code,
                                             const URLEntries &urls) {
  SLPPacketBuilder::BuildServiceReply(&m_output_stream, xid, EN_LANGUAGE_TAG,
                                      error_code, urls);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Inject a SrvAck
 */
void SLPServerTestHelper::InjectSrvAck(const IPV4SocketAddress &source,
                                       xid_t xid,
                                       uint16_t error_code) {
  SLPPacketBuilder::BuildServiceAck(
      &m_output_stream, xid, EN_LANGUAGE_TAG, error_code);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Inject a DAAdvert
 */
void SLPServerTestHelper::InjectDAAdvert(const IPV4SocketAddress &source,
                                         xid_t xid,
                                         bool multicast,
                                         uint16_t error_code,
                                         uint32_t boot_timestamp,
                                         const ScopeSet &scopes) {
  ostringstream str;
  str << "service:directory-agent://" << source.Host();
  InjectCustomDAAdvert(source, str.str(), xid, multicast, error_code,
      boot_timestamp, scopes);
}


/**
 * Inject a custom DAAdvert
 */
void SLPServerTestHelper::InjectCustomDAAdvert(const IPV4SocketAddress &source,
                                               const string &url,
                                               xid_t xid,
                                               bool multicast,
                                               uint16_t error_code,
                                               uint32_t boot_timestamp,
                                               const ScopeSet &scopes) {
  SLPPacketBuilder::BuildDAAdvert(
      &m_output_stream, xid, multicast, error_code, boot_timestamp, url,
      scopes);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Inject a SrvReg message
 */
void SLPServerTestHelper::InjectServiceRegistration(
    const IPV4SocketAddress &source,
    xid_t xid,
    bool fresh,
    const ScopeSet &scopes,
    const ServiceEntry &service) {
  SLPPacketBuilder::BuildServiceRegistration(
      &m_output_stream, xid, fresh, scopes, service);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Inject a SrvDeReg message
 */
void SLPServerTestHelper::InjectServiceDeRegistration(
    const IPV4SocketAddress &source,
    xid_t xid,
    const ScopeSet &scopes,
    const ServiceEntry &service) {
  SLPPacketBuilder::BuildServiceDeRegistration(
      &m_output_stream, xid, scopes, service);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Inject a SrvTypeRqst for all service types
 */
void SLPServerTestHelper::InjectAllServiceTypeRequest(
    const IPV4SocketAddress &source,
    xid_t xid,
    const set<IPV4Address> &pr_list,
    const ScopeSet &scopes) {
  SLPPacketBuilder::BuildAllServiceTypeRequest(
      &m_output_stream, xid, true, pr_list, scopes);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Inject a SrvTypeRqst for a particular naming auth
 */
void SLPServerTestHelper::InjectServiceTypeRequest(
    const IPV4SocketAddress &source,
    xid_t xid,
    const set<IPV4Address> &pr_list,
    const string &naming_auth,
    const ScopeSet &scopes) {
  SLPPacketBuilder::BuildServiceTypeRequest(
      &m_output_stream, xid, true, pr_list, naming_auth, scopes);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Inject an error (truncated SrvRepl or DAAdvert).
 */
void SLPServerTestHelper::InjectError(const IPV4SocketAddress &source,
                                      slp_function_id_t function_id,
                                      xid_t xid,
                                      uint16_t error_code) {
  SLPPacketBuilder::BuildError(
      &m_output_stream, function_id, xid, EN_LANGUAGE_TAG, error_code);
  m_udp_socket->InjectData(&m_output, source);
  OLA_ASSERT_TRUE(m_output.Empty());
}



/**
 * Expect a unicast SrvRqst
 */
void SLPServerTestHelper::ExpectServiceRequest(
    const IPV4SocketAddress &dest,
    xid_t xid,
    const string &service,
    const ScopeSet &scopes,
    const set<IPV4Address> &pr_list) {
  SLPPacketBuilder::BuildServiceRequest(&m_output_stream, xid, false,
                                        EN_LANGUAGE_TAG, pr_list,
                                        service, scopes);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Expect a multicast SrvRqst
 */
void SLPServerTestHelper::ExpectMulticastServiceRequest(
    xid_t xid,
    const string &service,
    const ScopeSet &scopes,
    const set<IPV4Address> &pr_list) {
  IPV4SocketAddress destination(IPV4Address::FromStringOrDie(SLP_MULTICAST_IP),
                                SLP_TEST_PORT);
  SLPPacketBuilder::BuildServiceRequest(&m_output_stream, xid, true,
                                        EN_LANGUAGE_TAG, pr_list,
                                        service, scopes);
  m_udp_socket->AddExpectedData(&m_output, destination);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Expect a SrvRply
 */
void SLPServerTestHelper::ExpectServiceReply(const IPV4SocketAddress &dest,
                                             xid_t xid,
                                             uint16_t error_code,
                                             const URLEntries &urls) {
  SLPPacketBuilder::BuildServiceReply(&m_output_stream, xid, EN_LANGUAGE_TAG,
                                      error_code, urls);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Expect a SrvRqst for service:directory-agent
 */
void SLPServerTestHelper::ExpectDAServiceRequest(
    xid_t xid,
    const set<IPV4Address> &pr_list,
    const ScopeSet &scopes) {
  ExpectMulticastServiceRequest(xid, "service:directory-agent", scopes,
                                pr_list);
}


/**
 * Expect a SrvReg.
 */
void SLPServerTestHelper::ExpectServiceRegistration(
    const IPV4SocketAddress &dest,
    xid_t xid,
    bool fresh,
    const ScopeSet &scopes,
    const ServiceEntry &service) {
  SLPPacketBuilder::BuildServiceRegistration(&m_output_stream, xid, fresh,
                                             scopes, service);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Expect a SrvDeReg.
 */
void SLPServerTestHelper::ExpectServiceDeRegistration(
    const IPV4SocketAddress &dest,
    xid_t xid,
    const ScopeSet &scopes,
    const ServiceEntry &service) {
  SLPPacketBuilder::BuildServiceDeRegistration(&m_output_stream, xid, scopes,
                                               service);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Expect a DAAdvert
 */
void SLPServerTestHelper::ExpectDAAdvert(const IPV4SocketAddress &dest,
                                         xid_t xid,
                                         bool multicast,
                                         uint16_t error_code,
                                         uint32_t boot_timestamp,
                                         const ScopeSet &scopes) {
  ostringstream str;
  str << "service:directory-agent://"
      << IPV4Address::FromStringOrDie(SERVER_IP);
  SLPPacketBuilder::BuildDAAdvert(&m_output_stream, xid, multicast, error_code,
                                  boot_timestamp, str.str(), scopes);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Expect a multicast DAAdvert.
 */
void SLPServerTestHelper::ExpectMulticastDAAdvert(xid_t xid,
                                                  uint32_t boot_timestamp,
                                                  const ScopeSet &scopes) {
  IPV4SocketAddress dest(IPV4Address::FromStringOrDie(SLP_MULTICAST_IP),
                         SLP_TEST_PORT);
  ExpectDAAdvert(dest, xid, true, SLP_OK, boot_timestamp, scopes);
}



/**
 * Expect a Service Type Reply
 */
void SLPServerTestHelper::ExpectServiceTypeReply(
    const IPV4SocketAddress &dest,
    xid_t xid,
    uint16_t error_code,
    const vector<string> &service_types) {

  SLPPacketBuilder::BuildServiceTypeReply(&m_output_stream, xid, error_code,
                                          service_types);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Expect a SAAdvert
 */
void SLPServerTestHelper::ExpectSAAdvert(const IPV4SocketAddress &dest,
                                         xid_t xid,
                                         const ScopeSet &scopes) {
  ostringstream str;
  str << "service:service-agent://"
      << IPV4Address::FromStringOrDie(SERVER_IP);
  SLPPacketBuilder::BuildSAAdvert(&m_output_stream, xid, false, str.str(),
                                  scopes);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Expect an SrvAck
 */
void SLPServerTestHelper::ExpectServiceAck(const IPV4SocketAddress &dest,
                                           xid_t xid,
                                           uint16_t error_code) {
  SLPPacketBuilder::BuildServiceAck(&m_output_stream, xid, EN_LANGUAGE_TAG,
                                    error_code);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/*
 * Expect an error message
 */
void SLPServerTestHelper::ExpectError(const IPV4SocketAddress &dest,
                                      slp_function_id_t function_id,
                                      xid_t xid,
                                      uint16_t error_code) {
  SLPPacketBuilder::BuildError(&m_output_stream, function_id, xid,
                               EN_LANGUAGE_TAG, error_code);
  m_udp_socket->AddExpectedData(&m_output, dest);
  OLA_ASSERT_TRUE(m_output.Empty());
}


/**
 * Verify the DAs that the server knows about matches the expected set
 */
void SLPServerTestHelper::VerifyKnownDAs(unsigned int line,
                                         SLPServer *server,
                                         const set<IPV4Address> &expected_das) {
  ostringstream str;
  str << "Line " << line;

  vector<DirectoryAgent> known_das;
  server->GetDirectoryAgents(&known_das);
  OLA_ASSERT_EQ(expected_das.size(), known_das.size());

  vector<DirectoryAgent>::const_iterator iter = known_das.begin();
  for (; iter != known_das.end(); ++iter) {
    ostringstream expected_url;
    expected_url << "service:directory-agent://" << iter->IPAddress();
    OLA_ASSERT_EQ_MSG(expected_url.str(), iter->URL(), str.str());

    OLA_ASSERT_TRUE_MSG(ola::STLContains(expected_das, iter->IPAddress()),
                        str.str());
  }
}
