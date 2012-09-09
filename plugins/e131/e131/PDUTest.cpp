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

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/OutputStream.h"
#include "ola/testing/TestUtils.h"
#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/PDUTestCommon.h"

namespace ola {
namespace plugin {
namespace e131 {


using ola::io::IOQueue;
using ola::testing::ASSERT_DATA_EQUALS;

class PDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PDUTest);
  CPPUNIT_TEST(testPDUBlock);
  CPPUNIT_TEST(testBlockToOutputStream);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testPDUBlock();
    void testBlockToOutputStream();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }
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
  CPPUNIT_ASSERT_EQUAL(12u, block_size);
  uint8_t *data = new uint8_t[block_size];
  unsigned int bytes_used = block_size;
  CPPUNIT_ASSERT(block.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL(block_size, bytes_used);

  unsigned int *test = (unsigned int*) data;
  CPPUNIT_ASSERT_EQUAL(1u, *test++);
  CPPUNIT_ASSERT_EQUAL(2u, *test++);
  CPPUNIT_ASSERT_EQUAL(42u, *test);
  delete[] data;

  block.Clear();
  CPPUNIT_ASSERT_EQUAL(0u, block.Size());
}


/*
 * Test that writing to an OutputStream works.
 */
void PDUTest::testBlockToOutputStream() {
  FakePDU pdu1(1);
  FakePDU pdu2(2);
  FakePDU pdu42(42);
  PDUBlock<FakePDU> block;
  block.AddPDU(&pdu1);
  block.AddPDU(&pdu2);
  block.AddPDU(&pdu42);

  IOQueue output;
  OutputStream stream(&output);
  block.Write(&stream);
  CPPUNIT_ASSERT_EQUAL(12u, output.Size());

  uint8_t *block_data = new uint8_t[output.Size()];
  unsigned int block_size = output.Peek(block_data, output.Size());
  CPPUNIT_ASSERT_EQUAL(output.Size(), block_size);

  uint8_t EXPECTED[] = {
    0, 0, 0, 1,
    0, 0, 0, 2,
    0, 0, 0, 42
  };
  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED, sizeof(EXPECTED),
                     block_data, block_size);
  output.Pop(output.Size());
  delete[] block_data;
}
}  // e131
}  // plugin
}  // ola
