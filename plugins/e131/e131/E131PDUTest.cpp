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

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include <ola/network/NetworkUtils.h>
#include "PDUTestCommon.h"
#include "E131PDU.h"

namespace ola {
namespace e131 {

using ola::network::HostToNetwork;

class E131PDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(E131PDUTest);
  CPPUNIT_TEST(testSimpleE131PDU);
  CPPUNIT_TEST(testNestedE131PDU);
  CPPUNIT_TEST_SUITE_END();

  public:
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
void E131PDUTest::testSimpleE131PDU() {
  const string source = "foo source";
  E131Header header(source, 1, 2, 6000);
  E131PDU pdu(TEST_VECTOR, header, NULL);

  CPPUNIT_ASSERT_EQUAL((unsigned int) 36, pdu.HeaderSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, pdu.DataSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 42, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  CPPUNIT_ASSERT(pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  // spot check the data
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0x70, data[0]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) bytes_used, data[1]);
  CPPUNIT_ASSERT_EQUAL((unsigned int) HostToNetwork(TEST_VECTOR),
                       *((unsigned int*) &data[2]));

  CPPUNIT_ASSERT(!memcmp(&data[6], source.data(), source.length()));
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, data[6 + E131Header::SOURCE_NAME_LEN]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, data[7 + E131Header::SOURCE_NAME_LEN]);
  CPPUNIT_ASSERT_EQUAL(HostToNetwork((uint16_t) 6000),
                       *((uint16_t*) (data + 8 + E131Header::SOURCE_NAME_LEN)));

  // test undersized buffer
  bytes_used = size - 1;
  CPPUNIT_ASSERT(!pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bytes_used);

  // test oversized buffer
  bytes_used = size + 1;
  CPPUNIT_ASSERT(pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL((unsigned int) size, bytes_used);

  delete[] data;
}


/*
 * Test that packing a E131PDU with nested data works
 */
void E131PDUTest::testNestedE131PDU() {
  //TODO: add this test
}


} // e131
} // ola
