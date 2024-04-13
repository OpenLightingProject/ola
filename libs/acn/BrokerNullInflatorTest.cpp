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
 * BrokerNullInflatorTest.cpp
 * Test fixture for the BrokerNullInflator class
 * Copyright (C) 2023 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/HeaderSet.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/BrokerNullInflator.h"
#include "libs/acn/BrokerNullPDU.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {

using ola::network::HostToNetwork;

class BrokerNullInflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BrokerNullInflatorTest);
  CPPUNIT_TEST(testDecodeHeader);
  CPPUNIT_TEST(testInflatePDU);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testInflatePDU();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BrokerNullInflatorTest);


/*
 * Check that we can inflate a BrokerNull PDU that contains other PDUs
 */
void BrokerNullInflatorTest::testInflatePDU() {
  // TODO(Peter): pass a different type of msg here as well
  BrokerNullPDU pdu(3);
  OLA_ASSERT_EQ((unsigned int) 5, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  BrokerNullInflator inflator;
  HeaderSet header_set;
  OLA_ASSERT(inflator.InflatePDUBlock(&header_set, data, size));
//  OLA_ASSERT(header == header_set.GetBrokerHeader());
  delete[] data;
}
}  // namespace acn
}  // namespace ola
