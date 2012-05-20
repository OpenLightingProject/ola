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
 * UIDTest.cpp
 * Test fixture for the UID classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/StringUtils.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"

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
  CPPUNIT_ASSERT_EQUAL(uid, uid2);
  CPPUNIT_ASSERT(!(uid != uid2));
  CPPUNIT_ASSERT_EQUAL((uint16_t) 1, uid.ManufacturerId());
  CPPUNIT_ASSERT_EQUAL((uint32_t) 2, uid.DeviceId());

  UID uid3(2, 10);
  CPPUNIT_ASSERT(uid != uid3);
  CPPUNIT_ASSERT(uid < uid3);
  CPPUNIT_ASSERT_EQUAL((uint16_t) 2, uid3.ManufacturerId());
  CPPUNIT_ASSERT_EQUAL((uint32_t) 10, uid3.DeviceId());

  // ToString
  CPPUNIT_ASSERT_EQUAL(string("0001:00000002"), uid.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0002:0000000a"), uid3.ToString());

  UID all_devices = UID::AllDevices();
  UID manufacturer_devices = UID::AllManufactureDevices(0x52);
  CPPUNIT_ASSERT_EQUAL(string("ffff:ffffffff"), all_devices.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0052:ffffffff"),
                       manufacturer_devices.ToString());
  CPPUNIT_ASSERT_EQUAL(all_devices.ManufacturerId(),
                       static_cast<uint16_t>(0xffff));
  CPPUNIT_ASSERT_EQUAL(all_devices.DeviceId(),
                       static_cast<uint32_t>(0xffffffff));
  CPPUNIT_ASSERT_EQUAL(manufacturer_devices.ManufacturerId(),
                       static_cast<uint16_t>(0x0052));
  CPPUNIT_ASSERT_EQUAL(manufacturer_devices.DeviceId(),
                       static_cast<uint32_t>(0xffffffff));
  CPPUNIT_ASSERT(all_devices.IsBroadcast());
  CPPUNIT_ASSERT(manufacturer_devices.IsBroadcast());

  // now test the packing & unpacking
  unsigned int buffer_size = UID::UID_SIZE;
  uint8_t *buffer = new uint8_t[buffer_size];
  CPPUNIT_ASSERT(uid.Pack(buffer, buffer_size));

  uint8_t expected[] = {0, 1, 0, 0, 0, 2};
  CPPUNIT_ASSERT(0 == memcmp(expected, buffer, buffer_size));
  UID unpacked_uid1(buffer);
  CPPUNIT_ASSERT_EQUAL(uid, unpacked_uid1);

  CPPUNIT_ASSERT(uid3.Pack(buffer, buffer_size));
  uint8_t expected2[] = {0, 2, 0, 0, 0, 0x0a};
  CPPUNIT_ASSERT(0 == memcmp(expected2, buffer, buffer_size));
  UID unpacked_uid2(buffer);
  CPPUNIT_ASSERT_EQUAL(uid3, unpacked_uid2);

  delete[] buffer;
}


/*
 * Test the UIDs inequalities work
 */
void UIDTest::testUIDInequalities() {
  uint16_t MOCK_ESTA_ID = 0x7a70;

  UID uid1(MOCK_ESTA_ID, 0);
  UID uid2(MOCK_ESTA_ID, 1);
  UID uid3(MOCK_ESTA_ID, 2);
  UID uid4(MOCK_ESTA_ID, 0xffffffff);
  UID uid5(MOCK_ESTA_ID - 1, 0xffffffff);

  CPPUNIT_ASSERT(uid1 < uid2);
  CPPUNIT_ASSERT(uid1 < uid3);
  CPPUNIT_ASSERT(uid2 < uid3);
  CPPUNIT_ASSERT(uid1 < uid4);
  CPPUNIT_ASSERT(uid2 < uid4);
  CPPUNIT_ASSERT(uid3 < uid4);

  CPPUNIT_ASSERT(uid5 < uid1);
  CPPUNIT_ASSERT(uid5 < uid2);
  CPPUNIT_ASSERT(uid5 < uid3);
  CPPUNIT_ASSERT(uid5 < uid4);
}


/*
 * Test the UIDSet
 */
void UIDTest::testUIDSet() {
  UIDSet set1;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, set1.Size());

  UID uid(1, 2);
  UID uid2(2, 10);
  set1.AddUID(uid);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, set1.Size());
  CPPUNIT_ASSERT_EQUAL(string("0001:00000002"), set1.ToString());
  CPPUNIT_ASSERT(set1.Contains(uid));
  CPPUNIT_ASSERT(!set1.Contains(uid2));
  set1.AddUID(uid);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, set1.Size());

  set1.AddUID(uid2);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, set1.Size());
  CPPUNIT_ASSERT_EQUAL(string("0001:00000002,0002:0000000a"), set1.ToString());
  CPPUNIT_ASSERT(set1.Contains(uid));
  CPPUNIT_ASSERT(set1.Contains(uid2));

  UIDSet set2(set1);
  CPPUNIT_ASSERT_EQUAL(set1, set2);
  UIDSet set3;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, set3.Size());
  set3 = set2;
  CPPUNIT_ASSERT_EQUAL(set1, set2);

  set3.RemoveUID(uid2);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, set3.Size());
  CPPUNIT_ASSERT_EQUAL(string("0001:00000002"), set3.ToString());

  UIDSet difference = set1.SetDifference(set3);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, difference.Size());
  CPPUNIT_ASSERT(set1.Contains(uid));
  CPPUNIT_ASSERT(set1.Contains(uid2));

  difference = set3.SetDifference(set1);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, difference.Size());
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
  set1.Union(set2);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4, set1.Size());
  CPPUNIT_ASSERT(set1.Contains(uid));
  CPPUNIT_ASSERT(set1.Contains(uid2));
  CPPUNIT_ASSERT(set1.Contains(uid3));
  CPPUNIT_ASSERT(set1.Contains(uid4));
}


/*
 * Test UID parsing
 */
void UIDTest::testUIDParse() {
  UID *uid = UID::FromString("ffff:00000000");
  CPPUNIT_ASSERT(uid);
  CPPUNIT_ASSERT_EQUAL(uid->ManufacturerId(), static_cast<uint16_t>(0xffff));
  CPPUNIT_ASSERT_EQUAL(uid->DeviceId(), static_cast<uint32_t>(0x00));
  CPPUNIT_ASSERT_EQUAL(uid->ToString(), string("ffff:00000000"));
  delete uid;

  uid = UID::FromString("1234:567890ab");
  CPPUNIT_ASSERT(uid);
  CPPUNIT_ASSERT_EQUAL(uid->ManufacturerId(), static_cast<uint16_t>(0x1234));
  CPPUNIT_ASSERT_EQUAL(uid->DeviceId(), static_cast<uint32_t>(0x567890ab));
  CPPUNIT_ASSERT_EQUAL(uid->ToString(), string("1234:567890ab"));
  delete uid;

  uid = UID::FromString("abcd:ef123456");
  CPPUNIT_ASSERT(uid);
  CPPUNIT_ASSERT_EQUAL(uid->ManufacturerId(), static_cast<uint16_t>(0xabcd));
  CPPUNIT_ASSERT_EQUAL(uid->DeviceId(), static_cast<uint32_t>(0xef123456));
  CPPUNIT_ASSERT_EQUAL(uid->ToString(), string("abcd:ef123456"));
  delete uid;

  CPPUNIT_ASSERT(!UID::FromString(""));
  CPPUNIT_ASSERT(!UID::FromString(":"));
  CPPUNIT_ASSERT(!UID::FromString("0:0"));
  CPPUNIT_ASSERT(!UID::FromString(":123456"));
  CPPUNIT_ASSERT(!UID::FromString(":123456"));
  CPPUNIT_ASSERT(!UID::FromString("abcd:123456"));
}


/**
 * Test DirectedToUID()
 */
void UIDTest::testDirectedToUID() {
  const uint16_t MANUFACTURER_ID = 0x7a70;
  UID device_uid(MANUFACTURER_ID, 10);

  // test a direct match
  CPPUNIT_ASSERT(device_uid.DirectedToUID(device_uid));

  // test a different device
  UID other_device(MANUFACTURER_ID, 9);
  CPPUNIT_ASSERT(!other_device.DirectedToUID(device_uid));

  // test broadcast
  UID broadcast_uid = UID::AllDevices();
  CPPUNIT_ASSERT(broadcast_uid.DirectedToUID(device_uid));

  // test vendorcast
  UID vendorcast_uid = UID::AllManufactureDevices(MANUFACTURER_ID);
  CPPUNIT_ASSERT(vendorcast_uid.DirectedToUID(device_uid));

  // test another vendor
  UID other_vendorcast_uid = UID::AllManufactureDevices(MANUFACTURER_ID - 1);
  CPPUNIT_ASSERT(!other_vendorcast_uid.DirectedToUID(device_uid));
}
