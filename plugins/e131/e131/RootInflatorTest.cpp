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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RootInflatorTest.cpp
 * Test fixture for the RootInflator class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>

#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/RootInflator.h"
#include "plugins/e131/e131/RootPDU.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace plugin {
namespace e131 {

class RootInflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RootInflatorTest);
  CPPUNIT_TEST(testInflatePDU);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testInflatePDU();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RootInflatorTest);


/*
 * Check that we can inflate a Root PDU that contains other PDUs
 */
void RootInflatorTest::testInflatePDU() {
  MockPDU pdu1(1, 2);
  MockPDU pdu2(4, 8);
  PDUBlock<PDU> block;
  block.AddPDU(&pdu1);
  block.AddPDU(&pdu2);

  CID cid = CID::Generate();
  RootPDU pdu(MockPDU::TEST_VECTOR, cid, &block);
  OLA_ASSERT_EQ((unsigned int) 50, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  MockInflator mock_inflator(cid);
  RootInflator inflator;
  inflator.AddInflator(&mock_inflator);
  HeaderSet header_set;
  OLA_ASSERT(inflator.InflatePDUBlock(header_set, data, size));
  delete[] data;
}
}  // e131
}  // plugin
}  // ola
