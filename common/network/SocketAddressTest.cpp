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
 * SocketAddressTest.cpp
 * Test fixture for the SocketAddress class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/TestUtils.h"

using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using std::string;

class SocketAddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SocketAddressTest);
  CPPUNIT_TEST(testIPV4SocketAddress);
  CPPUNIT_TEST(testIPV4SocketAddressFromString);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testIPV4SocketAddress();
    void testIPV4SocketAddressFromString();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketAddressTest);


/*
 * Test the IPV4 SocketAddress class works
 */
void SocketAddressTest::testIPV4SocketAddress() {
  IPV4Address ip_address;
  OLA_ASSERT_TRUE(IPV4Address::FromString("192.168.1.1", &ip_address));

  IPV4SocketAddress socket_address(ip_address, 8080);
  OLA_ASSERT_EQ(ip_address, socket_address.Host());
  OLA_ASSERT_EQ(static_cast<uint16_t>(8080), socket_address.Port());

  struct sockaddr sock_addr;
  OLA_ASSERT_FALSE(socket_address.ToSockAddr(&sock_addr, 0));
  OLA_ASSERT_TRUE(socket_address.ToSockAddr(&sock_addr, sizeof(sock_addr)));
  OLA_ASSERT_EQ(static_cast<uint16_t>(AF_INET),
                static_cast<uint16_t>(sock_addr.sa_family));

  struct sockaddr_in *sock_addr_in =
    reinterpret_cast<struct sockaddr_in*>(&sock_addr);
  OLA_ASSERT_EQ(ola::network::HostToNetwork(static_cast<uint16_t>(8080)),
                sock_addr_in->sin_port);
  IPV4Address actual_ip(sock_addr_in->sin_addr);
  OLA_ASSERT_EQ(ip_address, actual_ip);

  // test comparison operators
  IPV4SocketAddress socket_address2(ip_address, 8079);
  IPV4SocketAddress socket_address3(ip_address, 8081);
  IPV4Address ip_address2;
  OLA_ASSERT_TRUE(IPV4Address::FromString("182.168.1.2", &ip_address2));
  IPV4SocketAddress socket_address4(ip_address2, 8080);

  OLA_ASSERT_EQ(socket_address, socket_address);
  OLA_ASSERT_NE(socket_address, socket_address2);
  OLA_ASSERT_NE(socket_address, socket_address3);

  OLA_ASSERT_LT(socket_address2, socket_address);
  OLA_ASSERT_LT(socket_address, socket_address3);
  OLA_ASSERT_LT(socket_address, socket_address4);
  OLA_ASSERT_LT(socket_address3, socket_address4);

  // test assignment & copy constructor
  IPV4SocketAddress copy_address(socket_address);
  socket_address4 = socket_address;
  OLA_ASSERT_EQ(socket_address, copy_address);
  OLA_ASSERT_EQ(socket_address, socket_address4);
}

/**
 * Test that FromString() works
 */
void SocketAddressTest::testIPV4SocketAddressFromString() {
  IPV4SocketAddress socket_address;
  OLA_ASSERT_TRUE(
      IPV4SocketAddress::FromString("127.0.0.1:80", &socket_address));
  OLA_ASSERT_EQ(string("127.0.0.1"), socket_address.Host().ToString());
  OLA_ASSERT_EQ(static_cast<uint16_t>(80), socket_address.Port());

  OLA_ASSERT_FALSE(
      IPV4SocketAddress::FromString("127.0.0.1", &socket_address));
  OLA_ASSERT_FALSE(IPV4SocketAddress::FromString("foo", &socket_address));
  OLA_ASSERT_FALSE(IPV4SocketAddress::FromString(":80", &socket_address));
}
