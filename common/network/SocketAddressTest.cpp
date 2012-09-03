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
 * SocketAddressTest.cpp
 * Test fixture for the SocketAddress class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketAddress.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;

class SocketAddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SocketAddressTest);
  CPPUNIT_TEST(testIPV4SocketAddress);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testIPV4SocketAddress();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketAddressTest);


/*
 * Test the IPV4 SocketAddress class works
 */
void SocketAddressTest::testIPV4SocketAddress() {
  IPV4Address ip_address;
  CPPUNIT_ASSERT(IPV4Address::FromString("182.168.1.1", &ip_address));

  IPV4SocketAddress socket_address(ip_address, 8080);
  CPPUNIT_ASSERT_EQUAL(ip_address, socket_address.Host());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(8080), socket_address.Port());

  struct sockaddr sock_addr;
  CPPUNIT_ASSERT(!socket_address.ToSockAddr(&sock_addr, 0));
  CPPUNIT_ASSERT(socket_address.ToSockAddr(&sock_addr, sizeof(sock_addr)));
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(AF_INET),
                       static_cast<uint16_t>(sock_addr.sa_family));

  struct sockaddr_in *sock_addr_in =
    reinterpret_cast<struct sockaddr_in*>(&sock_addr);
  CPPUNIT_ASSERT_EQUAL(ola::network::HostToNetwork(static_cast<uint16_t>(8080)),
                       sock_addr_in->sin_port);
  IPV4Address actual_ip(sock_addr_in->sin_addr);
  CPPUNIT_ASSERT_EQUAL(ip_address, actual_ip);
}
