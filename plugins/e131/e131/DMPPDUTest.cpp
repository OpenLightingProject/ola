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
 * DMPPDUTest.cpp
 * Test fixture for the DMPPDU class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "PDUTestCommon.h"
#include "DMPPDU.h"

namespace ola {
namespace e131 {


class DMPPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DMPPDUTest);
  CPPUNIT_TEST(testSimpleDMPPDU);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSimpleDMPPDU();
  private:
    static const unsigned int TEST_VECTOR = 30;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DMPPDUTest);


/*
 * Test that packing a DMPPDU without data works.
 */
void DMPPDUTest::testSimpleDMPPDU() {
  DMPHeader header(true, false, DMPHeader::RANGE_SINGLE, DMPHeader::ONE_BYTES);
  DMPPDU pdu(TEST_VECTOR, header, NULL);

  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, pdu.HeaderSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, pdu.DataSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  CPPUNIT_ASSERT(pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  // spot check the data
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0x70, data[0]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) bytes_used, data[1]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) htonl(TEST_VECTOR), data[2]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) header.Heade(), data[3]);

  // test undersized buffer
  bytes_used = size - 1;
  CPPUNIT_ASSERT(!pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);

  // test oversized buffer
  bytes_used = size + 1;
  CPPUNIT_ASSERT(pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  delete[] data;
}


} // e131
} // ola
