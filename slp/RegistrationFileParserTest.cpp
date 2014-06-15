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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * RegistrationFileParserTest.cpp
 * Test fixture for the SLPStrings functions.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <sstream>
#include <string>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "slp/RegistrationFileParser.h"
#include "slp/SLPStrings.h"
#include "slp/ServiceEntry.h"
#include "slp/ScopeSet.h"

using ola::slp::RegistrationFileParser;
using ola::slp::ScopeSet;
using ola::slp::ServiceEntries;
using ola::slp::ServiceEntry;
using std::endl;
using std::set;
using std::string;
using std::stringstream;

class RegistrationFileParserTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RegistrationFileParserTest);
  CPPUNIT_TEST(testFromStream);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testFromStream();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

 private:
    RegistrationFileParser m_parser;
};


CPPUNIT_TEST_SUITE_REGISTRATION(RegistrationFileParserTest);

/**
 * Parse a stream.
 */
void RegistrationFileParserTest::testFromStream() {
  // This is a stringstream not a ostringstream as the other end needs an
  // istream
  stringstream str;
  str << "oNe  \tservice:foo://localhost    300" << endl;
  str << "tWO  \tservice:foo://192.168.1.1  600" << endl;
  str << "one,two,\tservice:bar://192.168.1.2\t300" << endl;
  // try some invalid lines
  str << "one,two,\tservice:bar://192.168.1.2" << endl;
  str << "one,two,\tservice:bar://192.168.1.2  foo" << endl;

  ServiceEntries services, expected_services;
  m_parser.ParseStream(&str, &services);
  OLA_ASSERT_EQ((size_t) 3, services.size());

  // and validate
  expected_services.push_back(
      ServiceEntry(ScopeSet("one"), "service:foo://localhost", 300));
  expected_services.push_back(
      ServiceEntry(ScopeSet("two"), "service:foo://192.168.1.1", 600));
  expected_services.push_back(
      ServiceEntry(ScopeSet("one,two"), "service:bar://192.168.1.2", 300));
  OLA_ASSERT_VECTOR_EQ(expected_services, services);

  // verify we don't get duplicates
  str.str("");
  str.clear();
  str << "one,two  service:foo://localhost  300" << endl;
  str << "one,two  service:foo://192.168.1.1  600" << endl;
  str << "one,two\tservice:bar://192.168.1.2\t300" << endl;
  str << "one,two  service:foo://192.168.1.1  600" << endl;
  services.clear();
  m_parser.ParseStream(&str, &services);
  OLA_ASSERT_EQ((size_t) 3, services.size());
}
