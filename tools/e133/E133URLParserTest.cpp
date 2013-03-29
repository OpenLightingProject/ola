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
 * E133URLParserTest.cpp
 * Test fixture for the E133URLParser class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <ola/testing/TestUtils.h>
#include <ola/Logging.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/UID.h>
#include <string>

#include "tools/e133/E133URLParser.h"

using std::string;
using ola::network::IPV4Address;


class E133URLParserTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(E133URLParserTest);
  CPPUNIT_TEST(testParseUrl);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

    void testParseUrl();
};

CPPUNIT_TEST_SUITE_REGISTRATION(E133URLParserTest);


/*
 * Check that ParseE133URL works.
 */
void E133URLParserTest::testParseUrl() {
  ola::rdm::UID uid(0, 0);
  ola::rdm::UID expected_uid(0x7a70, 1);

  IPV4Address address;
  IPV4Address expected_ip;
  IPV4Address::FromString("192.168.1.204", &expected_ip);

  OLA_ASSERT_FALSE(ParseE133URL("", &uid, &address));
  OLA_ASSERT_FALSE(ParseE133URL("foo", &uid, &address));
  OLA_ASSERT_FALSE(ParseE133URL("service:e133", &uid, &address));
  OLA_ASSERT_FALSE(ParseE133URL("service:rdmnet-device", &uid, &address));
  OLA_ASSERT_FALSE(ParseE133URL("service:e131.esta", &uid, &address));
  OLA_ASSERT_FALSE(ParseE133URL("service:rdmnet-device:", &uid, &address));
  OLA_ASSERT_FALSE(ParseE133URL("service:e131.esta://", &uid, &address));
  OLA_ASSERT_FALSE(ParseE133URL("service:rdmnet-device://", &uid, &address));
  OLA_ASSERT_FALSE(
      ParseE133URL("service:e131.esta://10.0.0.1", &uid, &address));
  OLA_ASSERT(
      !ParseE133URL("service:rdmnet-device:10.0.0.1:5568", &uid, &address));
  OLA_ASSERT(
      !ParseE133URL("service:rdmnet-device://foobar:5568/7a7000000001",
                   &uid,
                   &address));
  OLA_ASSERT(
      !ParseE133URL("service:rdmnet-device://192.168.1.204:5568:7a7000000001",
                   &uid,
                   &address));
  OLA_ASSERT(
      !ParseE133URL("service:rdmnet-device://192.168.1.204:5568",
                   &uid,
                   &address));
  OLA_ASSERT(
      !ParseE133URL("service:rdmnet-device://192.168.1.204:5555/7a7000000001",
                   &uid,
                   &address));
  OLA_ASSERT(
      !ParseE133URL("service:rdmnet-device://192.168.1.204:5568/7g7000000",
                   &uid,
                   &address));
  OLA_ASSERT(
      !ParseE133URL("service:rdmnet-device://192.168.1.204:5568/7g7000000001",
                   &uid,
                   &address));
  OLA_ASSERT(
      !ParseE133URL("service:rdmnet-device://192.168.1.204:5568/7a7000000g01",
                   &uid,
                   &address));

  // finally the working cases
  OLA_ASSERT(
      ParseE133URL("service:rdmnet-device://192.168.1.204/7a7000000001",
                  &uid,
                  &address));
  OLA_ASSERT_EQ(expected_uid, uid);
  OLA_ASSERT_EQ(expected_ip, address);

  OLA_ASSERT(
      ParseE133URL("service:rdmnet-device://10.0.80.43/4a6100000020",
                  &uid,
                  &address));
  ola::rdm::UID expected_uid2(19041, 32);
  IPV4Address::FromString("10.0.80.43", &expected_ip);
  OLA_ASSERT_EQ(expected_uid2, uid);
  OLA_ASSERT_EQ(expected_ip, address);
}
