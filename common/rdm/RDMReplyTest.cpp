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
 * RDMReplyTest.cpp
 * Test fixture for the RDMReply.
 * Copyright (C) 2015 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/base/Array.h"
#include "ola/io/ByteString.h"
#include "ola/rdm/RDMFrame.h"
#include "ola/rdm/RDMReply.h"
#include "ola/rdm/RDMResponseCodes.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"

using ola::io::ByteString;
using ola::rdm::RDMFrame;
using ola::rdm::RDMFrames;
using ola::rdm::RDMGetResponse;
using ola::rdm::RDMReply;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;

class RDMReplyTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMReplyTest);
  CPPUNIT_TEST(testReplyFromCode);
  CPPUNIT_TEST(testReplyWithResponse);
  CPPUNIT_TEST(testReplyWithFrames);
  CPPUNIT_TEST(testFromFrameHelper);
  CPPUNIT_TEST(testDUBHelper);
  CPPUNIT_TEST_SUITE_END();

 public:
  RDMReplyTest()
    : m_source(1, 2),
      m_destination(3, 4) {
  }

  void testReplyFromCode();
  void testReplyWithResponse();
  void testReplyWithFrames();
  void testFromFrameHelper();
  void testDUBHelper();

 private:
  UID m_source;
  UID m_destination;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMReplyTest);

void RDMReplyTest::testReplyFromCode() {
  RDMReply reply(ola::rdm::RDM_WAS_BROADCAST);

  OLA_ASSERT_EQ(ola::rdm::RDM_WAS_BROADCAST, reply.StatusCode());
  OLA_ASSERT_NULL(reply.Response());
  OLA_ASSERT_NULL(reply.MutableResponse());
  OLA_ASSERT_TRUE(reply.Frames().empty());
}

void RDMReplyTest::testReplyWithResponse() {
  RDMResponse *response = new RDMGetResponse(m_source,
                                             m_destination,
                                             0,  // transaction #
                                             0,  // response type
                                             0,  // message count
                                             10,  // sub device
                                             100,  // param id
                                             NULL,  // data
                                             0);

  // Ownership is transferred.
  RDMReply reply(ola::rdm::RDM_COMPLETED_OK, response);

  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, reply.StatusCode());
  OLA_ASSERT_EQ(static_cast<const RDMResponse*>(response), reply.Response());
  OLA_ASSERT_EQ(response, reply.MutableResponse());
  OLA_ASSERT_TRUE(reply.Frames().empty());
}

void RDMReplyTest::testReplyWithFrames() {
  const uint8_t response_data[] = {
    0xcc, 1, 28,  // start code, sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x21, 1, 40, 4,  // command, param id, param data length
    0x5a, 0x5a, 0x5a, 0x5a,  // param data
    0, 0  // checksum, anoything is fine.
  };

  RDMFrame frame(response_data, arraysize(response_data));
  frame.timing.response_time = 10000;
  frame.timing.mark_time = 32000;
  frame.timing.break_time = 8000;
  frame.timing.data_time = 45000;

  RDMFrames frames;
  frames.push_back(frame);

  RDMResponse *response = new RDMGetResponse(m_source,
                                             m_destination,
                                             0,  // transaction #
                                             0,  // response type
                                             0,  // message count
                                             10,  // sub device
                                             100,  // param id
                                             NULL,  // data
                                             0);

  RDMReply reply(ola::rdm::RDM_COMPLETED_OK, response, frames);

  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, reply.StatusCode());
  OLA_ASSERT_EQ(static_cast<const RDMResponse*>(response), reply.Response());
  OLA_ASSERT_EQ(response, reply.MutableResponse());
  OLA_ASSERT_EQ(static_cast<size_t>(1), reply.Frames().size());
  OLA_ASSERT_TRUE(frame == reply.Frames()[0]);
}

void RDMReplyTest::testFromFrameHelper() {
  const uint8_t response_data[] = {
    0xcc, 1, 28,  // start code, sub code & length
    0, 3, 0, 0, 0, 4,   // dst uid
    0, 1, 0, 0, 0, 2,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x21, 1, 40, 4,  // command, param id, param data length
    0x5a, 0x5a, 0x5a, 0x5a,  // param data
    2, 0xb4  // checksum, anoything is fine.
  };

  RDMFrame frame(response_data, arraysize(response_data));
  frame.timing.response_time = 10000;
  frame.timing.mark_time = 32000;
  frame.timing.break_time = 8000;
  frame.timing.data_time = 45000;

  auto_ptr<RDMReply> reply(RDMReply::FromFrame(frame));
  OLA_ASSERT_NOT_NULL(reply.get());

  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, reply->StatusCode());
  OLA_ASSERT_NOT_NULL(reply->Response());
  OLA_ASSERT_NOT_NULL(reply->MutableResponse());
  OLA_ASSERT_EQ(static_cast<size_t>(1), reply->Frames().size());
  OLA_ASSERT_TRUE(frame == reply->Frames()[0]);

  const RDMResponse *response = reply->Response();

  OLA_ASSERT_EQ(ola::rdm::SUB_START_CODE, response->SubStartCode());
  OLA_ASSERT_EQ((uint8_t) 28, response->MessageLength());
  OLA_ASSERT_EQ(m_source, response->SourceUID());
  OLA_ASSERT_EQ(m_destination, response->DestinationUID());
  OLA_ASSERT_EQ((uint8_t) 0, response->TransactionNumber());
  OLA_ASSERT_EQ((uint8_t) 1, response->PortIdResponseType());
  OLA_ASSERT_EQ((uint8_t) 0, response->MessageCount());
  OLA_ASSERT_EQ((uint16_t) 10, response->SubDevice());
  OLA_ASSERT_EQ(ola::rdm::RDMCommand::GET_COMMAND_RESPONSE,
                response->CommandClass());
  OLA_ASSERT_EQ((uint16_t) 296, response->ParamId());
  OLA_ASSERT_NOT_NULL(response->ParamData());
  OLA_ASSERT_EQ(4u, response->ParamDataSize());
}

void RDMReplyTest::testDUBHelper() {
  const uint8_t data[] = {1, 2, 3, 4};
  RDMFrame frame(data, arraysize(data));
  frame.timing.response_time = 10000;
  frame.timing.data_time = 45000;

  auto_ptr<RDMReply> reply(RDMReply::DUBReply(frame));
  OLA_ASSERT_NOT_NULL(reply.get());

  OLA_ASSERT_EQ(ola::rdm::RDM_DUB_RESPONSE, reply->StatusCode());
  OLA_ASSERT_NULL(reply->Response());
  OLA_ASSERT_NULL(reply->MutableResponse());
  OLA_ASSERT_EQ(static_cast<size_t>(1), reply->Frames().size());
  OLA_ASSERT_TRUE(frame == reply->Frames()[0]);
  OLA_ASSERT_EQ(10000u, reply->Frames()[0].timing.response_time);
  OLA_ASSERT_EQ(45000u, reply->Frames()[0].timing.data_time);
}
