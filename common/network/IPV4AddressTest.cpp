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
 * IPV4AddressTest.cpp
 * Test fixture for the IPV4Address class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#ifndef in_addr_t
#define in_addr_t uint32_t
#endif  // !in_addr_t
#endif  // _WIN32
#include "common/network/NetworkUtilsInternal.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"


using ola::network::IPV4Address;
using ola::network::HostToNetwork;
using std::unique_ptr;
using std::string;
using std::vector;

class IPAddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(IPAddressTest);
  CPPUNIT_TEST(testIPV4Address);
  CPPUNIT_TEST(testWildcard);
  CPPUNIT_TEST(testBroadcast);
  CPPUNIT_TEST(testLoopback);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testIPV4Address();
    void testWildcard();
    void testBroadcast();
    void testLoopback();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IPAddressTest);


/*
 * Test the IPV4 Address class works
 */
void IPAddressTest::testIPV4Address() {
  IPV4Address wildcard_address;
  OLA_ASSERT_EQ(string("0.0.0.0"), wildcard_address.ToString());
  OLA_ASSERT_EQ(static_cast<in_addr_t>(0), wildcard_address.AsInt());
  OLA_ASSERT_TRUE(wildcard_address.IsWildcard());

  IPV4Address address1 = IPV4Address::FromStringOrDie("192.168.1.1");
  int ip_as_int = address1.AsInt();
  OLA_ASSERT_NE(wildcard_address, address1);
  OLA_ASSERT_NE(HostToNetwork(0xc0a811), ip_as_int);
  OLA_ASSERT_EQ(HostToNetwork(0xc0a80101), static_cast<uint32_t>(ip_as_int));

  // Test Get()
  uint8_t addr[IPV4Address::LENGTH];
  address1.Get(addr);
  OLA_ASSERT_EQ(
      0,
      memcmp(addr, reinterpret_cast<uint8_t*>(&ip_as_int), sizeof(ip_as_int)));

  // test copy and assignment
  IPV4Address address2(address1);
  OLA_ASSERT_EQ(address1, address2);
  IPV4Address address3 = address1;
  OLA_ASSERT_EQ(address1, address3);

  // test stringification
  OLA_ASSERT_EQ(string("192.168.1.1"), address1.ToString());
  std::ostringstream str;
  str << address1;
  OLA_ASSERT_EQ(string("192.168.1.1"), str.str());

  // test from string
  unique_ptr<IPV4Address> string_address(IPV4Address::FromString("10.0.0.1"));
  OLA_ASSERT_NOT_NULL(string_address.get());
  OLA_ASSERT_EQ(string("10.0.0.1"), string_address->ToString());

  unique_ptr<IPV4Address> string_address2(IPV4Address::FromString("foo"));
  OLA_ASSERT_NULL(string_address2.get());

  // and the second form
  IPV4Address string_address3;
  OLA_ASSERT_TRUE(IPV4Address::FromString("172.16.4.1", &string_address3));
  OLA_ASSERT_EQ(string("172.16.4.1"), string_address3.ToString());

  IPV4Address string_address4;
  OLA_ASSERT_FALSE(IPV4Address::FromString("", &string_address4));

  // make sure sorting works
  vector<IPV4Address> addresses;
  addresses.push_back(address1);
  addresses.push_back(*string_address);
  addresses.push_back(string_address3);
  std::sort(addresses.begin(), addresses.end());

  // The comparisons take into account network byte order automagically.
  OLA_ASSERT_EQ(string("10.0.0.1"), addresses[0].ToString());
  OLA_ASSERT_EQ(string("172.16.4.1"), addresses[1].ToString());
  OLA_ASSERT_EQ(string("192.168.1.1"), addresses[2].ToString());

  uint8_t mask = 255;  // UINT8_MAX;
  OLA_ASSERT_TRUE(
      IPV4Address::ToCIDRMask(IPV4Address::FromStringOrDie("0.0.0.0"), &mask));
  OLA_ASSERT_EQ(0, static_cast<int>(mask));

  OLA_ASSERT_TRUE(
      IPV4Address::ToCIDRMask(IPV4Address::FromStringOrDie("255.0.0.0"),
                              &mask));
  OLA_ASSERT_EQ(8, static_cast<int>(mask));

  OLA_ASSERT_TRUE(
      IPV4Address::ToCIDRMask(IPV4Address::FromStringOrDie("255.255.255.0"),
                              &mask));
  OLA_ASSERT_EQ(24, static_cast<int>(mask));

  OLA_ASSERT_TRUE(
      IPV4Address::ToCIDRMask(IPV4Address::FromStringOrDie("255.255.255.252"),
                              &mask));
  OLA_ASSERT_EQ(30, static_cast<int>(mask));

  OLA_ASSERT_TRUE(
      IPV4Address::ToCIDRMask(IPV4Address::FromStringOrDie("255.255.255.255"),
                              &mask));
  OLA_ASSERT_EQ(32, static_cast<int>(mask));

  OLA_ASSERT_FALSE(
      IPV4Address::ToCIDRMask(IPV4Address::FromStringOrDie("255.0.0.255"),
                              &mask));
}


/*
 * Test the wildcard address works.
 */
void IPAddressTest::testWildcard() {
  IPV4Address wildcard_address;
  OLA_ASSERT_EQ(string("0.0.0.0"), wildcard_address.ToString());
  OLA_ASSERT_EQ(static_cast<in_addr_t>(0), wildcard_address.AsInt());
  OLA_ASSERT_TRUE(wildcard_address.IsWildcard());

  IPV4Address wildcard_address2 = IPV4Address::WildCard();
  OLA_ASSERT_EQ(wildcard_address, wildcard_address2);
}


/*
 * Test the broadcast address works.
 */
void IPAddressTest::testBroadcast() {
  IPV4Address broadcast_address = IPV4Address::Broadcast();
  OLA_ASSERT_EQ(string("255.255.255.255"),
                       broadcast_address.ToString());
}


/*
 * Test the loopback address works.
 */
void IPAddressTest::testLoopback() {
  IPV4Address loopback_address = IPV4Address::Loopback();
  OLA_ASSERT_EQ(string("127.0.0.1"), loopback_address.ToString());
}
