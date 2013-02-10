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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RDMPDUTest.cpp
 * Test fixture for the RDMPDU class
 * Copyright (C) 2012 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"
#include "plugins/e131/e131/PDUTestCommon.h"
#include "plugins/e131/e131/RDMPDU.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace plugin {
namespace e131 {

using ola::io::IOQueue;
using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::rdm::RDMGetRequest;
using ola::rdm::UID;
using ola::testing::ASSERT_DATA_EQUALS;

class RDMPDUTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMPDUTest);
  CPPUNIT_TEST(testEmptyPDU);
  CPPUNIT_TEST(testEmptyPDUToOutputStream);
  CPPUNIT_TEST(testSimpleRDMPDU);
  CPPUNIT_TEST(testSimpleRDMPDUToOutputStream);
  CPPUNIT_TEST(testRDMPDUWithData);
  CPPUNIT_TEST(testRDMPDUWithDataToOutputStream);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testEmptyPDU();
    void testEmptyPDUToOutputStream();
    void testSimpleRDMPDU();
    void testSimpleRDMPDUToOutputStream();
    void testRDMPDUWithData();
    void testRDMPDUWithDataToOutputStream();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

  private:
    static const unsigned int TEST_VECTOR;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMPDUTest);

const unsigned int RDMPDUTest::TEST_VECTOR = 0xcc;

/*
 * Test an empty PDU works.
 */
void RDMPDUTest::testEmptyPDU() {
  RDMPDU pdu(NULL);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, pdu.DataSize());
  OLA_ASSERT_EQ(3u, pdu.Size());

  unsigned int length = pdu.Size();
  uint8_t *buffer = new uint8_t[length];
  OLA_ASSERT(pdu.Pack(buffer, length));

  const uint8_t expected_data[] = {0x70, 3, TEST_VECTOR};
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     buffer, length);
  delete[] buffer;
}


/*
 * Test writing an empty PDU to an OutputStream works.
 */
void RDMPDUTest::testEmptyPDUToOutputStream() {
  RDMPDU pdu(NULL);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(0u, pdu.DataSize());
  OLA_ASSERT_EQ(3u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(3u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  const uint8_t EXPECTED[] = {0x70, 3, TEST_VECTOR};
  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED, sizeof(EXPECTED),
                     pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}


/*
 * Test that packing a RDM PDU works. This uses a command with no data.
 */
void RDMPDUTest::testSimpleRDMPDU() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest *command = new RDMGetRequest(
    source,
    destination,
    0,  // transaction #
    1,  // port id
    0,  // message count
    10,  // sub device
    296,  // param id
    NULL,  // data
    0);  // data length

  RDMPDU pdu(command);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(25u, pdu.DataSize());
  OLA_ASSERT_EQ(28u, pdu.Size());

  unsigned int length = pdu.Size();
  uint8_t *buffer = new uint8_t[length];
  OLA_ASSERT(pdu.Pack(buffer, length));

  uint8_t expected_data[] = {
    0x70, 0x1c, TEST_VECTOR,
    1, 24,  // start code, sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x20, 1, 40, 0,  // command, param id, param data length
    1, 0x43  // checksum
  };
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     buffer, length);
  delete[] buffer;
}


/*
 * Test that writing a RDMPDU with a RDM command works.
 */
void RDMPDUTest::testSimpleRDMPDUToOutputStream() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest *command = new RDMGetRequest(
    source,
    destination,
    0,  // transaction #
    1,  // port id
    0,  // message count
    10,  // sub device
    296,  // param id
    NULL,  // data
    0);  // data length

  RDMPDU pdu(command);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(25u, pdu.DataSize());
  OLA_ASSERT_EQ(28u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(28u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);

  uint8_t EXPECTED[] = {
    0x70, 0x1c, TEST_VECTOR,
    1, 24,  // start code, sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x20, 1, 40, 0,  // command, param id, param data length
    1, 0x43  // checksum
  };
  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED, sizeof(EXPECTED),
                     pdu_data, pdu_size);
  output.Pop(output.Size());
  delete[] pdu_data;
}


/*
 * Test that packing a RDM PDU works. This uses a command data.
 */
void RDMPDUTest::testRDMPDUWithData() {
  UID source(1, 2);
  UID destination(3, 4);
  uint8_t rdm_data[] = {0xa5, 0xa5, 0xa5, 0xa5};

  RDMGetRequest *command = new RDMGetRequest(
    source,
    destination,
    0,  // transaction #
    1,  // port id
    0,  // message count
    10,  // sub device
    296,  // param id
    rdm_data,  // data
    sizeof(rdm_data));  // data length

  RDMPDU pdu(command);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(29u, pdu.DataSize());
  OLA_ASSERT_EQ(32u, pdu.Size());

  unsigned int length = pdu.Size();
  uint8_t *buffer = new uint8_t[length];
  OLA_ASSERT(pdu.Pack(buffer, length));

  uint8_t expected_data[] = {
    0x70, 0x20, TEST_VECTOR,
    1, 0x1c,  // sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x20, 1, 40, 4,  // command, param id, param data length
    0xa5, 0xa5, 0xa5, 0xa5,  // data
    3, 0xdf  // checksum
  };
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     buffer, length);
  delete[] buffer;
}


/*
 * Test that packing a RDM PDU works. This uses a command data.
 */
void RDMPDUTest::testRDMPDUWithDataToOutputStream() {
  UID source(1, 2);
  UID destination(3, 4);
  uint8_t rdm_data[] = {0xa5, 0xa5, 0xa5, 0xa5};

  RDMGetRequest *command = new RDMGetRequest(
    source,
    destination,
    0,  // transaction #
    1,  // port id
    0,  // message count
    10,  // sub device
    296,  // param id
    rdm_data,  // data
    sizeof(rdm_data));  // data length

  RDMPDU pdu(command);

  OLA_ASSERT_EQ(0u, pdu.HeaderSize());
  OLA_ASSERT_EQ(29u, pdu.DataSize());
  OLA_ASSERT_EQ(32u, pdu.Size());

  IOQueue output;
  OutputStream stream(&output);
  pdu.Write(&stream);
  OLA_ASSERT_EQ(32u, output.Size());

  uint8_t *pdu_data = new uint8_t[output.Size()];
  unsigned int pdu_size = output.Peek(pdu_data, output.Size());
  OLA_ASSERT_EQ(output.Size(), pdu_size);


  uint8_t EXPECTED[] = {
    0x70, 0x20, TEST_VECTOR,
    1, 0x1c,  // sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x20, 1, 40, 4,  // command, param id, param data length
    0xa5, 0xa5, 0xa5, 0xa5,  // data
    3, 0xdf  // checksum
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
