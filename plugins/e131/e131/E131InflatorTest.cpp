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
 * E131InflatorTest.cpp
 * Test fixture for the E131Inflator class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/HeaderSet.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/E131Inflator.h"
#include "plugins/e131/e131/E131PDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;

class E131InflatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(E131InflatorTest);
  CPPUNIT_TEST(testDecodeHeader);
  CPPUNIT_TEST(testInflatePDU);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testDecodeHeader();
    void testInflatePDU();
};

CPPUNIT_TEST_SUITE_REGISTRATION(E131InflatorTest);


/*
 * Check that we can decode headers properly
 */
void E131InflatorTest::testDecodeHeader() {
  E131Header::e131_pdu_header header;
  E131Inflator inflator;
  HeaderSet header_set, header_set2;
  unsigned int bytes_used;
  const string source_name = "foobar";

  strncpy(header.source, source_name.data(), source_name.size() + 1);
  header.priority = 99;
  header.sequence = 10;
  header.universe = HostToNetwork(static_cast<uint16_t>(42));

  CPPUNIT_ASSERT(inflator.DecodeHeader(header_set,
                                       reinterpret_cast<uint8_t*>(&header),
                                       sizeof(header),
                                       bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) sizeof(header), bytes_used);
  E131Header decoded_header = header_set.GetE131Header();
  CPPUNIT_ASSERT(source_name == decoded_header.Source());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 99, decoded_header.Priority());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, decoded_header.Sequence());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 42, decoded_header.Universe());

  // try an undersized header
  CPPUNIT_ASSERT(!inflator.DecodeHeader(header_set,
                                        reinterpret_cast<uint8_t*>(&header),
                                        sizeof(header) - 1,
                                        bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);

  // test inherting the header from the prev call
  CPPUNIT_ASSERT(inflator.DecodeHeader(header_set2, NULL, 0, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);
  decoded_header = header_set2.GetE131Header();
  CPPUNIT_ASSERT(source_name == decoded_header.Source());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 99, decoded_header.Priority());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, decoded_header.Sequence());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 42, decoded_header.Universe());

  inflator.ResetHeaderField();
  CPPUNIT_ASSERT(!inflator.DecodeHeader(header_set2, NULL, 0, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);
}


/*
 * Check that we can inflate a E131 PDU that contains other PDUs
 */
void E131InflatorTest::testInflatePDU() {
  const string source = "foo source";
  E131Header header(source, 1, 2, 6000);
  // TODO(simon): pass a DMP msg here as well
  E131PDU pdu(3, header, NULL);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 42, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  CPPUNIT_ASSERT(pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  E131Inflator inflator;
  HeaderSet header_set;
  CPPUNIT_ASSERT(inflator.InflatePDUBlock(header_set, data, size));
  CPPUNIT_ASSERT(header == header_set.GetE131Header());
  delete[] data;
}
}  // e131
}  // plugin
}  // ola
