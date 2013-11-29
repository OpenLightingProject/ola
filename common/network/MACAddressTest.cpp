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
 * MACAddressTest.cpp
 * Test fixture for the MACAddress class
 * Copyright (C) 2013 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"


using ola::network::MACAddress;
using std::auto_ptr;
using std::string;

class MACAddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(MACAddressTest);
  CPPUNIT_TEST(testMACAddress);
  CPPUNIT_TEST(testMACAddressToString);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testMACAddress();
    void testMACAddressToString();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MACAddressTest);


/*
 * Test the MAC Address class works
 */
void MACAddressTest::testMACAddress() {
  uint8_t hw_address[ola::network::MACAddress::LENGTH] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab};
  ether_addr ether_addr1;
  memcpy(ether_addr1.ether_addr_octet, hw_address, MACAddress::LENGTH);
  MACAddress address1;
  OLA_ASSERT_TRUE(MACAddress::FromString(string("01:23:45:67:89:ab"),
                                         &address1));
  OLA_ASSERT_EQ(
      0,
      memcmp(address1.Address().ether_addr_octet,
             reinterpret_cast<uint8_t*>(&ether_addr1.ether_addr_octet),
             MACAddress::LENGTH));

  // Test Get()
  uint8_t addr[MACAddress::LENGTH];
  address1.Get(addr);
  OLA_ASSERT_EQ(
      0,
      memcmp(addr,
             reinterpret_cast<uint8_t*>(&ether_addr1),
             MACAddress::LENGTH));

  // test copy and assignment
  MACAddress address2(address1);
  OLA_ASSERT_EQ(address1, address2);
  MACAddress address3 = address1;
  OLA_ASSERT_EQ(address1, address3);

  // test stringification
  OLA_ASSERT_EQ(string("01:23:45:67:89:ab"), address1.ToString());
  std::stringstream str;
  str << address1;
  OLA_ASSERT_EQ(string("01:23:45:67:89:ab"), str.str());

  // test from string
  auto_ptr<MACAddress> string_address(
      MACAddress::FromString("fe:dc:ba:98:76:54"));
  OLA_ASSERT_NOT_NULL(string_address.get());
  OLA_ASSERT_EQ(string("fe:dc:ba:98:76:54"), string_address->ToString());

  auto_ptr<MACAddress> string_address2(
      MACAddress::FromString("98.76.54.fe.dc.ba"));
  OLA_ASSERT_NOT_NULL(string_address2.get());
  OLA_ASSERT_EQ(string("98:76:54:fe:dc:ba"), string_address2->ToString());

  auto_ptr<MACAddress> string_address3(MACAddress::FromString("foo"));
  OLA_ASSERT_NULL(string_address3.get());

  // and the second form
  MACAddress string_address4;
  OLA_ASSERT_TRUE(MACAddress::FromString("67:89:ab:01:23:45",
                                         &string_address4));
  OLA_ASSERT_EQ(string("67:89:ab:01:23:45"), string_address4.ToString());

  // make sure sorting works
  std::vector<MACAddress> addresses;
  addresses.push_back(address1);
  addresses.push_back(*string_address);
  addresses.push_back(string_address4);
  std::sort(addresses.begin(), addresses.end());

  // Addresses are in network byte order.
  if (ola::network::IsBigEndian()) {
    OLA_ASSERT_EQ(string("01:23:45:67:89:ab"), addresses[0].ToString());
    OLA_ASSERT_EQ(string("fe:dc:ba:98:76:54"), addresses[1].ToString());
    OLA_ASSERT_EQ(string("67:89:ab:01:23:45"), addresses[2].ToString());
  } else {
    OLA_ASSERT_EQ(string("01:23:45:67:89:ab"), addresses[0].ToString());
    OLA_ASSERT_EQ(string("67:89:ab:01:23:45"), addresses[1].ToString());
    OLA_ASSERT_EQ(string("fe:dc:ba:98:76:54"), addresses[2].ToString());
  }
}

/*
 * Check that MACAddress::ToString works
 */
void MACAddressTest::testMACAddressToString() {
  uint8_t hw_address[ola::network::MACAddress::LENGTH] = {
    0x0, 0xa, 0xff, 0x10, 0x25, 0x4};
  const std::string mac_address = MACAddress(hw_address).ToString();
  OLA_ASSERT_EQ(std::string("00:0a:ff:10:25:04"), mac_address);
}
