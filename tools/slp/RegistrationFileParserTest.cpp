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
#include "tools/slp/RegistrationFileParser.h"
#include "tools/slp/SLPStrings.h"
#include "tools/slp/ServiceEntry.h"

using ola::slp::RegistrationFileParser;
using ola::slp::SLPExtractScopes;
using ola::slp::ServiceEntries;
using ola::slp::ServiceEntry;
using std::endl;
using std::set;
using std::string;
using std::stringstream;


class RegistrationFileParserTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RegistrationFileParserTest);
  CPPUNIT_TEST(testParser);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testParser();

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
void RegistrationFileParserTest::testParser() {
  stringstream str;
  str << "one,two  service:foo://localhost  300" << endl;
  str << "one,two  service:foo://192.168.1.1  600" << endl;
  str << "one,two\tservice:bar://192.168.1.2\t300" << endl;

  ServiceEntries services, expected_services;
  m_parser.ParseStream(&str, &services);
  OLA_ASSERT_EQ((size_t) 3, services.size());

  // and validate
  set<string> scopes;
  SLPExtractScopes("one,two", &scopes);
  expected_services.insert(
      ServiceEntry(scopes, "service:foo://localhost", 300));
  expected_services.insert(
      ServiceEntry(scopes, "service:foo://192.168.1.1", 600));
  expected_services.insert(
      ServiceEntry(scopes, "service:bar://192.168.1.2", 300));
  OLA_ASSERT_SET_EQ(expected_services, services);

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
