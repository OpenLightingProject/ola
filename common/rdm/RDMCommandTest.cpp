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
 * RDMCommandTest.cpp
 * Test fixture for the RDMCommand classes
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#include "common/rdm/TestHelper.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/io/ByteString.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RDMPacket.h"
#include "ola/rdm/UID.h"
#include "ola/util/Utils.h"
#include "ola/testing/TestUtils.h"

using ola::io::ByteString;
using ola::rdm::RDMCommand;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryRequest;
using ola::rdm::RDMDiscoveryResponse;
using ola::rdm::RDMGetRequest;
using ola::rdm::RDMGetResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::RDMSetResponse;
using ola::rdm::UID;
using ola::utils::SplitUInt16;
using std::auto_ptr;
using std::ostringstream;
using std::string;

/*
 * Calculate a checksum for a packet and update it
 */
void UpdateChecksum(uint8_t *expected, unsigned int expected_length) {
  unsigned int checksum = ola::rdm::START_CODE;
  for (unsigned int i = 0 ; i < expected_length - 2; i++) {
    checksum += expected[i];
  }

  SplitUInt16(checksum, &expected[expected_length - 2],
              &expected[expected_length - 1]);
}

void UpdateChecksum(ByteString *data) {
  unsigned int checksum = ola::rdm::START_CODE;
  for (unsigned int i = 0 ; i < data->size() - 2; i++) {
    checksum += (*data)[i];
  }

  SplitUInt16(checksum, &data->at(data->size() - 2),
              &data->at(data->size() - 1));
}


class RDMCommandTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMCommandTest);
  CPPUNIT_TEST(testGetRequestCreation);
  CPPUNIT_TEST(testGetWithExtraData);
  CPPUNIT_TEST(testSetWithParamData);
  CPPUNIT_TEST(testRequestOverrides);
  CPPUNIT_TEST(testRequestMutation);
  CPPUNIT_TEST(testRequestInflation);
  CPPUNIT_TEST(testResponseMutation);
  CPPUNIT_TEST(testResponseInflation);
  CPPUNIT_TEST(testNackWithReason);
  CPPUNIT_TEST(testGetResponseFromData);
  CPPUNIT_TEST(testCombineResponses);
  CPPUNIT_TEST(testDiscoveryCommand);
  CPPUNIT_TEST(testMuteRequest);
  CPPUNIT_TEST(testUnMuteRequest);
  CPPUNIT_TEST(testCommandInflation);
  CPPUNIT_TEST(testDiscoveryResponseInflation);
  CPPUNIT_TEST_SUITE_END();

 public:
  RDMCommandTest()
    : m_source(1, 2),
      m_destination(3, 4) {
  }

  void setUp();

  void testGetRequestCreation();
  void testGetWithExtraData();
  void testSetWithParamData();
  void testRequestOverrides();
  void testRequestMutation();
  void testRequestInflation();
  void testResponseMutation();
  void testResponseInflation();
  void testNackWithReason();
  void testGetResponseFromData();
  void testCombineResponses();
  void testDiscoveryCommand();
  void testMuteRequest();
  void testUnMuteRequest();
  void testCommandInflation();
  void testDiscoveryResponseInflation();

 private:
  UID m_source;
  UID m_destination;

  static uint8_t EXPECTED_GET_BUFFER[];
  static uint8_t EXPECTED_SET_BUFFER[];
  static uint8_t EXPECTED_GET_RESPONSE_BUFFER[];
  static uint8_t EXPECTED_DISCOVERY_REQUEST[];
  static uint8_t EXPECTED_MUTE_REQUEST[];
  static uint8_t EXPECTED_UNMUTE_REQUEST[];
  static uint8_t MUTE_RESPONSE[];
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMCommandTest);


uint8_t RDMCommandTest::EXPECTED_GET_BUFFER[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x20, 1, 40, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};

uint8_t RDMCommandTest::EXPECTED_SET_BUFFER[] = {
  1, 28,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x30, 1, 40, 4,  // command, param id, param data length
  0xa5, 0xa5, 0xa5, 0xa5,  // param data
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandTest::EXPECTED_GET_RESPONSE_BUFFER[] = {
  1, 28,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  0x21, 1, 40, 4,  // command, param id, param data length
  0x5a, 0x5a, 0x5a, 0x5a,  // param data
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandTest::EXPECTED_DISCOVERY_REQUEST[] = {
  1, 36,  // sub code & length
  255, 255, 255, 255, 255, 255,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 1, 12,  // command, param id, param data length
  1, 2, 0, 0, 3, 4,  // lower uid
  5, 6, 0, 0, 7, 8,  // upper uid
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandTest::EXPECTED_MUTE_REQUEST[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 2, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};


uint8_t RDMCommandTest::EXPECTED_UNMUTE_REQUEST[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  1, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
  0x10, 0, 3, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};

uint8_t RDMCommandTest::MUTE_RESPONSE[] = {
  1, 24,  // sub code & length
  0, 1, 0, 0, 0, 2,   // dst uid
  0, 3, 0, 0, 0, 4,   // src uid
  1, 1, 0, 0, 0,  // transaction, response type, msg count & sub device
  0x11, 0, 2, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};


/*
 * Fill in the checksums
 */
void RDMCommandTest::setUp() {
  UpdateChecksum(EXPECTED_GET_BUFFER, sizeof(EXPECTED_GET_BUFFER));
  UpdateChecksum(EXPECTED_SET_BUFFER, sizeof(EXPECTED_SET_BUFFER));
  UpdateChecksum(EXPECTED_GET_RESPONSE_BUFFER,
                 sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  UpdateChecksum(EXPECTED_DISCOVERY_REQUEST,
                 sizeof(EXPECTED_DISCOVERY_REQUEST));
  UpdateChecksum(EXPECTED_MUTE_REQUEST,
                 sizeof(EXPECTED_MUTE_REQUEST));
  UpdateChecksum(EXPECTED_UNMUTE_REQUEST,
                 sizeof(EXPECTED_UNMUTE_REQUEST));
  UpdateChecksum(MUTE_RESPONSE, sizeof(MUTE_RESPONSE));
}


void RDMCommandTest::testGetRequestCreation() {
  RDMGetRequest command(m_source,
                        m_destination,
                        0,  // transaction #
                        1,  // port id
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, command.SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 24, command.MessageLength());
  OLA_ASSERT_EQ(m_source, command.SourceUID());
  OLA_ASSERT_EQ(m_destination, command.DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, command.TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, command.PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 1, command.PortId());
  OLA_ASSERT_EQ((uint8_t) 0, command.MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, command.SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND, command.CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, command.ParamId());
  OLA_ASSERT_NULL(command.ParamData());
  OLA_ASSERT_EQ(0u, command.ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 0, command.Checksum(0));
  OLA_ASSERT_FALSE(command.IsDUB());

  // Test ToString() and <<.
  ostringstream str;
  str << command;
  OLA_ASSERT_FALSE(str.str().empty());
  OLA_ASSERT_EQ(str.str(), command.ToString());
}

void RDMCommandTest::testGetWithExtraData() {
  uint8_t data[232];
  memset(data, 0, arraysize(data));
  RDMGetRequest command(m_source,
                        m_destination,
                        1,  // transaction #
                        1,  // port id
                        10,  // sub device
                        123,  // param id
                        data,  // data
                        arraysize(data));  // data length

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, command.SubStartCode());
  OLA_ASSERT_EQ(m_source, command.SourceUID());
  OLA_ASSERT_EQ(m_destination, command.DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 1, command.TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, command.PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 1, command.PortId());
  OLA_ASSERT_EQ((uint8_t) 0, command.MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, command.SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND, command.CommandClass());
  OLA_ASSERT_EQ((uint16_t) 123, command.ParamId());
  OLA_ASSERT_DATA_EQUALS(reinterpret_cast<const uint8_t*>(&data),
                         arraysize(data), command.ParamData(),
                         command.ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 0, command.Checksum(0));
  OLA_ASSERT_FALSE(command.IsDUB());
}

void RDMCommandTest::testSetWithParamData() {
  uint32_t data_value = 0xa5a5a5a5;
  RDMSetRequest command(m_source,
                         m_destination,
                         3,  // transaction #
                         1,  // port id
                         13,  // sub device
                         296,  // param id
                         reinterpret_cast<uint8_t*>(&data_value),  // data
                         sizeof(data_value));  // data length

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, command.SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 28, command.MessageLength());
  OLA_ASSERT_EQ(m_source, command.SourceUID());
  OLA_ASSERT_EQ(m_destination, command.DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 3, command.TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, command.PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 1, command.PortId());
  OLA_ASSERT_EQ((uint8_t) 0, command.MessageCount());
  OLA_ASSERT_EQ((uint16_t) 13, command.SubDevice());
  OLA_ASSERT_EQ(RDMCommand::SET_COMMAND, command.CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, command.ParamId());
  OLA_ASSERT_DATA_EQUALS(reinterpret_cast<const uint8_t*>(&data_value),
                         sizeof(data_value), command.ParamData(),
                         command.ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 0, command.Checksum(0));
  OLA_ASSERT_FALSE(command.IsDUB());
}

void RDMCommandTest::testRequestOverrides() {
  RDMRequest::OverrideOptions options;
  options.SetMessageLength(10);
  options.SetChecksum(999);
  options.sub_start_code = 5;
  options.message_count = 9;

  RDMGetRequest command(m_source,
                        m_destination,
                        0,  // transaction #
                        1,  // port id
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0,  // data length
                        options);

  OLA_ASSERT_EQ((uint8_t) 5, command.SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 10, command.MessageLength());
  OLA_ASSERT_EQ(m_source, command.SourceUID());
  OLA_ASSERT_EQ(m_destination, command.DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, command.TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, command.PortId());
  OLA_ASSERT_EQ((uint8_t) 9, command.MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, command.SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND, command.CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, command.ParamId());
  OLA_ASSERT_NULL(command.ParamData());
  OLA_ASSERT_EQ(0u, command.ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 999u, command.Checksum(0));
}

void RDMCommandTest::testRequestMutation() {
  UID new_uid(5, 6);

  RDMGetRequest command(m_source,
                        m_destination,
                        0,  // transaction #
                        1,  // port id
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);

  command.SetSourceUID(new_uid);
  command.SetTransactionNumber(99);
  command.SetPortId(5);

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, command.SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 24, command.MessageLength());
  OLA_ASSERT_EQ(new_uid, command.SourceUID());
  OLA_ASSERT_EQ(m_destination, command.DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 99, command.TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 5, command.PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 5, command.PortId());
  OLA_ASSERT_EQ((uint8_t) 0, command.MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, command.SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND, command.CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, command.ParamId());
  OLA_ASSERT_NULL(command.ParamData());
  OLA_ASSERT_EQ(0u, command.ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 0, command.Checksum(0));
  OLA_ASSERT_FALSE(command.IsDUB());
}

/*
 * Test that we can inflate RDM request messages correctly
 */
void RDMCommandTest::testRequestInflation() {
  UID source(1, 2);
  UID destination(3, 4);
  auto_ptr<RDMRequest> command(RDMRequest::InflateFromData(NULL, 10));
  OLA_ASSERT_NULL(command.get());

  // now try a proper command but with no length
  command.reset(RDMRequest::InflateFromData(EXPECTED_GET_BUFFER, 0));
  OLA_ASSERT_NULL(command.get());

  // Try a valid command
  command.reset(RDMRequest::InflateFromData(EXPECTED_GET_BUFFER,
                                            arraysize(EXPECTED_GET_BUFFER)));
  OLA_ASSERT_NOT_NULL(command.get());

  RDMGetRequest expected_command(source,
                                 destination,
                                 0,  // transaction #
                                 1,  // port id
                                 10,  // sub device
                                 296,  // param id
                                 NULL,  // data
                                 0);  // data length
  OLA_ASSERT_TRUE(expected_command == *command);

  // An invalid Command class
  ByteString invalid_command(EXPECTED_GET_BUFFER,
                             arraysize(EXPECTED_GET_BUFFER));
  invalid_command[19] = 0x44;
  command.reset(RDMRequest::InflateFromData(invalid_command.data(),
                                            invalid_command.size()));
  OLA_ASSERT_NULL(command.get());

  // A SET request
  command.reset(RDMRequest::InflateFromData(EXPECTED_SET_BUFFER,
                                            arraysize(EXPECTED_SET_BUFFER)));
  OLA_ASSERT_NOT_NULL(command.get());
  uint8_t expected_data[] = {0xa5, 0xa5, 0xa5, 0xa5};
  OLA_ASSERT_DATA_EQUALS(expected_data, arraysize(expected_data),
                         command->ParamData(), command->ParamDataSize());

  // Change the param length and make sure the checksum fails
  ByteString bad_packet(EXPECTED_GET_BUFFER, arraysize(EXPECTED_GET_BUFFER));
  bad_packet[22] = 255;
  command.reset(RDMRequest::InflateFromData(bad_packet.data(),
                                            bad_packet.size()));
  OLA_ASSERT_NULL(command.get());

  // Make sure we can't pass a bad param length larger than the buffer
  UpdateChecksum(&bad_packet);
  command.reset(RDMRequest::InflateFromData(bad_packet.data(),
                                            bad_packet.size()));
  OLA_ASSERT_NULL(command.get());

  // Change the param length of another packet and make sure the checksum fails
  bad_packet.assign(EXPECTED_SET_BUFFER, arraysize(EXPECTED_SET_BUFFER));
  bad_packet[22] = 5;
  UpdateChecksum(&bad_packet);
  command.reset(RDMRequest::InflateFromData(bad_packet.data(),
                                            bad_packet.size()));
  OLA_ASSERT_NULL(command.get());

  // A non-0 length with a NULL pointer
  command.reset(RDMRequest::InflateFromData(NULL, 32));
  OLA_ASSERT_NULL(command.get());

  // Try to inflate a response
  command.reset(RDMRequest::InflateFromData(
      EXPECTED_GET_RESPONSE_BUFFER,
      arraysize(EXPECTED_GET_RESPONSE_BUFFER)));
  OLA_ASSERT_NULL(command.get());
}


void RDMCommandTest::testResponseMutation() {
  UID new_uid(5, 6);

  RDMGetResponse command(m_source,
                        m_destination,
                        0,  // transaction #
                        0,  // response type
                        0,  // message count
                        10,  // sub device
                        100,  // param id
                        NULL,  // data
                        0);

  command.SetDestinationUID(new_uid);
  command.SetTransactionNumber(99);

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, command.SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 24, command.MessageLength());
  OLA_ASSERT_EQ(m_source, command.SourceUID());
  OLA_ASSERT_EQ(new_uid, command.DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 99, command.TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 0, command.PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, command.MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, command.SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE, command.CommandClass());
  OLA_ASSERT_EQ((uint16_t) 100, command.ParamId());
  OLA_ASSERT_NULL(command.ParamData());
  OLA_ASSERT_EQ(0u, command.ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 0, command.Checksum(0));
}

/*
 * Test that we can inflate RDM response correctly
 */
void RDMCommandTest::testResponseInflation() {
  UID source(1, 2);
  UID destination(3, 4);
  ola::rdm::RDMStatusCode code;
  RDMResponse *command = RDMResponse::InflateFromData(NULL, 10, &code);
  OLA_ASSERT_NULL(command);
  OLA_ASSERT_EQ(ola::rdm::RDM_PACKET_TOO_SHORT, code);

  ByteString empty_string;
  command = RDMResponse::InflateFromData(empty_string, &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_PACKET_TOO_SHORT, code);
  OLA_ASSERT_NULL(command);

  command = RDMResponse::InflateFromData(EXPECTED_GET_RESPONSE_BUFFER, 0,
                                         &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_PACKET_TOO_SHORT, code);
  OLA_ASSERT_NULL(command);

  command = RDMResponse::InflateFromData(
      EXPECTED_GET_RESPONSE_BUFFER,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER),
      &code);
  OLA_ASSERT_NOT_NULL(command);
  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, code);
  uint8_t expected_data[] = {0x5a, 0x5a, 0x5a, 0x5a};
  OLA_ASSERT_EQ(4u, command->ParamDataSize());
  OLA_ASSERT_EQ(0, memcmp(expected_data, command->ParamData(),
                          command->ParamDataSize()));

  uint32_t data_value = 0x5a5a5a5a;
  RDMGetResponse expected_command(source,
                                  destination,
                                  0,  // transaction #
                                  1,  // port id
                                  0,  // message count
                                  10,  // sub device
                                  296,  // param id
                                  reinterpret_cast<uint8_t*>(&data_value),
                                  sizeof(data_value));  // data length
  OLA_ASSERT_TRUE(expected_command == *command);
  delete command;

  // now try from a string
  ByteString response_string(EXPECTED_GET_RESPONSE_BUFFER,
                             sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  command = RDMResponse::InflateFromData(response_string, &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, code);
  OLA_ASSERT_NOT_NULL(command);
  OLA_ASSERT_EQ(4u, command->ParamDataSize());
  OLA_ASSERT_EQ(0, memcmp(expected_data, command->ParamData(),
                          command->ParamDataSize()));
  OLA_ASSERT_TRUE(expected_command == *command);
  delete command;

  // change the param length and make sure the checksum fails
  uint8_t *bad_packet = new uint8_t[arraysize(EXPECTED_GET_RESPONSE_BUFFER)];
  memcpy(bad_packet, EXPECTED_GET_RESPONSE_BUFFER,
         arraysize(EXPECTED_GET_RESPONSE_BUFFER));
  bad_packet[22] = 255;

  command = RDMResponse::InflateFromData(
      bad_packet,
      arraysize(EXPECTED_GET_RESPONSE_BUFFER),
      &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_CHECKSUM_INCORRECT, code);
  OLA_ASSERT_NULL(command);

  // now make sure we can't pass a bad param length larger than the buffer
  UpdateChecksum(bad_packet, sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  command = RDMResponse::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER),
      &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_PARAM_LENGTH_MISMATCH, code);
  OLA_ASSERT_NULL(command);
  delete[] bad_packet;

  // change the param length of another packet and make sure the checksum fails
  bad_packet = new uint8_t[sizeof(EXPECTED_SET_BUFFER)];
  memcpy(bad_packet, EXPECTED_SET_BUFFER, sizeof(EXPECTED_SET_BUFFER));
  bad_packet[22] = 5;
  UpdateChecksum(bad_packet, sizeof(EXPECTED_SET_BUFFER));
  command = RDMResponse::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_SET_BUFFER),
      &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_PARAM_LENGTH_MISMATCH, code);
  OLA_ASSERT_NULL(command);
  delete[] bad_packet;

  // now try to inflate a request
  command = RDMResponse::InflateFromData(
      EXPECTED_GET_BUFFER,
      sizeof(EXPECTED_GET_BUFFER),
      &code);
  OLA_ASSERT_NULL(command);

  ByteString request_string(EXPECTED_GET_BUFFER,
                            arraysize(EXPECTED_GET_BUFFER));
  command = RDMResponse::InflateFromData(request_string, &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_INVALID_COMMAND_CLASS, code);
  OLA_ASSERT_NULL(command);
}


/**
 * Test that we can build nack messages
 */
void RDMCommandTest::testNackWithReason() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest get_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  auto_ptr<RDMResponse> response(
      NackWithReason(&get_command, ola::rdm::NR_UNKNOWN_PID));
  OLA_ASSERT_NOT_NULL(response.get());
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_NACK_REASON, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_EQ(2u, response->ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) ola::rdm::NR_UNKNOWN_PID,
                NackReasonFromResponse(response.get()));

  response.reset(NackWithReason(&get_command,
                                ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE));
  OLA_ASSERT_NOT_NULL(response.get());
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_NACK_REASON, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_EQ(2u, response->ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE,
                NackReasonFromResponse(response.get()));

  RDMSetRequest set_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  response.reset(NackWithReason(&set_command, ola::rdm::NR_WRITE_PROTECT));
  OLA_ASSERT_NOT_NULL(response.get());
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_NACK_REASON, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::SET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_EQ(2u, response->ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) ola::rdm::NR_WRITE_PROTECT,
                NackReasonFromResponse(response.get()));
}


void RDMCommandTest::testGetResponseFromData() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest get_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  RDMResponse *response = GetResponseFromData(&get_command, NULL, 0);
  OLA_ASSERT_NOT_NULL(response);
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_ACK, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_NULL(response->ParamData());
  OLA_ASSERT_EQ(0u, response->ParamDataSize());
  delete response;

  RDMSetRequest set_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  response = GetResponseFromData(&set_command, NULL, 0);
  OLA_ASSERT_NOT_NULL(response);
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_ACK, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::SET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_NULL(response->ParamData());
  OLA_ASSERT_EQ(0u, response->ParamDataSize());
  delete response;

  uint32_t data_value = 0xa5a5a5a5;
  response = GetResponseFromData(
      &get_command,
      reinterpret_cast<const uint8_t*>(&data_value),
      sizeof(data_value));
  OLA_ASSERT_NOT_NULL(response);
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_ACK, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_EQ(data_value,
                *(reinterpret_cast<const uint32_t*>(response->ParamData())));
  OLA_ASSERT_EQ((unsigned int) sizeof(data_value), response->ParamDataSize());
  delete response;
}


/**
 * Check that CombineResponses() works.
 */
void RDMCommandTest::testCombineResponses() {
  UID source(1, 2);
  UID destination(3, 4);
  uint16_t param_id = 296;

  uint32_t data_value = 0x5a5a5a5a;
  RDMGetResponse response1(source,
                           destination,
                           0,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           reinterpret_cast<uint8_t*>(&data_value),
                           sizeof(data_value));  // data length

  uint32_t data_value2 = 0xa5a5a5a5;
  RDMGetResponse response2(source,
                           destination,
                           1,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           reinterpret_cast<uint8_t*>(&data_value2),
                           sizeof(data_value2));  // data length

  const RDMResponse *combined_response = RDMResponse::CombineResponses(
      &response1,
      &response2);
  OLA_ASSERT_NOT_NULL(combined_response);
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE,
                combined_response->CommandClass());
  OLA_ASSERT_EQ(source, combined_response->SourceUID());
  OLA_ASSERT_EQ(destination, combined_response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, combined_response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 0, combined_response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, combined_response->SubDevice());
  OLA_ASSERT_EQ(param_id, combined_response->ParamId());
  OLA_ASSERT_EQ(8u, combined_response->ParamDataSize());
  const uint8_t *combined_data = combined_response->ParamData();
  const uint32_t expected_data[] = {0x5a5a5a5a, 0xa5a5a5a5};
  OLA_ASSERT_EQ(0, memcmp(expected_data, combined_data, sizeof(expected_data)));
  delete combined_response;

  // try to combine with a response with no data
  RDMGetResponse response3(source,
                           destination,
                           1,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           NULL,
                           0);

  combined_response = RDMResponse::CombineResponses(
      &response1,
      &response3);
  OLA_ASSERT_NOT_NULL(combined_response);
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE,
                combined_response->CommandClass());
  OLA_ASSERT_EQ(source, combined_response->SourceUID());
  OLA_ASSERT_EQ(destination, combined_response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, combined_response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 0, combined_response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, combined_response->SubDevice());
  OLA_ASSERT_EQ(param_id, combined_response->ParamId());
  OLA_ASSERT_EQ(4u, combined_response->ParamDataSize());
  combined_data = combined_response->ParamData();
  OLA_ASSERT_EQ(data_value,
                *(reinterpret_cast<const uint32_t*>(combined_data)));
  delete combined_response;

  // combining a GetResponse with a SetResponse is invalid
  RDMSetResponse response4(source,
                           destination,
                           1,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           NULL,
                           0);
  combined_response = RDMResponse::CombineResponses(
      &response1,
      &response4);
  OLA_ASSERT_FALSE(combined_response);
  combined_response = RDMResponse::CombineResponses(
      &response4,
      &response1);
  OLA_ASSERT_FALSE(combined_response);

  // combine two set responses
  RDMSetResponse response5(source,
                           destination,
                           0,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           reinterpret_cast<uint8_t*>(&data_value),
                           sizeof(data_value));  // data length

  combined_response = RDMResponse::CombineResponses(
      &response5,
      &response4);
  OLA_ASSERT_NOT_NULL(combined_response);
  OLA_ASSERT_EQ(RDMCommand::SET_COMMAND_RESPONSE,
                combined_response->CommandClass());
  OLA_ASSERT_EQ(source, combined_response->SourceUID());
  OLA_ASSERT_EQ(destination, combined_response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, combined_response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 0, combined_response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, combined_response->SubDevice());
  OLA_ASSERT_EQ(param_id, combined_response->ParamId());
  OLA_ASSERT_EQ(4u, combined_response->ParamDataSize());
  combined_data = combined_response->ParamData();
  OLA_ASSERT_EQ(data_value,
                *(reinterpret_cast<const uint32_t*>(combined_data)));
  delete combined_response;

  // combine two responses so that the size exceeds 231 bytes
  const unsigned int first_block_size = 200;
  const unsigned int second_block_size = 100;
  uint8_t data_block1[first_block_size];
  uint8_t data_block2[second_block_size];
  uint8_t expected_combined_data[first_block_size + second_block_size];
  memset(data_block1, 0xff, first_block_size);
  memset(data_block2, 0xac, second_block_size);
  memcpy(expected_combined_data, data_block1, first_block_size);
  memcpy(expected_combined_data + first_block_size, data_block2,
         second_block_size);

  RDMSetResponse response6(source,
                           destination,
                           0,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           data_block1,
                           sizeof(data_block1));
  RDMSetResponse response7(source,
                           destination,
                           1,  // transaction #
                           ola::rdm::RDM_ACK,  // response type
                           0,  // message count
                           10,  // sub device
                           param_id,  // param id
                           data_block2,
                           sizeof(data_block2));

  combined_response = RDMResponse::CombineResponses(
      &response6,
      &response7);
  OLA_ASSERT_NOT_NULL(combined_response);
  OLA_ASSERT_EQ(RDMCommand::SET_COMMAND_RESPONSE,
                combined_response->CommandClass());
  OLA_ASSERT_EQ(source, combined_response->SourceUID());
  OLA_ASSERT_EQ(destination, combined_response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, combined_response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 0, combined_response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, combined_response->SubDevice());
  OLA_ASSERT_EQ(param_id, combined_response->ParamId());
  OLA_ASSERT_EQ(300u, combined_response->ParamDataSize());
  OLA_ASSERT_DATA_EQUALS(expected_combined_data,
                         first_block_size + second_block_size,
                         combined_response->ParamData(),
                         combined_response->ParamDataSize());
  delete combined_response;
}


/**
 * Check the discovery command
 */
void RDMCommandTest::testDiscoveryCommand() {
  UID lower(0x0102, 0x0304);
  UID upper(0x0506, 0x0708);

  const uint8_t expected_data[] = {
    1, 2, 0, 0, 3, 4,
    5, 6, 0, 0, 7, 8
  };

  auto_ptr<RDMDiscoveryRequest> request(
      NewDiscoveryUniqueBranchRequest(m_source, lower, upper, 1));

  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, request->SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 36, request->MessageLength());
  OLA_ASSERT_EQ(m_source, request->SourceUID());
  OLA_ASSERT_EQ(UID::AllDevices(), request->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 1, request->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, request->PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 1, request->PortId());
  OLA_ASSERT_EQ((uint8_t) 0, request->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 0, request->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 0x0001, request->ParamId());
  OLA_ASSERT_DATA_EQUALS(expected_data, arraysize(expected_data),
                         request->ParamData(), request->ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 0, request->Checksum(0));

  OLA_ASSERT_TRUE(request->IsDUB());
}

/**
 * Check the mute command
 */
void RDMCommandTest::testMuteRequest() {
  auto_ptr<RDMDiscoveryRequest> request(
      NewMuteRequest(m_source, m_destination, 1));

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, request->SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 24, request->MessageLength());
  OLA_ASSERT_EQ(m_source, request->SourceUID());
  OLA_ASSERT_EQ(m_destination, request->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 1, request->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, request->PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 1, request->PortId());
  OLA_ASSERT_EQ((uint8_t) 0, request->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 0, request->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 0x0002, request->ParamId());
  OLA_ASSERT_NULL(request->ParamData());
  OLA_ASSERT_EQ(0u, request->ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 0, request->Checksum(0));

  OLA_ASSERT_FALSE(request->IsDUB());
}

/**
 * Check the UnMute Command
 */
void RDMCommandTest::testUnMuteRequest() {
  auto_ptr<RDMDiscoveryRequest> request(
      NewUnMuteRequest(m_source, m_destination, 1));

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, request->SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 24, request->MessageLength());
  OLA_ASSERT_EQ(m_source, request->SourceUID());
  OLA_ASSERT_EQ(m_destination, request->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 1, request->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, request->PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 1, request->PortId());
  OLA_ASSERT_EQ((uint8_t) 0, request->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 0, request->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 0x0003, request->ParamId());
  OLA_ASSERT_NULL(request->ParamData());
  OLA_ASSERT_EQ(0u, request->ParamDataSize());
  OLA_ASSERT_EQ((uint16_t) 0, request->Checksum(0));

  OLA_ASSERT_FALSE(request->IsDUB());
}


/**
 * Test that the generic InflateFromData method works.
 */
void RDMCommandTest::testCommandInflation() {
  const UID source(1, 2);
  const UID destination(3, 4);
  const UID lower(0x0102, 0x0304);
  const UID upper(0x0506, 0x0708);
  auto_ptr<RDMCommand> command(RDMCommand::Inflate(NULL, 10));
  OLA_ASSERT_NULL(command.get());

  command.reset(RDMCommand::Inflate(EXPECTED_GET_BUFFER, 0));
  OLA_ASSERT_NULL(command.get());

  command.reset(RDMRequest::Inflate(EXPECTED_GET_BUFFER,
                                    sizeof(EXPECTED_GET_BUFFER)));
  OLA_ASSERT_NOT_NULL(command.get());

  RDMGetRequest expected_request(source,
                                 destination,
                                 0,  // transaction #
                                 1,  // port id
                                 10,  // sub device
                                 296,  // param id
                                 NULL,  // data
                                 0);  // data length
  OLA_ASSERT_TRUE(expected_request == *command);

  command.reset(RDMCommand::Inflate(EXPECTED_SET_BUFFER,
                                    sizeof(EXPECTED_SET_BUFFER)));
  OLA_ASSERT_NOT_NULL(command.get());
  uint8_t expected_data[] = {0xa5, 0xa5, 0xa5, 0xa5};
  OLA_ASSERT_EQ(4u, command->ParamDataSize());
  OLA_ASSERT_EQ(0, memcmp(expected_data, command->ParamData(),
                          command->ParamDataSize()));

  // now try a response.
  command.reset(RDMCommand::Inflate(EXPECTED_GET_RESPONSE_BUFFER,
                                    sizeof(EXPECTED_GET_RESPONSE_BUFFER)));
  OLA_ASSERT_NOT_NULL(command.get());
  uint8_t expected_data2[] = {0x5a, 0x5a, 0x5a, 0x5a};
  OLA_ASSERT_EQ(4u, command->ParamDataSize());
  OLA_ASSERT_EQ(0, memcmp(expected_data2, command->ParamData(),
                          command->ParamDataSize()));

  uint32_t data_value = 0x5a5a5a5a;
  RDMGetResponse expected_response(source,
                                   destination,
                                   0,  // transaction #
                                   1,  // port id
                                   0,  // message count
                                   10,  // sub device
                                   296,  // param id
                                   reinterpret_cast<uint8_t*>(&data_value),
                                   sizeof(data_value));  // data length
  OLA_ASSERT_TRUE(expected_response == *command);

  // test DUB
  command.reset(RDMCommand::Inflate(EXPECTED_DISCOVERY_REQUEST,
                                    sizeof(EXPECTED_DISCOVERY_REQUEST)));
  OLA_ASSERT_NOT_NULL(command.get());
  auto_ptr<RDMDiscoveryRequest> discovery_request(
      NewDiscoveryUniqueBranchRequest(source, lower, upper, 1));
  OLA_ASSERT_TRUE(*discovery_request == *command);

  // test mute
  command.reset(RDMCommand::Inflate(EXPECTED_MUTE_REQUEST,
                                    sizeof(EXPECTED_MUTE_REQUEST)));
  OLA_ASSERT_NOT_NULL(command.get());
  discovery_request.reset(NewMuteRequest(source, destination, 1));
  OLA_ASSERT_TRUE(*discovery_request == *command);

  // test unmute
  command.reset(RDMCommand::Inflate(EXPECTED_UNMUTE_REQUEST,
                                    sizeof(EXPECTED_UNMUTE_REQUEST)));
  OLA_ASSERT_NOT_NULL(command.get());
  discovery_request.reset(NewUnMuteRequest(source, destination, 1));
  OLA_ASSERT_TRUE(*discovery_request == *command);
}

void RDMCommandTest::testDiscoveryResponseInflation() {
  auto_ptr<RDMCommand> command;

  command.reset(RDMRequest::Inflate(MUTE_RESPONSE, arraysize(MUTE_RESPONSE)));
  OLA_ASSERT_NOT_NULL(command.get());
  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND_RESPONSE, command->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 2, command->ParamId());
  OLA_ASSERT_EQ(0u, command->ParamDataSize());
}
