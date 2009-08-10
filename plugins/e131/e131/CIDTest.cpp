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
 * CIDTest.cpp
 * Test fixture for the CID class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include <iostream>
#include <cppunit/extensions/HelperMacros.h>

#include "CID.h"

using namespace ola::e131;
using std::string;

class CIDTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CIDTest);
  CPPUNIT_TEST(testCID);
  CPPUNIT_TEST(testSetPack);
  CPPUNIT_TEST(testGenerate);
  CPPUNIT_TEST(testToString);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testCID();
    void testSetPack();
    void testGenerate();
    void testToString();
  private:
    static const uint8_t TEST_DATA[];
};


const uint8_t CIDTest::TEST_DATA[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
CPPUNIT_TEST_SUITE_REGISTRATION(CIDTest);

/*
 * Check that basic assignment & equality works
 */
void CIDTest::testCID() {
  CID cid;
  CPPUNIT_ASSERT(cid.IsNil());

  CID cid1 = CID::Generate();
  CPPUNIT_ASSERT(!cid1.IsNil());

  CID cid2 = cid1;
  CPPUNIT_ASSERT(cid2 == cid1);
  CPPUNIT_ASSERT(!cid2.IsNil());

  CID cid3;
  cid3 = cid1;
  CPPUNIT_ASSERT(cid3 == cid1);
  CPPUNIT_ASSERT(!cid3.IsNil());
}


void CIDTest::testSetPack() {
  uint8_t buffer[CID::CID_LENGTH];
  CID cid = CID::FromData(TEST_DATA);
  cid.Pack(buffer);
  CPPUNIT_ASSERT(!memcmp(TEST_DATA, buffer, CID::CID_LENGTH));
}


/*
 * Check that generate works
 */
void CIDTest::testGenerate() {
  CID cid1 = CID::Generate();
  CID cid2 = CID::Generate();

  CPPUNIT_ASSERT(cid1 != cid2);
}

/*
 * Check that ToString works.
 */
void CIDTest::testToString() {
  CID cid = CID::FromData(TEST_DATA);
  string cid_str = cid.ToString();
  CPPUNIT_ASSERT_EQUAL(string("00010203-0405-0607-0809-0A0B0C0D0E0F"),
                       cid_str);
}

