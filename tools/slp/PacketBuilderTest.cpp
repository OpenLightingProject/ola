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
 * PacketBuilderTester.cpp
 * Test fixture for the SLPPacketBuilder class
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/BigEndianStream.h"
#include "ola/network/IPV4Address.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/URLEntry.h"

using ola::io::BigEndianOutputStream;
using ola::io::IOQueue;
using ola::network::IPV4Address;
using ola::slp::SLPPacketBuilder;
using ola::slp::URLEntries;
using ola::slp::URLEntry;
using ola::testing::ASSERT_DATA_EQUALS;
using std::set;
using std::string;

class PacketBuilderTest: public CppUnit::TestFixture {
  public:
    PacketBuilderTest() : output(&ioqueue) {}

    CPPUNIT_TEST_SUITE(PacketBuilderTest);
    CPPUNIT_TEST(testBuildServiceRequest);
    CPPUNIT_TEST(testBuildServiceReply);
    CPPUNIT_TEST(testBuildServiceRegistration);
    CPPUNIT_TEST(testBuildDAAdvert);
    CPPUNIT_TEST(testBuildServiceAck);
    CPPUNIT_TEST_SUITE_END();

    void testBuildServiceRequest();
    void testBuildServiceReply();
    void testBuildServiceRegistration();
    void testBuildDAAdvert();
    void testBuildServiceAck();

  public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
      xid = 0x1234;
    }

  private:
    IOQueue ioqueue;
    BigEndianOutputStream output;
    ola::slp::xid_t xid;

    uint8_t* WriteToBuffer(IOQueue *data, unsigned int *size) {
      *size = data->Size();
      uint8_t *data_buffer = new uint8_t[*size];
      *size = data->Peek(data_buffer, *size);
      data->Pop(*size);
      return data_buffer;
    }

    void DumpData(uint8_t *data, unsigned int size) {
      for (unsigned int i = 0; i < size; ++i)
        OLA_INFO << i << ": 0x" << std::hex << static_cast<int>(data[i]) <<
          " : " << data[i];
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(PacketBuilderTest);


/*
 * Check that BuildServiceRequest() works.
 */
void PacketBuilderTest::testBuildServiceRequest() {
  set<IPV4Address> pr_list;
  IPV4Address first_ip, second_ip;
  CPPUNIT_ASSERT(IPV4Address::FromString("1.1.1.2", &first_ip));
  CPPUNIT_ASSERT(IPV4Address::FromString("1.1.1.8", &second_ip));
  pr_list.insert(first_ip);
  pr_list.insert(second_ip);

  set<string> scope_list;
  scope_list.insert("ACN");
  scope_list.insert("MYORG,");

  SLPPacketBuilder::BuildServiceRequest(&output, xid, pr_list,
                                        "rdmnet-device", scope_list);
  CPPUNIT_ASSERT_EQUAL(66u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 1, 0, 0, 66, 0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1', '.',
    '8',  // pr-llist
    0, 13, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
    0, 0xc, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G', '\\', '2', 'c',
    0, 0,  // pred string
    0, 0,  // SPI string
  };

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/**
 * Check that BuildServiceReply() works.
 */
void PacketBuilderTest::testBuildServiceReply() {
  URLEntry entry1("service:foo://1.1.1.1", 0x1234);
  URLEntry entry2("service:foo://1.1.1.10", 0x5678);
  URLEntries url_entries;
  url_entries.insert(entry1);
  url_entries.insert(entry2);

  SLPPacketBuilder::BuildServiceReply(&output, xid, 12, url_entries);
  CPPUNIT_ASSERT_EQUAL(75u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 2, 0, 0, 0x4b, 0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
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

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/**
 * Check that BuildServiceRegistration() works.
 */
void PacketBuilderTest::testBuildServiceRegistration() {
  URLEntry entry1("service:foo://1.1.1.1", 0x1234);
  set<string> scope_list;
  scope_list.insert("ACN");
  scope_list.insert("MYORG,");

  SLPPacketBuilder::BuildServiceRegistration(&output, xid, true, entry1,
                                             "foo", scope_list);
  CPPUNIT_ASSERT_EQUAL(65u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 3, 0, 0, 0x41, 0x40, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    // entry 1
    0, 0x12, 0x34, 0, 21,
    's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
    '1', '.', '1', '.', '1', '.', '1',
    0,  // # of auth blocks
    0, 3, 'f', 'o', 'o',  // service-type
    0, 0xc, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G', '\\', '2', 'c',
    0, 0,
    0  // attr auths
  };

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;

  // now test a re-registration
  SLPPacketBuilder::BuildServiceRegistration(&output, xid + 1, false, entry1,
                                             "foo", scope_list);
  CPPUNIT_ASSERT_EQUAL(65u, ioqueue.Size());

  output_data = WriteToBuffer(&ioqueue, &data_size);
  expected_data[5] = 0;
  expected_data[11] = 0x35;
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/*
 * Check that BuildDAAdvert() works.
 */
void PacketBuilderTest::testBuildDAAdvert() {
  set<string> scope_list;
  scope_list.insert("ACN");
  scope_list.insert("MYORG,");

  SLPPacketBuilder::BuildDAAdvert(&output, xid, true, 12, 0x12345678,
      "service:foo", scope_list);
  CPPUNIT_ASSERT_EQUAL(54u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 8, 0, 0, 0x36, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 0,  // error code is zeroed out if multicast
    0x12, 0x34, 0x56, 0x78,  // boot timestamp
    0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // service
    0, 0xc, 'A', 'C', 'N', ',', 'M', 'Y', 'O', 'R', 'G', '\\', '2', 'c',
    0, 0,  // attr list
    0, 0,  // SPI list
    0  // auth blocks
  };

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;

  // try with a non-multicast packet
  SLPPacketBuilder::BuildDAAdvert(&output, xid, false, 12, 0x12345678,
      "service:foo", scope_list);
  CPPUNIT_ASSERT_EQUAL(54u, ioqueue.Size());
  output_data = WriteToBuffer(&ioqueue, &data_size);
  expected_data[5] = 0;
  expected_data[17] = 0xc;
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/*
 * Check that BuildServiceAck() works.
 */
void PacketBuilderTest::testBuildServiceAck() {
  SLPPacketBuilder::BuildServiceAck(&output, xid, 0x5678);
  CPPUNIT_ASSERT_EQUAL(18u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 5, 0, 0, 18, 0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0x56, 0x78
  };
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}
