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

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include "ola/Logging.h"
#include "plugins/e131/e131/DMPAddress.h"
#include "plugins/e131/e131/DMPInflator.h"
#include "plugins/e131/e131/DMPPDU.h"
#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace plugin {
namespace e131 {

class MockDMPInflator: public DMPInflator {
  public:
    MockDMPInflator(): DMPInflator(),
                       expected_vector(0),
                       expected_virtual(false),
                       expected_relative(false),
                       expected_type(NON_RANGE),
                       expected_size(RES_BYTES) ,
                       expected_start(0),
                       expected_increment(0),
                       expected_number(0) {}
    ~MockDMPInflator() {}

    unsigned int expected_length;
    unsigned int expected_data_length;
    unsigned int expected_vector;
    bool expected_virtual;
    bool expected_relative;
    dmp_address_type expected_type;
    dmp_address_size expected_size;

    unsigned int expected_start;
    unsigned int expected_increment;
    unsigned int expected_number;

  protected:
    bool HandlePDUData(uint32_t vector, HeaderSet &headers,
                       const uint8_t *data, unsigned int pdu_len);
};

class DMPPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DMPPDUTest);
  CPPUNIT_TEST(testGetProperty);
  CPPUNIT_TEST(testSetProperty);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testGetProperty();
    void testSetProperty();

  private:
    void PackPduAndInflate(const DMPPDU *pdu);
    MockDMPInflator m_inflator;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DMPPDUTest);


/*
 * Verify a PDU is what we expected
 */
bool MockDMPInflator::HandlePDUData(uint32_t vector,
                                    HeaderSet &headers,
                                    const uint8_t *data,
                                    unsigned int pdu_len) {
  DMPHeader header = headers.GetDMPHeader();
  OLA_ASSERT_EQ(expected_vector, vector);
  OLA_ASSERT_EQ(expected_virtual, header.IsVirtual());
  OLA_ASSERT_EQ(expected_relative, header.IsRelative());
  OLA_ASSERT(expected_type == header.Type());
  OLA_ASSERT(expected_size == header.Size());

  if (vector == DMP_GET_PROPERTY_VECTOR ||
      vector == DMP_SET_PROPERTY_VECTOR) {
    unsigned int length = pdu_len;
    const BaseDMPAddress *addr = DecodeAddress(header.Size(), header.Type(),
                                               data, length);
    OLA_ASSERT(addr);
    OLA_ASSERT_EQ(expected_start, addr->Start());
    OLA_ASSERT_EQ(expected_increment, addr->Increment());
    OLA_ASSERT_EQ(expected_number, addr->Number());
    delete addr;
  }
  return true;
}


/*
 * Pack a PDU and check it inflates correctly.
 */
void DMPPDUTest::PackPduAndInflate(const DMPPDU *pdu) {
  unsigned int size = pdu->Size() + 10;  // overallocate to catch overflows
  uint8_t *data = new uint8_t[size];
  OLA_ASSERT(pdu->Pack(data, size));
  OLA_ASSERT_EQ(pdu->Size(), size);

  HeaderSet headers;
  m_inflator.InflatePDUBlock(headers, data, size);
  delete[] data;
}


/*
 * Test that GetProperty PDUs can be constructed
 */
void DMPPDUTest::testGetProperty() {
  m_inflator.expected_vector = DMP_GET_PROPERTY_VECTOR;
  m_inflator.expected_type = NON_RANGE;
  m_inflator.expected_size = ONE_BYTES;
  m_inflator.expected_virtual = false;
  m_inflator.expected_relative = true;
  m_inflator.expected_start = 10;
  m_inflator.expected_increment = 0;
  m_inflator.expected_number = 1;

  // Non-ranged GetProperty PDUs
  const DMPPDU *pdu = NewDMPGetProperty(false, true, 10);
  OLA_ASSERT(pdu);
  OLA_ASSERT_EQ((unsigned int) 5, pdu->Size());
  PackPduAndInflate(pdu);
  delete pdu;

  m_inflator.expected_start = 1024;
  pdu = NewDMPGetProperty(true, true, 1024);
  OLA_ASSERT(pdu);
  OLA_ASSERT_EQ((unsigned int) 6, pdu->Size());
  m_inflator.expected_size = TWO_BYTES;
  m_inflator.expected_virtual = true;
  PackPduAndInflate(pdu);
  delete pdu;

  // Ranged GetProperty PDUs
  m_inflator.expected_start = 10;
  m_inflator.expected_increment = 1;
  m_inflator.expected_number = 20;
  m_inflator.expected_type = RANGE_SINGLE;
  pdu = NewRangeDMPGetProperty(true, false, 10, 1, 20);
  OLA_ASSERT(pdu);
  OLA_ASSERT_EQ((unsigned int) 7, pdu->Size());
  m_inflator.expected_size = ONE_BYTES;
  m_inflator.expected_relative = false;
  PackPduAndInflate(pdu);
  delete pdu;

  m_inflator.expected_start = 10;
  m_inflator.expected_increment = 1;
  m_inflator.expected_number = 1024;
  pdu = NewRangeDMPGetProperty(false, false, 10, 1, 1024);
  OLA_ASSERT(pdu);
  OLA_ASSERT_EQ((unsigned int) 10, pdu->Size());
  m_inflator.expected_size = TWO_BYTES;
  m_inflator.expected_virtual = false;
  PackPduAndInflate(pdu);
  delete pdu;
}


/*
 * Test that packing a DMPPDU without data works.
 */
void DMPPDUTest::testSetProperty() {
  m_inflator.expected_vector = DMP_SET_PROPERTY_VECTOR;
  m_inflator.expected_type = NON_RANGE;
  m_inflator.expected_size = ONE_BYTES;
  m_inflator.expected_virtual = false;
  m_inflator.expected_relative = false;
  m_inflator.expected_start = 10;
  m_inflator.expected_increment = 0;
  m_inflator.expected_number = 1;

  // non-range first
  uint8_t data = 0xab;
  OneByteDMPAddress addr(10);
  DMPAddressData<OneByteDMPAddress> chunk(&addr, &data, sizeof(data));
  vector<DMPAddressData<OneByteDMPAddress> > chunks;
  chunks.push_back(chunk);
  const DMPPDU *pdu = NewDMPSetProperty<uint8_t>(false, false, chunks);
  OLA_ASSERT(pdu);
  OLA_ASSERT_EQ((unsigned int) 6, pdu->Size());
  PackPduAndInflate(pdu);
  delete pdu;

  // ranged address
  m_inflator.expected_type = RANGE_SINGLE;
  m_inflator.expected_increment = 1;
  m_inflator.expected_number = 20;
  OneByteRangeDMPAddress range_addr(10, 1, 20);
  DMPAddressData<OneByteRangeDMPAddress> range_chunk(&range_addr, &data,
                                                     sizeof(data));
  vector<DMPAddressData<OneByteRangeDMPAddress> > ranged_chunks;
  ranged_chunks.push_back(range_chunk);

  // range single first
  pdu = NewRangeDMPSetProperty<uint8_t>(false, false, ranged_chunks, false);
  OLA_ASSERT(pdu);
  OLA_ASSERT_EQ((unsigned int) 8, pdu->Size());
  PackPduAndInflate(pdu);
  delete pdu;

  // range equal
  m_inflator.expected_type = RANGE_EQUAL;
  pdu = NewRangeDMPSetProperty<uint8_t>(false, false, ranged_chunks);
  OLA_ASSERT(pdu);
  OLA_ASSERT_EQ((unsigned int) 8, pdu->Size());
  PackPduAndInflate(pdu);
  delete pdu;
}
}  // e131
}  // plugin
}  // ola
