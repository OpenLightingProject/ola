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
 * RPTInflatorTest.cpp
 * Test fixture for the RPTInflator class
 * Copyright (C) 2024 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "libs/acn/HeaderSet.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/RPTInflator.h"
#include "libs/acn/RPTPDU.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {

using ola::network::HostToNetwork;
using ola::rdm::UID;

class RPTInflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RPTInflatorTest);
  CPPUNIT_TEST(testDecodeHeader);
  CPPUNIT_TEST(testInflatePDU);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testDecodeHeader();
  void testInflatePDU();
 private:
  static const uint8_t TEST_DATA[];
  static const uint8_t TEST_DATA2[];
};

const uint8_t RPTInflatorTest::TEST_DATA[] = {0, 1, 2, 3, 4, 5};
const uint8_t RPTInflatorTest::TEST_DATA2[] = {10, 11, 12, 13, 14, 15};
CPPUNIT_TEST_SUITE_REGISTRATION(RPTInflatorTest);


/*
 * Check that we can decode headers properly
 */
void RPTInflatorTest::testDecodeHeader() {
  RPTHeader::rpt_pdu_header header;
  memset(&header, 0, sizeof(header));
  RPTInflator inflator;
  HeaderSet header_set, header_set2;
  unsigned int bytes_used;
  const ola::rdm::UID source_uid = UID(TEST_DATA);
  const ola::rdm::UID destination_uid = UID(TEST_DATA2);

  source_uid.Pack(header.source_uid, sizeof(header.source_uid));
  header.source_endpoint = HostToNetwork((uint16_t) 1234u);
  destination_uid.Pack(header.destination_uid, sizeof(header.destination_uid));
  header.destination_endpoint = HostToNetwork((uint16_t) 5678u);
  header.sequence = HostToNetwork(72650u);

  OLA_ASSERT(inflator.DecodeHeader(&header_set,
                                   reinterpret_cast<uint8_t*>(&header),
                                   sizeof(header),
                                   &bytes_used));
  OLA_ASSERT_EQ((unsigned int) sizeof(header), bytes_used);
  RPTHeader decoded_header = header_set.GetRPTHeader();

  // try an undersized header
  OLA_ASSERT_FALSE(inflator.DecodeHeader(
      &header_set,
      reinterpret_cast<uint8_t*>(&header),
      static_cast<unsigned int>(sizeof(header) - 1),
      &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);

  // test inheriting the header from the prev call
  OLA_ASSERT(inflator.DecodeHeader(&header_set2, NULL, 0, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
  decoded_header = header_set2.GetRPTHeader();
  OLA_ASSERT(source_uid == decoded_header.SourceUID());
  OLA_ASSERT_EQ((uint16_t) 1234, decoded_header.SourceEndpoint());
  OLA_ASSERT(destination_uid == decoded_header.DestinationUID());
  OLA_ASSERT_EQ((uint16_t) 5678, decoded_header.DestinationEndpoint());
  OLA_ASSERT_EQ((uint32_t) 72650, decoded_header.Sequence());

  inflator.ResetHeaderField();
  OLA_ASSERT_FALSE(inflator.DecodeHeader(&header_set2, NULL, 0, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
}


/*
 * Check that we can inflate a RPT PDU that contains other PDUs
 */
void RPTInflatorTest::testInflatePDU() {
  RPTHeader header(UID(1, 2), 42, UID(10, 20), 99, 2370);
  // TODO(Peter): pass a different type of msg here as well
  RPTPDU pdu(3, header, NULL);
  OLA_ASSERT_EQ((unsigned int) 28, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  RPTInflator inflator;
  HeaderSet header_set;
  OLA_ASSERT(inflator.InflatePDUBlock(&header_set, data, size));
  OLA_ASSERT(header == header_set.GetRPTHeader());
  delete[] data;
}
}  // namespace acn
}  // namespace ola
