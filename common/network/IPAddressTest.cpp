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

#include <iostream>
#include <string>
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"

using ola::network::IPV4Address;
using std::string;

class IPAddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(IPAddressTest);
  CPPUNIT_TEST(testIPV4Address);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testIPV4Address();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IPAddressTest);


/*
 * Test the IPV4 Address class works
 */
void IPAddressTest::testIPV4Address() {
  IPV4Address zero_address;
  CPPUNIT_ASSERT_EQUAL(string("0.0.0.0"), zero_address.ToString());
  CPPUNIT_ASSERT_EQUAL(0, zero_address.Address().s_addr);

  struct in_addr in_addr1;
  CPPUNIT_ASSERT(ola::network::StringToAddress("192.168.1.1", in_addr1));
  IPV4Address address1(in_addr1);
  CPPUNIT_ASSERT_EQUAL(in_addr1.s_addr, address1.Address().s_addr);

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
}
