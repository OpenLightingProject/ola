/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * RDMCommandSerializerTest.cpp
 * Test fixture for the RDMCommandSerializer.
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <iomanip>
#include <memory>
#include <sstream>

#include "common/rdm/TestHelper.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/io/ByteString.h"
#include "ola/io/IOStack.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMPacket.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"
#include "ola/util/Utils.h"

using ola::io::IOStack;
using ola::io::ByteString;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryRequest;
using ola::rdm::RDMGetRequest;
using ola::rdm::RDMRequest;
using ola::rdm::RDMSetRequest;
using ola::rdm::UID;
using std::auto_ptr;

void UpdateChecksum(uint8_t *expected, unsigned int expected_length) {
  unsigned int checksum = ola::rdm::START_CODE;
  for (unsigned int i = 0 ; i < expected_length - 2; i++)
    checksum += expected[i];

  ola::utils::SplitUInt16(checksum, &expected[expected_length - 2],
                          &expected[expected_length - 1]);
}


class RDMCommandSerializerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMCommandSerializerTest);
  CPPUNIT_TEST(testGetRequest);
  CPPUNIT_TEST(testRequestOverrides);
  CPPUNIT_TEST(testPackWithStartCode);
  CPPUNIT_TEST(testDUB);
  CPPUNIT_TEST(testMuteRequest);
  CPPUNIT_TEST(testUnMuteRequest);
  CPPUNIT_TEST(testPackAndInflate);
  CPPUNIT_TEST(testIOStack);
  CPPUNIT_TEST_SUITE_END();

 public:
  RDMCommandSerializerTest()
    : m_source(1, 2),
      m_destination(3, 4) {
  }

  void setUp();

  void testGetRequest();
  void testRequestOverrides();
  void testPackWithStartCode();
  void testDUB();
  void testMuteRequest();
  void testUnMuteRequest();
  void testPackAndInflate();
  void testIOStack();

 private:
  UID m_source;
  UID m_destination;

  static uint8_t EXPECTED_GET_BUFFER[];
  static uint8_t EXPECTED_SET_BUFFER[];
  static uint8_t EXPECTED_DISCOVERY_REQUEST[];
  static uint8_t EXPECTED_MUTE_REQUEST[];
  static uint8_t EXPECTED_UNMUTE_REQUEST[];
  static uint8_t MUTE_RESPONSE[];
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMCommandSerializerTest);


uint8_t RDMCommandSerializerTest::EXPECTED_GET_BUFFER[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x20, 1, 40, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};

uint8_t RDMCommandSerializerTest::EXPECTED_SET_BUFFER[] = {
  1, 28,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x30, 1, 40, 4,  // command, param id, param data length
  0xa5, 0xa5, 0xa5, 0xa5,  // param data
  0, 0  // checksum, filled in below
};

uint8_t RDMCommandSerializerTest::EXPECTED_DISCOVERY_REQUEST[] = {
  1, 36,  // sub code & length
  255, 255, 255, 255, 255, 255,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 1, 12,  // command, param id, param data length
  1, 2, 0, 0, 3, 4,  // lower uid
  5, 6, 0, 0, 7, 8,  // upper uid
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandSerializerTest::EXPECTED_MUTE_REQUEST[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 2, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandSerializerTest::EXPECTED_UNMUTE_REQUEST[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 3, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};

/*
 * Fill in the checksums
 */
void RDMCommandSerializerTest::setUp() {
  UpdateChecksum(EXPECTED_GET_BUFFER, arraysize(EXPECTED_GET_BUFFER));
  UpdateChecksum(EXPECTED_SET_BUFFER, arraysize(EXPECTED_SET_BUFFER));
  UpdateChecksum(EXPECTED_DISCOVERY_REQUEST,
                 arraysize(EXPECTED_DISCOVERY_REQUEST));
  UpdateChecksum(EXPECTED_MUTE_REQUEST,
                 arraysize(EXPECTED_MUTE_REQUEST));
  UpdateChecksum(EXPECTED_UNMUTE_REQUEST,
                 arraysize(EXPECTED_UNMUTE_REQUEST));
}

void RDMCommandSerializerTest::testGetRequest() {
  RDMGetRequest request(m_source,
                        m_destination,
                        0,  // transaction #
                        1,  // port id
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length

  unsigned int length = RDMCommandSerializer::RequiredSize(request);
  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(request, data, &length));
  OLA_ASSERT_DATA_EQUALS(EXPECTED_GET_BUFFER,
                         arraysize(EXPECTED_GET_BUFFER),
                         data,
                         length);

  ByteString output;
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(request, &output));
  OLA_ASSERT_DATA_EQUALS(EXPECTED_GET_BUFFER,
                         arraysize(EXPECTED_GET_BUFFER),
                         output.data(), output.length());

  delete[] data;
}

void RDMCommandSerializerTest::testRequestOverrides() {
  RDMRequest::OverrideOptions options;
  options.SetMessageLength(10);
  options.SetChecksum(999);
  options.sub_start_code = 5;
  options.message_count = 9;

  RDMGetRequest request(m_source,
                        m_destination,
                        0,  // transaction #
                        1,  // port id
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0,  // data length
                        options);

  const uint8_t expected_data[] = {
    5, 10,  // sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 9, 0, 10,  // transaction, port id, msg count & sub device
    0x20, 1, 40, 0,  // command, param id, param data length
    0x3, 0xe7  // checksum,
  };

  unsigned int length = RDMCommandSerializer::RequiredSize(request);
  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(request, data, &length));
  OLA_ASSERT_DATA_EQUALS(expected_data, arraysize(expected_data),
                         data,
                         length);
  delete[] data;

  ByteString output;
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(request, &output));
  OLA_ASSERT_DATA_EQUALS(expected_data,
                         arraysize(expected_data),
                         output.data(), output.length());
}

void RDMCommandSerializerTest::testPackWithStartCode() {
  RDMGetRequest request(m_source,
                        m_destination,
                        0,  // transaction #
                        1,  // port id
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length

  const uint8_t expected_data[] = {
    0xcc, 1, 24,  // start code, sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x20, 1, 40, 0,  // command, param id, param data length
    0x1, 0x43  // checksum,
  };

  ByteString output;
  OLA_ASSERT_TRUE(RDMCommandSerializer::PackWithStartCode(request, &output));
  OLA_ASSERT_DATA_EQUALS(expected_data,
                         arraysize(expected_data),
                         output.data(), output.length());
}

void RDMCommandSerializerTest::testDUB() {
  UID lower(0x0102, 0x0304);
  UID upper(0x0506, 0x0708);

  auto_ptr<RDMDiscoveryRequest> request(
      NewDiscoveryUniqueBranchRequest(m_source, lower, upper, 1));

  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());
  OLA_ASSERT_TRUE(request->IsDUB());

  // test pack
  unsigned int length = RDMCommandSerializer::RequiredSize(*request);
  OLA_ASSERT_EQ(37u, length);

  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request, data, &length));

  OLA_ASSERT_DATA_EQUALS(EXPECTED_DISCOVERY_REQUEST,
                         arraysize(EXPECTED_DISCOVERY_REQUEST),
                         data,
                         length);

  ByteString output;
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request.get(), &output));
  OLA_ASSERT_DATA_EQUALS(EXPECTED_DISCOVERY_REQUEST,
                         arraysize(EXPECTED_DISCOVERY_REQUEST),
                         output.data(), output.length());
  delete[] data;
}

void RDMCommandSerializerTest::testMuteRequest() {
  auto_ptr<RDMDiscoveryRequest> request(
      NewMuteRequest(m_source, m_destination, 1));

  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  // test pack
  unsigned int length = RDMCommandSerializer::RequiredSize(*request);
  OLA_ASSERT_EQ(25u, length);
  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request, data, &length));

  OLA_ASSERT_DATA_EQUALS(EXPECTED_MUTE_REQUEST,
                         arraysize(EXPECTED_MUTE_REQUEST), data, length);

  ByteString output;
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request.get(), &output));
  OLA_ASSERT_DATA_EQUALS(EXPECTED_MUTE_REQUEST,
                         arraysize(EXPECTED_MUTE_REQUEST),
                         output.data(), output.length());
  delete[] data;
}

void RDMCommandSerializerTest::testUnMuteRequest() {
  auto_ptr<RDMDiscoveryRequest> request(
      NewUnMuteRequest(m_source, m_destination, 1));

  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  // test pack
  unsigned int length = RDMCommandSerializer::RequiredSize(*request);
  OLA_ASSERT_EQ(25u, length);
  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request, data, &length));

  OLA_ASSERT_DATA_EQUALS(EXPECTED_UNMUTE_REQUEST,
                         arraysize(EXPECTED_UNMUTE_REQUEST),
                         data,
                         length);

  ByteString output;
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request.get(), &output));
  OLA_ASSERT_DATA_EQUALS(EXPECTED_UNMUTE_REQUEST,
                         arraysize(EXPECTED_UNMUTE_REQUEST),
                         output.data(), output.length());
  delete[] data;
}

void RDMCommandSerializerTest::testPackAndInflate() {
  RDMGetRequest get_command(m_source,
                            m_destination,
                            99,  // transaction #
                            1,  // port id
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  unsigned int length = RDMCommandSerializer::RequiredSize(get_command);
  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(get_command, data, &length));

  auto_ptr<RDMRequest> command(RDMRequest::InflateFromData(data, length));
  OLA_ASSERT_NOT_NULL(command.get());

  OLA_ASSERT_EQ(m_source, command->SourceUID());
  OLA_ASSERT_EQ(m_destination, command->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 99, command->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, command->PortId());
  OLA_ASSERT_EQ((uint8_t) 0, command->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, command->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND, command->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, command->ParamId());
  OLA_ASSERT_EQ(static_cast<const uint8_t*>(NULL), command->ParamData());
  OLA_ASSERT_EQ(0u, command->ParamDataSize());
  OLA_ASSERT_EQ(25u, RDMCommandSerializer::RequiredSize(*command));
  delete[] data;
}


/*
 * Test writing to an IOStack works.
 */
void RDMCommandSerializerTest::testIOStack() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest command(source,
                        destination,
                        0,  // transaction #
                        1,  // port id
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length

  IOStack stack;
  OLA_ASSERT_TRUE(RDMCommandSerializer::Write(command, &stack));

  unsigned int raw_command_size = stack.Size();
  OLA_ASSERT_EQ(raw_command_size, RDMCommandSerializer::RequiredSize(command));
  uint8_t raw_command[raw_command_size];
  OLA_ASSERT_EQ(raw_command_size, stack.Read(raw_command, raw_command_size));
  OLA_ASSERT_EQ(0u, stack.Size());

  OLA_ASSERT_DATA_EQUALS(EXPECTED_GET_BUFFER, arraysize(EXPECTED_GET_BUFFER),
                         raw_command, raw_command_size);

  // now try a command with data
  uint32_t data_value = 0xa5a5a5a5;
  RDMSetRequest command2(source,
                         destination,
                         0,  // transaction #
                         1,  // port id
                         10,  // sub device
                         296,  // param id
                         reinterpret_cast<uint8_t*>(&data_value),  // data
                         sizeof(data_value));  // data length

  OLA_ASSERT_EQ(29u, RDMCommandSerializer::RequiredSize(command2));
  OLA_ASSERT_TRUE(RDMCommandSerializer::Write(command2, &stack));

  raw_command_size = stack.Size();
  OLA_ASSERT_EQ(raw_command_size, RDMCommandSerializer::RequiredSize(command2));
  uint8_t raw_command2[raw_command_size];
  OLA_ASSERT_EQ(raw_command_size, stack.Read(raw_command2, raw_command_size));
  OLA_ASSERT_EQ(0u, stack.Size());

  OLA_ASSERT_DATA_EQUALS(EXPECTED_SET_BUFFER, arraysize(EXPECTED_SET_BUFFER),
                         raw_command2, raw_command_size);
}
