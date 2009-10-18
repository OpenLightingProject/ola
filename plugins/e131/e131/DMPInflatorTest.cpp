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
 * DMPInflatorTest.cpp
 * Test fixture for the DMPInflator class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "HeaderSet.h"
#include "PDUTestCommon.h"
#include "DMPAddress.h"
#include "DMPInflator.h"
#include "DMPPDU.h"

namespace ola {
namespace e131 {


class DMPInflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DMPInflatorTest);
  CPPUNIT_TEST(testDecodeHeader);
  CPPUNIT_TEST(testInflatePDU);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testDecodeHeader();
    void testInflatePDU();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DMPInflatorTest);


/*
 * Check that we can decode headers properly
 */
void DMPInflatorTest::testDecodeHeader() {
  DMPHeader header(true, true, NON_RANGE, TWO_BYTES);
  DMPInflator inflator;
  HeaderSet header_set, header_set2;
  unsigned int bytes_used;
  uint8_t header_data = header.Header();

  CPPUNIT_ASSERT(inflator.DecodeHeader(header_set,
                                       &header_data,
                                       sizeof(header_data),
                                       bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(header_data), bytes_used);
  DMPHeader decoded_header = header_set.GetDMPHeader();
  CPPUNIT_ASSERT(decoded_header.IsVirtual());
  CPPUNIT_ASSERT(decoded_header.IsRelative());
  CPPUNIT_ASSERT(NON_RANGE == decoded_header.Type());
  CPPUNIT_ASSERT(TWO_BYTES == decoded_header.Size());

  // try an undersized header
  CPPUNIT_ASSERT(!inflator.DecodeHeader(header_set,
                                        &header_data,
                                        0,
                                        bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);

  // test inherting the header from the prev call
  CPPUNIT_ASSERT(inflator.DecodeHeader(header_set2, NULL, 0, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);
  decoded_header = header_set2.GetDMPHeader();
  CPPUNIT_ASSERT(decoded_header.IsVirtual());
  CPPUNIT_ASSERT(decoded_header.IsRelative());
  CPPUNIT_ASSERT(NON_RANGE == decoded_header.Type());
  CPPUNIT_ASSERT(TWO_BYTES == decoded_header.Size());

  inflator.ResetHeaderField();
  CPPUNIT_ASSERT(!inflator.DecodeHeader(header_set2, NULL, 0, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);
}


/*
 * Check that we can inflate a DMP PDU that contains other PDUs
 */
void DMPInflatorTest::testInflatePDU() {
  DMPHeader header(true, true, NON_RANGE, ONE_BYTES);
  const DMPPDU *pdu = NewDMPGetProperty(true, true, 1);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 5, pdu->Size());

  unsigned int size = pdu->Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  CPPUNIT_ASSERT(pdu->Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  DMPInflator inflator;
  HeaderSet header_set;
  CPPUNIT_ASSERT(inflator.InflatePDUBlock(header_set, data, size));
  CPPUNIT_ASSERT(header == header_set.GetDMPHeader());
  delete[] data;
  delete pdu;
}


} // e131
} // ola
