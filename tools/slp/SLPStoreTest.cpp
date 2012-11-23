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
 * SLPStoreTest.cpp
 * Test fixture for the SLPStore class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/Asserter.h>
#include <cppunit/SourceLine.h>
#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>

#include "ola/Logging.h"
#include "ola/Clock.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPStore.h"

using ola::MockClock;
using ola::TimeStamp;
using ola::slp::SLPStore;
using ola::slp::ServiceEntries;
using ola::slp::ServiceEntry;
using ola::slp::URLEntries;
using std::set;
using std::string;


class SLPStoreTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SLPStoreTest);
  CPPUNIT_TEST(testInsertAndLookup);
  CPPUNIT_TEST(testURLEntryLookup);
  CPPUNIT_TEST(testDoubleInsert);
  CPPUNIT_TEST(testBulkInsert);
  CPPUNIT_TEST(testRemove);
  CPPUNIT_TEST(testAging);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testInsertAndLookup();
    void testURLEntryLookup();
    void testDoubleInsert();
    void testBulkInsert();
    void testRemove();
    void testAging();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      m_clock.CurrentTime(&now);
      test_scopes.insert(SCOPE1);
      test_scopes.insert(SCOPE2);
      disjoint_scopes.insert(SCOPE3);
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
    set<string> test_scopes;
    set<string> disjoint_scopes;

    void AdvanceTime(unsigned int seconds) {
      m_clock.AdvanceTime(seconds, 0);
      m_clock.CurrentTime(&now);
    }
};

const char SLPStoreTest::SCOPE1[] = "scope1";
const char SLPStoreTest::SCOPE2[] = "scope2";
const char SLPStoreTest::SCOPE3[] = "scope3";
const char SLPStoreTest::SERVICE1[] = "one";
const char SLPStoreTest::SERVICE2[] = "two";
const char SLPStoreTest::SERVICE1_URL1[] = "service:one://192.168.1.1";
const char SLPStoreTest::SERVICE1_URL2[] = "service:one://192.168.1.2";
const char SLPStoreTest::SERVICE2_URL1[] = "service:two://192.168.1.1";
const char SLPStoreTest::SERVICE2_URL2[] = "service:two://192.168.1.3";


/**
 * A helper function to check that two ServiceEntries match
 */
void AssertServiceEntriesMatch(const CPPUNIT_NS::SourceLine &source_line,
                               const ServiceEntries &expected,
                               const ServiceEntries &actual) {
  CPPUNIT_NS::assertEquals(expected.size(), actual.size(), source_line,
                            "ServiceEntries sizes not equal");

  ServiceEntries::const_iterator iter1 = expected.begin();
  ServiceEntries::const_iterator iter2 = actual.begin();
  while (iter1 != expected.end()) {
    CPPUNIT_NS::assertEquals(*iter1, *iter2, source_line,
                             "ServiceEntries element not equal");
    // the == operator just checks the url, so check the lifetime as well
    CPPUNIT_NS::assertEquals(iter1->Lifetime(), iter2->Lifetime(), source_line,
                             "ServiceEntries elements lifetime not equal");
    OLA_ASSERT_SET_EQ(iter1->Scopes(), iter2->Scopes());
    iter1++;
    iter2++;
  }
}

#define OLA_ASSERT_URLENTRIES_EQ(expected, output)  \
  AssertServiceEntriesMatch(CPPUNIT_SOURCELINE(), (expected), (output))

CPPUNIT_TEST_SUITE_REGISTRATION(SLPStoreTest);

/*
 * Check that we can insert and lookup entries.
 */
void SLPStoreTest::testInsertAndLookup() {
  // first we should get nothing for either service
  ServiceEntries services, expected_services;
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_TRUE(services.empty());
  m_store.Lookup(now, test_scopes, SERVICE2, &services);
  OLA_ASSERT_TRUE(services.empty());

  // insert a service and confirm it's there
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service1));
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  expected_services.insert(service1);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);
  services.clear();

  // try the same service in different scopes
  m_store.Lookup(now, disjoint_scopes, SERVICE1, &services);
  OLA_ASSERT_TRUE(services.empty());

  // the second service should still be empty in both scopes
  m_store.Lookup(now, test_scopes, SERVICE2, &services);
  OLA_ASSERT_TRUE(services.empty());
  m_store.Lookup(now, disjoint_scopes, SERVICE2, &services);
  OLA_ASSERT_TRUE(services.empty());

  // insert a second entry for the same service
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service2));
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  expected_services.insert(service2);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  // insert an entry for a different service
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service3));

  // check that the first service still returns the correct results
  services.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  // check the second service is there
  services.clear();
  m_store.Lookup(now, test_scopes, SERVICE2, &services);
  expected_services.clear();
  expected_services.insert(service3);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  // but again, not for the other scopes
  services.clear();
  m_store.Lookup(now, disjoint_scopes, SERVICE2, &services);
  OLA_ASSERT_TRUE(services.empty());
}


/*
 * Check that the URLEntry form of Lookup works.
 */
void SLPStoreTest::testURLEntryLookup() {
  // first we should get nothing for either service
  URLEntries services, expected_services;
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_TRUE(services.empty());
  m_store.Lookup(now, test_scopes, SERVICE2, &services);
  OLA_ASSERT_TRUE(services.empty());

  // insert a service and confirm it's there
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service1));
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  expected_services.insert(service1);
  OLA_ASSERT_EQ(expected_services.size(), services.size());
  OLA_ASSERT_EQ(*(expected_services.begin()), *(services.begin()));
}


/*
 * Insert an entry into the Store twice. This checks we take the higher lifetime
 * of two entries as long as the scope list is the same
 */
void SLPStoreTest::testDoubleInsert() {
  ServiceEntries services, expected_services;
  m_store.Lookup(now, test_scopes, SERVICE1, &services);

  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry service_shorter(test_scopes, SERVICE1_URL1, 5);
  ServiceEntry service_longer(test_scopes, SERVICE1_URL1, 20);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service));

  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  expected_services.insert(service);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  // now insert the shorter one
  services.clear();
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service_shorter));
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  // now insert the longer one
  services.clear();
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service_longer));
  expected_services.clear();
  expected_services.insert(service_longer);
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  // Inserting the same url with different scopes should fail
  ServiceEntry different_scopes_service(disjoint_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::SCOPE_MISMATCH,
                m_store.Insert(now, different_scopes_service));
}


/*
 * Test the bulk loader.
 */
void SLPStoreTest::testBulkInsert() {
  ServiceEntries entries_to_insert;
  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry service2(test_scopes, SERVICE1_URL2, 10);
  entries_to_insert.insert(service);
  entries_to_insert.insert(service2);
  OLA_ASSERT_TRUE(m_store.BulkInsert(now, entries_to_insert));

  ServiceEntries services, expected_services;
  expected_services.insert(service);
  expected_services.insert(service2);
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  m_store.Reset();
  entries_to_insert.clear();
  expected_services.clear();
  services.clear();

  // now try it with Entries that have different services
  ServiceEntry service3(test_scopes, SERVICE2_URL1, 10);
  entries_to_insert.insert(service);
  entries_to_insert.insert(service3);
  OLA_ASSERT_FALSE(m_store.BulkInsert(now, entries_to_insert));
  expected_services.insert(service);
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  m_store.Reset();
  entries_to_insert.clear();
  expected_services.clear();
  services.clear();

  // now try it with Entries with different scopes, this should be fine
  ServiceEntry service4(disjoint_scopes, SERVICE1_URL2, 10);
  entries_to_insert.insert(service);
  entries_to_insert.insert(service4);
  OLA_ASSERT_TRUE(m_store.BulkInsert(now, entries_to_insert));

  expected_services.insert(service);
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  expected_services.clear();
  services.clear();

  expected_services.insert(service4);
  m_store.Lookup(now, disjoint_scopes, SERVICE1, &services);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);
}


/**
 * Test Remove()
 */
void SLPStoreTest::testRemove() {
  ServiceEntries services, expected_services;
  ServiceEntry service1(test_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service1));

  // verify it's there
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  expected_services.insert(service1);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);
  expected_services.clear();

  // now try to remove it with a different set of scopes
  ServiceEntry different_scopes_service(disjoint_scopes, SERVICE1_URL1, 10);
  OLA_ASSERT_EQ(SLPStore::SCOPE_MISMATCH,
                m_store.Remove(different_scopes_service));

  // verify it's still there
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  expected_services.insert(service1);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);
  expected_services.clear();
  services.clear();

  // now actually remove it
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Remove(service1));

  // confirm it's no longer there
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_TRUE(services.empty());

  // the number of services should be zero, which indicates we've cleaned up
  // the service map correctly.
  OLA_ASSERT_EQ(0u, m_store.ServiceCount());
}


/*
 * Test aging.
 */
void SLPStoreTest::testAging() {
  ServiceEntry service(test_scopes, SERVICE1_URL1, 10);
  ServiceEntry short_service(test_scopes, SERVICE1_URL1, 5);
  ServiceEntry service2(test_scopes, SERVICE2_URL1, 10);
  ServiceEntry short_service2(test_scopes, SERVICE2_URL1, 5);
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service));

  AdvanceTime(10);

  ServiceEntries services, expected_services;
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_TRUE(services.empty());

  // insert it again
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service));
  AdvanceTime(5);
  // insert an entry for the second service
  OLA_ASSERT_EQ(SLPStore::OK, m_store.Insert(now, service2));

  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  expected_services.insert(short_service);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  services.clear();
  expected_services.clear();
  m_store.Lookup(now, test_scopes, SERVICE2, &services);
  expected_services.insert(service2);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  AdvanceTime(5);
  expected_services.clear();
  services.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_TRUE(services.empty());

  m_store.Lookup(now, test_scopes, SERVICE2, &services);
  expected_services.insert(short_service2);
  OLA_ASSERT_URLENTRIES_EQ(expected_services, services);

  AdvanceTime(5);
  expected_services.clear();
  services.clear();
  m_store.Lookup(now, test_scopes, SERVICE1, &services);
  OLA_ASSERT_TRUE(services.empty());
  m_store.Lookup(now, test_scopes, SERVICE2, &services);
  OLA_ASSERT_TRUE(services.empty());
}
