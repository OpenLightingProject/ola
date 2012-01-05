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
 * IPAddressTest.cpp
 * Test fixture for the IPV4Address class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"

using ola::network::IPV4Address;
using std::auto_ptr;
using std::string;

class IPAddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(IPAddressTest);
  CPPUNIT_TEST(testIPV4Address);
  CPPUNIT_TEST(testWildcard);
  CPPUNIT_TEST(testBroadcast);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testIPV4Address();
    void testWildcard();
    void testBroadcast();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IPAddressTest);


/*
 * Test the IPV4 Address class works
 */
void IPAddressTest::testIPV4Address() {
  IPV4Address wildcard_address;
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), wildcard_address.ToString());
  CPPUNIT_ASSERT(0 == wildcard_address.Address().s_addr);
  CPPUNIT_ASSERT(wildcard_address.IsWildcard());

  struct in_addr in_addr1;
  CPPUNIT_ASSERT(ola::network::StringToAddress("192.168.1.1", in_addr1));
  IPV4Address address1(in_addr1);
  CPPUNIT_ASSERT_EQUAL(in_addr1.s_addr, address1.Address().s_addr);
  CPPUNIT_ASSERT(wildcard_address != address1);

  // Test Get()
  uint8_t addr[IPV4Address::LENGTH];
  address1.Get(addr);
  CPPUNIT_ASSERT_EQUAL(
      0,
      memcmp(addr,
             reinterpret_cast<uint8_t*>(&in_addr1),
             IPV4Address::LENGTH));

  // test copy and assignment
  IPV4Address address2(address1);
  CPPUNIT_ASSERT_EQUAL(address1, address2);
  IPV4Address address3 = address1;
  CPPUNIT_ASSERT_EQUAL(address1, address3);

  // test stringification
  CPPUNIT_ASSERT_EQUAL(string("192.168.1.1"), address1.ToString());
  std::stringstream str;
  str << address1;
  CPPUNIT_ASSERT_EQUAL(string("192.168.1.1"), str.str());

  // test from string
  auto_ptr<IPV4Address> string_address(IPV4Address::FromString("10.0.0.1"));
  CPPUNIT_ASSERT(string_address.get());
  CPPUNIT_ASSERT_EQUAL(string("10.0.0.1"), string_address->ToString());

  auto_ptr<IPV4Address> string_address2(IPV4Address::FromString("foo"));
  CPPUNIT_ASSERT(!string_address2.get());

  // and the second form
  IPV4Address string_address3;
  CPPUNIT_ASSERT(IPV4Address::FromString("172.16.4.1", &string_address3));
  CPPUNIT_ASSERT_EQUAL(string("172.16.4.1"), string_address3.ToString());

  // make sure sorting works
  std::vector<IPV4Address> addresses;
  addresses.push_back(address1);
  addresses.push_back(*string_address);
  addresses.push_back(string_address3);
  std::sort(addresses.begin(), addresses.end());

  // Addresses are in network byte order.
  if (ola::network::IsBigEndian()) {
    CPPUNIT_ASSERT_EQUAL(string("10.0.0.1"), addresses[0].ToString());
    CPPUNIT_ASSERT_EQUAL(string("172.16.4.1"), addresses[1].ToString());
    CPPUNIT_ASSERT_EQUAL(string("192.168.1.1"), addresses[2].ToString());
  } else {
    CPPUNIT_ASSERT_EQUAL(string("10.0.0.1"), addresses[0].ToString());
    CPPUNIT_ASSERT_EQUAL(string("192.168.1.1"), addresses[1].ToString());
    CPPUNIT_ASSERT_EQUAL(string("172.16.4.1"), addresses[2].ToString());
  }
}


/*
 * Test the wildcard address works.
 */
void IPAddressTest::testWildcard() {
  IPV4Address wildcard_address;
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), wildcard_address.ToString());
  CPPUNIT_ASSERT(0 == wildcard_address.Address().s_addr);
  CPPUNIT_ASSERT(wildcard_address.IsWildcard());

  IPV4Address wildcard_address2 = IPV4Address::WildCard();
  CPPUNIT_ASSERT_EQUAL(wildcard_address, wildcard_address2);
}


/*
 * Test the broadcast address works.
 */
void IPAddressTest::testBroadcast() {
  IPV4Address broadcast_address = IPV4Address::Broadcast();
  CPPUNIT_ASSERT_EQUAL(string("255.255.255.255"),
                       broadcast_address.ToString());
}
