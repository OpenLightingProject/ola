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
 * E131PDUTest.cpp
 * Test fixture for the E131PDU class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/network/NetworkUtils.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/E131PDU.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace plugin {
namespace e131 {

using ola::network::HostToNetwork;

class E131PDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(E131PDUTest);
  CPPUNIT_TEST(testSimpleRev2E131PDU);
  CPPUNIT_TEST(testSimpleE131PDU);
  CPPUNIT_TEST(testNestedE131PDU);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSimpleRev2E131PDU();
    void testSimpleE131PDU();
    void testNestedE131PDU();
  private:
    static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(E131PDUTest);

const unsigned int E131PDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a E131PDU without data works.
 */
void E131PDUTest::testSimpleRev2E131PDU() {
  const string source = "foo source";
  E131Rev2Header header(source, 1, 2, 6000);
  E131PDU pdu(TEST_VECTOR, header, NULL);

  OLA_ASSERT_EQ((unsigned int) 36, pdu.HeaderSize());
  OLA_ASSERT_EQ((unsigned int) 0, pdu.DataSize());
  OLA_ASSERT_EQ((unsigned int) 42, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  // spot check the data
  OLA_ASSERT_EQ((uint8_t) 0x70, data[0]);
  OLA_ASSERT_EQ((uint8_t) bytes_used, data[1]);

  unsigned int actual_value;
  memcpy(&actual_value, data + 2, sizeof(actual_value));
  OLA_ASSERT_EQ((unsigned int) HostToNetwork(TEST_VECTOR),
                       actual_value);

  OLA_ASSERT_FALSE(memcmp(&data[6], source.data(), source.length()));
  OLA_ASSERT_EQ((uint8_t) 1,
                       data[6 + E131Rev2Header::REV2_SOURCE_NAME_LEN]);
  OLA_ASSERT_EQ((uint8_t) 2,
                       data[7 + E131Rev2Header::REV2_SOURCE_NAME_LEN]);
  OLA_ASSERT_EQ(
      HostToNetwork((uint16_t) 6000),
      *(reinterpret_cast<uint16_t*>(
          data + 8 + E131Rev2Header::REV2_SOURCE_NAME_LEN)));

  // test undersized buffer
  bytes_used = size - 1;
  OLA_ASSERT_FALSE(pdu.Pack(data, bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);

  // test oversized buffer
  bytes_used = size + 1;
  OLA_ASSERT(pdu.Pack(data, bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  delete[] data;
}


/*
 * Test that packing a E131PDU without data works.
 */
void E131PDUTest::testSimpleE131PDU() {
  const string source = "foo source";
  E131Header header(source, 1, 2, 6000, true, true);
  E131PDU pdu(TEST_VECTOR, header, NULL);

  OLA_ASSERT_EQ((unsigned int) 71, pdu.HeaderSize());
  OLA_ASSERT_EQ((unsigned int) 0, pdu.DataSize());
  OLA_ASSERT_EQ((unsigned int) 77, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);

  // spot check the data
  OLA_ASSERT_EQ((uint8_t) 0x70, data[0]);
  OLA_ASSERT_EQ((uint8_t) bytes_used, data[1]);
  unsigned int actual_value;
  memcpy(&actual_value, data + 2, sizeof(actual_value));
  OLA_ASSERT_EQ((unsigned int) HostToNetwork(TEST_VECTOR),
                       actual_value);

  OLA_ASSERT_FALSE(memcmp(&data[6], source.data(), source.length()));
  OLA_ASSERT_EQ((uint8_t) 1, data[6 + E131Header::SOURCE_NAME_LEN]);
  OLA_ASSERT_EQ((uint8_t) 2, data[9 + E131Header::SOURCE_NAME_LEN]);
  uint16_t actual_universe;
  memcpy(&actual_universe, data + 11 + E131Header::SOURCE_NAME_LEN,
         sizeof(actual_universe));
  OLA_ASSERT_EQ(HostToNetwork((uint16_t) 6000), actual_universe);

  // test undersized buffer
  bytes_used = size - 1;
  OLA_ASSERT_FALSE(pdu.Pack(data, bytes_used));
  OLA_ASSERT_EQ((unsigned int) 0, bytes_used);

  // test oversized buffer
  bytes_used = size + 1;
  OLA_ASSERT(pdu.Pack(data, bytes_used));
  OLA_ASSERT_EQ((unsigned int) size, bytes_used);
  delete[] data;
}


/*
 * Test that packing a E131PDU with nested data works
 */
void E131PDUTest::testNestedE131PDU() {
  // TODO(simon): add this test
}
}  // ola
}  // e131
}  // plugin
