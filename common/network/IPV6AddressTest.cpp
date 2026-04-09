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
 * IPV6AddressTest.cpp
 * Test fixture for the IPV6Address class
 * Copyright (C) 2023 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "common/network/NetworkUtilsInternal.h"
#include "ola/network/IPV6Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"


using ola::network::IPV6Address;
using ola::network::HostToNetwork;
using std::auto_ptr;
using std::string;
using std::vector;

class IPV6AddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(IPV6AddressTest);
  CPPUNIT_TEST(testIPV6Address);
  CPPUNIT_TEST(testWildcard);
  CPPUNIT_TEST(testLoopback);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testIPV6Address();
    void testWildcard();
    // TODO(Peter): Test the all-nodes link-local multicast group if we add it
    void testLoopback();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IPV6AddressTest);


/*
 * Test the IPV6 Address class works
 */
void IPV6AddressTest::testIPV6Address() {
  IPV6Address wildcard_address;
  OLA_ASSERT_EQ(string("::"), wildcard_address.ToString());
//  OLA_ASSERT_EQ(static_cast<in_addr_t>(0), wildcard_address.AsInt());
  OLA_ASSERT_TRUE(wildcard_address.IsWildcard());

  IPV6Address address1 = IPV6Address::FromStringOrDie("::ffff:c0a8:101");
//  int ip_as_int = address1.AsInt();
  OLA_ASSERT_NE(wildcard_address, address1);
//  OLA_ASSERT_NE(HostToNetwork(0xc0a811), ip_as_int);
//  OLA_ASSERT_EQ(HostToNetwork(0xc0a80101), static_cast<uint32_t>(ip_as_int));
  OLA_ASSERT_EQ(string("::ffff:192.168.1.1"), address1.ToString());

  IPV6Address address2 = IPV6Address::FromStringOrDie(
      "2001:db8:1234:5678:90ab:cdef:feed:face");
//  int ip_as_int = address2.AsInt();
  OLA_ASSERT_NE(wildcard_address, address2);
//  OLA_ASSERT_NE(HostToNetwork(0xc0a811), ip_as_int);
//  OLA_ASSERT_EQ(HostToNetwork(0xc0a80101), static_cast<uint32_t>(ip_as_int));

  const uint8_t big_endian_address_data[] =
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 10, 0, 0, 1};
  IPV6Address binary_address(big_endian_address_data);
  OLA_ASSERT_EQ(string("::ffff:10.0.0.1"), binary_address.ToString());

  // Test Get()
  uint8_t address_data[] = {32, 1, 13, 184, 18, 52, 86, 120,
                            144, 171, 205, 239, 254, 237, 250, 206};
  uint8_t addr[IPV6Address::LENGTH];
  address2.Get(addr);
  OLA_ASSERT_DATA_EQUALS(addr,
                         sizeof(addr),
                         reinterpret_cast<uint8_t*>(&address_data),
                         sizeof(address_data));

  // test copy and assignment
  IPV6Address address3(address1);
  OLA_ASSERT_EQ(address1, address3);
  IPV6Address address4 = address1;
  OLA_ASSERT_EQ(address1, address4);

  // test stringification
  OLA_ASSERT_EQ(string("::ffff:192.168.1.1"), address1.ToString());
  std::ostringstream str;
  str << address1;
  OLA_ASSERT_EQ(string("::ffff:192.168.1.1"), str.str());

  // test from string
  auto_ptr<IPV6Address> string_address(
      IPV6Address::FromString("::ffff:10.0.0.1"));
  OLA_ASSERT_NOT_NULL(string_address.get());
  OLA_ASSERT_EQ(string("::ffff:10.0.0.1"), string_address->ToString());

  auto_ptr<IPV6Address> string_address2(IPV6Address::FromString("foo"));
  OLA_ASSERT_NULL(string_address2.get());

  // and the second form
  IPV6Address string_address3;
  OLA_ASSERT_TRUE(IPV6Address::FromString(
      "::ffff:172.16.4.1", &string_address3));
  OLA_ASSERT_EQ(string("::ffff:172.16.4.1"), string_address3.ToString());

  IPV6Address string_address4;
  // Add the leading zero to the second group
  OLA_ASSERT_TRUE(IPV6Address::FromString(
      "2001:0db8:1234:5678:90ab:cdef:feed:face", &string_address4));
  // Confirm it's not rendered when we convert to a string
  OLA_ASSERT_EQ(string("2001:db8:1234:5678:90ab:cdef:feed:face"),
                string_address4.ToString());

  IPV6Address string_address5;
  OLA_ASSERT_TRUE(IPV6Address::FromString(
      "2001:db8:dead:beef:dead:beef:dead:beef", &string_address5));
  OLA_ASSERT_EQ(string("2001:db8:dead:beef:dead:beef:dead:beef"),
                string_address5.ToString());

  IPV6Address string_address6;
  OLA_ASSERT_FALSE(IPV6Address::FromString("", &string_address6));

  // make sure sorting works
  vector<IPV6Address> addresses;
  addresses.push_back(address1);
  addresses.push_back(*string_address);
  addresses.push_back(string_address3);
  addresses.push_back(string_address4);
  addresses.push_back(string_address5);
  std::sort(addresses.begin(), addresses.end());

  // The comparisons take into account network byte order automagically.
  OLA_ASSERT_EQ(string("::ffff:10.0.0.1"), addresses[0].ToString());
  OLA_ASSERT_EQ(string("::ffff:172.16.4.1"), addresses[1].ToString());
  OLA_ASSERT_EQ(string("::ffff:192.168.1.1"), addresses[2].ToString());
  OLA_ASSERT_EQ(string("2001:db8:1234:5678:90ab:cdef:feed:face"),
                addresses[3].ToString());
  OLA_ASSERT_EQ(string("2001:db8:dead:beef:dead:beef:dead:beef"),
                addresses[4].ToString());

/*  uint8_t mask = 255;  // UINT8_MAX;
  OLA_ASSERT_TRUE(
      IPV6Address::ToCIDRMask(IPV6Address::FromStringOrDie("0.0.0.0"), &mask));
  OLA_ASSERT_EQ(0, static_cast<int>(mask));

  OLA_ASSERT_TRUE(
      IPV6Address::ToCIDRMask(IPV6Address::FromStringOrDie("255.0.0.0"),
                              &mask));
  OLA_ASSERT_EQ(8, static_cast<int>(mask));

  OLA_ASSERT_TRUE(
      IPV6Address::ToCIDRMask(IPV6Address::FromStringOrDie("255.255.255.0"),
                              &mask));
  OLA_ASSERT_EQ(24, static_cast<int>(mask));

  OLA_ASSERT_TRUE(
      IPV6Address::ToCIDRMask(IPV6Address::FromStringOrDie("255.255.255.252"),
                              &mask));
  OLA_ASSERT_EQ(30, static_cast<int>(mask));

  OLA_ASSERT_TRUE(
      IPV6Address::ToCIDRMask(IPV6Address::FromStringOrDie("255.255.255.255"),
                              &mask));
  OLA_ASSERT_EQ(32, static_cast<int>(mask));

  OLA_ASSERT_FALSE(
      IPV6Address::ToCIDRMask(IPV6Address::FromStringOrDie("255.0.0.255"),
                              &mask));*/
}


/*
 * Test the wildcard address works.
 */
void IPV6AddressTest::testWildcard() {
  IPV6Address wildcard_address;
  OLA_ASSERT_EQ(string("::"), wildcard_address.ToString());
//  OLA_ASSERT_EQ(static_cast<in_addr_t>(0), wildcard_address.AsInt());
  OLA_ASSERT_TRUE(wildcard_address.IsWildcard());

  IPV6Address wildcard_address2 = IPV6Address::WildCard();
  OLA_ASSERT_EQ(wildcard_address, wildcard_address2);
}


/*
 * Test the loopback address works.
 */
void IPV6AddressTest::testLoopback() {
  IPV6Address loopback_address = IPV6Address::Loopback();
  OLA_ASSERT_EQ(string("::1"), loopback_address.ToString());
}
