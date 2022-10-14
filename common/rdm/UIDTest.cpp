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
 * UIDTest.cpp
 * Test fixture for the UID classes
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/testing/TestUtils.h"


using std::string;
using ola::rdm::UID;
using ola::rdm::UIDSet;

class UIDTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UIDTest);
  CPPUNIT_TEST(testUID);
  CPPUNIT_TEST(testUIDInequalities);
  CPPUNIT_TEST(testUIDSet);
  CPPUNIT_TEST(testUIDSetUnion);
  CPPUNIT_TEST(testUIDParse);
  CPPUNIT_TEST(testDirectedToUID);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testUID();
    void testUIDInequalities();
    void testUIDSet();
    void testUIDSetUnion();
    void testUIDParse();
    void testDirectedToUID();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UIDTest);


/*
 * Test the UIDs work.
 */
void UIDTest::testUID() {
  UID uid(1, 2);
  UID uid2 = uid;
  OLA_ASSERT_EQ(uid, uid2);
  OLA_ASSERT_FALSE(uid != uid2);
  OLA_ASSERT_EQ((uint16_t) 1, uid.ManufacturerId());
  OLA_ASSERT_EQ((uint32_t) 2, uid.DeviceId());

  UID uid3(2, 10);
  OLA_ASSERT_NE(uid, uid3);
  OLA_ASSERT_LT(uid, uid3);
  OLA_ASSERT_EQ((uint16_t) 2, uid3.ManufacturerId());
  OLA_ASSERT_EQ((uint32_t) 10, uid3.DeviceId());

  UID uid4(0x0000000400000002ULL);
  OLA_ASSERT_EQ((uint16_t) 4, uid4.ManufacturerId());
  OLA_ASSERT_EQ((uint32_t) 2, uid4.DeviceId());

  // ToUInt64
  OLA_ASSERT_EQ((uint64_t) 0x000100000002ULL, uid.ToUInt64());
  OLA_ASSERT_EQ((uint64_t) 0x00020000000aULL, uid3.ToUInt64());

  // ToString
  OLA_ASSERT_EQ(string("0001:00000002"), uid.ToString());
  OLA_ASSERT_EQ(string("0002:0000000a"), uid3.ToString());

  UID all_devices = UID::AllDevices();
  UID manufacturer_devices = UID::VendorcastAddress(0x52);
  OLA_ASSERT_EQ(string("ffff:ffffffff"), all_devices.ToString());
  OLA_ASSERT_EQ(string("0052:ffffffff"),
                manufacturer_devices.ToString());
  OLA_ASSERT_EQ(all_devices.ManufacturerId(),
                static_cast<uint16_t>(0xffff));
  OLA_ASSERT_EQ(all_devices.DeviceId(),
                static_cast<uint32_t>(0xffffffff));
  OLA_ASSERT_EQ(manufacturer_devices.ManufacturerId(),
                static_cast<uint16_t>(0x0052));
  OLA_ASSERT_EQ(manufacturer_devices.DeviceId(),
                static_cast<uint32_t>(0xffffffff));
  OLA_ASSERT_TRUE(all_devices.IsBroadcast());
  OLA_ASSERT_TRUE(manufacturer_devices.IsBroadcast());

  // now test the packing & unpacking
  unsigned int buffer_size = UID::UID_SIZE;
  uint8_t *buffer = new uint8_t[buffer_size];
  OLA_ASSERT_TRUE(uid.Pack(buffer, buffer_size));

  uint8_t expected[] = {0, 1, 0, 0, 0, 2};
  OLA_ASSERT_EQ(0, memcmp(expected, buffer, buffer_size));
  UID unpacked_uid1(buffer);
  OLA_ASSERT_EQ(uid, unpacked_uid1);

  OLA_ASSERT_TRUE(uid3.Pack(buffer, buffer_size));
  uint8_t expected2[] = {0, 2, 0, 0, 0, 0x0a};
  OLA_ASSERT_EQ(0, memcmp(expected2, buffer, buffer_size));
  UID unpacked_uid2(buffer);
  OLA_ASSERT_EQ(uid3, unpacked_uid2);

  delete[] buffer;
}


/*
 * Test the UIDs inequalities work
 */
void UIDTest::testUIDInequalities() {
  uint16_t MOCK_ESTA_ID = 0x7a70;

  // check comparisons on the device id
  UID uid1(MOCK_ESTA_ID, 0);
  UID uid2(MOCK_ESTA_ID, 1);
  UID uid3(MOCK_ESTA_ID, 2);

  OLA_ASSERT_TRUE(uid1 < uid2);
  OLA_ASSERT_TRUE(uid1 < uid3);
  OLA_ASSERT_TRUE(uid2 < uid3);
  OLA_ASSERT_TRUE(uid3 > uid1);
  OLA_ASSERT_TRUE(uid2 > uid1);
  OLA_ASSERT_TRUE(uid3 > uid2);

  // check we're using unsigned ints for the device id
  UID uid4(MOCK_ESTA_ID, 0x80000000);
  UID uid5(MOCK_ESTA_ID, 0xffffffff);

  OLA_ASSERT_LT(uid1, uid4);
  OLA_ASSERT_LT(uid2, uid4);
  OLA_ASSERT_LT(uid3, uid4);
  OLA_ASSERT_LT(uid1, uid5);
  OLA_ASSERT_LT(uid2, uid5);
  OLA_ASSERT_LT(uid3, uid5);
  OLA_ASSERT_LT(uid4, uid5);
  OLA_ASSERT_GT(uid4, uid1);
  OLA_ASSERT_GT(uid4, uid2);
  OLA_ASSERT_GT(uid4, uid3);
  OLA_ASSERT_GT(uid5, uid1);
  OLA_ASSERT_GT(uid5, uid2);
  OLA_ASSERT_GT(uid5, uid3);
  OLA_ASSERT_GT(uid5, uid4);

  // test the manufacturer ID
  UID uid6(MOCK_ESTA_ID - 1, 0xffffffff);
  OLA_ASSERT_LT(uid6, uid1);
  OLA_ASSERT_LT(uid6, uid4);
  OLA_ASSERT_LT(uid6, uid5);
  OLA_ASSERT_GT(uid1, uid6);
  OLA_ASSERT_GT(uid4, uid6);
  OLA_ASSERT_GT(uid5, uid6);

  UID uid7(MOCK_ESTA_ID + 1, 0);
  OLA_ASSERT_LT(uid1, uid7);
  OLA_ASSERT_LT(uid4, uid7);
  OLA_ASSERT_LT(uid5, uid7);
  OLA_ASSERT_LT(uid6, uid7);
  OLA_ASSERT_GT(uid7, uid1);
  OLA_ASSERT_GT(uid7, uid4);
  OLA_ASSERT_GT(uid7, uid5);
  OLA_ASSERT_GT(uid7, uid6);

  // now some tests that would expose problems if we used signed ints
  UID uid8(0x8000, 0);

  OLA_ASSERT_LT(uid1, uid8);
  OLA_ASSERT_LT(uid2, uid8);
  OLA_ASSERT_LT(uid3, uid8);
  OLA_ASSERT_LT(uid4, uid8);
  OLA_ASSERT_LT(uid5, uid8);
  OLA_ASSERT_LT(uid6, uid8);

  OLA_ASSERT_GT(uid8, uid1);
  OLA_ASSERT_GT(uid8, uid4);
  OLA_ASSERT_GT(uid8, uid5);
  OLA_ASSERT_GT(uid8, uid6);
  OLA_ASSERT_GT(uid8, uid7);
}


/*
 * Test the UIDSet
 */
void UIDTest::testUIDSet() {
  UIDSet set1;
  OLA_ASSERT_EQ(0u, set1.Size());

  UID uid(1, 2);
  UID uid2(2, 10);
  set1.AddUID(uid);
  OLA_ASSERT_EQ(1u, set1.Size());
  OLA_ASSERT_EQ(string("0001:00000002"), set1.ToString());
  OLA_ASSERT_TRUE(set1.Contains(uid));
  OLA_ASSERT_FALSE(set1.Contains(uid2));
  set1.AddUID(uid);
  OLA_ASSERT_EQ(1u, set1.Size());

  set1.AddUID(uid2);
  OLA_ASSERT_EQ(2u, set1.Size());
  OLA_ASSERT_EQ(string("0001:00000002,0002:0000000a"), set1.ToString());
  OLA_ASSERT_TRUE(set1.Contains(uid));
  OLA_ASSERT_TRUE(set1.Contains(uid2));

  UIDSet set2(set1);
  OLA_ASSERT_EQ(set1, set2);
  UIDSet set3;
  OLA_ASSERT_EQ(0u, set3.Size());
  set3 = set2;
  OLA_ASSERT_EQ(set1, set2);

  set3.RemoveUID(uid2);
  OLA_ASSERT_EQ(1u, set3.Size());
  OLA_ASSERT_EQ(string("0001:00000002"), set3.ToString());

  UIDSet difference = set1.SetDifference(set3);
  OLA_ASSERT_EQ(1u, difference.Size());
  OLA_ASSERT_TRUE(set1.Contains(uid));
  OLA_ASSERT_TRUE(set1.Contains(uid2));

  difference = set3.SetDifference(set1);
  OLA_ASSERT_EQ(0u, difference.Size());
}


/*
 * Test the UIDSet Union method.
 */
void UIDTest::testUIDSetUnion() {
  UIDSet set1, set2, expected;

  UID uid(1, 2);
  UID uid2(2, 10);
  UID uid3(3, 10);
  UID uid4(4, 10);
  set1.AddUID(uid);
  set2.AddUID(uid2);
  set2.AddUID(uid3);
  set2.AddUID(uid4);
  UIDSet union_set = set1.Union(set2);
  OLA_ASSERT_EQ(4u, union_set.Size());
  OLA_ASSERT_TRUE(union_set.Contains(uid));
  OLA_ASSERT_TRUE(union_set.Contains(uid2));
  OLA_ASSERT_TRUE(union_set.Contains(uid3));
  OLA_ASSERT_TRUE(union_set.Contains(uid4));
}


/*
 * Test UID parsing
 */
void UIDTest::testUIDParse() {
  UID *uid = UID::FromString("ffff:00000000");
  OLA_ASSERT_NOT_NULL(uid);
  OLA_ASSERT_EQ(uid->ManufacturerId(), static_cast<uint16_t>(0xffff));
  OLA_ASSERT_EQ(uid->DeviceId(), static_cast<uint32_t>(0x00));
  OLA_ASSERT_EQ(uid->ToString(), string("ffff:00000000"));
  delete uid;

  uid = UID::FromString("1234:567890ab");
  OLA_ASSERT_NOT_NULL(uid);
  OLA_ASSERT_EQ(uid->ManufacturerId(), static_cast<uint16_t>(0x1234));
  OLA_ASSERT_EQ(uid->DeviceId(), static_cast<uint32_t>(0x567890ab));
  OLA_ASSERT_EQ(uid->ToString(), string("1234:567890ab"));
  delete uid;

  uid = UID::FromString("abcd:ef123456");
  OLA_ASSERT_NOT_NULL(uid);
  OLA_ASSERT_EQ(uid->ManufacturerId(), static_cast<uint16_t>(0xabcd));
  OLA_ASSERT_EQ(uid->DeviceId(), static_cast<uint32_t>(0xef123456));
  OLA_ASSERT_EQ(uid->ToString(), string("abcd:ef123456"));
  delete uid;

  OLA_ASSERT_FALSE(UID::FromString(""));
  OLA_ASSERT_FALSE(UID::FromString(":"));
  OLA_ASSERT_FALSE(UID::FromString("0:0"));
  OLA_ASSERT_FALSE(UID::FromString(":123456"));
  OLA_ASSERT_FALSE(UID::FromString(":123456"));
  OLA_ASSERT_FALSE(UID::FromString("abcd:123456"));
}


/**
 * Test DirectedToUID()
 */
void UIDTest::testDirectedToUID() {
  const uint16_t MANUFACTURER_ID = 0x7a70;
  UID device_uid(MANUFACTURER_ID, 10);

  // test a direct match
  OLA_ASSERT_TRUE(device_uid.DirectedToUID(device_uid));

  // test a different device
  UID other_device(MANUFACTURER_ID, 9);
  OLA_ASSERT_FALSE(other_device.DirectedToUID(device_uid));

  // test broadcast
  UID broadcast_uid = UID::AllDevices();
  OLA_ASSERT_TRUE(broadcast_uid.DirectedToUID(device_uid));

  // test vendorcast passing manufacturer ID
  UID vendorcast_uid = UID::VendorcastAddress(MANUFACTURER_ID);
  OLA_ASSERT_TRUE(vendorcast_uid.DirectedToUID(device_uid));

  // test vendorcast passing UID
  UID other_device_uid(MANUFACTURER_ID, 11);
  UID vendorcast_uid_2 = UID::VendorcastAddress(other_device_uid);
  OLA_ASSERT_TRUE(vendorcast_uid_2.DirectedToUID(device_uid));

  // test another vendor passing manufacturer ID
  UID other_vendorcast_uid = UID::VendorcastAddress(MANUFACTURER_ID - 1);
  OLA_ASSERT_FALSE(other_vendorcast_uid.DirectedToUID(device_uid));

  // test another vendor passing UID
  UID other_manufacturer_uid(MANUFACTURER_ID - 1, 10);
  UID other_vendorcast_uid_2 = UID::VendorcastAddress(other_manufacturer_uid);
  OLA_ASSERT_FALSE(other_vendorcast_uid_2.DirectedToUID(device_uid));
}
