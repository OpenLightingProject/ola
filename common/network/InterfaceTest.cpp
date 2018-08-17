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
 * InterfaceTest.cpp
 * Test fixture for the Interface & InterfaceBuilder classes.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Logging.h"
#include "ola/network/Interface.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/MACAddress.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"


using ola::network::IPV4Address;
using ola::network::Interface;
using ola::network::InterfaceBuilder;
using ola::network::MACAddress;
using std::string;

class InterfaceTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(InterfaceTest);
  CPPUNIT_TEST(testBuilder);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testBuilder();
};


CPPUNIT_TEST_SUITE_REGISTRATION(InterfaceTest);


/*
 * Check that we find at least one candidate interface.
 */
void InterfaceTest::testBuilder() {
  InterfaceBuilder builder;

  Interface interface = builder.Construct();
  OLA_ASSERT_EQ(string(""), interface.name);
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.ip_address.ToString());
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.bcast_address.ToString());
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.subnet_mask.ToString());
  OLA_ASSERT_EQ(string("00:00:00:00:00:00"), interface.hw_address.ToString());

  // set & build an interface (mostly) from strings
  builder.SetName("eth0");
  OLA_ASSERT_TRUE(builder.SetAddress("192.168.1.1"));
  OLA_ASSERT_TRUE(builder.SetBroadcast("192.168.1.255"));
  OLA_ASSERT_TRUE(builder.SetSubnetMask("255.255.255.0"));
  builder.SetHardwareAddress(MACAddress::FromStringOrDie("e4:ff:29:36:74:12"));

  interface = builder.Construct();
  OLA_ASSERT_EQ(string("eth0"), interface.name);
  OLA_ASSERT_EQ(string("192.168.1.1"), interface.ip_address.ToString());
  OLA_ASSERT_EQ(string("192.168.1.255"), interface.bcast_address.ToString());
  OLA_ASSERT_EQ(string("255.255.255.0"), interface.subnet_mask.ToString());
  OLA_ASSERT_EQ(string("e4:ff:29:36:74:12"), interface.hw_address.ToString());

  // check the alternate form of mac address
  builder.SetHardwareAddress(MACAddress::FromStringOrDie("12.34.56.78.90.ab"));
  interface = builder.Construct();
  OLA_ASSERT_EQ(string("12:34:56:78:90:ab"), interface.hw_address.ToString());

  // reset the builder
  builder.Reset();
  interface = builder.Construct();
  OLA_ASSERT_EQ(string(""), interface.name);
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.ip_address.ToString());
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.bcast_address.ToString());
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.subnet_mask.ToString());
  OLA_ASSERT_EQ(string("00:00:00:00:00:00"), interface.hw_address.ToString());

  // now check we can't use bad data
  OLA_ASSERT_FALSE(builder.SetAddress("192.168.1."));
  OLA_ASSERT_FALSE(builder.SetBroadcast("192.168.1.255.255"));
  OLA_ASSERT_FALSE(builder.SetSubnetMask("foobarbaz"));
  MACAddress addr3, addr4;
  OLA_ASSERT_FALSE(MACAddress::FromString(string("e4:ff:29:36:74:12:ac"),
                                         &addr3));
  builder.SetHardwareAddress(addr3);
  OLA_ASSERT_FALSE(MACAddress::FromString(string("e4:ff:29:36:74:hh"),
                                          &addr4));
  builder.SetHardwareAddress(addr4);

  // none of this should have changed.
  interface = builder.Construct();
  OLA_ASSERT_EQ(string(""), interface.name);
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.ip_address.ToString());
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.bcast_address.ToString());
  OLA_ASSERT_EQ(string("0.0.0.0"), interface.subnet_mask.ToString());
  OLA_ASSERT_EQ(string("00:00:00:00:00:00"), interface.hw_address.ToString());

  // now build from IPV4Address and MACAddress objects
  IPV4Address ip_address, netmask, broadcast_address;
  IPV4Address::FromString("10.0.0.1", &ip_address);
  IPV4Address::FromString("255.255.0.0", &netmask);
  IPV4Address::FromString("10.0.255.255", &broadcast_address);
  MACAddress mac_address;
  MACAddress::FromString("ba:98:76:54:32:10", &mac_address);

  builder.SetName("eth1");
  builder.SetAddress(ip_address);
  builder.SetBroadcast(broadcast_address);
  builder.SetSubnetMask(netmask);
  builder.SetHardwareAddress(mac_address);

  interface = builder.Construct();
  OLA_ASSERT_EQ(string("eth1"), interface.name);
  OLA_ASSERT_EQ(ip_address, interface.ip_address);
  OLA_ASSERT_EQ(broadcast_address, interface.bcast_address);
  OLA_ASSERT_EQ(netmask, interface.subnet_mask);
  OLA_ASSERT_EQ(mac_address, interface.hw_address);

  // test stringification
  OLA_ASSERT_EQ(string("eth1, Index: -1, IP: 10.0.0.1, Broadcast: "
                       "10.0.255.255, Subnet: 255.255.0.0, Type: 65535, MAC: "
                       "ba:98:76:54:32:10, Loopback: 0"),
                interface.ToString());
  OLA_ASSERT_EQ(string("eth1|Index: -1|IP: 10.0.0.1|Broadcast: 10.0.255.255|"
                       "Subnet: 255.255.0.0|Type: 65535|MAC: "
                       "ba:98:76:54:32:10|Loopback: 0"),
                interface.ToString("|"));
  std::ostringstream str;
  str << interface;
  OLA_ASSERT_EQ(string("eth1, Index: -1, IP: 10.0.0.1, Broadcast: "
                       "10.0.255.255, Subnet: 255.255.0.0, Type: 65535, MAC: "
                       "ba:98:76:54:32:10, Loopback: 0"),
                str.str());
}
