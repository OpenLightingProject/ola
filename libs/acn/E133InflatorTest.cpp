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
 * E133InflatorTest.cpp
 * Test fixture for the E133Inflator class
 * Copyright (C) 2011 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "libs/acn/HeaderSet.h"
#include "libs/acn/PDUTestCommon.h"
#include "libs/acn/E133Inflator.h"
#include "libs/acn/E133PDU.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace acn {

using ola::network::HostToNetwork;
using std::string;

class E133InflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(E133InflatorTest);
  CPPUNIT_TEST(testDecodeHeader);
  CPPUNIT_TEST(testInflatePDU);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testDecodeHeader();
    void testInflatePDU();
};

CPPUNIT_TEST_SUITE_REGISTRATION(E133InflatorTest);


/*
 * Check that we can decode headers properly
 */
void E133InflatorTest::testDecodeHeader() {
  E133Header::e133_pdu_header header;
  memset(&header, 0, sizeof(header));
  E133Inflator inflator;
  HeaderSet header_set, header_set2;
  unsigned int bytes_used;
  const string source_name = "foobar";

  strncpy(header.source, source_name.data(), source_name.size() + 1);
  header.sequence = HostToNetwork(72650u);
  header.endpoint = HostToNetwork(static_cast<uint16_t>(42));

  OLA_ASSERT(inflator.DecodeHeader(&header_set,
                                   reinterpret_cast<uint8_t*>(&header),
                                   sizeof(header),
                                   &bytes_used));
  OLA_ASSERT_EQ((unsigned int) sizeof(header), bytes_used);
  E133Header decoded_header = header_set.GetE133Header();
  OLA_ASSERT(source_name == decoded_header.Source());
  OLA_ASSERT_EQ((uint32_t) 72650, decoded_header.Sequence());
  OLA_ASSERT_EQ((uint16_t) 42, decoded_header.Endpoint());

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
  decoded_header = header_set2.GetE133Header();
  OLA_ASSERT(source_name == decoded_header.Source());
  OLA_ASSERT_EQ((uint32_t) 72650, decoded_header.Sequence());
  OLA_ASSERT_EQ((uint16_t) 42, decoded_header.Endpoint());

  inflator.ResetHeaderField();
  OLA_ASSERT_FALSE(inflator.DecodeHeader(&header_set2, NULL, 0, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);
}


/*
 * Check that we can inflate a E133 PDU that contains other PDUs
 */
void E133InflatorTest::testInflatePDU() {
  const string source = "foobar source";
  E133Header header(source, 2370, 2);
  // TODO(simon): pass a DMP msg here as well
  E133PDU pdu(3, header, NULL);
  OLA_ASSERT_EQ((unsigned int) 77, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, &bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  E133Inflator inflator;
  HeaderSet header_set;
  OLA_ASSERT(inflator.InflatePDUBlock(&header_set, data, size));
  OLA_ASSERT(header == header_set.GetE133Header());
  delete[] data;
}
}  // namespace acn
}  // namespace ola
