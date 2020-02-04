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
 * LLRPInflatorTest.cpp
 * Test fixture for the LLRPInflator class
 * Copyright (C) 2020 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/HeaderSet.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/LLRPInflator.h"
#include "libs/acn/LLRPPDU.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {

using ola::network::HostToNetwork;

class LLRPInflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LLRPInflatorTest);
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

const uint8_t LLRPInflatorTest::TEST_DATA[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                                               10, 11, 12, 13, 14, 15};
const uint8_t LLRPInflatorTest::TEST_DATA2[] = {10, 11, 12, 13, 14, 15, 16, 17,
                                                18, 19, 20, 21, 22, 23, 24,
                                                25};
CPPUNIT_TEST_SUITE_REGISTRATION(LLRPInflatorTest);


/*
 * Check that we can decode headers properly
 */
void LLRPInflatorTest::testDecodeHeader() {
  LLRPHeader::llrp_pdu_header header;
  memset(&header, 0, sizeof(header));
  LLRPInflator inflator;
  HeaderSet header_set, header_set2;
  unsigned int bytes_used;
  const ola::acn::CID destination_cid = CID::FromData(TEST_DATA);

  destination_cid.Pack(header.destination_cid);
  header.transaction_number = HostToNetwork(72650u);

  OLA_ASSERT(inflator.DecodeHeader(&header_set,
                                   reinterpret_cast<uint8_t*>(&header),
                                   sizeof(header),
                                   &bytes_used));
  OLA_ASSERT_EQ((unsigned int) sizeof(header), bytes_used);
  LLRPHeader decoded_header = header_set.GetLLRPHeader();
  OLA_ASSERT(destination_cid == decoded_header.DestinationCid());
  OLA_ASSERT_EQ((uint32_t) 72650, decoded_header.TransactionNumber());

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
  decoded_header = header_set2.GetLLRPHeader();
  OLA_ASSERT(destination_cid == decoded_header.DestinationCid());
  OLA_ASSERT_EQ((uint32_t) 72650, decoded_header.TransactionNumber());

  inflator.ResetHeaderField();
  OLA_ASSERT_FALSE(inflator.DecodeHeader(&header_set2, NULL, 0, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
}


/*
 * Check that we can inflate a LLRP PDU that contains other PDUs
 */
void LLRPInflatorTest::testInflatePDU() {
  const ola::acn::CID destination_cid = CID::FromData(TEST_DATA2);
  LLRPHeader header(destination_cid, 2370);
  // TODO(Peter): pass a different type of msg here as well
  LLRPPDU pdu(3, header, NULL);
  OLA_ASSERT_EQ((unsigned int) 27, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  LLRPInflator inflator;
  HeaderSet header_set;
  OLA_ASSERT(inflator.InflatePDUBlock(&header_set, data, size));
  OLA_ASSERT(header == header_set.GetLLRPHeader());
  delete[] data;
}
}  // namespace acn
}  // namespace ola
