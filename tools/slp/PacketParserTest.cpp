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
#include "tools/slp/ServiceEntry.h"

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
    CPPUNIT_TEST(testParseServiceReply);
    CPPUNIT_TEST(testParseServiceRegistration);
    CPPUNIT_TEST(testParseServiceAck);
    CPPUNIT_TEST(testParseDAAdvert);
    CPPUNIT_TEST(testParseServiceDeRegister);
    CPPUNIT_TEST(testExtractHeader);
    CPPUNIT_TEST(testExtractURLEntry);
    CPPUNIT_TEST_SUITE_END();

    void testDetermineFunctionID();
    void testParseServiceRequest();
    void testParseServiceReply();
    void testParseServiceRegistration();
    void testParseServiceAck();
    void testParseDAAdvert();
    void testParseServiceDeRegister();
    void testExtractHeader();
    void testExtractURLEntry();

  public:
    void setUp() {
      OLA_ASSERT(IPV4Address::FromString("1.1.1.2", &ip1));
      OLA_ASSERT(IPV4Address::FromString("1.1.1.8", &ip2));
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      expected_scopes = "ACN,MYORG";

      expected_urls.push_back(URLEntry("service:foo://1.1.1.1", 0x1244));
      expected_urls.push_back(URLEntry("service:foo://1.1.1.10", 0x5678));
    }

  private:
    SLPPacketParser m_parser;
    IPV4Address ip1, ip2;
    string expected_scopes;
    vector<URLEntry> expected_urls;
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
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 63, 0xe0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1',
      '.', '8',  // pr-list
      0, 13, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'f', 'o', 'o',  // pred string
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

    vector<IPV4Address> expected_pr_list;
    expected_pr_list.push_back(ip1);
    expected_pr_list.push_back(ip2);
    OLA_ASSERT_VECTOR_EQ(expected_pr_list, packet->pr_list);
    OLA_ASSERT_EQ(string("rdmnet-device"), packet->service_type);

    OLA_ASSERT_EQ(expected_scopes, packet->scope_list);
    OLA_ASSERT_EQ(string("foo"), packet->predicate);
    OLA_ASSERT_EQ(string(""), packet->spi);
  }

  // test invalid PR list, this is non-fatal
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 51, 0xe0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 3, 'f', 'o', 'o',
      0, 13, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'f', 'o', 'o',  // pred string
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

    vector<IPV4Address> expected_pr_list;
    OLA_ASSERT_VECTOR_EQ(expected_pr_list, packet->pr_list);
    OLA_ASSERT_EQ(string("rdmnet-device"), packet->service_type);

    OLA_ASSERT_EQ(expected_scopes, packet->scope_list);
    OLA_ASSERT_EQ(string("foo"), packet->predicate);
    OLA_ASSERT_EQ(string(""), packet->spi);
  }

  // now try a different packet
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 63, 0x80, 0, 0, 0, 0, 0, 0x78, 0, 4, 'e', 'n', 'n', 'g',
      0, 0,  // no pr-list
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
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x78), packet->xid);
    OLA_ASSERT_EQ(true, packet->Overflow());
    OLA_ASSERT_EQ(false, packet->Fresh());
    OLA_ASSERT_EQ(false, packet->Multicast());
    OLA_ASSERT_EQ(string("enng"), packet->language);

    vector<IPV4Address> expected_pr_list;
    OLA_ASSERT_VECTOR_EQ(expected_pr_list, packet->pr_list);
    OLA_ASSERT_EQ(string("rdmnet-device"), packet->service_type);

    OLA_ASSERT_EQ(expected_scopes, packet->scope_list);
    OLA_ASSERT_EQ(string(""), packet->predicate);
    OLA_ASSERT_EQ(string(""), packet->spi);
  }

  // confirm strings are un-escaped correctly
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 69, 0x80, 0, 0, 0, 0, 0, 0x78, 0, 4, 'e', 'n', 'n', 'g',
      0, 0,  // no pr-list
      0, 16, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
      '\\', '5', 'c',
      0, 0xc, 'A', 'C', 'N', '\\', '2', 'c', ',', 'M', 'Y', 'O', 'R', 'G',
      // scope list
      0, 0,  // pred string
      0, 0,  // SPI string
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);

    auto_ptr<const ola::slp::ServiceRequestPacket> packet(
      m_parser.UnpackServiceRequest(&stream));
    OLA_ASSERT(packet.get());

    // verify the contents of the packet
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x78), packet->xid);
    OLA_ASSERT_EQ(true, packet->Overflow());
    OLA_ASSERT_EQ(false, packet->Fresh());
    OLA_ASSERT_EQ(false, packet->Multicast());
    OLA_ASSERT_EQ(string("enng"), packet->language);

    vector<IPV4Address> expected_pr_list;
    OLA_ASSERT_VECTOR_EQ(expected_pr_list, packet->pr_list);
    OLA_ASSERT_EQ(string("rdmnet-device\\"), packet->service_type);

    OLA_ASSERT_EQ(string("ACN\\2c,MYORG"), packet->scope_list);
    OLA_ASSERT_EQ(string(""), packet->predicate);
    OLA_ASSERT_EQ(string(""), packet->spi);
  }

  // try with an empty scope list
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 69, 0x80, 0, 0, 0, 0, 0, 0x78, 0, 4, 'e', 'n', 'n', 'g',
      0, 0,  // no pr-list
      0, 16, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
      '\\', '5', 'c',
      0, 0x0,  // scope list
      0, 0,  // pred string
      0, 0,  // SPI string
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);

    auto_ptr<const ola::slp::ServiceRequestPacket> packet(
      m_parser.UnpackServiceRequest(&stream));
    OLA_ASSERT(packet.get());

    // verify the contents of the packet
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x78), packet->xid);
    OLA_ASSERT_EQ(true, packet->Overflow());
    OLA_ASSERT_EQ(false, packet->Fresh());
    OLA_ASSERT_EQ(false, packet->Multicast());
    OLA_ASSERT_EQ(string("enng"), packet->language);

    vector<IPV4Address> expected_pr_list;
    OLA_ASSERT_VECTOR_EQ(expected_pr_list, packet->pr_list);
    OLA_ASSERT_EQ(string("rdmnet-device\\"), packet->service_type);

    OLA_ASSERT_EQ(string(""), packet->scope_list);
    OLA_ASSERT_EQ(string(""), packet->predicate);
    OLA_ASSERT_EQ(string(""), packet->spi);
  }

  // short packet
  {
    MemoryBuffer buffer(NULL, 0);
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRequest(&stream));
  }

  // invalid PR List
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 63, 0xe0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 255,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRequest(&stream));
  }

  // invalid service-type
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 63, 0xe0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1',
      '.', '8',  // pr-list
      0, 13,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRequest(&stream));
  }

  // invalid scope list
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 63, 0xe0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1',
      '.', '8',  // pr-list
      0, 13, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
      0, 9,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRequest(&stream));
  }

  // invalid predicate
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 63, 0xe0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1',
      '.', '8',  // pr-list
      0, 13, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRequest(&stream));
  }

  // invalid SPI string
  {
    const uint8_t input_data[] = {
      2, 1, 0, 0, 63, 0xe0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1',
      '.', '8',  // pr-list
      0, 13, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'f', 'o', 'o',  // pred string
      0, 10,  // SPI string
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRequest(&stream));
  }
}


/*
 * Check that UnpackServiceReply() works.
 */
void PacketParserTest::testParseServiceReply() {
  {
    uint8_t input_data[] = {
      2, 2, 0, 0, 0x4b, 0x40, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 12,  // error code
      0, 2,  // url entry count
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of auth blocks
      // entry 2
      0, 0x56, 0x78, 0, 22,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1', '0',
      0,  // # of auth blocks
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);

    auto_ptr<const ola::slp::ServiceReplyPacket> packet(
      m_parser.UnpackServiceReply(&stream));
    OLA_ASSERT(packet.get());

    // verify the contents of the packet
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x1234), packet->xid);
    OLA_ASSERT_EQ(false, packet->Overflow());
    OLA_ASSERT_EQ(true, packet->Fresh());
    OLA_ASSERT_EQ(false, packet->Multicast());
    OLA_ASSERT_EQ(string("en"), packet->language);
    OLA_ASSERT_VECTOR_EQ(expected_urls, packet->url_entries);
  }

  // packet with auth blocks
  {
    uint8_t input_data[] = {
      2, 2, 0, 0, 0x4b, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 12,  // error code
      0, 2,  // url entry count
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      1,  // # of auth blocks
      0x80, 0x00, 0, 19,  // type and length
      0x12, 0x34, 0x56, 0x78,  // timestamp
      0 , 3, 's', 'p', 'i',  // spi string
      'a', 'b', 'c', 'd', 'e', 'f',  // auth block data
      // entry 2
      0, 0x56, 0x78, 0, 22,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1', '0',
      1,  // # of auth blocks, this one doesn't contain any data
      0x80, 0x00, 0, 13,  // type and length
      0x12, 0x34, 0x56, 0x78,  // timestamp
      0 , 3, 's', 'p', 'i',  // spi string
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);

    auto_ptr<const ola::slp::ServiceReplyPacket> packet(
      m_parser.UnpackServiceReply(&stream));
    OLA_ASSERT(packet.get());

    // verify the contents of the packet
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x1234), packet->xid);
    OLA_ASSERT_EQ(false, packet->Overflow());
    OLA_ASSERT_EQ(false, packet->Fresh());
    OLA_ASSERT_EQ(true, packet->Multicast());
    OLA_ASSERT_EQ(string("en"), packet->language);
    OLA_ASSERT_VECTOR_EQ(expected_urls, packet->url_entries);
  }

  // short packet
  {
    MemoryBuffer buffer(NULL, 0);
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceReply(&stream));
  }

  // missing error code
  {
    uint8_t input_data[] = {
      2, 2, 0, 0, 0x4b, 0x40, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceReply(&stream));
  }

  // missing url count
  {
    uint8_t input_data[] = {
      2, 2, 0, 0, 0x4b, 0x40, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 12,  // error code
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceReply(&stream));
  }
}


/*
 * Check that UnpackServiceRegistration() works.
 */
void PacketParserTest::testParseServiceRegistration() {
  {
    uint8_t input_data[] = {
      2, 3, 0, 0, 0x41, 0x60, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of auth blocks
      0, 3, 'f', 'o', 'o',  // service-type
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'b', 'a', 'r',  // attr list
      0  // attr auths
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);

    auto_ptr<const ola::slp::ServiceRegistrationPacket> packet(
      m_parser.UnpackServiceRegistration(&stream));
    OLA_ASSERT(packet.get());

    // verify the contents of the packet
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x1234), packet->xid);
    OLA_ASSERT_EQ(false, packet->Overflow());
    OLA_ASSERT_EQ(true, packet->Fresh());
    OLA_ASSERT_EQ(true, packet->Multicast());
    OLA_ASSERT_EQ(string("en"), packet->language);

    OLA_ASSERT_EQ(string("service:foo://1.1.1.1"), packet->url.url());
    OLA_ASSERT_EQ(static_cast<uint16_t>(0x1234), packet->url.lifetime());
    OLA_ASSERT_EQ(string("foo"), packet->service_type);
    OLA_ASSERT_EQ(expected_scopes, packet->scope_list);
    OLA_ASSERT_EQ(string("bar"), packet->attr_list);
  }

  // short packet
  {
    MemoryBuffer buffer(NULL, 0);
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRegistration(&stream));
  }

  // invalid url entry
  {
    uint8_t input_data[] = {
      2, 3, 0, 0, 0x41, 0x60, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRegistration(&stream));
  }

  // invalid service-type
  {
    uint8_t input_data[] = {
      2, 3, 0, 0, 0x41, 0x60, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of auth blocks
      0, 10, 'f', 'o', 'o',  // service-type
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRegistration(&stream));
  }

  // invalid scope list
  {
    uint8_t input_data[] = {
      2, 3, 0, 0, 0x41, 0x60, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of auth blocks
      0, 3, 'f', 'o', 'o',  // service-type
      0, 9,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRegistration(&stream));
  }

  // scope list
  {
    uint8_t input_data[] = {
      2, 3, 0, 0, 0x41, 0x60, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of auth blocks
      0, 3, 'f', 'o', 'o',  // service-type
      0, 9,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRegistration(&stream));
  }

  // invalid attr list
  {
    uint8_t input_data[] = {
      2, 3, 0, 0, 0x41, 0x60, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of auth blocks
      0, 3, 'f', 'o', 'o',  // service-type
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRegistration(&stream));
  }

  // missing url auth blocks length
  {
    uint8_t input_data[] = {
      2, 3, 0, 0, 0x41, 0x60, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of auth blocks
      0, 3, 'f', 'o', 'o',  // service-type
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'b', 'a', 'r',  // attr list
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRegistration(&stream));
  }

  // invalid url auth blocks
  {
    uint8_t input_data[] = {
      2, 3, 0, 0, 0x41, 0x60, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of auth blocks
      0, 3, 'f', 'o', 'o',  // service-type
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'b', 'a', 'r',  // attr list
      10,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceRegistration(&stream));
  }
}


/*
 * Check that UnpackServiceAck() works.
 */
void PacketParserTest::testParseServiceAck() {
  {
    uint8_t input_data[] = {
      2, 5, 0, 0, 18, 0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0x56, 0x78
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);

    auto_ptr<const ola::slp::ServiceAckPacket> packet(
      m_parser.UnpackServiceAck(&stream));
    OLA_ASSERT(packet.get());

    // verify the contents of the packet
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x1234), packet->xid);
    OLA_ASSERT_EQ(false, packet->Overflow());
    OLA_ASSERT_EQ(false, packet->Fresh());
    OLA_ASSERT_EQ(false, packet->Multicast());
    OLA_ASSERT_EQ(string("en"), packet->language);
    OLA_ASSERT_EQ(static_cast<uint16_t>(0x5678), packet->error_code);
  }

  // test a packet missing the error code
  {
    uint8_t input_data[] = {
      2, 5, 0, 0, 16, 0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }

  // short packet
  {
    MemoryBuffer buffer(NULL, 0);
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }
}


/*
 * Check that UnpackDAAdvert() works.
 */
void PacketParserTest::testParseDAAdvert() {
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0,  // error code is zeroed out if multicast
      0x12, 0x34, 0x56, 0x78,  // boot timestamp
      0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // service
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'b', 'a', 'r',  // attr list
      0, 0,  // SPI list
      0  // auth blocks
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);

    auto_ptr<const ola::slp::DAAdvertPacket> packet(
      m_parser.UnpackDAAdvert(&stream));
    OLA_ASSERT(packet.get());

    // verify the contents of the packet
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x1234), packet->xid);
    OLA_ASSERT_EQ(false, packet->Overflow());
    OLA_ASSERT_EQ(false, packet->Fresh());
    OLA_ASSERT_EQ(true, packet->Multicast());
    OLA_ASSERT_EQ(string("en"), packet->language);

    OLA_ASSERT_EQ(static_cast<uint16_t>(0), packet->error_code);
    OLA_ASSERT_EQ(0x12345678u, packet->boot_timestamp);
    OLA_ASSERT_EQ(string("service:foo"), packet->url);
    OLA_ASSERT_EQ(expected_scopes, packet->scope_list);
    OLA_ASSERT_EQ(string("bar"), packet->attr_list);
  }

  // test a short packet
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0,  // error code is zeroed out if multicast
      0x12, 0x34, 0x56, 0x78,  // boot timestamp
      0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // service
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'b', 'a', 'r',  // attr list
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }

  // short packet
  {
    MemoryBuffer buffer(NULL, 0);
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }

  // missing error code
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }

  // missing boot timestamp
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0,  // error code is zeroed out if multicast
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }

  // missing DA URL
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0,  // error code is zeroed out if multicast
      0x12, 0x34, 0x56, 0x78,  // boot timestamp
      0, 11,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }

  // missing scope list
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0,  // error code is zeroed out if multicast
      0x12, 0x34, 0x56, 0x78,  // boot timestamp
      0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // service
      0, 9,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }

  // missing attr list
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0,  // error code is zeroed out if multicast
      0x12, 0x34, 0x56, 0x78,  // boot timestamp
      0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // service
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }

  // missing auth block length
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0,  // error code is zeroed out if multicast
      0x12, 0x34, 0x56, 0x78,  // boot timestamp
      0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // service
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'b', 'a', 'r',  // attr list
      0, 0,  // SPI list
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }

  // too many auth blocks
  {
    uint8_t input_data[] = {
      2, 8, 0, 0, 0x33, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0,  // error code is zeroed out if multicast
      0x12, 0x34, 0x56, 0x78,  // boot timestamp
      0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // service
      0, 9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',  // scope list
      0, 3, 'b', 'a', 'r',  // attr list
      0, 0,  // SPI list
      10  // auth blocks
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackDAAdvert(&stream));
  }
}


/*
 * Check that UnpackServiceDeRegistration() works.
 */
void PacketParserTest::testParseServiceDeRegister() {
  {
    uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // scope list
      0, 0x9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of URL auths
      0, 0  // tag list length
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);

    auto_ptr<const ola::slp::ServiceDeRegistrationPacket> packet(
      m_parser.UnpackServiceDeRegistration(&stream));
    OLA_ASSERT(packet.get());

    // verify the contents of the packet
    OLA_ASSERT_EQ(static_cast<ola::slp::xid_t>(0x1234), packet->xid);
    OLA_ASSERT_EQ(false, packet->Overflow());
    OLA_ASSERT_EQ(false, packet->Fresh());
    OLA_ASSERT_EQ(false, packet->Multicast());
    OLA_ASSERT_EQ(string("en"), packet->language);

    OLA_ASSERT_EQ(string("service:foo://1.1.1.1"), packet->url.url());
    OLA_ASSERT_EQ(static_cast<uint16_t>(0x1234), packet->url.lifetime());
    OLA_ASSERT_EQ(expected_scopes, packet->scope_list);
    OLA_ASSERT_EQ(string(""), packet->tag_list);
  }

  // short packet
  {
    MemoryBuffer buffer(NULL, 0);
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }

  // missing scope list
  {
    const uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // scope list
      0, 0x9,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }

  // missing url entry
  {
    const uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // scope list
      0, 0x9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }

  // missing tag list
  {
    const uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // scope list
      0, 0x9, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      0,  // # of URL auths
      0, 10  // tag list length
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }
}


/*
 * Check error handling for the SLP header. The Ack is the easiest packet to
 * test this with.
 */
void PacketParserTest::testExtractHeader() {
  // wrong version
  {
    uint8_t input_data[] = {3};
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }

  // no function id
  {
    uint8_t input_data[] = {2};
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }

  // no length
  {
    uint8_t input_data[] = {2, 5};
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }

  // no flags
  {
    uint8_t input_data[] = {2, 5, 0, 0, 18};
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }

  // no next-ext offset
  {
    uint8_t input_data[] = {2, 5, 0, 0, 18, 0, 0};
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }

  // non-0 next-ext offset
  {
    uint8_t input_data[] = {2, 5, 0, 0, 18, 0, 0, 0, 0, 1};
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }

  // no xid
  {
    uint8_t input_data[] = {2, 5, 0, 0, 18, 0, 0, 0, 0, 0};
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }

  // no lang
  {
    uint8_t input_data[] = {
      2, 5, 0, 0, 18, 0, 0, 0, 0, 0, 0x12, 0x34, 0, 2};
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceAck(&stream));
  }
}


/**
 * test extracting the URL Entry, the easiest way is with a SrvDeReg message
 */
void PacketParserTest::testExtractURLEntry() {
  // no reserved field
  {
    uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0x3, 'A', 'C', 'N',
      // entry 1
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }

  // no lifetime
  {
    uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0x3, 'A', 'C', 'N',
      // entry 1
      0,
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }

  // no url length
  {
    uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 0x3, 'A', 'C', 'N',
      // entry 1
      0, 2, 0,
    };

    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }

  // invalid url
  {
    uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // scope list
      0, 0x3, 'A', 'C', 'N',
      // entry 1
      0, 0x12, 0x34, 0, 21,
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }

  // missing # of url auths
  {
    uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // scope list
      0, 0x3, 'A', 'C', 'N',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }

  // not enough url auths
  {
    uint8_t input_data[] = {
      2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      // scope list
      0, 0x3, 'A', 'C', 'N',
      // entry 1
      0, 0x12, 0x34, 0, 21,
      's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
      '1', '.', '1', '.', '1', '.', '1',
      1,  // # of URL auths
    };
    MemoryBuffer buffer(input_data, sizeof(input_data));
    BigEndianInputStream stream(&buffer);
    OLA_ASSERT_NULL(m_parser.UnpackServiceDeRegistration(&stream));
  }
}
