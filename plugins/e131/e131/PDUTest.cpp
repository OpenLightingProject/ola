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
 * PDUTest.cpp
 * Test fixture for the PDU class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "PDU.h"
#include "PDUTestCommon.h"

namespace ola {
namespace e131 {


class PDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PDUTest);
  CPPUNIT_TEST(testPDUBlock);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testPDUBlock();
};

CPPUNIT_TEST_SUITE_REGISTRATION(PDUTest);


/*
 * Test that packing a PDUBlock works.
 */
void PDUTest::testPDUBlock() {
  FakePDU pdu1(1);
  FakePDU pdu2(2);
  FakePDU pdu42(42);
  PDUBlock<FakePDU> block;
  block.AddPDU(&pdu1);
  block.AddPDU(&pdu2);
  block.AddPDU(&pdu42);

  unsigned int block_size = block.Size();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 12, block_size);
  uint8_t *data = new uint8_t[block_size];
  unsigned int bytes_used = block_size;
  CPPUNIT_ASSERT(block.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL(block_size, bytes_used);

  unsigned int *test = (unsigned int*) data;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, *test++);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, *test++);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 42, *test);
  delete data;

  block.Clear();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, block.Size());
}


} // e131
} // ola
