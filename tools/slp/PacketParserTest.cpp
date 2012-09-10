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
 * PacketParserTester.cpp
 * Test fixture for the SLPPacketParser class
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/BigEndianStream.h"
#include "ola/io/MemoryBuffer.h"
#include "ola/network/IPV4Address.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/SLPPacketParser.h"
#include "tools/slp/URLEntry.h"

using ola::io::BigEndianInputStream;
using ola::io::MemoryBuffer;
using ola::network::IPV4Address;
using ola::slp::SLPPacketParser;
using ola::slp::URLEntries;
using ola::slp::URLEntry;
using ola::testing::ASSERT_DATA_EQUALS;
using std::auto_ptr;
using std::string;
using std::vector;

class PacketParserTest: public CppUnit::TestFixture {
  public:
    CPPUNIT_TEST_SUITE(PacketParserTest);
    CPPUNIT_TEST(testDetermineFunctionID);
    CPPUNIT_TEST(testParseServiceRequest);
    CPPUNIT_TEST_SUITE_END();

    void testDetermineFunctionID();
    void testParseServiceRequest();

  public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

  private:
    SLPPacketParser m_parser;
    /*
    uint8_t* WriteToBuffer(IOQueue *data, unsigned int *size) {
      *size = data->Size();
      uint8_t *data_buffer = new uint8_t[*size];
      *size = data->Peek(data_buffer, *size);
      data->Pop(*size);
      return data_buffer;
    }
    */
};

CPPUNIT_TEST_SUITE_REGISTRATION(PacketParserTest);


/*
 * Check that DetermineFunctionID() works.
 */
void PacketParserTest::testDetermineFunctionID() {
  uint8_t data[] = {2, 1};
  OLA_ASSERT_EQ(static_cast<uint8_t>(ola::slp::SERVICE_REQUEST),
                m_parser.DetermineFunctionID(data, sizeof(data)));

  data[1] = 2;
  OLA_ASSERT_EQ(static_cast<uint8_t>(ola::slp::SERVICE_REPLY),
                m_parser.DetermineFunctionID(data, sizeof(data)));

  // check error cases
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), m_parser.DetermineFunctionID(data, 0));
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), m_parser.DetermineFunctionID(data, 1));
}


/*
 * Check that UnpackServiceRequest() works.
 */
void PacketParserTest::testParseServiceRequest() {
  const uint8_t input_data[] = {
    2, 1, 0, 0, 63, 0xe0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1', '.',
    '8',  // pr-llist
    0, 13, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
    0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
    0, 0,  // pred string
    0, 0,  // SPI string
  };
  MemoryBuffer buffer(input_data, sizeof(input_data));
  BigEndianInputStream stream(&buffer);

  auto_ptr<const ola::slp::ServiceRequestPacket> packet(
    m_parser.UnpackServiceRequest(&stream));
  OLA_ASSERT(packet.get());

  // verify the contents of the packet
  OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x1234), packet->xid);
  OLA_ASSERT_EQ(true, packet->Overflow());
  OLA_ASSERT_EQ(true, packet->Fresh());
  OLA_ASSERT_EQ(true, packet->Multicast());
  OLA_ASSERT_EQ(string("en"), packet->language);
}
