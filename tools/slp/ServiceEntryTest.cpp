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
 * ServiceEntryTest.cpp
 * Test fixture for the ServiceEntry classes.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>
#include <sstream>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/ServiceEntry.h"
#include "tools/slp/URLEntry.h"

using ola::slp::ScopeSet;
using ola::slp::ServiceEntry;
using ola::slp::URLEntry;
using ola::testing::ASSERT_DATA_EQUALS;
using std::set;
using std::string;


class ServiceEntryTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ServiceEntryTest);
  CPPUNIT_TEST(testConstructors);
  CPPUNIT_TEST(testMutation);
  CPPUNIT_TEST(testToString);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testConstructors();
    void testMutation();
    void testToString();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      scope_set = ScopeSet("one,two");
      service_type = "service:test";
      url = "service:test://localhost";
      lifetime = 300;
    }

  private:
    ScopeSet scope_set;
    string service_type;
    string url;
    uint16_t lifetime;
};


CPPUNIT_TEST_SUITE_REGISTRATION(ServiceEntryTest);


/**
 * Test the various constructors
 */
void ServiceEntryTest::testConstructors() {
  URLEntry url_entry(url, lifetime);

  ServiceEntry entry1(scope_set, service_type, url, lifetime);
  OLA_ASSERT_EQ(scope_set, entry1.scopes());
  OLA_ASSERT_EQ(service_type, entry1.service_type());
  OLA_ASSERT_EQ(url, entry1.url_string());
  OLA_ASSERT_EQ(url_entry, entry1.url());
  OLA_ASSERT_FALSE(entry1.local());

  // try a local service
  ServiceEntry entry2(scope_set, service_type, url, lifetime, true);
  OLA_ASSERT_EQ(scope_set, entry2.scopes());
  OLA_ASSERT_EQ(service_type, entry2.service_type());
  OLA_ASSERT_EQ(url, entry2.url_string());
  OLA_ASSERT_EQ(url_entry, entry2.url());
  OLA_ASSERT_TRUE(entry2.local());

  // these should not be equal
  OLA_ASSERT_NE(entry1, entry2);

  // test the copy constructor
  ServiceEntry entry3(entry1);
  OLA_ASSERT_EQ(scope_set, entry3.scopes());
  OLA_ASSERT_EQ(service_type, entry3.service_type());
  OLA_ASSERT_EQ(url, entry3.url_string());
  OLA_ASSERT_EQ(url_entry, entry3.url());
  OLA_ASSERT_FALSE(entry3.local());

  OLA_ASSERT_EQ(entry1, entry3);

  // The version that uses the service_type from the url
  ServiceEntry entry4(scope_set, url, lifetime);
  OLA_ASSERT_EQ(scope_set, entry4.scopes());
  OLA_ASSERT_EQ(service_type, entry4.service_type());
  OLA_ASSERT_EQ(url, entry4.url_string());
  OLA_ASSERT_EQ(url_entry, entry4.url());
  OLA_ASSERT_FALSE(entry4.local());

  OLA_ASSERT_EQ(entry1, entry4);

  // try a local service
  ServiceEntry entry5(scope_set, url, lifetime, true);
  OLA_ASSERT_EQ(scope_set, entry5.scopes());
  OLA_ASSERT_EQ(service_type, entry5.service_type());
  OLA_ASSERT_EQ(url, entry5.url_string());
  OLA_ASSERT_EQ(url_entry, entry5.url());
  OLA_ASSERT_TRUE(entry5.local());

  OLA_ASSERT_EQ(entry2, entry5);

  // test assignment
  entry3 = entry2;
  OLA_ASSERT_EQ(entry3, entry5);
}


/**
 * Test mutation works
 */
void ServiceEntryTest::testMutation() {
  uint16_t new_lifetime = 10;

  ServiceEntry entry1(scope_set, url, lifetime);
  OLA_ASSERT_EQ(scope_set, entry1.scopes());
  OLA_ASSERT_EQ(service_type, entry1.service_type());
  OLA_ASSERT_EQ(lifetime, entry1.url().lifetime());
  OLA_ASSERT_EQ(url, entry1.url_string());
  OLA_ASSERT_FALSE(entry1.local());

  entry1.set_local(true);
  OLA_ASSERT_TRUE(entry1.local());

  entry1.set_local(false);
  OLA_ASSERT_FALSE(entry1.local());

  entry1.mutable_url().set_lifetime(new_lifetime);
  OLA_ASSERT_EQ(new_lifetime, entry1.url().lifetime());
  OLA_ASSERT_EQ(url, entry1.url_string());
}


/**
 * Check converting to a string works
 */
void ServiceEntryTest::testToString() {
  ServiceEntry entry1(scope_set, url, lifetime);
  OLA_ASSERT_EQ(scope_set, entry1.scopes());
  OLA_ASSERT_EQ(service_type, entry1.service_type());
  OLA_ASSERT_EQ(lifetime, entry1.url().lifetime());
  OLA_ASSERT_EQ(url, entry1.url_string());
  OLA_ASSERT_FALSE(entry1.local());

  OLA_ASSERT_EQ(string("service:test://localhost(300), [one,two]"),
                entry1.ToString());
}
