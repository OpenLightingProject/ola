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
 * PacketBuilderTest.cpp
 * Test fixture for the SLPPacketBuilder class
 * Copyright (C) 2012 Simon Newton
 */

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/BigEndianStream.h"
#include "ola/network/IPV4Address.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPPacketBuilder.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/ServiceEntry.h"
#include "tools/slp/ScopeSet.h"
#include "tools/slp/URLEntry.h"

using ola::io::BigEndianOutputStream;
using ola::io::IOQueue;
using ola::network::IPV4Address;
using ola::slp::SLPPacketBuilder;
using ola::slp::ScopeSet;
using ola::slp::ServiceEntry;
using ola::slp::URLEntries;
using ola::slp::URLEntry;
using ola::testing::ASSERT_DATA_EQUALS;
using std::set;
using std::string;
using std::vector;

class PacketBuilderTest: public CppUnit::TestFixture {
  public:
    PacketBuilderTest() : output(&ioqueue) {}

    CPPUNIT_TEST_SUITE(PacketBuilderTest);
    CPPUNIT_TEST(testBuildServiceRequest);
    CPPUNIT_TEST(testBuildServiceReply);
    CPPUNIT_TEST(testBuildServiceRegistration);
    CPPUNIT_TEST(testBuildServiceDeRegistration);
    CPPUNIT_TEST(testBuildDAAdvert);
    CPPUNIT_TEST(testBuildServiceTypeRequest);
    CPPUNIT_TEST(testBuildServiceTypeReply);
    CPPUNIT_TEST(testBuildSAAdvert);
    CPPUNIT_TEST(testBuildServiceAck);
    CPPUNIT_TEST(testBuildError);
    CPPUNIT_TEST_SUITE_END();

    void testBuildServiceRequest();
    void testBuildServiceReply();
    void testBuildServiceRegistration();
    void testBuildServiceDeRegistration();
    void testBuildDAAdvert();
    void testBuildServiceTypeRequest();
    void testBuildServiceTypeReply();
    void testBuildSAAdvert();
    void testBuildServiceAck();
    void testBuildError();

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
  OLA_ASSERT_TRUE(IPV4Address::FromString("1.1.1.2", &first_ip));
  OLA_ASSERT_TRUE(IPV4Address::FromString("1.1.1.8", &second_ip));
  pr_list.insert(first_ip);
  pr_list.insert(second_ip);
  ScopeSet scopes("ACN,MYORG\\2c");

  SLPPacketBuilder::BuildServiceRequest(&output, xid, true, pr_list,
                                        "rdmnet-device", scopes);
  OLA_ASSERT_EQ(66u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 1, 0, 0, 66, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1', '.',
    '8',  // pr-llist
    0, 13, 'r', 'd', 'm', 'n', 'e', 't', '-', 'd', 'e', 'v', 'i', 'c', 'e',
    0, 0xc, 'a', 'c', 'n', ',', 'm', 'y', 'o', 'r', 'g', '\\', '2', 'c',
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
  url_entries.push_back(entry1);
  url_entries.push_back(entry2);

  SLPPacketBuilder::BuildServiceReply(&output, xid, 12, url_entries);
  OLA_ASSERT_EQ(75u, ioqueue.Size());

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
  ScopeSet service_scopes("ACN,MYORG,FOO");
  ScopeSet scopes("ACN,MYORG\\2c");
  ServiceEntry service_entry(service_scopes, "service:foo://1.1.1.1", 0x1234);

  SLPPacketBuilder::BuildServiceRegistration(&output, xid, true, scopes,
                                             service_entry);
  OLA_ASSERT_EQ(73u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 3, 0, 0, 0x49, 0x40, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    // entry 1
    0, 0x12, 0x34, 0, 21,
    's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
    '1', '.', '1', '.', '1', '.', '1',
    0,  // # of auth blocks
    0, 0xb, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',
    0, 0xc, 'a', 'c', 'n', ',', 'm', 'y', 'o', 'r', 'g', '\\', '2', 'c',
    0, 0,
    0,  // attr auths
  };

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;

  // now test a re-registration
  SLPPacketBuilder::BuildServiceRegistration(&output, xid + 1, false, scopes,
      service_entry);
  OLA_ASSERT_EQ(73u, ioqueue.Size());

  output_data = WriteToBuffer(&ioqueue, &data_size);
  expected_data[5] = 0;
  expected_data[11] = 0x35;
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/**
 * Check that BuildServiceDeRegistration() works.
 */
void PacketBuilderTest::testBuildServiceDeRegistration() {
  ScopeSet service_scopes("ACN,MYORG,FOO");
  ScopeSet scopes("ACN,MYORG");
  ServiceEntry service_entry(service_scopes, "service:foo://1.1.1.1", 0x1234);

  SLPPacketBuilder::BuildServiceDeRegistration(&output, xid, scopes,
                                               service_entry);

  OLA_ASSERT_EQ(56u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 4, 0, 0, 0x38, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    // scope list
    0, 0x9, 'a', 'c', 'n', ',', 'm', 'y', 'o', 'r', 'g',
    // entry 1
    0, 0x12, 0x34, 0, 21,
    's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o', ':', '/', '/',
    '1', '.', '1', '.', '1', '.', '1',
    0,  // # of URL auths
    0, 0  // tag list length
  };

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/*
 * Check that BuildDAAdvert() works.
 */
void PacketBuilderTest::testBuildDAAdvert() {
  ScopeSet scopes("ACN,MYORG\\2c");

  SLPPacketBuilder::BuildDAAdvert(&output, xid, true, 12, 0x12345678,
      "service:foo", scopes);
  OLA_ASSERT_EQ(54u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 8, 0, 0, 0x36, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 0,  // error code is zeroed out if multicast
    0x12, 0x34, 0x56, 0x78,  // boot timestamp
    0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // service
    0, 0xc, 'a', 'c', 'n', ',', 'm', 'y', 'o', 'r', 'g', '\\', '2', 'c',
    0, 0,  // attr list
    0, 0,  // SPI list
    0  // auth blocks
  };

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;

  // try with a non-multicast packet
  SLPPacketBuilder::BuildDAAdvert(&output, xid, false, 12, 0x12345678,
      "service:foo", scopes);
  OLA_ASSERT_EQ(54u, ioqueue.Size());
  output_data = WriteToBuffer(&ioqueue, &data_size);
  expected_data[5] = 0;
  // update error code
  expected_data[17] = 0xc;
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/*
 * Check that BuildServiceTypeRequest() works.
 */
void PacketBuilderTest::testBuildServiceTypeRequest() {
  set<IPV4Address> pr_list;
  IPV4Address first_ip, second_ip;
  OLA_ASSERT_TRUE(IPV4Address::FromString("1.1.1.2", &first_ip));
  OLA_ASSERT_TRUE(IPV4Address::FromString("1.1.1.8", &second_ip));
  pr_list.insert(first_ip);
  pr_list.insert(second_ip);

  ScopeSet scopes("ACN,MYORG\\2c");

  {
    // request for all service-types
    SLPPacketBuilder::BuildAllServiceTypeRequest(&output, xid, true, pr_list,
                                                 scopes);
    OLA_ASSERT_EQ(49u, ioqueue.Size());

    unsigned int data_size;
    uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
    uint8_t expected_data[] = {
      2, 9, 0, 0, 0x31, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1',
      '.', '8',  // pr-list
      0xff, 0xff,   // naming auth length
      0, 0xc, 'a', 'c', 'n', ',', 'm', 'y', 'o', 'r', 'g', '\\', '2', 'c'
    };

    ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                       output_data, data_size);
    delete[] output_data;
  }

  {
    // request for IANA types
    SLPPacketBuilder::BuildServiceTypeRequest(&output, xid, true, pr_list, "",
                                              scopes);
    OLA_ASSERT_EQ(49u, ioqueue.Size());

    unsigned int data_size;
    uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
    uint8_t expected_data[] = {
      2, 9, 0, 0, 0x31, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1',
      '.', '8',  // pr-list
      0x0, 0x0,   // naming auth length
      0, 0xc, 'a', 'c', 'n', ',', 'm', 'y', 'o', 'r', 'g', '\\', '2', 'c'
    };

    ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                       output_data, data_size);
    delete[] output_data;
  }

  {
    // request for a specific naming auth
    SLPPacketBuilder::BuildServiceTypeRequest(&output, xid, true, pr_list,
                                              "foo", scopes);
    OLA_ASSERT_EQ(52u, ioqueue.Size());

    unsigned int data_size;
    uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
    uint8_t expected_data[] = {
      2, 9, 0, 0, 0x34, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
      0, 15, '1', '.', '1', '.', '1', '.', '2', ',', '1', '.', '1', '.', '1',
      '.', '8',  // pr-list
      0x0, 0x3, 'f', 'o', 'o',  // naming auth length
      0, 0xc, 'a', 'c', 'n', ',', 'm', 'y', 'o', 'r', 'g', '\\', '2', 'c'
    };

    ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                       output_data, data_size);
    delete[] output_data;
  }
}


/*
 * Check that BuildServiceTypeReply() works.
 */
void PacketBuilderTest::testBuildServiceTypeReply() {
  ScopeSet scopes("ACN,MYORG\\2c");
  vector<string> service_types;
  service_types.push_back("lpr");
  service_types.push_back("foo,bar");  // check escaping

  SLPPacketBuilder::BuildServiceTypeReply(&output, xid, 0, service_types);
  OLA_ASSERT_EQ(33u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 10, 0, 0, 0x21, 0x0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 0,  // error code
    0, 13,
    'l', 'p', 'r', ',', 'f', 'o', 'o', '\\', '2', 'c', 'b', 'a', 'r'
  };

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/*
 * Check that BuildSAAdvert() works.
 */
void PacketBuilderTest::testBuildSAAdvert() {
  ScopeSet scopes("ACN,MYORG\\2c");

  SLPPacketBuilder::BuildSAAdvert(&output, xid, true, "service:foo", scopes);
  OLA_ASSERT_EQ(46u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 11, 0, 0, 0x2e, 0x20, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 11, 's', 'e', 'r', 'v', 'i', 'c', 'e', ':', 'f', 'o', 'o',  // url
    0, 0xc, 'a', 'c', 'n', ',', 'm', 'y', 'o', 'r', 'g', '\\', '2', 'c',
    0, 0,  // attr list
    0  // auth blocks
  };

  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;

  // try with a non-multicast packet
  SLPPacketBuilder::BuildSAAdvert(&output, xid, false, "service:foo", scopes);
  OLA_ASSERT_EQ(46u, ioqueue.Size());
  output_data = WriteToBuffer(&ioqueue, &data_size);
  expected_data[5] = 0;
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;
}


/*
 * Check that BuildServiceAck() works.
 */
void PacketBuilderTest::testBuildServiceAck() {
  SLPPacketBuilder::BuildServiceAck(&output, xid, 0x5678);
  OLA_ASSERT_EQ(18u, ioqueue.Size());

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


/*
 * Check that BuildError() works.
 */
void PacketBuilderTest::testBuildError() {
  SLPPacketBuilder::BuildError(&output, ola::slp::SERVICE_REPLY, xid,
                               ola::slp::LANGUAGE_NOT_SUPPORTED);
  OLA_ASSERT_EQ(18u, ioqueue.Size());

  unsigned int data_size;
  uint8_t *output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data[] = {
    2, 2, 0, 0, 18, 0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 1
  };
  ASSERT_DATA_EQUALS(__LINE__, expected_data, sizeof(expected_data),
                     output_data, data_size);
  delete[] output_data;

  // try a different function-id
  SLPPacketBuilder::BuildError(&output, ola::slp::DA_ADVERTISEMENT, xid,
                               ola::slp::INTERNAL_ERROR);
  OLA_ASSERT_EQ(18u, ioqueue.Size());

  output_data = WriteToBuffer(&ioqueue, &data_size);
  uint8_t expected_data2[] = {
    2, 8, 0, 0, 18, 0, 0, 0, 0, 0, 0x12, 0x34, 0, 2, 'e', 'n',
    0, 10
  };
  ASSERT_DATA_EQUALS(__LINE__, expected_data2, sizeof(expected_data2),
                     output_data, data_size);
  delete[] output_data;
}
