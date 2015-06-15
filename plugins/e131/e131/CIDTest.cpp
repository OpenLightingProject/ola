/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * CIDTest.cpp
 * Test fixture for the CID class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "ola/acn/CID.h"
#include "ola/io/IOQueue.h"
#include "ola/io/OutputBuffer.h"
#include "ola/testing/TestUtils.h"

using ola::acn::CID;
using std::string;

class CIDTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CIDTest);
  CPPUNIT_TEST(testCID);
  CPPUNIT_TEST(testSetPack);
  CPPUNIT_TEST(testGenerate);
  CPPUNIT_TEST(testToString);
  CPPUNIT_TEST(testFromString);
  CPPUNIT_TEST(testToOutputBuffer);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testCID();
    void testSetPack();
    void testGenerate();
    void testToString();
    void testFromString();
    void testToOutputBuffer();
 private:
    static const uint8_t TEST_DATA[];
};


const uint8_t CIDTest::TEST_DATA[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                      12, 13, 14, 15};
CPPUNIT_TEST_SUITE_REGISTRATION(CIDTest);

/*
 * Check that basic assignment & equality works
 */
void CIDTest::testCID() {
  CID cid;
  OLA_ASSERT(cid.IsNil());

  CID cid1 = CID::Generate();
  OLA_ASSERT_FALSE(cid1.IsNil());

  CID cid2 = cid1;
  OLA_ASSERT(cid2 == cid1);
  OLA_ASSERT_FALSE(cid2.IsNil());

  CID cid3;
  cid3 = cid1;
  OLA_ASSERT(cid3 == cid1);
  OLA_ASSERT_FALSE(cid3.IsNil());
}


void CIDTest::testSetPack() {
  uint8_t buffer[CID::CID_LENGTH];
  CID cid = CID::FromData(TEST_DATA);
  cid.Pack(buffer);
  OLA_ASSERT_FALSE(memcmp(TEST_DATA, buffer, CID::CID_LENGTH));
}


/*
 * Check that generate works
 */
void CIDTest::testGenerate() {
  CID cid1 = CID::Generate();
  CID cid2 = CID::Generate();

  OLA_ASSERT_NE(cid1, cid2);
}

/*
 * Check that ToString works.
 */
void CIDTest::testToString() {
  CID cid = CID::FromData(TEST_DATA);
  string cid_str = cid.ToString();
  transform(cid_str.begin(), cid_str.end(), cid_str.begin(), toupper);
  OLA_ASSERT_EQ(string("00010203-0405-0607-0809-0A0B0C0D0E0F"),
                       cid_str);
}

/*
 * Check that from string works.
 */
void CIDTest::testFromString() {
  const string uuid = "00010203-0405-0607-0809-0A0B0C0D0E0F";
  CID cid = CID::FromString(uuid);
  string cid_str = cid.ToString();
  transform(cid_str.begin(), cid_str.end(), cid_str.begin(), toupper);
  OLA_ASSERT_EQ(uuid, cid_str);

  const string bad_uuid = "foo";
  cid = CID::FromString(bad_uuid);
  OLA_ASSERT(cid.IsNil());
}


/**
 * Check writing to an OutputBuffer works.
 */
void CIDTest::testToOutputBuffer() {
  ola::io::IOQueue output;
  const string uuid = "00010203-0405-0607-0809-0A0B0C0D0E0F";
  CID cid = CID::FromString(uuid);

  cid.Write(&output);
  OLA_ASSERT_EQ(16u, output.Size());
  unsigned int size = output.Size();
  uint8_t cid_data[size];
  OLA_ASSERT_EQ(size, output.Read(cid_data, size));

  OLA_ASSERT_DATA_EQUALS(TEST_DATA, sizeof(TEST_DATA), cid_data, size);
}
