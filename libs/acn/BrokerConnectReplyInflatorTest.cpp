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
 * BrokerConectReplyInflatorTest.cpp
 * Test fixture for the BrokerConnectReplyInflator class
 * Copyright (C) 2023 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/HeaderSet.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/BrokerInflator.h"
#include "libs/acn/BrokerPDU.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {

using ola::network::HostToNetwork;

class BrokerConnectReplyInflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BrokerConnectReplyInflatorTest);
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

const uint8_t BrokerConnectReplyInflatorTest::TEST_DATA[] = {0, 1, 2, 3, 4, 5,
                                                             6, 7, 8, 9, 10,
                                                             11, 12, 13, 14,
                                                             15};
const uint8_t BrokerConnectReplyInflatorTest::TEST_DATA2[] = {10, 11, 12, 13,
                                                              14, 15, 16, 17,
                                                              18, 19, 20, 21,
                                                              22, 23, 24, 25};
CPPUNIT_TEST_SUITE_REGISTRATION(BrokerConnectReplyInflatorTest);


/*
 * Check that we can decode headers properly
 */
void BrokerConnectReplyInflatorTest::testDecodeHeader() {
  OLA_ASSERT(inflator.DecodeHeader(&header_set,
                                   reinterpret_cast<uint8_t*>(&header),
                                   sizeof(header),
                                   &bytes_used));
  OLA_ASSERT_EQ((unsigned int) sizeof(header), bytes_used);
  BrokerHeader decoded_header = header_set.GetBrokerHeader();

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
  decoded_header = header_set2.GetBrokerHeader();
  OLA_ASSERT(destination_cid == decoded_header.DestinationCid());
  OLA_ASSERT_EQ((uint32_t) 72650, decoded_header.TransactionNumber());

  inflator.ResetHeaderField();
  OLA_ASSERT_FALSE(inflator.DecodeHeader(&header_set2, NULL, 0, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
}


/*
 * Check that we can inflate a BrokerConnectReply PDU that contains other PDUs
 */
void BrokerConnectReplyInflatorTest::testInflatePDU() {
  // TODO(Peter): pass a different type of msg here as well
  BrokerConnectReplyPDU pdu(3, NULL);
  OLA_ASSERT_EQ((unsigned int) 23, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  BrokerConnectReplyInflator inflator;
  HeaderSet header_set;
  OLA_ASSERT(inflator.InflatePDUBlock(&header_set, data, size));
  // TODO(Peter): Test that the callback has the right data
  delete[] data;
}
}  // namespace acn
}  // namespace ola
