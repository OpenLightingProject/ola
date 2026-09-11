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
 * ResponderHelperTest.cpp
 * Test fixture for the ResponderHelper class
 * Copyright (C) 2025 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>

#include "common/rdm/FakeNetworkManager.h"
#include "common/rdm/TestHelper.h"
#include "ola/base/Array.h"
#include "ola/network/Interface.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/ResponderHelper.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"

using ola::io::ByteString;
using ola::network::HostToNetwork;
using ola::network::Interface;
using ola::rdm::FakeNetworkManager;
using ola::rdm::RDMGetRequest;
using ola::rdm::ResponderHelper;
using ola::rdm::UID;
using std::string;
using std::vector;

class ResponderHelperTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ResponderHelperTest);
  CPPUNIT_TEST(testExtractString);
  CPPUNIT_TEST(testListInterfacesIsUnique);
  CPPUNIT_TEST_SUITE_END();

 public:
  ResponderHelperTest()
    : m_source(1, 2),
      m_destination(3, 4) {
  }

  void setUp();

  void testExtractString();
  void testListInterfacesIsUnique();

 private:
  UID m_source;
  UID m_destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ResponderHelperTest);


void ResponderHelperTest::setUp() {
}


void ResponderHelperTest::testExtractString() {
  // No data
  RDMGetRequest request1(m_source,
                         m_destination,
                         0,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         NULL,  // data
                         0);  // data length

  string str1;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request1, &str1));
  OLA_ASSERT_TRUE(str1.empty());

  // Normal length
  uint8_t data2[3] = {'f', 'o', 'o'};
  RDMGetRequest request2(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data2,  // data
                         arraysize(data2));  // data length

  string str2;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request2, &str2));
  OLA_ASSERT_EQ((size_t) 3, str2.length());
  OLA_ASSERT_EQ(string("foo"), str2);

  // Normal length with null termination
  uint8_t data3[4] = {'f', 'o', 'o', 0};
  RDMGetRequest request3(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data3,  // data
                         arraysize(data3));  // data length

  string str3;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request3, &str3));
  OLA_ASSERT_EQ((size_t) 3, str3.length());  // Length doesn't include null
  OLA_ASSERT_EQ(string("foo"), str3);

  // Max length, no null
  uint8_t data4[32] = {'t', 'h', 'i', 's', ' ', 'i', 's', ' ', 'a', ' ',
                      's', 't', 'r', 'i', 'n', 'g', ' ', 'w', 'i', 't', 'h',
                      ' ', '3', '2', ' ', 'c', 'h', 'a', 'r', 'a', 'c', 't'};
  RDMGetRequest request4(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data4,  // data
                         arraysize(data4));  // data length

  string str4;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request4, &str4));
  OLA_ASSERT_EQ((size_t) 32, str4.length());
  OLA_ASSERT_EQ(string("this is a string with 32 charact"), str4);

  // Max data as nulls
  uint8_t data5[32];
  memset(data5, 0, arraysize(data5));
  RDMGetRequest request5(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data5,  // data
                         arraysize(data5));  // data length

  string str5;
  OLA_ASSERT_TRUE(ResponderHelper::ExtractString(&request5, &str5));
  OLA_ASSERT_EQ((size_t) 0, str5.length());
  OLA_ASSERT_TRUE(str5.empty());

  // More than max data as nulls
  uint8_t data6[33];
  memset(data6, 0, arraysize(data6));
  RDMGetRequest request6(m_source,
                         m_destination,
                         1,  // transaction #
                         1,  // port id
                         0,  // sub device
                         130,  // param id
                         data6,  // data
                         arraysize(data6));  // data length

  string str6;
  OLA_ASSERT_FALSE(ResponderHelper::ExtractString(&request6, &str6));
  OLA_ASSERT_TRUE(str6.empty());
}


void ResponderHelperTest::testListInterfacesIsUnique() {
  Interface first;
  first.index = 3;
  first.type = Interface::ARP_ETHERNET_TYPE;
  Interface second_address = first;
  Interface second;
  second.index = 7;
  second.type = Interface::ARP_ETHERNET_TYPE;
  vector<Interface> interfaces;
  interfaces.push_back(second_address);
  interfaces.push_back(second);
  interfaces.push_back(first);

  FakeNetworkManager network_manager(
      interfaces, 3, ola::network::IPV4Address::Loopback(),
      "host", "example", vector<ola::network::IPV4Address>());
  RDMGetRequest request(m_source,
                        m_destination,
                        0,  // transaction #
                        1,  // port id
                        0,  // sub device
                        ola::rdm::PID_LIST_INTERFACES,
                        NULL,
                        0);
  std::auto_ptr<ola::rdm::RDMResponse> response(
      ResponderHelper::GetListInterfaces(&request, &network_manager));

  PACK(
  struct list_interface_s {
    uint32_t index;
    uint16_t type;
  });
  const list_interface_s expected[] = {
    {HostToNetwork(static_cast<uint32_t>(3)),
     HostToNetwork(Interface::ARP_ETHERNET_TYPE)},
    {HostToNetwork(static_cast<uint32_t>(7)),
     HostToNetwork(Interface::ARP_ETHERNET_TYPE)}
  };
  OLA_ASSERT_DATA_EQUALS(
      reinterpret_cast<const uint8_t*>(expected), sizeof(expected),
      response->ParamData(), response->ParamDataSize());
}
