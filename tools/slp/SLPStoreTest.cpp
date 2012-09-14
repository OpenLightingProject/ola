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

#include "ola/Logging.h"
#include "ola/Clock.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPStore.h"
#include "tools/slp/URLEntry.h"

using ola::MockClock;
using ola::TimeStamp;
using ola::slp::SLPStore;
using ola::slp::URLEntry;
using ola::slp::URLEntries;


class SLPStoreTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SLPStoreTest);
  CPPUNIT_TEST(testInsertAndLookup);
  CPPUNIT_TEST(testDoubleInsert);
  CPPUNIT_TEST(testBulkInsert);
  CPPUNIT_TEST(testAging);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testInsertAndLookup();
    void testDoubleInsert();
    void testBulkInsert();
    void testAging();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      m_clock.CurrentTime(&now);
    }

  private:
    SLPStore m_store;
    MockClock m_clock;
    TimeStamp now;

    void AdvanceTime(unsigned int seconds) {
      m_clock.AdvanceTime(seconds, 0);
      m_clock.CurrentTime(&now);
    }
};


/**
 * A helper function to check that two URLEntries match
 */
void AssertURLEntriesMatch(const CPPUNIT_NS::SourceLine &source_line,
                           const URLEntries &expected,
                           const URLEntries &actual) {
  CPPUNIT_NS::assertEquals(expected.size(), actual.size(), source_line,
                            "URLEntries sizes not equal");

  URLEntries::const_iterator iter1 = expected.begin();
  URLEntries::const_iterator iter2 = actual.begin();
  while (iter1 != expected.end()) {
    CPPUNIT_NS::assertEquals(*iter1, *iter2, source_line,
                             "URLEntries element not equal");
    // the == operator just checks the url, so check the lifetime as well
    CPPUNIT_NS::assertEquals(iter1->Lifetime(), iter2->Lifetime(), source_line,
                             "URLEntries elements lifetime not equal");
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
  // first we should get nothing for either service
  URLEntries url_entries, expected_url_entries;
  m_store.Lookup(now, "foo", &url_entries);
  OLA_ASSERT_TRUE(url_entries.empty());
  m_store.Lookup(now, "bar", &url_entries);
  OLA_ASSERT_TRUE(url_entries.empty());

  // insert a service
  URLEntry service1("one", 10);
  m_store.Insert(now, "foo", service1);
  m_store.Lookup(now, "foo", &url_entries);
  expected_url_entries.insert(service1);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);
  url_entries.clear();

  // bar should still be empty
  m_store.Lookup(now, "bar", &url_entries);
  OLA_ASSERT_TRUE(url_entries.empty());

  // insert a second "foo" service
  URLEntry service2("two", 10);
  m_store.Insert(now, "foo", service2);
  m_store.Lookup(now, "foo", &url_entries);
  expected_url_entries.insert(service2);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);

  // insert a bar service
  URLEntry service3("one", 10);
  m_store.Insert(now, "bar", service3);

  // check that foo is still ok
  url_entries.clear();
  m_store.Lookup(now, "foo", &url_entries);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);

  // check bar is ok
  url_entries.clear();
  m_store.Lookup(now, "bar", &url_entries);
  expected_url_entries.clear();
  expected_url_entries.insert(service3);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);
}


/*
 * Insert an entry into the Store twice. This checks we take the higher lifetime
 * of two entries.
 */
void SLPStoreTest::testDoubleInsert() {
  URLEntries url_entries, expected_url_entries;
  m_store.Lookup(now, "foo", &url_entries);

  URLEntry service("one", 10);
  URLEntry service_shorter("one", 5);
  URLEntry service_longer("one", 20);
  m_store.Insert(now, "foo", service);

  m_store.Lookup(now, "foo", &url_entries);
  expected_url_entries.insert(service);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);

  // now insert the shorter one
  url_entries.clear();
  m_store.Insert(now, "foo", service_shorter);
  m_store.Lookup(now, "foo", &url_entries);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);

  // now insert the longer one
  url_entries.clear();
  m_store.Insert(now, "foo", service_longer);
  expected_url_entries.clear();
  expected_url_entries.insert(service_longer);
  m_store.Lookup(now, "foo", &url_entries);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);
}


/*
 * Test the bulk loader.
 */
void SLPStoreTest::testBulkInsert() {
  URLEntries entries_to_insert;
  URLEntry service("one", 10);
  URLEntry service2("two", 10);
  entries_to_insert.insert(service);
  entries_to_insert.insert(service2);
  m_store.BulkInsert(now, "foo", entries_to_insert);

  URLEntries url_entries, expected_url_entries;
  expected_url_entries.insert(service);
  expected_url_entries.insert(service2);
  m_store.Lookup(now, "foo", &url_entries);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);
}


/*
 * Test aging.
 */
void SLPStoreTest::testAging() {
  URLEntry service("one", 10);
  URLEntry short_service("one", 5);
  URLEntry service2("two", 10);
  URLEntry short_service2("two", 5);
  m_store.Insert(now, "foo", service);

  AdvanceTime(10);

  URLEntries url_entries, expected_url_entries;
  m_store.Lookup(now, "foo", &url_entries);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);

  // insert it again
  m_store.Insert(now, "foo", service);
  AdvanceTime(5);
  m_store.Insert(now, "foo", service2);

  m_store.Lookup(now, "foo", &url_entries);
  expected_url_entries.insert(short_service);
  expected_url_entries.insert(service2);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);

  AdvanceTime(5);
  expected_url_entries.clear();
  url_entries.clear();
  m_store.Lookup(now, "foo", &url_entries);
  expected_url_entries.insert(short_service2);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);

  AdvanceTime(5);
  expected_url_entries.clear();
  url_entries.clear();
  m_store.Lookup(now, "foo", &url_entries);
  OLA_ASSERT_URLENTRIES_EQ(expected_url_entries, url_entries);
}
