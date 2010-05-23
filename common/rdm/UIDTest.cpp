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
#include <string>

#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"

using std::string;
using ola::rdm::UID;
using ola::rdm::UIDSet;

class UIDTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UIDTest);
  CPPUNIT_TEST(testUID);
  CPPUNIT_TEST(testUIDSet);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testUID();
    void testUIDSet();
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

  UID uid3(2, 10);
  CPPUNIT_ASSERT(uid != uid3);
  CPPUNIT_ASSERT(uid < uid3);

  // ToString
  CPPUNIT_ASSERT_EQUAL(string("0001:00000002"), uid.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0002:0000000a"), uid3.ToString());

  UID all_devices = UID::AllDevices();
  UID manufacturer_devices = UID::AllManufactureDevices(0x52);
  CPPUNIT_ASSERT_EQUAL(string("ffff:ffffffff"), all_devices.ToString());
  CPPUNIT_ASSERT_EQUAL(string("0052:ffffffff"),
                       manufacturer_devices.ToString());
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
