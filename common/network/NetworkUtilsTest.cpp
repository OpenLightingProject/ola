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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * NetworkUtilsTest.cpp
 * Test fixture for the NetworkUtils class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#endif  // _WIN32

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
using ola::network::DefaultRoute;
using ola::network::NetworkToHost;

using std::string;
using std::vector;

class NetworkUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(NetworkUtilsTest);
  CPPUNIT_TEST(testToFromNetwork);
  CPPUNIT_TEST(testToFromLittleEndian);
  CPPUNIT_TEST(testNameProcessing);
  CPPUNIT_TEST(testNameServers);
  CPPUNIT_TEST(testDefaultRoute);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void tearDown();
    void testToFromNetwork();
    void testToFromLittleEndian();
    void testNameProcessing();
    void testNameServers();
    void testDefaultRoute();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NetworkUtilsTest);

/*
 * Setup networking subsystem
 */
void NetworkUtilsTest::setUp() {
#if _WIN32
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2, 0), &wsa_data);
  OLA_ASSERT_EQ(result, 0);
#endif  // _WIN32
}


/*
 * Cleanup the networking subsystem
 */
void NetworkUtilsTest::tearDown() {
#ifdef _WIN32
  WSACleanup();
#endif  // _WIN32
}


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

  uint64_t v4 = 0x0102030405060708;
  OLA_ASSERT_EQ(v4, NetworkToHost(HostToNetwork(v4)));
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

  uint64_t v4 = 0x0102030405060708;
  OLA_ASSERT_EQ(v4, LittleEndianToHost(HostToLittleEndian(v4)));

  int8_t v5 = -10;
  OLA_ASSERT_EQ(v5, HostToLittleEndian(v5));
  OLA_ASSERT_EQ(v5, LittleEndianToHost(HostToLittleEndian(v5)));

  int16_t v6 = -0x0102;
  OLA_ASSERT_EQ(v6, LittleEndianToHost(HostToLittleEndian(v6)));

  int32_t v7 = -0x01020304;
  OLA_ASSERT_EQ(v7, LittleEndianToHost(HostToLittleEndian(v7)));

  int64_t v8 = -0x0102030405060708;
  OLA_ASSERT_EQ(v8, LittleEndianToHost(HostToLittleEndian(v8)));
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
 * Check that name server fetching returns true (it may not actually return any)
 */
void NetworkUtilsTest::testNameServers() {
  vector<IPV4Address> name_servers;
  OLA_ASSERT_TRUE(NameServers(&name_servers));
}


/*
 * Check that default route fetching returns true (it may not actually return
 * one)
 */
void NetworkUtilsTest::testDefaultRoute() {
  int32_t if_index;
  IPV4Address default_gateway;
  OLA_ASSERT_TRUE(DefaultRoute(&if_index, &default_gateway));
}
