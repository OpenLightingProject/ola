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
 * SLPStoreTest.cpp
 * Test fixture for the SLPStore class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/Asserter.h>
#include <cppunit/SourceLine.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/ServiceEntry.h"

using ola::MockClock;
using ola::TimeStamp;
using ola::slp::INVALID_UPDATE;
using ola::slp::SCOPE_NOT_SUPPORTED;
using ola::slp::SLPStore;
using ola::slp::SLP_OK;
using ola::slp::ScopeSet;
using ola::slp::ServiceEntries;
using ola::slp::ServiceEntry;
using ola::slp::URLEntries;
using std::string;
using std::vector;


class SLPStoreTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SLPStoreTest);
  CPPUNIT_TEST(testInsertAndLookup);
  CPPUNIT_TEST(testDoubleInsert);
  CPPUNIT_TEST(testNonFresh);
  CPPUNIT_TEST(testRemove);
  CPPUNIT_TEST(testCheckIfScopesMatch);
  CPPUNIT_TEST(testGetLocalServices);
  CPPUNIT_TEST(testGetAllServices);
  CPPUNIT_TEST(testGetServiceTypesByNamingAuth);
  CPPUNIT_TEST(testAging);
  CPPUNIT_TEST(testClean);
  CPPUNIT_TEST(testDump);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testInsertAndLookup();
    void testDoubleInsert();
    void testNonFresh();
    void testRemove();
    void testCheckIfScopesMatch();
    void testGetLocalServices();
    void testGetAllServices();
    void testGetServiceTypesByNamingAuth();
    void testAging();
    void testClean();
    void testDump();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      m_clock.CurrentTime(&now);
      test_scopes = ScopeSet("scope1,scope2");
      test_scopes2 = ScopeSet("scope1");
      disjoint_scopes = ScopeSet("scope3");
    }

    static const char SCOPE1[];
    static const char SCOPE2[];
    static const char SCOPE3[];
    static const char SERVICE1[];
    static const char SERVICE2[];
    static const char SERVICE1_URL1[];
    static const char SERVICE1_URL2[];
    static const char SERVICE2_URL1[];
    static const char SERVICE2_URL2[];

  private:
    SLPStore m_store;
    MockClock m_clock;
    TimeStamp now;
    ScopeSet test_scopes;
    ScopeSet test_scopes2;
    ScopeSet disjoint_scopes;

    void AdvanceTime(unsigned int seconds) {
      m_clock.AdvanceTime(seconds, 0);
      m_clock.CurrentTime(&now);
    }
};

const char SLPStoreTest::SCOPE1[] = "scope1";
const char SLPStoreTest::SCOPE2[] = "scope2";
const char SLPStoreTest::SCOPE3[] = "scope3";
const char SLPStoreTest::SERVICE1[] = "service:one";
const char SLPStoreTest::SERVICE2[] = "service:two";
const char SLPStoreTest::SERVICE1_URL1[] = "service:one://192.168.1.1";
const char SLPStoreTest::SERVICE1_URL2[] = "service:one://192.168.1.2";
const char SLPStoreTest::SERVICE2_URL1[] = "service:two://192.168.1.1";
const char SLPStoreTest::SERVICE2_URL2[] = "service:two://192.168.1.3";


/**
 * A helper function to check that two ServiceEntries match
 */
void AssertURLEntriesMatch(const CPPUNIT_NS::SourceLine &source_line,
                               const URLEntries &expected,
                               const URLEntries &actual) {
  CPPUNIT_NS::assertEquals(expected.size(), actual.size(), source_line,
                            "ServiceEntries sizes not equal");

  URLEntries::const_iterator iter1 = expected.begin();
  URLEntries::const_iterator iter2 = actual.begin();
  while (iter1 != expected.end()) {
    CPPUNIT_NS::assertEquals(*iter1, *iter2, source_line,
                             "ServiceEntries element not equal");
    CPPUNIT_NS::assertEquals(iter1->lifetime(), iter2->lifetime(), source_line,
                             "ServiceEntries elements lifetime not equal");
    iter1++;
    iter2++;
  }
}

#define OLA_ASSERT_URLENTRIES_EQ(expected, output)  \
  AssertURLEntriesMatch(CPPUNIT_SOURCELINE(), (expected), (output))

CPPUNIT_TEST_SUITE_REGISTRATION(SLPStoreTest);

/*
 * Check that we can insert and lookup entries.
 */
void SLPStoreTest::testInsertAndLookup() {
  OLA_ASSERT_EQ(0u, m_store.ServiceTypeCount());

  // first we should get nothing for either service
  URLEntries urls, expected_urls;
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // insert a service and confirm it's there
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service1));
  OLA_ASSERT_EQ(1u, m_store.ServiceTypeCount());
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service1.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // confirm case-insensitive service-type matching
  urls.clear();
  m_store.Lookup(now, test_scopes, "SERVICE:OnE", &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  urls.clear();

  // try the same service in different scopes
  m_store.Lookup(now, disjoint_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // the second service should still be empty in both scopes
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());
  m_store.Lookup(now, disjoint_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // insert a second entry for the same service
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service2));
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service2.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // insert an entry for a different service
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 10);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service3));

  // check that the first service still returns the correct results
  urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // check the second service is there
  urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  expected_urls.clear();
  expected_urls.push_back(service3.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // but again, not for the other scopes
  urls.clear();
  m_store.Lookup(now, disjoint_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());
}


/*
 * Insert an entry into the Store twice. This checks we take the higher lifetime
 * of two entries as long as the scope list is the same.
 */
void SLPStoreTest::testDoubleInsert() {
  URLEntries urls, expected_urls;
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);

  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry service_shorter(test_scopes, SERVICE1_URL1, 5);
  ServiceEntry service_longer(test_scopes, SERVICE1_URL1, 20);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service));

  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // now insert the shorter one
  urls.clear();
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service_shorter));
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // now insert the longer one
  urls.clear();
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service_longer));
  expected_urls.clear();
  expected_urls.push_back(service_longer.url());
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  // Inserting the same url with different scopes should fail
  ServiceEntry different_scopes_service(disjoint_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SCOPE_NOT_SUPPORTED,
                m_store.Insert(now, different_scopes_service));
}


/*
 * Test that if we don't set the FRESH flag for a new service, we receive an
 * INVALID_UPDATE.
 */
void SLPStoreTest::testNonFresh() {
  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(INVALID_UPDATE, m_store.Insert(now, service, false));
}


/**
 * Test Remove()
 */
void SLPStoreTest::testRemove() {
  URLEntries urls, expected_urls;
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry other_service(test_scopes, SERVICE1_URL2, 10);

  // try to remove a non-existant service
  OLA_ASSERT_EQ(SLP_OK, m_store.Remove(service1));

  // insert
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service1));

  // verify it's there
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service1.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);
  expected_urls.clear();
  urls.clear();

  // try to remove a service that doesn't exist, but has the same service-type
  OLA_ASSERT_EQ(SLP_OK, m_store.Remove(other_service));

  // now try to remove the first service but with a different set of scopes
  ServiceEntry different_scopes_service(disjoint_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SCOPE_NOT_SUPPORTED, m_store.Remove(different_scopes_service));

  // verify it's still there
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(service1.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);
  expected_urls.clear();
  urls.clear();

  // now actually remove it
  OLA_ASSERT_EQ(SLP_OK, m_store.Remove(service1));

  // confirm it's no longer there
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // the number of urls should be zero, which indicates we've cleaned up
  // the service map correctly.
  OLA_ASSERT_EQ(0u, m_store.ServiceTypeCount());
}


/*
 * Test the CheckIfScopesMatch() method
 */
void SLPStoreTest::testCheckIfScopesMatch() {
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  // the same url, with different scopes
  ServiceEntry service2(test_scopes2, SERVICE1_URL1, 10);
  ServiceEntry service3(disjoint_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service1));

  OLA_ASSERT_EQ(SLPStore::OK, m_store.CheckIfScopesMatch(now, service1));
  OLA_ASSERT_EQ(SLPStore::SCOPE_MISMATCH,
                m_store.CheckIfScopesMatch(now, service2));
  OLA_ASSERT_EQ(SLPStore::SCOPE_MISMATCH,
                m_store.CheckIfScopesMatch(now, service3));

  // These services aren't in the store
  ServiceEntry service4(test_scopes, SERVICE1_URL2, 10);
  ServiceEntry service5(test_scopes, SERVICE2_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::NOT_FOUND,
                m_store.CheckIfScopesMatch(now, service4));
  OLA_ASSERT_EQ(SLPStore::NOT_FOUND,
                m_store.CheckIfScopesMatch(now, service5));

  // if the service has expired we should return NOT_FOUND
  AdvanceTime(15);
  OLA_ASSERT_EQ(SLPStore::NOT_FOUND,
                m_store.CheckIfScopesMatch(now, service1));
}


/*
 * Test the GetLocalServices() method
 */
void SLPStoreTest::testGetLocalServices() {
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10, true);
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10, false);
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 12, true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service1));
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service2));
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service3));

  // advance the clock by 2 seconds
  m_clock.AdvanceTime(2, 0);
  m_clock.CurrentTime(&now);

  ServiceEntries services;
  m_store.GetLocalServices(now, test_scopes, &services);
  OLA_ASSERT_EQ((size_t) 2, services.size());
  OLA_ASSERT_EQ(service1, services[0]);
  OLA_ASSERT_EQ(service3, services[1]);
  OLA_ASSERT_EQ((uint16_t) 8, services[0].url().lifetime());
  OLA_ASSERT_EQ((uint16_t) 10, services[1].url().lifetime());

  // age the entries out
  AdvanceTime(15);
  services.clear();
  m_store.GetLocalServices(now, test_scopes, &services);
  OLA_ASSERT_EQ((size_t) 0, services.size());
}


/*
 * Test the GetAllServiceTypes() method
 */
void SLPStoreTest::testGetAllServices() {
  // service:one, scopes: scope1
  ServiceEntry service1(test_scopes2, SERVICE1_URL1, 10, true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service1));
  // service:one, scopes: scope1,scope2
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10, false);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service2));
  // service:two, scopes: scope1,scope2
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 12, true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service3));
  // service:two, scopes: scope3
  ServiceEntry service4(disjoint_scopes, SERVICE2_URL2, 12, true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service4));

  // service:one.foo, scopes: scope3
  ServiceEntry service5(disjoint_scopes, "service:one.foo://192.168.1.1", 12,
                        true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service5));

  {
    vector<string> service_types, expected_service_types;
    ScopeSet scopes(SCOPE1);
    m_store.GetAllServiceTypes(scopes, &service_types);

    expected_service_types.push_back(SERVICE1);
    expected_service_types.push_back(SERVICE2);
    OLA_ASSERT_VECTOR_EQ(expected_service_types, service_types);
  }

  {
    vector<string> service_types, expected_service_types;
    ScopeSet scopes(SCOPE3);
    m_store.GetAllServiceTypes(scopes, &service_types);

    expected_service_types.push_back("service:one.foo");
    expected_service_types.push_back(SERVICE2);
    OLA_ASSERT_VECTOR_EQ(expected_service_types, service_types);
  }

  {
    vector<string> service_types, expected_service_types;
    ScopeSet scopes(SCOPE2);
    m_store.GetAllServiceTypes(scopes, &service_types);

    expected_service_types.push_back(SERVICE1);
    expected_service_types.push_back(SERVICE2);
    OLA_ASSERT_VECTOR_EQ(expected_service_types, service_types);
  }
}


/*
 * Test the GetServiceTypesByNamingAuth() method
 */
void SLPStoreTest::testGetServiceTypesByNamingAuth() {
  // service:one, scopes: scope1
  ServiceEntry service1(test_scopes2, SERVICE1_URL1, 10, true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service1));
  // service:one, scopes: scope1,scope2
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10, false);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service2));
  // service:two, scopes: scope1,scope2
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 12, true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service3));
  // service:two, scopes: scope3
  ServiceEntry service4(disjoint_scopes, SERVICE2_URL2, 12, true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service4));

  // service:two.bar, scopes: scope1
  ServiceEntry service5(test_scopes2, "service:one.foo://192.168.1.1", 12,
                        true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service5));

  // service:two., scopes: scope1
  ServiceEntry service6(test_scopes2, "service:two.://192.168.1.1", 12,
                        true);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service6));

  // Get IANA services
  {
    vector<string> service_types, expected_service_types;
    ScopeSet scopes(SCOPE1);
    m_store.GetServiceTypesByNamingAuth("", scopes, &service_types);

    expected_service_types.push_back(SERVICE1);
    expected_service_types.push_back(SERVICE2);
    expected_service_types.push_back("service:two.");
    OLA_ASSERT_VECTOR_EQ(expected_service_types, service_types);
  }

  // Get 'foo' services
  {
    vector<string> service_types, expected_service_types;
    ScopeSet scopes(SCOPE1);
    m_store.GetServiceTypesByNamingAuth("foo", scopes, &service_types);
    expected_service_types.push_back("service:one.foo");
    OLA_ASSERT_VECTOR_EQ(expected_service_types, service_types);
  }
}


/*
 * Test aging.
 */
void SLPStoreTest::testAging() {
  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry short_service(test_scopes, SERVICE1_URL1, 5);
  ServiceEntry service2(test_scopes, SERVICE2_URL1, 10);
  ServiceEntry short_service2(test_scopes, SERVICE2_URL1, 5);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service));

  AdvanceTime(10);

  URLEntries urls, expected_urls;
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  // insert it again
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service));
  AdvanceTime(5);
  // insert an entry for the second service
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service2));

  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  expected_urls.push_back(short_service.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  urls.clear();
  expected_urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  expected_urls.push_back(service2.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  AdvanceTime(5);
  expected_urls.clear();
  urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());

  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  expected_urls.push_back(short_service2.url());
  OLA_ASSERT_URLENTRIES_EQ(expected_urls, urls);

  AdvanceTime(5);
  expected_urls.clear();
  urls.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &urls);
  OLA_ASSERT_TRUE(urls.empty());
  m_store.Lookup(now, test_scopes, SERVICE2, &urls);
  OLA_ASSERT_TRUE(urls.empty());
}

/**
 * test Cleaning
 */
void SLPStoreTest::testClean() {
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10);
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 10);
  ServiceEntry service4(test_scopes, SERVICE2_URL2, 20);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service1));
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service2));
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service3));
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service4));

  OLA_ASSERT_EQ(2u, m_store.ServiceTypeCount());

  AdvanceTime(15);
  m_store.Clean(now);
  OLA_ASSERT_EQ(1u, m_store.ServiceTypeCount());

  AdvanceTime(10);
  m_store.Clean(now);
  OLA_ASSERT_EQ(0u, m_store.ServiceTypeCount());
}


/**
 * Check Dump() works
 */
void SLPStoreTest::testDump() {
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10);
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 10);
  ServiceEntry service4(test_scopes, SERVICE2_URL2, 20);
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service1));
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service2));
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service3));
  OLA_ASSERT_EQ(SLP_OK, m_store.Insert(now, service4));
  // this writes to stdout so we can't verify it. We can at least check we
  // don't crash.
  m_store.Dump(now);
}
