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
 * UniverseChannelAddressTest.cpp
 * Test fixture for the UniverseChannelAddress class
 * Copyright (C) 2023 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/dmx/UniverseChannelAddress.h"
#include "ola/testing/TestUtils.h"

using ola::dmx::UniverseChannelAddress;
using std::string;

class UniverseChannelAddressTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UniverseChannelAddressTest);
  CPPUNIT_TEST(testUniverseChannelAddress);
  CPPUNIT_TEST(testUniverseChannelAddressFromString);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testUniverseChannelAddress();
    void testUniverseChannelAddressFromString();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UniverseChannelAddressTest);


/*
 * Test the UniverseChannelAddress class works
 */
void UniverseChannelAddressTest::testUniverseChannelAddress() {
  UniverseChannelAddress universe_channel_address(10, 500);
  OLA_ASSERT_EQ(static_cast<unsigned int>(10), universe_channel_address.Universe());
  OLA_ASSERT_EQ(static_cast<uint16_t>(500), universe_channel_address.Channel());

  // TODO(Peter): Test out of range channel values

  // test comparison operators
  UniverseChannelAddress universe_channel_address2(10, 499);
  UniverseChannelAddress universe_channel_address3(10, 501);
  UniverseChannelAddress universe_channel_address4(9, 500);
  UniverseChannelAddress universe_channel_address5(11, 500);

  OLA_ASSERT_EQ(universe_channel_address, universe_channel_address);
  OLA_ASSERT_NE(universe_channel_address, universe_channel_address2);
  OLA_ASSERT_NE(universe_channel_address, universe_channel_address3);
  OLA_ASSERT_NE(universe_channel_address, universe_channel_address4);
  OLA_ASSERT_NE(universe_channel_address, universe_channel_address5);

  OLA_ASSERT_LT(universe_channel_address2, universe_channel_address);
  OLA_ASSERT_LT(universe_channel_address, universe_channel_address3);
  OLA_ASSERT_LT(universe_channel_address4, universe_channel_address);
  OLA_ASSERT_LT(universe_channel_address4, universe_channel_address3);

  OLA_ASSERT_GT(universe_channel_address, universe_channel_address2);
  OLA_ASSERT_GT(universe_channel_address3, universe_channel_address);
  OLA_ASSERT_GT(universe_channel_address, universe_channel_address4);
  OLA_ASSERT_GT(universe_channel_address3, universe_channel_address4);

  // test assignment & copy constructor
  UniverseChannelAddress copy_address(universe_channel_address);
  universe_channel_address4 = universe_channel_address;
  OLA_ASSERT_EQ(universe_channel_address, copy_address);
  OLA_ASSERT_EQ(universe_channel_address, universe_channel_address4);
}

/**
 * Test that FromString() works
 */
void UniverseChannelAddressTest::testUniverseChannelAddressFromString() {
  UniverseChannelAddress universe_channel_address;
  OLA_ASSERT_TRUE(
      UniverseChannelAddress::FromString("127:80", &universe_channel_address));
  OLA_ASSERT_EQ(static_cast<unsigned int>(127), universe_channel_address.Universe());
  OLA_ASSERT_EQ(static_cast<uint16_t>(80), universe_channel_address.Channel());

  OLA_ASSERT_FALSE(
      UniverseChannelAddress::FromString("127", &universe_channel_address));
  OLA_ASSERT_FALSE(
      UniverseChannelAddress::FromString("foo", &universe_channel_address));
  OLA_ASSERT_FALSE(
      UniverseChannelAddress::FromString(":80", &universe_channel_address));
}
