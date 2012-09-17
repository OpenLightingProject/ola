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
 * RootPDUTest.cpp
 * Test fixture for the RootPDU class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ola/testing/TestUtils.h"

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/RootPDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::io::IOQueue;
using ola::io::OutputStream;
using ola::network::NetworkToHost;
using ola::testing::ASSERT_DATA_EQUALS;

class RootPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RootPDUTest);
  CPPUNIT_TEST(testSimpleRootPDU);
  CPPUNIT_TEST(testSimpleRootPDUToOutputStream);
  CPPUNIT_TEST(testNestedRootPDU);
  CPPUNIT_TEST(testNestedRootPDUToOutputStream);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSimpleRootPDU();
    void testSimpleRootPDUToOutputStream();
    void testNestedRootPDU();
    void testNestedRootPDUToOutputStream();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }

  private:
    static const unsigned int TEST_VECTOR = 4;
    static const unsigned int TEST_VECTOR2 = 99;
};


CPPUNIT_TEST_SUITE_REGISTRATION(RootPDUTest);


/*
 * Test that packing a RootPDU without data works.
 */
void RootPDUTest::testSimpleRootPDU() {
  CID cid = CID::Generate();
  RootPDU pdu1(TEST_VECTOR, cid, NULL);
  OLA_ASSERT(cid == pdu1.Cid());
  OLA_ASSERT_EQ(22u, pdu1.Size());

  unsigned int size = pdu1.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu1.Pack(data, bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);

  // spot check the data
  OLA_ASSERT_EQ((uint8_t) 0x70, data[0]);
  OLA_ASSERT_EQ((uint8_t) bytes_used, data[1]);
  unsigned int actual_value;
  memcpy(&actual_value, data + 2, sizeof(actual_value));
  OLA_ASSERT_EQ(HostToNetwork(TEST_VECTOR), actual_value);
  CID cid2 = CID::FromData(&data[6]);
  OLA_ASSERT(cid2 == cid);

  // test undersized buffer
  bytes_used = size - 1;
  OLA_ASSERT_FALSE(pdu1.Pack(data, bytes_used));
  OLA_ASSERT_EQ(0u, bytes_used);

  // test oversized buffer
  bytes_used = size + 1;
  OLA_ASSERT(pdu1.Pack(data, bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);

  // change the vector
  pdu1.SetVector(TEST_VECTOR2);
  OLA_ASSERT(pdu1.Pack(data, bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);
  OLA_ASSERT_EQ((uint8_t) 0x70, data[0]);
  OLA_ASSERT_EQ((uint8_t) bytes_used, data[1]);
  memcpy(&actual_value, data + 2, sizeof(actual_value));
  OLA_ASSERT_EQ(HostToNetwork(TEST_VECTOR2), actual_value);
  cid2 = CID::FromData(&data[6]);
  OLA_ASSERT(cid2 == cid);

  // use the other constructor
  RootPDU pdu2(TEST_VECTOR);
  pdu2.Cid(cid);

  OLA_ASSERT(cid == pdu1.Cid());
  OLA_ASSERT_EQ(22u, pdu1.Size());
  bytes_used = size;
  uint8_t *data2 = new uint8_t[size];
  OLA_ASSERT(pdu1.Pack(data2, bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);
  OLA_ASSERT_FALSE(memcmp(data, data2, bytes_used));

  delete[] data;
  delete[] data2;
}


/**
 * Check that writing an Root PDU to an output stream works
 */
void RootPDUTest::testSimpleRootPDUToOutputStream() {
  CID cid = CID::Generate();
  RootPDU pdu1(TEST_VECTOR, cid, NULL);
  OLA_ASSERT(cid == pdu1.Cid());

  OLA_ASSERT_EQ(16u, pdu1.HeaderSize());
  OLA_ASSERT_EQ(4u, pdu1.VectorSize());
  OLA_ASSERT_EQ(0u, pdu1.DataSize());
  OLA_ASSERT_EQ(22u, pdu1.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu1.Write(&stream);

  OLA_ASSERT_EQ(22u, output.Size());

  uint8_t *raw_pdu = new uint8_t[output.Size()];
  unsigned int raw_pdu_size = output.Peek(raw_pdu, output.Size());
  OLA_ASSERT_EQ(output.Size(), raw_pdu_size);

  uint8_t EXPECTED[] = {
    0x70, 22,
    0, 0, 0, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  cid.Pack(EXPECTED + 6);
  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED, sizeof(EXPECTED),
                     raw_pdu, raw_pdu_size);

  output.Pop(output.Size());
  delete[] raw_pdu;
}


/*
 * Test that packing a RootPDU with nested data works
 */
void RootPDUTest::testNestedRootPDU() {
  FakePDU pdu1(1);
  FakePDU pdu2(42);
  PDUBlock<PDU> block;
  block.AddPDU(&pdu1);
  block.AddPDU(&pdu2);

  CID cid = CID::Generate();
  RootPDU pdu(TEST_VECTOR, cid, &block);

  OLA_ASSERT(cid == pdu.Cid());
  OLA_ASSERT_EQ(30u, pdu.Size());

  unsigned int size = pdu.Size();
  uint8_t *data = new uint8_t[size];
  unsigned int bytes_used = size;
  OLA_ASSERT(pdu.Pack(data, bytes_used));
  OLA_ASSERT_EQ(size, bytes_used);

  // spot check
  unsigned int actual_value;
  memcpy(&actual_value, data + 22, sizeof(actual_value));
  OLA_ASSERT_EQ(1u, actual_value);
  memcpy(&actual_value, data + 26, sizeof(actual_value));
  OLA_ASSERT_EQ(42u, actual_value);

  delete[] data;
}


/*
 * Test that packing a RootPDU with nested data works
 */
void RootPDUTest::testNestedRootPDUToOutputStream() {
  FakePDU pdu1(1);
  FakePDU pdu2(42);
  PDUBlock<PDU> block;
  block.AddPDU(&pdu1);
  block.AddPDU(&pdu2);

  CID cid = CID::Generate();
  RootPDU pdu(TEST_VECTOR, cid, &block);

  OLA_ASSERT(cid == pdu.Cid());
  OLA_ASSERT_EQ(30u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(30u, output.Size());

  uint8_t *raw_pdu = new uint8_t[output.Size()];
  unsigned int raw_pdu_size = output.Peek(raw_pdu, output.Size());
  OLA_ASSERT_EQ(output.Size(), raw_pdu_size);

  uint8_t EXPECTED[] = {
    0x70, 30,
    0, 0, 0, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1,
    0, 0, 0, 42
  };
  cid.Pack(EXPECTED + 6);
  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED, sizeof(EXPECTED),
                     raw_pdu, raw_pdu_size);

  output.Pop(output.Size());
  delete[] raw_pdu;
}
}  // e131
}  // plugin
}  // ola
