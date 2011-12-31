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
 * InterfaceTest.cpp
 * Test fixture for the Interface & InterfaceBuilder classes.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Logging.h"
#include "ola/network/Interface.h"
#include "ola/network/NetworkUtils.h"

using ola::network::HardwareAddressToString;
using ola::network::IPV4Address;
using ola::network::Interface;
using ola::network::InterfaceBuilder;
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
  CPPUNIT_ASSERT_EQUAL(string(""), interface.name);
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.ip_address.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.bcast_address.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.subnet_mask.ToString());
  CPPUNIT_ASSERT_EQUAL(string("00:00:00:00:00:00"),
                       HardwareAddressToString(interface.hw_address));

  // set & build an interface from strings
  builder.SetName("eth0");
  CPPUNIT_ASSERT(builder.SetAddress("192.168.1.1"));
  CPPUNIT_ASSERT(builder.SetBroadcast("192.168.1.255"));
  CPPUNIT_ASSERT(builder.SetSubnetMask("255.255.255.0"));
  CPPUNIT_ASSERT(builder.SetHardwareAddress("e4:ff:29:36:74:12"));

  interface = builder.Construct();
  CPPUNIT_ASSERT_EQUAL(string("eth0"), interface.name);
  CPPUNIT_ASSERT_EQUAL(string("192.168.1.1"), interface.ip_address.ToString());
  CPPUNIT_ASSERT_EQUAL(string("192.168.1.255"),
                       interface.bcast_address.ToString());
  CPPUNIT_ASSERT_EQUAL(string("255.255.255.0"),
                       interface.subnet_mask.ToString());
  CPPUNIT_ASSERT_EQUAL(string("e4:ff:29:36:74:12"),
                       HardwareAddressToString(interface.hw_address));

  // check the alternate form of mac address
  CPPUNIT_ASSERT(builder.SetHardwareAddress("12.34.56.78.90.ab"));
  interface = builder.Construct();
  CPPUNIT_ASSERT_EQUAL(string("12:34:56:78:90:ab"),
                       HardwareAddressToString(interface.hw_address));

  // reset the builder
  builder.Reset();
  interface = builder.Construct();
  CPPUNIT_ASSERT_EQUAL(string(""), interface.name);
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.ip_address.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.bcast_address.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.subnet_mask.ToString());
  CPPUNIT_ASSERT_EQUAL(string("00:00:00:00:00:00"),
                       HardwareAddressToString(interface.hw_address));

  // now check we can't use bad data
  CPPUNIT_ASSERT(!builder.SetAddress("192.168.1."));
  CPPUNIT_ASSERT(!builder.SetBroadcast("192.168.1.255.255"));
  CPPUNIT_ASSERT(!builder.SetSubnetMask("foobarbaz"));
  CPPUNIT_ASSERT(!builder.SetHardwareAddress("e4:ff:29:36:74:12:ac"));
  CPPUNIT_ASSERT(!builder.SetHardwareAddress("e4:ff:29:36:74:hh"));

  // none of this should have changed.
  interface = builder.Construct();
  CPPUNIT_ASSERT_EQUAL(string(""), interface.name);
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.ip_address.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.bcast_address.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), interface.subnet_mask.ToString());
  CPPUNIT_ASSERT_EQUAL(string("00:00:00:00:00:00"),
                       HardwareAddressToString(interface.hw_address));

  // now build from IPV4Address objects
  IPV4Address ip_address, netmask, broadcast_address;
  IPV4Address::FromString("10.0.0.1", &ip_address);
  IPV4Address::FromString("10.0.255.255", &netmask);
  IPV4Address::FromString("10.0.255.255", &broadcast_address);

  builder.SetName("eth1");
  builder.SetAddress(ip_address);
  builder.SetBroadcast(broadcast_address);
  builder.SetSubnetMask(netmask);

  interface = builder.Construct();
  CPPUNIT_ASSERT_EQUAL(string("eth1"), interface.name);
  CPPUNIT_ASSERT_EQUAL(ip_address, interface.ip_address);
  CPPUNIT_ASSERT_EQUAL(broadcast_address, interface.bcast_address);
  CPPUNIT_ASSERT_EQUAL(netmask, interface.subnet_mask);
}
