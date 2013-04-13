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
 * DATrackerTest.cpp
 * Test fixture for the DATracker class
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <queue>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/TestUtils.h"
#include "slp/SLPPacketParser.h"
#include "slp/ScopeSet.h"
#include "slp/DATracker.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::slp::DAAdvertPacket;
using ola::slp::DATracker;
using ola::slp::DirectoryAgent;
using ola::slp::ScopeSet;
using std::ostringstream;
using std::queue;
using std::sort;
using std::string;
using std::vector;

class DATrackerTest: public CppUnit::TestFixture {
  public:
    CPPUNIT_TEST_SUITE(DATrackerTest);
    CPPUNIT_TEST(testDirectoryAgent);
    CPPUNIT_TEST(testTracking);
    CPPUNIT_TEST(testDALookup);
    CPPUNIT_TEST(testScopeMatching);
    CPPUNIT_TEST_SUITE_END();

    void testDirectoryAgent();
    void testTracking();
    void testDALookup();
    void testScopeMatching();

  public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      ip1 = IPV4Address::FromStringOrDie(IP_ADDRESS1);
      ip2 = IPV4Address::FromStringOrDie(IP_ADDRESS2);
      ip3 = IPV4Address::FromStringOrDie(IP_ADDRESS3);
      ip4 = IPV4Address::FromStringOrDie(IP_ADDRESS4);
    }

    void Verify() {
      OLA_ASSERT_TRUE(m_expected_das.empty());
    }

    void ExpectDA(const DirectoryAgent &da) {
      m_expected_das.push(da);
    }

    void NewDACallback(const DirectoryAgent &da) {
      OLA_ASSERT_FALSE(m_expected_das.empty());
      OLA_ASSERT_EQ(m_expected_das.front(), da);
      m_expected_das.pop();
    }

  private:
    DATracker m_tracker;
    queue<DirectoryAgent> m_expected_das;
    IPV4Address ip1, ip2, ip3, ip4;

    void SendDAAdvert(uint16_t error,
                      const ScopeSet &scopes,
                      const string &url,
                      const uint32_t boot_timestamp,
                      const IPV4SocketAddress &address);

    static const char URL1[];
    static const char IP_ADDRESS1[];
    static const char URL2[];
    static const char IP_ADDRESS2[];
    static const char URL3[];
    static const char IP_ADDRESS3[];
    static const char URL4[];
    static const char IP_ADDRESS4[];
};


const char DATrackerTest::URL1[] = "service:directory-agent://192.168.0.1";
const char DATrackerTest::IP_ADDRESS1[] = "182.168.0.1";
const char DATrackerTest::URL2[] = "service:directory-agent://10.0.0.1";
const char DATrackerTest::IP_ADDRESS2[] = "10.0.0.1";
const char DATrackerTest::URL3[] = "service:directory-agent://10.0.0.2";
const char DATrackerTest::IP_ADDRESS3[] = "10.0.0.2";
const char DATrackerTest::URL4[] = "service:directory-agent://10.0.0.3";
const char DATrackerTest::IP_ADDRESS4[] = "10.0.0.3";

CPPUNIT_TEST_SUITE_REGISTRATION(DATrackerTest);


/**
 * Simulate the arrival of a DAAdvert
 */
void DATrackerTest::SendDAAdvert(uint16_t error,
                                 const ScopeSet &scopes,
                                 const string &url,
                                 const uint32_t boot_timestamp,
                                 const IPV4SocketAddress &address) {
  DAAdvertPacket da_advert;
  da_advert.xid = 0;  // nothing checks the xid
  da_advert.flags = 0;  // nothing checks multicast
  da_advert.language = "EN";
  da_advert.error_code = error;
  da_advert.boot_timestamp = boot_timestamp;
  da_advert.url = url;
  da_advert.scope_list = scopes.AsEscapedString();
  m_tracker.NewDAAdvert(da_advert, address);
}


/**
 * Check the DirectoryAgent class.
 */
void DATrackerTest::testDirectoryAgent() {
  const ScopeSet scopes("one,two");
  const DirectoryAgent da1(scopes, URL1, ip1, 1234);
  OLA_ASSERT_EQ(scopes, da1.scopes());
  OLA_ASSERT_EQ(string(URL1), da1.URL());
  OLA_ASSERT_EQ(ip1, da1.IPAddress());
  OLA_ASSERT_EQ(1234u, da1.BootTimestamp());
  OLA_ASSERT_EQ(0u, da1.MinRefreshInteral());

  DirectoryAgent da2(scopes, URL2, ip2, 5678);
  OLA_ASSERT_EQ(scopes, da2.scopes());
  OLA_ASSERT_EQ(string(URL2), da2.URL());
  OLA_ASSERT_EQ(ip2, da2.IPAddress());
  OLA_ASSERT_EQ(5678u, da2.BootTimestamp());
  OLA_ASSERT_EQ(0u, da2.MinRefreshInteral());

  OLA_ASSERT_NE(da1, da2);

  // test assignment
  da2 = da1;
  OLA_ASSERT_EQ(da1, da2);

  ostringstream str;
  str << da1;
  OLA_ASSERT_EQ(
      string("service:directory-agent://192.168.0.1(1234), [one,two]"),
      str.str());
}


/**
 * test tracking DAs works
 */
void DATrackerTest::testTracking() {
  DATracker::NewDACallback *da_callback = ola::NewCallback(
      this, &DATrackerTest::NewDACallback);
  m_tracker.AddNewDACallback(da_callback);
  OLA_ASSERT_EQ(0u, m_tracker.DACount());

  const ScopeSet scopes("one,two");
  const IPV4SocketAddress da1_address(ip1, 427);
  const DirectoryAgent da1(scopes, URL1, da1_address.Host(), 1234);

  // verify we don't do anything if the DAAdvert contains an error
  SendDAAdvert(ola::slp::INTERNAL_ERROR, scopes, URL1, 1234, da1_address);
  Verify();
  OLA_ASSERT_EQ(0u, m_tracker.DACount());

  // now pretend we get a DAAdvert
  ExpectDA(da1);
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL1, 1234, da1_address);
  Verify();
  OLA_ASSERT_EQ(1u, m_tracker.DACount());

  // pretend another DAAdvert was sent, because we've already been notified we
  // shouldn't get the callback again.
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL1, 1234, da1_address);
  Verify();
  OLA_ASSERT_EQ(1u, m_tracker.DACount());

  // pretend the DA is going down
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL1, 0, da1_address);
  Verify();
  OLA_ASSERT_EQ(0u, m_tracker.DACount());

  // now we should get the callback again
  ExpectDA(da1);
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL1, 1234, da1_address);
  Verify();
  OLA_ASSERT_EQ(1u, m_tracker.DACount());

  // now pretend the DA reboots, this should trigger the callback
  ExpectDA(da1);
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL1, 1235, da1_address);
  Verify();
  OLA_ASSERT_EQ(1u, m_tracker.DACount());

  // add a second DA just for fun
  const IPV4SocketAddress da2_address(ip2, 427);
  const DirectoryAgent da2(scopes, URL2, da2_address.Host(), 5678);

  ExpectDA(da2);
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL2, 5678, da2_address);
  Verify();
  OLA_ASSERT_EQ(2u, m_tracker.DACount());

  // check that we run the callback if the DA's scopes change
  const ScopeSet reduced_scopes("one");
  const DirectoryAgent reduced_da1(reduced_scopes, URL1, da1_address.Host(),
                                   1234);
  ExpectDA(da1);
  SendDAAdvert(ola::slp::SLP_OK, reduced_scopes, URL1, 1235, da1_address);
  Verify();
  OLA_ASSERT_EQ(2u, m_tracker.DACount());

  // confirm we don't run the callback if we get a DAAdvert with a smaller
  // boot_timestamp.
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL2, 1, da2_address);
  Verify();
  OLA_ASSERT_EQ(2u, m_tracker.DACount());

  // test malformed DAAdverts
  const IPV4SocketAddress da3_address(ip3, 427);
  SendDAAdvert(ola::slp::SLP_OK, scopes, "service:directory-agent://foobar", 1,
               da3_address);
  Verify();
  OLA_ASSERT_EQ(2u, m_tracker.DACount());

  SendDAAdvert(ola::slp::SLP_OK, scopes, "service:foobar", 1, da3_address);
  Verify();
  OLA_ASSERT_EQ(2u, m_tracker.DACount());
  m_tracker.RemoveNewDACallback(da_callback);
}


/**
 * Check that LookupDA works.
 */
void DATrackerTest::testDALookup() {
  const ScopeSet scopes("one,two");
  const IPV4SocketAddress da1_address(ip1, 427);
  const DirectoryAgent da1(scopes, URL1, da1_address.Host(), 1234);
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL1, 1234, da1_address);

  const IPV4SocketAddress da2_address(ip2, 427);
  const DirectoryAgent da2(scopes, URL2, da2_address.Host(), 5678);
  SendDAAdvert(ola::slp::SLP_OK, scopes, URL2, 1234, da2_address);

  OLA_ASSERT_EQ(2u, m_tracker.DACount());

  DirectoryAgent da;
  OLA_ASSERT_TRUE(m_tracker.LookupDA(URL1, &da));
  OLA_ASSERT_EQ(da1, da);

  OLA_ASSERT_TRUE(m_tracker.LookupDA(URL2, &da));
  OLA_ASSERT_EQ(da2, da);

  vector<DirectoryAgent> das, expected_das;
  m_tracker.GetDirectoryAgents(&das);
  expected_das.push_back(da1);
  expected_das.push_back(da2);
  sort(das.begin(), das.end());
  sort(expected_das.begin(), expected_das.end());
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  // now mark the DAs as bad
  m_tracker.MarkAsBad(URL1);
  OLA_ASSERT_EQ(1u, m_tracker.DACount());
  OLA_ASSERT_FALSE(m_tracker.LookupDA(URL1, &da));

  m_tracker.MarkAsBad(URL2);
  OLA_ASSERT_EQ(0u, m_tracker.DACount());
  OLA_ASSERT_FALSE(m_tracker.LookupDA(URL2, &da));
}


/**
 * Check the scope orientated lookup methods work
 */
void DATrackerTest::testScopeMatching() {
  const ScopeSet scopes1("one,two");
  const IPV4SocketAddress da1_address(ip1, 427);
  const DirectoryAgent da1(scopes1, URL1, da1_address.Host(), 1234);
  SendDAAdvert(ola::slp::SLP_OK, scopes1, URL1, 1234, da1_address);

  const ScopeSet scopes2("three,four");
  const IPV4SocketAddress da2_address(ip2, 427);
  const DirectoryAgent da2(scopes2, URL2, da2_address.Host(), 5678);
  SendDAAdvert(ola::slp::SLP_OK, scopes2, URL2, 1234, da2_address);

  const ScopeSet scopes3("five,six");
  const IPV4SocketAddress da3_address(ip3, 427);
  const DirectoryAgent da3(scopes3, URL3, da3_address.Host(), 5678);
  SendDAAdvert(ola::slp::SLP_OK, scopes3, URL3, 1234, da3_address);

  OLA_ASSERT_EQ(3u, m_tracker.DACount());

  // test GetDAsForScopes
  vector<DirectoryAgent> das, expected_das;
  m_tracker.GetDAsForScopes(ScopeSet("one"), &das);
  expected_das.push_back(da1);
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  das.clear();
  m_tracker.GetDAsForScopes(ScopeSet("one,three"), &das);
  expected_das.push_back(da2);
  sort(expected_das.begin(), expected_das.end());
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  das.clear();
  m_tracker.GetDAsForScopes(ScopeSet("one,three,four"), &das);
  sort(expected_das.begin(), expected_das.end());
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  das.clear();
  m_tracker.GetDAsForScopes(ScopeSet("one,three,four,six"), &das);
  expected_das.push_back(da3);
  sort(expected_das.begin(), expected_das.end());
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  // test GetMinimalCoveringList()
  expected_das.clear();
  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one"), &das);
  expected_das.push_back(da1);
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one,two"), &das);
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one,two,three"), &das);
  expected_das.push_back(da2);
  sort(expected_das.begin(), expected_das.end());
  sort(das.begin(), das.end());
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one,four"), &das);
  sort(das.begin(), das.end());
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  // try a scope that we don't have a DA for
  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one,four,seven"), &das);
  sort(das.begin(), das.end());
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  // add another DA which will change the min spanning set
  const ScopeSet scopes4("one,two,three,four");
  const IPV4SocketAddress da4_address(ip4, 427);
  const DirectoryAgent da4(scopes4, URL4, da4_address.Host(), 5678);
  SendDAAdvert(ola::slp::SLP_OK, scopes4, URL4, 1244, da4_address);

  expected_das.clear();
  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one,four"), &das);
  expected_das.push_back(da4);
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one,four,six"), &das);
  expected_das.push_back(da3);
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one,four,six,seven"), &das);
  OLA_ASSERT_VECTOR_EQ(expected_das, das);

  // mark da4 as bad
  m_tracker.MarkAsBad(URL4);
  expected_das.clear();
  das.clear();
  m_tracker.GetMinimalCoveringList(ScopeSet("one,four,six,seven"), &das);
  expected_das.push_back(da1);
  expected_das.push_back(da2);
  expected_das.push_back(da3);
  sort(expected_das.begin(), expected_das.end());
  sort(das.begin(), das.end());
  OLA_ASSERT_VECTOR_EQ(expected_das, das);
}
