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
 * RootPDUTest.cpp
 * Test fixture for the RootPDU class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "CID.h"
#include "PDUTestCommon.h"
#include "RootPDU.h"

namespace ola {
namespace e131 {


class RootPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RootPDUTest);
  CPPUNIT_TEST(testSimpleRootPDU);
  CPPUNIT_TEST(testNestedRootPDU);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSimpleRootPDU();
    void testNestedRootPDU();
  private:
    static const unsigned int TEST_VECTOR;
};

const unsigned int RootPDUTest::TEST_VECTOR = 4;

CPPUNIT_TEST_SUITE_REGISTRATION(RootPDUTest);


/*
 * Test that packing a RootPDU without data works.
 */
void RootPDUTest::testSimpleRootPDU() {
  CID cid;
  cid.Generate();
  RootPDU pdu1(TEST_VECTOR, cid, NULL);

  CPPUNIT_ASSERT_EQUAL(TEST_VECTOR, pdu1.Vector());
  CPPUNIT_ASSERT(cid == pdu1.Cid());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 22, pdu1.Size());

  unsigned int size = pdu1.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  CPPUNIT_ASSERT(pdu1.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  // spot check the data
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0x70, data[0]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) bytes_used, data[1]);
  CPPUNIT_ASSERT_EQUAL((unsigned int) htonl(TEST_VECTOR),
                       *((unsigned int*) &data[2]));
  CID cid2;
  cid.FromData(&data[6]);
  CPPUNIT_ASSERT(cid2 == cid);

  // test undersized buffer
  bytes_used = size - 1;
  CPPUNIT_ASSERT(!pdu1.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);

  // test oversized buffer
  bytes_used = size + 1;
  CPPUNIT_ASSERT(pdu1.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  // use the other constructor
  RootPDU pdu2;
  pdu2.Vector(TEST_VECTOR);
  pdu2.Cid(cid);

  CPPUNIT_ASSERT_EQUAL(TEST_VECTOR, pdu1.Vector());
  CPPUNIT_ASSERT(cid == pdu1.Cid());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 22, pdu1.Size());
  bytes_used = size;
  uint8_t *data2 = new uint8_t[size];
  CPPUNIT_ASSERT(pdu1.Pack(data2, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);
  CPPUNIT_ASSERT(!memcmp(data, data2, bytes_used));

  delete data;
  delete data2;
}


/*
 * Test that packing a RootPDU with nested data works
 */
void RootPDUTest::testNestedRootPDU() {
  MockPDU pdu1(1);
  MockPDU pdu2(42);
  PDUBlock<PDU> block;
  block.AddPDU(&pdu1);
  block.AddPDU(&pdu2);

  CID cid;
  cid.Generate();
  RootPDU pdu(TEST_VECTOR, cid, &block);

  CPPUNIT_ASSERT_EQUAL(TEST_VECTOR, pdu.Vector());
  CPPUNIT_ASSERT(cid == pdu.Cid());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 30, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  CPPUNIT_ASSERT(pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  // spot check
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, *((unsigned int*) &data[22]));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 42, *((unsigned int*) &data[26]));

  delete data;
}


} // e131
} // ola
