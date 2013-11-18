/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NetworkUtilsTest.cpp
 * Test fixture for the NetworkUtils class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <vector>

#include "ola/network/NetworkUtils.h"
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"

using ola::network::FQDN;
using ola::network::DomainNameFromFQDN;
using ola::network::Hostname;
using ola::network::HostnameFromFQDN;
using ola::network::HostToLittleEndian;
using ola::network::HostToNetwork;
using ola::network::IPV4Address;
using ola::network::LittleEndianToHost;
using ola::network::NameServers;
using ola::network::NetworkToHost;

using std::string;
using std::vector;

class NetworkUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(NetworkUtilsTest);
  CPPUNIT_TEST(testToFromNetwork);
  CPPUNIT_TEST(testToFromLittleEndian);
  CPPUNIT_TEST(testNameProcessing);
  CPPUNIT_TEST(testNameServers);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testToFromNetwork();
    void testToFromLittleEndian();
    void testNameProcessing();
    void testNameServers();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NetworkUtilsTest);


/*
 * Check that we can convert to/from network byte order
 */
void NetworkUtilsTest::testToFromNetwork() {
  uint8_t v1 = 10;
  OLA_ASSERT_EQ(v1, HostToNetwork(v1));
  OLA_ASSERT_EQ(v1, NetworkToHost(HostToNetwork(v1)));

  uint16_t v2 = 0x0102;
  OLA_ASSERT_EQ(v2, NetworkToHost(HostToNetwork(v2)));

  uint32_t v3 = 0x01020304;
  OLA_ASSERT_EQ(v3, NetworkToHost(HostToNetwork(v3)));
}


/*
 * Check that we can convert to/from little endian order
 */
void NetworkUtilsTest::testToFromLittleEndian() {
  uint8_t v1 = 10;
  OLA_ASSERT_EQ(v1, HostToLittleEndian(v1));
  OLA_ASSERT_EQ(v1, LittleEndianToHost(HostToLittleEndian(v1)));

  uint16_t v2 = 0x0102;
  OLA_ASSERT_EQ(v2, LittleEndianToHost(HostToLittleEndian(v2)));

  uint32_t v3 = 0x01020304;
  OLA_ASSERT_EQ(v3, LittleEndianToHost(HostToLittleEndian(v3)));
}


/*
 * Check that name processing works
 */
void NetworkUtilsTest::testNameProcessing() {
  // HostnameFromFQDN
  OLA_ASSERT_EQ(string(""), HostnameFromFQDN(""));
  OLA_ASSERT_EQ(string("foo"), HostnameFromFQDN("foo"));
  OLA_ASSERT_EQ(string("foo"), HostnameFromFQDN("foo.bar"));
  OLA_ASSERT_EQ(string("foo"), HostnameFromFQDN("foo.barbaz"));
  OLA_ASSERT_EQ(string("foo"), HostnameFromFQDN("foo.bar.com"));

  // DomainNameFromFQDN
  OLA_ASSERT_EQ(string(""), DomainNameFromFQDN(""));
  OLA_ASSERT_EQ(string(""), DomainNameFromFQDN("foo"));
  OLA_ASSERT_EQ(string("bar"), DomainNameFromFQDN("foo.bar"));
  OLA_ASSERT_EQ(string("barbaz"), DomainNameFromFQDN("foo.barbaz"));
  OLA_ASSERT_EQ(string("bar.com"), DomainNameFromFQDN("foo.bar.com"));

  // Check we were able to get the hostname
  OLA_ASSERT_GT(FQDN().length(), 0);
  OLA_ASSERT_GT(Hostname().length(), 0);
}


/*
 * Check that we can fetch some name servers
 */
void NetworkUtilsTest::testNameServers() {
  vector<IPV4Address> name_servers;
  OLA_ASSERT_TRUE(NameServers(&name_servers));
}
