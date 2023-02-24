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
 * DMPInflatorTest.cpp
 * Test fixture for the DMPInflator class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "libs/acn/DMPAddress.h"
#include "libs/acn/DMPInflator.h"
#include "libs/acn/DMPPDU.h"
#include "libs/acn/HeaderSet.h"
#include "libs/acn/PDUTestCommon.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {


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

  OLA_ASSERT(inflator.DecodeHeader(&header_set, &header_data,
                                   sizeof(header_data), &bytes_used));
  OLA_ASSERT_EQ((unsigned int) sizeof(header_data), bytes_used);
  DMPHeader decoded_header = header_set.GetDMPHeader();
  OLA_ASSERT(decoded_header.IsVirtual());
  OLA_ASSERT(decoded_header.IsRelative());
  OLA_ASSERT(NON_RANGE == decoded_header.Type());
  OLA_ASSERT(TWO_BYTES == decoded_header.Size());

  // try an undersized header
  OLA_ASSERT_FALSE(inflator.DecodeHeader(&header_set, &header_data, 0,
                                         &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);

  // test inheriting the header from the prev call
  OLA_ASSERT(inflator.DecodeHeader(&header_set2, NULL, 0, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  decoded_header = header_set2.GetDMPHeader();
  OLA_ASSERT(decoded_header.IsVirtual());
  OLA_ASSERT(decoded_header.IsRelative());
  OLA_ASSERT(NON_RANGE == decoded_header.Type());
  OLA_ASSERT(TWO_BYTES == decoded_header.Size());

  inflator.ResetHeaderField();
  OLA_ASSERT_FALSE(inflator.DecodeHeader(&header_set2, NULL, 0, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
}


/*
 * Check that we can inflate a DMP PDU that contains other PDUs
 */
void DMPInflatorTest::testInflatePDU() {
  DMPHeader header(true, true, NON_RANGE, ONE_BYTES);
  const DMPPDU *pdu = NewDMPGetProperty(true, true, 1);
  OLA_ASSERT_EQ((unsigned int) 5, pdu->Size());

  unsigned int size = pdu->Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu->Pack(data, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  DMPInflator inflator;
  HeaderSet header_set;
  OLA_ASSERT(inflator.InflatePDUBlock(&header_set, data, size));
  OLA_ASSERT(header == header_set.GetDMPHeader());
  delete[] data;
  delete pdu;
}
}  // namespace acn
}  // namespace ola
