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
 * SLPServerUATest.cpp
 * Tests the UA functionality of the SLPServer class
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
using ola::slp::SCOPE_NOT_SUPPORTED;
using ola::slp::SERVICE_REPLY;
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
using std::set;
using std::string;


class SLPServerUATest: public CppUnit::TestFixture {
  public:
    SLPServerUATest()
        : m_helper(&m_udp_socket),
          url1("service:foo://192.168.0.1", 300),
          url2("service:foo://192.168.0.8", 255) {
    }

  public:
    CPPUNIT_TEST_SUITE(SLPServerUATest);
    CPPUNIT_TEST(testFindServiceNoDA);
    CPPUNIT_TEST(testFindServiceWithDA);
    CPPUNIT_TEST(testFindServiceDATimeout);
    CPPUNIT_TEST(testFindServiceDAChangesScopes);
    CPPUNIT_TEST(testFindServiceCoLocatedDA);
    CPPUNIT_TEST(testFindServiceOnlyCoLocatedDA);
    CPPUNIT_TEST(testFindServiceOnlyCoLocatedDANoResults);
    CPPUNIT_TEST(testFindServiceMultipleDAs);
    CPPUNIT_TEST(testPassiveDADiscovery);
    CPPUNIT_TEST_SUITE_END();

    void testFindServiceNoDA();
    void testFindServiceWithDA();
    void testFindServiceDATimeout();
    void testFindServiceDAChangesScopes();
    void testFindServiceCoLocatedDA();
    void testFindServiceOnlyCoLocatedDA();
    void testFindServiceOnlyCoLocatedDANoResults();
    void testFindServiceMultipleDAs();
    void testPassiveDADiscovery();

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
    const URLEntry url1;
    const URLEntry url2;

    static const char FOO_SERVICE[];
};

const char SLPServerUATest::FOO_SERVICE[] = "service:foo";


CPPUNIT_TEST_SUITE_REGISTRATION(SLPServerUATest);


/**
 * This class allows us to verify the results of a FindService callback.
 */
class URLListVerifier {
  public:
    typedef ola::BaseCallback1<void, const URLEntries&> FindServiceCallback;

    explicit URLListVerifier(const URLEntries &expected_urls)
        : m_callback(ola::NewCallback(this, &URLListVerifier::NewServices)),
          m_expected_urls(expected_urls),
          m_received_callback(false) {
    }

    ~URLListVerifier() {
      if (!std::uncaught_exception())
        OLA_ASSERT_TRUE(m_received_callback);
    }

    FindServiceCallback *GetCallback() const { return m_callback.get(); }

    void Reset() {
      m_received_callback = false;
    }

    bool CallbackRan() const { return m_received_callback; }

    void NewServices(const URLEntries &urls) {
      OLA_ASSERT_VECTOR_EQ(m_expected_urls, urls);
      m_received_callback = true;
    }

  private:
    auto_ptr<FindServiceCallback> m_callback;
    const URLEntries m_expected_urls;
    bool m_received_callback;
};


/**
 * Test finding services where there are no DAs
 */
void SLPServerUATest::testFindServiceNoDA() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  m_helper.HandleInitialActiveDADiscovery("one");

  IPV4SocketAddress sa1 = IPV4SocketAddress::FromStringOrDie(
      "192.168.0.1:5570");
  IPV4SocketAddress sa2 = IPV4SocketAddress::FromStringOrDie(
      "192.168.0.8:5570");
  xid_t xid = 1;
  ScopeSet scopes("one");
  set<string> search_scopes;
  search_scopes.insert("one");

  // send a multicast SrvRqst, nothing responds
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries urls;
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectMulticastServiceRequest(xid, FOO_SERVICE, scopes, pr_list);

    server->FindService(search_scopes, FOO_SERVICE, url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    m_helper.ExpectMulticastServiceRequest(xid, FOO_SERVICE, scopes, pr_list);
    m_helper.AdvanceTime(2);  // first timeout

    m_helper.AdvanceTime(4);  // second timeout
  }

  xid++;

  // do the same, but this time two SAs respond. This checks we don't just take
  // the first response.
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(url1);
    urls.push_back(url2);
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectMulticastServiceRequest(xid, FOO_SERVICE, scopes, pr_list);

    server->FindService(search_scopes, FOO_SERVICE, url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    // now the SAs respond
    URLEntries sa1_urls;
    sa1_urls.push_back(url1);
    m_helper.InjectServiceReply(sa1, xid, SLP_OK, sa1_urls);

    URLEntries sa2_urls;
    sa2_urls.push_back(url2);
    m_helper.InjectServiceReply(sa2, xid, SLP_OK, sa2_urls);

    pr_list.insert(sa1.Host());
    pr_list.insert(sa2.Host());
    // the PR list changed, so we need a new xid
    m_helper.ExpectMulticastServiceRequest(++xid, FOO_SERVICE, scopes, pr_list);
    m_helper.AdvanceTime(2);  // first timeout

    m_helper.AdvanceTime(4);  // second timeout
  }

  // try the same thing, but this time have one of the SAs return an error
  // this shouldn't happen since SAs aren't supposed to return errors to
  // multicast requests, but you never know.
  xid++;

  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(url1);
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectMulticastServiceRequest(xid, FOO_SERVICE, scopes, pr_list);

    server->FindService(search_scopes, FOO_SERVICE, url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    // now the SAs respond
    URLEntries sa1_urls;
    sa1_urls.push_back(url1);
    m_helper.InjectServiceReply(sa1, xid, SLP_OK, sa1_urls);

    m_helper.InjectServiceReply(sa2, xid, SCOPE_NOT_SUPPORTED, URLEntries());

    pr_list.insert(sa1.Host());
    m_helper.ExpectMulticastServiceRequest(++xid, FOO_SERVICE, scopes, pr_list);
    m_helper.AdvanceTime(2);  // first timeout

    m_helper.AdvanceTime(4);  // second timeout
  }
}


/**
 * Test finding a service with a DA present
 */
void SLPServerUATest::testFindServiceWithDA() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  m_helper.HandleInitialActiveDADiscovery("one");

  xid_t xid = 1;
  ScopeSet scopes("one");
  set<string> search_scopes;
  search_scopes.insert("one");

  // now a DA appears
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 1, scopes);
  DAList da_list;
  da_list.insert(da1.Host());
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);

  // now try to find a service
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(url1);
    urls.push_back(url2);
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectServiceRequest(da1, xid, FOO_SERVICE, scopes, pr_list);
    server->FindService(search_scopes, FOO_SERVICE, url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    // now the DA responds
    URLEntries da_urls;
    da_urls.push_back(url1);
    da_urls.push_back(url2);
    m_helper.InjectServiceReply(da1, xid, SLP_OK, da_urls);
    OLA_ASSERT_TRUE(url_verifier.CallbackRan());

    m_helper.AdvanceTime(2);  // ensure nothing else happens
  }

  xid++;
  // try the same thing again, but this time the DA responds with an error,
  // this forces us back to multicast mode.
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(url1);
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectServiceRequest(da1, xid, FOO_SERVICE, scopes, pr_list);
    server->FindService(search_scopes, FOO_SERVICE, url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    // now the DA responds, triggering a multicast SrvRqst
    m_helper.ExpectMulticastServiceRequest(xid + 1, FOO_SERVICE, scopes,
                                           pr_list);
    m_helper.InjectError(da1, SERVICE_REPLY, xid, ola::slp::INTERNAL_ERROR);
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    // now a SA responds
    IPV4SocketAddress sa1 = IPV4SocketAddress::FromStringOrDie(
        "192.168.0.1:5570");
    URLEntries sa1_urls;
    sa1_urls.push_back(url1);
    m_helper.InjectServiceReply(sa1, ++xid, SLP_OK, sa1_urls);

    // let the request timeout, this triggers another SrvRqst, with sa1 in the
    // PRlist
    pr_list.insert(sa1.Host());
    m_helper.ExpectMulticastServiceRequest(++xid, FOO_SERVICE, scopes, pr_list);
    m_helper.AdvanceTime(2);

    // timeout the second multicast SrvRqst, which runs the callback
    m_helper.AdvanceTime(4);
  }
}


/**
 * Test falling back from one DA to another if it times out.
 */
void SLPServerUATest::testFindServiceDATimeout() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  m_helper.HandleInitialActiveDADiscovery("one");

  xid_t xid = 1;
  ScopeSet scopes("one");
  set<string> search_scopes;
  search_scopes.insert("one");

  // now a 2 DAs appear
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  IPV4SocketAddress da2 = IPV4SocketAddress::FromStringOrDie("10.0.1.2:5570");
  m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 1, scopes);
  m_helper.InjectDAAdvert(da2, 0, true, SLP_OK, 1, scopes);

  // now try to find a service, the first DA doesn't respond, causing us to
  // fall back to the second one
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(url1);
    urls.push_back(url2);
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectServiceRequest(da1, xid, FOO_SERVICE, scopes, pr_list);
    server->FindService(search_scopes, FOO_SERVICE, url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    m_helper.ExpectServiceRequest(da1, xid, FOO_SERVICE, scopes, pr_list);
    m_helper.AdvanceTime(2);
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    m_helper.ExpectServiceRequest(da1, xid, FOO_SERVICE, scopes, pr_list);
    m_helper.AdvanceTime(4);
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    m_helper.ExpectServiceRequest(da2, ++xid, FOO_SERVICE, scopes, pr_list);
    m_helper.AdvanceTime(8);
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    m_helper.ExpectServiceRequest(da2, xid, FOO_SERVICE, scopes, pr_list);
    // the DA doesn't respond to the first request
    m_helper.AdvanceTime(2);

    // now it responds
    URLEntries da_urls;
    da_urls.push_back(url1);
    da_urls.push_back(url2);
    m_helper.InjectServiceReply(da2, xid, SLP_OK, da_urls);
  }
}


/**
 * Test the case where a DA doesn't respond, and then changes it's supported
 * scopes.
 */
void SLPServerUATest::testFindServiceDAChangesScopes() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  m_helper.HandleInitialActiveDADiscovery("one");

  xid_t xid = 1;
  ScopeSet scopes("one,two");
  ScopeSet new_scopes("two");
  ScopeSet search_scopes("one");
  set<string> search_scopes_set;
  search_scopes_set.insert("one");

  // now a 2 DAs appear, both supports scopes "one" and "two"
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  IPV4SocketAddress da2 = IPV4SocketAddress::FromStringOrDie("10.0.1.2:5570");
  m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 1, scopes);
  m_helper.InjectDAAdvert(da2, 0, true, SLP_OK, 1, scopes);

  // now try to find a service
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries urls;
    urls.push_back(url1);
    urls.push_back(url2);
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectServiceRequest(da1, xid, FOO_SERVICE, search_scopes,
                                  pr_list);
    server->FindService(search_scopes_set, FOO_SERVICE,
                        url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    // now the DA we're using changes the scopes, this should cause us to
    // switch over the the second DA
    m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 1, new_scopes);
    m_helper.ExpectServiceRequest(da2, ++xid, FOO_SERVICE, search_scopes,
                                  pr_list);
    m_helper.AdvanceTime(2);

    // now it responds
    URLEntries da_urls;
    da_urls.push_back(url1);
    da_urls.push_back(url2);
    m_helper.InjectServiceReply(da2, xid, SLP_OK, da_urls);
  }
}


/**
 * Test the case where the UA is co-located with the DA, but we still need to
 * multicast to cover some scopes.
 */
void SLPServerUATest::testFindServiceCoLocatedDA() {
  IPV4SocketAddress sa1 = IPV4SocketAddress::FromStringOrDie(
      "192.168.0.1:5570");
  ScopeSet scopes("one");

  // expect a DAAdvert on startup
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup("one"));

  xid_t xid = 1;
  ScopeSet multicast_search_scopes("two");
  set<string> search_scopes_set;
  search_scopes_set.insert("one");
  search_scopes_set.insert("two");

  // register a service
  ServiceEntry service("one", url1.url(), url1.lifetime());
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  // now try to find a service
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    // one service is local, the other we find using multicast
    URLEntries urls;
    urls.push_back(url1);
    urls.push_back(url2);
    URLListVerifier url_verifier(urls);

    PRList pr_list;
    m_helper.ExpectMulticastServiceRequest(xid, FOO_SERVICE,
                                           multicast_search_scopes, pr_list);
    server->FindService(search_scopes_set, FOO_SERVICE,
                        url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());

    // the SA responds
    URLEntries sa1_urls;
    sa1_urls.push_back(url2);
    m_helper.InjectServiceReply(sa1, xid, SLP_OK, sa1_urls);

    pr_list.insert(sa1.Host());
    // the PR list changed, so we need a new xid
    m_helper.ExpectMulticastServiceRequest(++xid, FOO_SERVICE,
                                           multicast_search_scopes, pr_list);
    m_helper.AdvanceTime(2);  // first timeout

    m_helper.AdvanceTime(4);  // second timeout
  }
  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Test the case where the UA is co-located with the DA and it covers all the
 * scopes we're searching.
 */
void SLPServerUATest::testFindServiceOnlyCoLocatedDA() {
  IPV4SocketAddress sa1 = IPV4SocketAddress::FromStringOrDie(
      "192.168.0.1:5570");
  ScopeSet scopes("one");

  // expect a DAAdvert on startup
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup("one"));

  set<string> search_scopes_set;
  search_scopes_set.insert("one");

  // register a service
  ServiceEntry service("one", url1.url(), url1.lifetime());
  OLA_ASSERT_EQ((uint16_t) SLP_OK, server->RegisterService(service));

  // now try to find a service
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    // one service is local, the other we find using multicast
    URLEntries urls;
    urls.push_back(url1);
    URLListVerifier url_verifier(urls);

    server->FindService(search_scopes_set, FOO_SERVICE,
                        url_verifier.GetCallback());
  }
  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Test the case where the UA is co-located with the DA, it covers all the
 * scopes we're searching and no urls are returned.
 */
void SLPServerUATest::testFindServiceOnlyCoLocatedDANoResults() {
  IPV4SocketAddress sa1 = IPV4SocketAddress::FromStringOrDie(
      "192.168.0.1:5570");
  ScopeSet scopes("one");

  // expect a DAAdvert on startup
  auto_ptr<SLPServer> server(m_helper.CreateDAAndHandleStartup("one"));

  set<string> search_scopes_set;
  search_scopes_set.insert("one");

  // now try to find a service
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    // one service is local, the other we find using multicast
    URLEntries urls;
    URLListVerifier url_verifier(urls);

    server->FindService(search_scopes_set, FOO_SERVICE,
                        url_verifier.GetCallback());
  }
  m_helper.ExpectMulticastDAAdvert(0, 0, scopes);
}


/**
 * Test finding a service with multiple DAs present.
 */
void SLPServerUATest::testFindServiceMultipleDAs() {
  auto_ptr<SLPServer> server(m_helper.CreateNewServer(false, "one"));
  m_helper.HandleInitialActiveDADiscovery("one");

  ScopeSet scopes("one");
  set<string> search_scopes;
  search_scopes.insert("one");
  search_scopes.insert("two");

  // now a DA appears
  IPV4SocketAddress da1 = IPV4SocketAddress::FromStringOrDie("10.0.1.1:5570");
  ScopeSet da1_scopes("one");
  m_helper.InjectDAAdvert(da1, 0, true, SLP_OK, 1, da1_scopes);

  IPV4SocketAddress da2 = IPV4SocketAddress::FromStringOrDie("10.0.1.2:5570");
  ScopeSet da2_scopes("two");
  m_helper.InjectDAAdvert(da2, 0, true, SLP_OK, 1, da2_scopes);

  DAList da_list;
  da_list.insert(da1.Host());
  da_list.insert(da2.Host());
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);


  URLEntries urls;
  urls.push_back(url1);
  urls.push_back(url2);
  URLListVerifier url_verifier(urls);

  // now try to find a service, searching both scopes
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    PRList pr_list;
    m_helper.ExpectServiceRequest(da1, 1, FOO_SERVICE, da1_scopes, pr_list);
    m_helper.ExpectServiceRequest(da2, 2, FOO_SERVICE, da2_scopes, pr_list);
    server->FindService(search_scopes, FOO_SERVICE, url_verifier.GetCallback());
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());
  }

  // now the first DA responds
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries da_urls;
    da_urls.push_back(url1);
    m_helper.InjectServiceReply(da1, 1, SLP_OK, da_urls);
    OLA_ASSERT_FALSE(url_verifier.CallbackRan());
  }

  // now the second DA responds
  {
    SocketVerifier socket_verifier(&m_udp_socket);

    URLEntries da_urls;
    da_urls.push_back(url2);
    m_helper.InjectServiceReply(da2, 2, SLP_OK, da_urls);
    OLA_ASSERT_TRUE(url_verifier.CallbackRan());
  }

  {
    SocketVerifier socket_verifier(&m_udp_socket);
    m_helper.AdvanceTime(2);  // ensure nothing else happens
  }
}


/**
 * Test Passive DA Discovery behaviour
 */
void SLPServerUATest::testPassiveDADiscovery() {
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

  // Try a DAAdvert with a different url scheme. See Appendix C.
  IPV4SocketAddress da4 = IPV4SocketAddress::FromStringOrDie("10.0.1.4:5570");
  m_helper.InjectCustomDAAdvert(da4, "service:foobar://192.168.0.4", 0, true,
                                SLP_OK, 1, scopes);
  m_helper.VerifyKnownDAs(__LINE__, server.get(), da_list);
}
