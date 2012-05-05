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
 * E133PDUTest.cpp
 * Test fixture for the E133PDU class
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"
#include "plugins/e131/e131/E133PDU.h"
#include "plugins/e131/e131/PDUTestCommon.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::io::IOQueue;
using ola::network::HostToNetwork;
using ola::testing::ASSERT_DATA_EQUALS;

class E133PDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(E133PDUTest);
  CPPUNIT_TEST(testSimpleE133PDU);
  CPPUNIT_TEST(testSimpleE133PDUToOutputStream);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSimpleE133PDU();
    void testSimpleE133PDUToOutputStream();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }

  private:
    static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(E133PDUTest);

const unsigned int E133PDUTest::TEST_VECTOR = 39;


/*
 * Test that packing a E133PDU without data works.
 */
void E133PDUTest::testSimpleE133PDU() {
  const string source = "foo source";
  E133Header header(source, 101, 2, false);
  E133PDU pdu(TEST_VECTOR, header, NULL);

  CPPUNIT_ASSERT_EQUAL(71u, pdu.HeaderSize());
  CPPUNIT_ASSERT_EQUAL(0u, pdu.DataSize());
  CPPUNIT_ASSERT_EQUAL(77u, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  CPPUNIT_ASSERT(pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL(size, bytes_used);

  // spot check the data
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0x70, data[0]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) bytes_used, data[1]);
  unsigned int actual_value;
  memcpy(&actual_value, data + 2, sizeof(actual_value));
  CPPUNIT_ASSERT_EQUAL(HostToNetwork(TEST_VECTOR), actual_value);

  CPPUNIT_ASSERT(!memcmp(&data[6], source.data(), source.length()));
  // universe
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, data[6 + E133Header::SOURCE_NAME_LEN]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, data[6 + E133Header::SOURCE_NAME_LEN + 1]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, data[6 + E133Header::SOURCE_NAME_LEN + 2]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 101,
                       data[6 + E133Header::SOURCE_NAME_LEN + 3]);
  // endpoint
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, data[6 + E133Header::SOURCE_NAME_LEN + 4]);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 2, data[6 + E133Header::SOURCE_NAME_LEN + 5]);
  // options
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, data[6 + E133Header::SOURCE_NAME_LEN + 6]);

  // test undersized buffer
  bytes_used = size - 1;
  CPPUNIT_ASSERT(!pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL(0u, bytes_used);

  // test oversized buffer
  bytes_used = size + 1;
  CPPUNIT_ASSERT(pdu.Pack(data, bytes_used));
  CPPUNIT_ASSERT_EQUAL(size, bytes_used);
  delete[] data;
}


/*
 * Test that writing to an output stream works.
 */
void E133PDUTest::testSimpleE133PDUToOutputStream() {
  const string source = "foo source";
  E133Header header(source, 101, 2, false);
  E133PDU pdu(TEST_VECTOR, header, NULL);

  CPPUNIT_ASSERT_EQUAL(71u, pdu.HeaderSize());
  CPPUNIT_ASSERT_EQUAL(0u, pdu.DataSize());
  CPPUNIT_ASSERT_EQUAL(77u, pdu.Size());

  IOQueue output;
  pdu.Write(&output);
  CPPUNIT_ASSERT_EQUAL(77u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  CPPUNIT_ASSERT_EQUAL(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0x70, 77,
    0, 0, 0, 39,
    'f', 'o', 'o', ' ', 's', 'o', 'u', 'r', 'c', 'e', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 101, // seq #
    0, 2, // endpoint
    0,
  };
  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED, sizeof(EXPECTED),
                     pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}
}  // ola
}  // e131
}  // plugin
