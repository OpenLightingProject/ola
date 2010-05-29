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
 * RDMCommandTest.cpp
 * Test fixture for the RDMCommand classes
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <iomanip>

#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UID.h"

using std::string;
using ola::rdm::RDMCommand;
using ola::rdm::UID;

class RDMCommandTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMCommandTest);
  CPPUNIT_TEST(testRDMCommand);
  CPPUNIT_TEST(testInflation);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testRDMCommand();
    void testInflation();

  private:
    void PackAndVerify(const RDMCommand &command,
                       const uint8_t *expected,
                       unsigned int expected_length);
    void UpdateChecksum(uint8_t *expected,
                        unsigned int expected_length);

    static uint8_t EXPECTED_BUFFER[];
    static uint8_t EXPECTED_BUFFER2[];
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMCommandTest);


uint8_t RDMCommandTest::EXPECTED_BUFFER[] = {
  1, 24,  // sub code & length
  0, 3, 0, 0, 0, 4,   // dst uid
  0, 1, 0, 0, 0, 2,   // src uid
  0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
  32, 1, 40, 0,  // command, param id, param data length
  0, 0  // checksum, filled in below
};

uint8_t RDMCommandTest::EXPECTED_BUFFER2[] = {
    1, 28,  // sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    48, 1, 40, 4,  // command, param id, param data length
    0xa5, 0xa5, 0xa5, 0xa5,  // param data
    0, 0  // checksum, filled in below
};


/*
 *
 */
void RDMCommandTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  UpdateChecksum(EXPECTED_BUFFER, sizeof(EXPECTED_BUFFER));
  UpdateChecksum(EXPECTED_BUFFER2, sizeof(EXPECTED_BUFFER2));
}


/*
 * Test the RDMCommands work.
 */
void RDMCommandTest::testRDMCommand() {
  UID source(1, 2);
  UID destination(3, 4);

  RDMCommand command(source,
                     destination,
                     0,  // transaction #
                     1,  // port id
                     0,  // message count
                     10,  // sub device
                     RDMCommand::GET_COMMAND,
                     296,  // param id
                     NULL,  // data
                     0);  // data length

  CPPUNIT_ASSERT_EQUAL(source, command.SourceUID());
  CPPUNIT_ASSERT_EQUAL(destination, command.DestinationUID());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, command.TransactionNumber());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, command.PortId());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, command.MessageCount());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 10, command.SubDevice());
  CPPUNIT_ASSERT_EQUAL(RDMCommand::GET_COMMAND, command.CommandClass());
  CPPUNIT_ASSERT_EQUAL((uint16_t) 296, command.ParamId());
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint8_t*>(NULL), command.ParamData());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, command.ParamDataSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 25, command.Size());

  // copy and assignment
  RDMCommand command2(command);
  CPPUNIT_ASSERT(command == command2);

  // try one with extra long data
  uint8_t *data = new uint8_t[232];
  RDMCommand long_command(source,
                          destination,
                          0,  // transaction #
                          1,  // port id
                          0,  // message count
                          10,  // sub device
                          RDMCommand::GET_COMMAND,
                          123,  // param id
                          data,  // data
                          232);  // data length

  CPPUNIT_ASSERT_EQUAL((unsigned int) 231, long_command.ParamDataSize());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 256, long_command.Size());
  PackAndVerify(command, EXPECTED_BUFFER, sizeof(EXPECTED_BUFFER));

  uint32_t data_value = 0xa5a5a5a5;
  RDMCommand command3(source,
                     destination,
                     0,  // transaction #
                     1,  // port id
                     0,  // message count
                     10,  // sub device
                     RDMCommand::SET_COMMAND,
                     296,  // param id
                     reinterpret_cast<uint8_t*>(&data_value),  // data
                     sizeof(data_value));  // data length

  CPPUNIT_ASSERT_EQUAL((unsigned int) 29, command3.Size());
  PackAndVerify(command3, EXPECTED_BUFFER2, sizeof(EXPECTED_BUFFER2));

  delete[] data;
}


/*
 * Test that we can inflate RDM messages correctly
 */
void RDMCommandTest::testInflation() {
  RDMCommand *command = RDMCommand::InflateFromData(NULL, 10);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<RDMCommand*>(NULL), command);

  command = RDMCommand::InflateFromData(EXPECTED_BUFFER, 0);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<RDMCommand*>(NULL), command);

  command = RDMCommand::InflateFromData(
      EXPECTED_BUFFER,
      sizeof(EXPECTED_BUFFER));
  CPPUNIT_ASSERT(NULL != command);
  delete command;

  command = RDMCommand::InflateFromData(
      EXPECTED_BUFFER2,
      sizeof(EXPECTED_BUFFER2));
  CPPUNIT_ASSERT(NULL != command);
  delete command;

  // change the param length and make sure the checksum fails
  uint8_t *bad_packet = new uint8_t[sizeof(EXPECTED_BUFFER)];
  memcpy(bad_packet, EXPECTED_BUFFER, sizeof(EXPECTED_BUFFER));
  bad_packet[22] = 255;

  command = RDMCommand::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_BUFFER));
  CPPUNIT_ASSERT(NULL == command);
  delete command;

  // now make sure we can't pass a bad param length larger than the buffer
  UpdateChecksum(bad_packet, sizeof(EXPECTED_BUFFER));
  command = RDMCommand::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_BUFFER));
  CPPUNIT_ASSERT(NULL == command);
  delete command;

  delete[] bad_packet;

  // change the param length of another packet and make sure the checksum fails
  bad_packet = new uint8_t[sizeof(EXPECTED_BUFFER2)];
  memcpy(bad_packet, EXPECTED_BUFFER2, sizeof(EXPECTED_BUFFER2));
  bad_packet[22] = 5;
  UpdateChecksum(bad_packet, sizeof(EXPECTED_BUFFER2));
  command = RDMCommand::InflateFromData(
      bad_packet,
      sizeof(EXPECTED_BUFFER2));
  CPPUNIT_ASSERT(NULL == command);
  delete command;

  delete[] bad_packet;
}


/*
 * Calculate the checksum for a packet, and verify
 */
void RDMCommandTest::PackAndVerify(const RDMCommand &command,
                                   const uint8_t *expected,
                                   unsigned int expected_length) {
  // now check packing
  unsigned int buffer_size = command.Size();
  uint8_t *buffer = new uint8_t[buffer_size];
  CPPUNIT_ASSERT(command.Pack(buffer, &buffer_size));

  for (unsigned int i = 0 ; i < expected_length; i++) {
    std::stringstream str;
    str << "Offset " << i << ", expected " << static_cast<int>(expected[i]) <<
      ", got " << static_cast<int>(buffer[i]);
    CPPUNIT_ASSERT_MESSAGE(str.str(), buffer[i] == expected[i]);
  }
  delete[] buffer;
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
