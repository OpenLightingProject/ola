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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * RDMCommandTest.cpp
 * Test fixture for the RDMCommand classes
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>
#include <iomanip>
#include <memory>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/IOStack.h"
#include "ola/io/OutputStream.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"


using ola::io::IOQueue;
using ola::io::IOStack;
using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::rdm::GuessMessageType;
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
using ola::testing::ASSERT_DATA_EQUALS;
using std::auto_ptr;
using std::string;

class RDMCommandTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMCommandTest);
  CPPUNIT_TEST(testRDMCommand);
  CPPUNIT_TEST(testOutputStream);
  CPPUNIT_TEST(testIOStack);
  CPPUNIT_TEST(testRequestInflation);
  CPPUNIT_TEST(testResponseInflation);
  CPPUNIT_TEST(testNackWithReason);
  CPPUNIT_TEST(testGetResponseFromData);
  CPPUNIT_TEST(testCombineResponses);
  CPPUNIT_TEST(testPack);
  CPPUNIT_TEST(testGuessMessageType);
  CPPUNIT_TEST(testDiscoveryCommand);
  CPPUNIT_TEST(testMuteRequest);
  CPPUNIT_TEST(testUnMuteRequest);
  CPPUNIT_TEST(testCommandInflation);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testRDMCommand();
    void testOutputStream();
    void testIOStack();
    void testRequestInflation();
    void testResponseInflation();
    void testNackWithReason();
    void testGetResponseFromData();
    void testCombineResponses();
    void testPack();
    void testGuessMessageType();
    void testDiscoveryCommand();
    void testMuteRequest();
    void testUnMuteRequest();
    void testCommandInflation();

  private:
    void PackAndVerify(const RDMCommand &command,
                       const uint8_t *expected,
                       unsigned int expected_length);
    void UpdateChecksum(uint8_t *expected,
                        unsigned int expected_length);

    static uint8_t EXPECTED_GET_BUFFER[];
    static uint8_t EXPECTED_SET_BUFFER[];
    static uint8_t EXPECTED_GET_RESPONSE_BUFFER[];
    static uint8_t EXPECTED_DISCOVERY_REQUEST[];
    static uint8_t EXPECTED_MUTE_REQUEST[];
    static uint8_t EXPECTED_UNMUTE_REQUEST[];
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


/*
 * Fill in the checksums
 */
void RDMCommandTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
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
}


/*
 * Test the RDMCommands work.
 */
void RDMCommandTest::testRDMCommand() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest command(source,
                        destination,
                        0,  // transaction #
                        1,  // port id
                        0,  // message count
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length

  OLA_ASSERT_EQ(source, command.SourceUID());
  OLA_ASSERT_EQ(destination, command.DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, command.TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, command.PortId());
  OLA_ASSERT_EQ((uint8_t) 0, command.MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, command.SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND, command.CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, command.ParamId());
  OLA_ASSERT_EQ(static_cast<uint8_t*>(NULL), command.ParamData());
  OLA_ASSERT_EQ(0u, command.ParamDataSize());
  OLA_ASSERT_EQ(ola::rdm::RDM_REQUEST, command.CommandType());
  PackAndVerify(command, EXPECTED_GET_BUFFER, sizeof(EXPECTED_GET_BUFFER));

  // try one with extra long data
  uint8_t *data = new uint8_t[232];
  RDMGetRequest long_command(source,
                             destination,
                             0,  // transaction #
                             1,  // port id
                             0,  // message count
                             10,  // sub device
                             123,  // param id
                             data,  // data
                             232);  // data length

  OLA_ASSERT_EQ(232u, long_command.ParamDataSize());

  uint32_t data_value = 0xa5a5a5a5;
  RDMSetRequest command3(source,
                         destination,
                         0,  // transaction #
                         1,  // port id
                         0,  // message count
                         10,  // sub device
                         296,  // param id
                         reinterpret_cast<uint8_t*>(&data_value),  // data
                         sizeof(data_value));  // data length

  OLA_ASSERT_EQ(ola::rdm::RDM_REQUEST, command.CommandType());
  PackAndVerify(command3, EXPECTED_SET_BUFFER, sizeof(EXPECTED_SET_BUFFER));
  delete[] data;
}


/*
 * Test write to an output stream works.
 */
void RDMCommandTest::testOutputStream() {
  IOQueue output;
  OutputStream stream(&output);
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest command(source,
                        destination,
                        0,  // transaction #
                        1,  // port id
                        0,  // message count
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length
  command.Write(&stream);

  uint8_t *raw_command = new uint8_t[output.Size()];
  unsigned int raw_command_size = output.Peek(raw_command, output.Size());
  OLA_ASSERT_EQ(raw_command_size, RDMCommandSerializer::RequiredSize(command));

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_GET_BUFFER,
                     sizeof(EXPECTED_GET_BUFFER),
                     raw_command,
                     raw_command_size);
  output.Pop(raw_command_size);
  OLA_ASSERT_EQ(0u, output.Size());
  delete[] raw_command;

  // now try a command with data
  uint32_t data_value = 0xa5a5a5a5;
  RDMSetRequest command2(source,
                         destination,
                         0,  // transaction #
                         1,  // port id
                         0,  // message count
                         10,  // sub device
                         296,  // param id
                         reinterpret_cast<uint8_t*>(&data_value),  // data
                         sizeof(data_value));  // data length

  OLA_ASSERT_EQ(29u, RDMCommandSerializer::RequiredSize(command2));

  command2.Write(&stream);
  OLA_ASSERT_EQ(RDMCommandSerializer::RequiredSize(command2), output.Size());

  raw_command = new uint8_t[output.Size()];
  raw_command_size = output.Peek(raw_command, output.Size());
  OLA_ASSERT_EQ(raw_command_size, RDMCommandSerializer::RequiredSize(command2));

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_SET_BUFFER,
                     sizeof(EXPECTED_SET_BUFFER),
                     raw_command,
                     raw_command_size);
  output.Pop(raw_command_size);
  OLA_ASSERT_EQ(0u, output.Size());
  delete[] raw_command;
}


/*
 * Test writing to an IOStack works.
 */
void RDMCommandTest::testIOStack() {
  IOStack output;
  UID source(1, 2);
  UID destination(3, 4);
  IOStack stack;

  RDMGetRequest command(source,
                        destination,
                        0,  // transaction #
                        1,  // port id
                        0,  // message count
                        10,  // sub device
                        296,  // param id
                        NULL,  // data
                        0);  // data length
  OLA_ASSERT_TRUE(RDMCommandSerializer::Write(command, &stack));

  unsigned int raw_command_size = stack.Size();
  OLA_ASSERT_EQ(raw_command_size, RDMCommandSerializer::RequiredSize(command));
  uint8_t raw_command[raw_command_size];
  OLA_ASSERT_EQ(raw_command_size, stack.Read(raw_command, raw_command_size));
  OLA_ASSERT_EQ(0u, stack.Size());

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_GET_BUFFER,
                     sizeof(EXPECTED_GET_BUFFER),
                     raw_command,
                     raw_command_size);

  // now try a command with data
  uint32_t data_value = 0xa5a5a5a5;
  RDMSetRequest command2(source,
                         destination,
                         0,  // transaction #
                         1,  // port id
                         0,  // message count
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

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_SET_BUFFER,
                     sizeof(EXPECTED_SET_BUFFER),
                     raw_command2,
                     raw_command_size);
}


/*
 * Test that we can inflate RDM request messages correctly
 */
void RDMCommandTest::testRequestInflation() {
  UID source(1, 2);
  UID destination(3, 4);
  auto_ptr<RDMRequest> command(RDMRequest::InflateFromData(NULL, 10));
  OLA_ASSERT_NULL(command.get());

  string empty_string;
  command.reset(RDMRequest::InflateFromData(empty_string));
  OLA_ASSERT_NULL(command.get());

  // now try a proper command but with no length
  command.reset(RDMRequest::InflateFromData(EXPECTED_GET_BUFFER, 0));
  OLA_ASSERT_NULL(command.get());

  command.reset(RDMRequest::InflateFromData(
      EXPECTED_GET_BUFFER,
      sizeof(EXPECTED_GET_BUFFER)));
  OLA_ASSERT_NOT_NULL(command.get());

  RDMGetRequest expected_command(source,
                                 destination,
                                 0,  // transaction #
                                 1,  // port id
                                 0,  // message count
                                 10,  // sub device
                                 296,  // param id
                                 NULL,  // data
                                 0);  // data length
  OLA_ASSERT_TRUE(expected_command == *command);

  string get_request_str(reinterpret_cast<char*>(EXPECTED_GET_BUFFER),
                         sizeof(EXPECTED_GET_BUFFER));
  command.reset(RDMRequest::InflateFromData(get_request_str));
  OLA_ASSERT_NOT_NULL(command.get());
  OLA_ASSERT_TRUE(expected_command == *command);

  // now try a set request
  command.reset(RDMRequest::InflateFromData(
      EXPECTED_SET_BUFFER,
      sizeof(EXPECTED_SET_BUFFER)));
  OLA_ASSERT_NOT_NULL(command.get());
  uint8_t expected_data[] = {0xa5, 0xa5, 0xa5, 0xa5};
  OLA_ASSERT_EQ(4u, command->ParamDataSize());
  OLA_ASSERT_EQ(0, memcmp(expected_data, command->ParamData(),
                          command->ParamDataSize()));

  // set request as a string
  string set_request_string(reinterpret_cast<char*>(EXPECTED_SET_BUFFER),
                            sizeof(EXPECTED_SET_BUFFER));
  command.reset(RDMRequest::InflateFromData(set_request_string));
  OLA_ASSERT_NOT_NULL(command.get());
  OLA_ASSERT_EQ(4u, command->ParamDataSize());
  OLA_ASSERT_EQ(0, memcmp(expected_data, command->ParamData(),
                          command->ParamDataSize()));

  // change the param length and make sure the checksum fails
  uint8_t *bad_packet = new uint8_t[sizeof(EXPECTED_GET_BUFFER)];
  memcpy(bad_packet, EXPECTED_GET_BUFFER, sizeof(EXPECTED_GET_BUFFER));
  bad_packet[22] = 255;

  command.reset(RDMRequest::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_GET_BUFFER)));
  OLA_ASSERT_NULL(command.get());

  get_request_str[22] = 255;
  command.reset(RDMRequest::InflateFromData(get_request_str));
  OLA_ASSERT_NULL(command.get());

  // now make sure we can't pass a bad param length larger than the buffer
  UpdateChecksum(bad_packet, sizeof(EXPECTED_GET_BUFFER));
  command.reset(RDMRequest::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_GET_BUFFER)));
  OLA_ASSERT_NULL(command.get());
  delete[] bad_packet;

  // change the param length of another packet and make sure the checksum fails
  bad_packet = new uint8_t[sizeof(EXPECTED_SET_BUFFER)];
  memcpy(bad_packet, EXPECTED_SET_BUFFER, sizeof(EXPECTED_SET_BUFFER));
  bad_packet[22] = 5;
  UpdateChecksum(bad_packet, sizeof(EXPECTED_SET_BUFFER));
  command.reset(RDMRequest::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_SET_BUFFER)));
  OLA_ASSERT_NULL(command.get());
  delete[] bad_packet;

  // now try to inflate a response
  command.reset(RDMRequest::InflateFromData(
      EXPECTED_GET_RESPONSE_BUFFER,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER)));
  OLA_ASSERT_NULL(command.get());

  string response_string(reinterpret_cast<char*>(EXPECTED_GET_RESPONSE_BUFFER),
                         sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  command.reset(RDMRequest::InflateFromData(response_string));
  OLA_ASSERT_NULL(command.get());
}


/*
 * Calculate the checksum for a packet, and verify
 */
void RDMCommandTest::PackAndVerify(const RDMCommand &command,
                                   const uint8_t *expected,
                                   unsigned int expected_length) {
  // now check packing
  unsigned int buffer_size = RDMCommandSerializer::RequiredSize(command);
  uint8_t *buffer = new uint8_t[buffer_size];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(command, buffer, &buffer_size));

  ASSERT_DATA_EQUALS(__LINE__, expected, expected_length,
                     buffer, buffer_size);
  delete[] buffer;
}


/*
 * Test that we can inflate RDM response correctly
 */
void RDMCommandTest::testResponseInflation() {
  UID source(1, 2);
  UID destination(3, 4);
  ola::rdm::rdm_response_code code;
  RDMResponse *command = RDMResponse::InflateFromData(NULL, 10, &code);
  OLA_ASSERT_EQ(static_cast<RDMResponse*>(NULL), command);
  OLA_ASSERT_EQ(ola::rdm::RDM_INVALID_RESPONSE, code);

  string empty_string;
  command = RDMResponse::InflateFromData(empty_string, &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_PACKET_TOO_SHORT, code);
  OLA_ASSERT_EQ(static_cast<RDMResponse*>(NULL), command);

  command = RDMResponse::InflateFromData(EXPECTED_GET_RESPONSE_BUFFER, 0,
                                         &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_PACKET_TOO_SHORT, code);
  OLA_ASSERT_EQ(static_cast<RDMResponse*>(NULL), command);

  command = RDMResponse::InflateFromData(
      EXPECTED_GET_RESPONSE_BUFFER,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER),
      &code);
  OLA_ASSERT_NOT_NULL(command);
  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, code);
  uint8_t expected_data[] = {0x5a, 0x5a, 0x5a, 0x5a};
  OLA_ASSERT_EQ(4u, command->ParamDataSize());
  OLA_ASSERT_EQ(ola::rdm::RDM_RESPONSE, command->CommandType());
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
  string response_string(reinterpret_cast<char*>(EXPECTED_GET_RESPONSE_BUFFER),
                         sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  command = RDMResponse::InflateFromData(response_string, &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, code);
  OLA_ASSERT_NOT_NULL(command);
  OLA_ASSERT_EQ(4u, command->ParamDataSize());
  OLA_ASSERT_EQ(0, memcmp(expected_data, command->ParamData(),
                          command->ParamDataSize()));
  OLA_ASSERT_TRUE(expected_command == *command);

  // change the param length and make sure the checksum fails
  uint8_t *bad_packet = new uint8_t[sizeof(EXPECTED_GET_RESPONSE_BUFFER)];
  memcpy(bad_packet, EXPECTED_GET_RESPONSE_BUFFER,
         sizeof(EXPECTED_GET_RESPONSE_BUFFER));
  bad_packet[22] = 255;
  delete command;

  command = RDMResponse::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_GET_RESPONSE_BUFFER),
      &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_CHECKSUM_INCORRECT, code);
  OLA_ASSERT_NULL(command);

  response_string[22] = 255;
  command = RDMResponse::InflateFromData(response_string, &code);
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
  OLA_ASSERT_EQ(static_cast<RDMResponse*>(NULL), command);

  string request_string(reinterpret_cast<char*>(EXPECTED_GET_BUFFER),
                        sizeof(EXPECTED_GET_BUFFER));
  command = RDMResponse::InflateFromData(request_string, &code);
  OLA_ASSERT_EQ(ola::rdm::RDM_INVALID_COMMAND_CLASS, code);
  OLA_ASSERT_EQ(static_cast<RDMResponse*>(NULL), command);
}


/*
 * Calculate a checksum for a packet and update it
 */
void RDMCommandTest::UpdateChecksum(uint8_t *expected,
                                    unsigned int expected_length) {
  unsigned int checksum = RDMCommand::START_CODE;
  for (unsigned int i = 0 ; i < expected_length - 2; i++)
    checksum += expected[i];

  expected[expected_length - 2] = checksum >> 8;
  expected[expected_length - 1] = checksum & 0xff;
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
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  RDMResponse *response = NackWithReason(&get_command,
                                         ola::rdm::NR_UNKNOWN_PID);
  uint16_t reason = ola::rdm::NR_UNKNOWN_PID;
  OLA_ASSERT_NOT_NULL(response);
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_NACK_REASON, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_EQ(HostToNetwork(reason),
                *(reinterpret_cast<uint16_t*>(response->ParamData())));
  OLA_ASSERT_EQ((unsigned int) sizeof(reason), response->ParamDataSize());
  delete response;

  response = NackWithReason(&get_command,
                            ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  reason = ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE;
  OLA_ASSERT_NOT_NULL(response);
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_NACK_REASON, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_EQ(HostToNetwork(reason),
                *(reinterpret_cast<uint16_t*>(response->ParamData())));
  OLA_ASSERT_EQ((unsigned int) sizeof(reason), response->ParamDataSize());
  delete response;

  RDMSetRequest set_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  response = NackWithReason(&set_command,
                            ola::rdm::NR_WRITE_PROTECT);
  reason = ola::rdm::NR_WRITE_PROTECT;
  OLA_ASSERT_NOT_NULL(response);
  OLA_ASSERT_EQ(destination, response->SourceUID());
  OLA_ASSERT_EQ(source, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) ola::rdm::RDM_NACK_REASON, response->ResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::SET_COMMAND_RESPONSE, response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_EQ(HostToNetwork(reason),
                 *(reinterpret_cast<uint16_t*>(response->ParamData())));
  OLA_ASSERT_EQ((unsigned int) sizeof(reason), response->ParamDataSize());
  delete response;
}


void RDMCommandTest::testGetResponseFromData() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMGetRequest get_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
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
  OLA_ASSERT_EQ(static_cast<uint8_t*>(NULL), response->ParamData());
  OLA_ASSERT_EQ(0u, response->ParamDataSize());
  delete response;

  RDMSetRequest set_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
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
  OLA_ASSERT_EQ(static_cast<uint8_t*>(NULL), response->ParamData());
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
                *(reinterpret_cast<uint32_t*>(response->ParamData())));
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
  ASSERT_DATA_EQUALS(__LINE__, expected_combined_data,
                     first_block_size + second_block_size,
                     combined_response->ParamData(),
                     combined_response->ParamDataSize());
  delete combined_response;
}


/**
 * Test that cloning works
 */
void RDMCommandTest::testPack() {
  UID source(1, 2);
  UID destination(3, 4);
  UID new_source(7, 8);

  RDMGetRequest get_command(source,
                            destination,
                            0,  // transaction #
                            1,  // port id
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length

  unsigned int length = RDMCommandSerializer::RequiredSize(get_command);
  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(
        get_command, data, &length, new_source, 99, 10));

  RDMRequest *command = RDMRequest::InflateFromData(data, length);
  OLA_ASSERT_NOT_NULL(command);

  OLA_ASSERT_EQ(new_source, command->SourceUID());
  OLA_ASSERT_EQ(destination, command->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 99, command->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 10, command->PortId());
  OLA_ASSERT_EQ((uint8_t) 0, command->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, command->SubDevice());
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND, command->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, command->ParamId());
  OLA_ASSERT_EQ(static_cast<uint8_t*>(NULL), command->ParamData());
  OLA_ASSERT_EQ(0u, command->ParamDataSize());
  OLA_ASSERT_EQ(25u, RDMCommandSerializer::RequiredSize(*command));
  delete[] data;
  delete command;
}


/**
 * Check that GuessMessageType works
 */
void RDMCommandTest::testGuessMessageType() {
  ola::rdm::rdm_message_type message_type;
  RDMCommand::RDMCommandClass command_class;
  OLA_ASSERT_FALSE(GuessMessageType(&message_type, &command_class, NULL, 0));
  OLA_ASSERT_FALSE(GuessMessageType(&message_type, &command_class, NULL, 20));
  OLA_ASSERT_FALSE(GuessMessageType(
        &message_type, &command_class, EXPECTED_GET_BUFFER, 0));
  OLA_ASSERT_FALSE(GuessMessageType(
        &message_type, &command_class, EXPECTED_GET_BUFFER, 18));
  OLA_ASSERT_FALSE(GuessMessageType(
        &message_type, &command_class, EXPECTED_GET_BUFFER, 19));

  OLA_ASSERT_TRUE(GuessMessageType(
        &message_type,
        &command_class,
        EXPECTED_GET_BUFFER,
        sizeof(EXPECTED_GET_BUFFER)));
  OLA_ASSERT_EQ(ola::rdm::RDM_REQUEST, message_type);
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND, command_class);

  OLA_ASSERT_TRUE(GuessMessageType(
        &message_type,
        &command_class,
        EXPECTED_SET_BUFFER,
        sizeof(EXPECTED_SET_BUFFER)));
  OLA_ASSERT_EQ(ola::rdm::RDM_REQUEST, message_type);
  OLA_ASSERT_EQ(RDMCommand::SET_COMMAND, command_class);

  OLA_ASSERT_TRUE(GuessMessageType(
        &message_type,
        &command_class,
        EXPECTED_GET_RESPONSE_BUFFER,
        sizeof(EXPECTED_GET_RESPONSE_BUFFER)));
  OLA_ASSERT_EQ(ola::rdm::RDM_RESPONSE, message_type);
  OLA_ASSERT_EQ(RDMCommand::GET_COMMAND_RESPONSE, command_class);
}


/**
 * Check the discovery command
 */
void RDMCommandTest::testDiscoveryCommand() {
  UID source(1, 2);
  UID lower(0x0102, 0x0304);
  UID upper(0x0506, 0x0708);

  auto_ptr<RDMDiscoveryRequest> request(
      NewDiscoveryUniqueBranchRequest(source, lower, upper, 1));

  OLA_ASSERT_EQ(ola::rdm::RDM_REQUEST, request->CommandType());
  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  // test pack
  unsigned int length = RDMCommandSerializer::RequiredSize(*request);
  OLA_ASSERT_EQ(37u, length);

  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request, data, &length));

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_DISCOVERY_REQUEST,
                     sizeof(EXPECTED_DISCOVERY_REQUEST),
                     data,
                     length);
  delete[] data;
}


/**
 * Check the mute command
 */
void RDMCommandTest::testMuteRequest() {
  UID source(1, 2);
  UID destination(3, 4);
  auto_ptr<RDMDiscoveryRequest> request(
      NewMuteRequest(source, destination, 1));

  OLA_ASSERT_EQ(ola::rdm::RDM_REQUEST, request->CommandType());
  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  // test pack
  unsigned int length = RDMCommandSerializer::RequiredSize(*request);
  OLA_ASSERT_EQ(25u, length);
  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request, data, &length));

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_MUTE_REQUEST,
                     sizeof(EXPECTED_MUTE_REQUEST),
                     data,
                     length);
  delete[] data;
}


/**
 * Check the UnMute Command
 */
void RDMCommandTest::testUnMuteRequest() {
  UID source(1, 2);
  UID destination(3, 4);
  auto_ptr<RDMDiscoveryRequest> request(
      NewUnMuteRequest(source, destination, 1));

  OLA_ASSERT_EQ(ola::rdm::RDM_REQUEST, request->CommandType());
  OLA_ASSERT_EQ(RDMCommand::DISCOVER_COMMAND, request->CommandClass());

  // test pack
  unsigned int length = RDMCommandSerializer::RequiredSize(*request);
  OLA_ASSERT_EQ(25u, length);
  uint8_t *data = new uint8_t[length];
  OLA_ASSERT_TRUE(RDMCommandSerializer::Pack(*request, data, &length));

  ASSERT_DATA_EQUALS(__LINE__,
                     EXPECTED_UNMUTE_REQUEST,
                     sizeof(EXPECTED_UNMUTE_REQUEST),
                     data,
                     length);
  delete[] data;
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
                                 0,  // message count
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
  OLA_ASSERT_EQ(ola::rdm::RDM_RESPONSE, command->CommandType());
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
  OLA_ASSERT_TRUE(expected_response ==  *command);

  // test DUB
  command.reset(RDMCommand::Inflate(EXPECTED_DISCOVERY_REQUEST,
                                    sizeof(EXPECTED_DISCOVERY_REQUEST)));
  OLA_ASSERT_NOT_NULL(command.get());
  auto_ptr<RDMDiscoveryRequest> discovery_request(
      NewDiscoveryUniqueBranchRequest(source, lower, upper, 1));
  OLA_ASSERT_TRUE(*discovery_request ==  *command);

  // test mute
  command.reset(RDMCommand::Inflate(EXPECTED_MUTE_REQUEST,
                                    sizeof(EXPECTED_MUTE_REQUEST)));
  OLA_ASSERT_NOT_NULL(command.get());
  discovery_request.reset(NewMuteRequest(source, destination, 1));
  OLA_ASSERT_TRUE(*discovery_request ==  *command);

  // test unmute
  command.reset(RDMCommand::Inflate(EXPECTED_UNMUTE_REQUEST,
                                    sizeof(EXPECTED_UNMUTE_REQUEST)));
  OLA_ASSERT_NOT_NULL(command.get());
  discovery_request.reset(NewUnMuteRequest(source, destination, 1));
  OLA_ASSERT_TRUE(*discovery_request ==  *command);
}
