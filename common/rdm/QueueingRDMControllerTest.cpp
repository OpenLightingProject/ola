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
 * QueueingRDMControllerTest.cpp
 * Test fixture for the UID classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <queue>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/Callback.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/QueueingRDMController.h"

using ola::rdm::RDMCallback;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMGetResponse;
using ola::rdm::UID;
using ola::rdm::RDM_ACK;
using ola::rdm::ACK_OVERFLOW;
using std::string;
using std::vector;

class QueueingRDMControllerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(QueueingRDMControllerTest);
  CPPUNIT_TEST(testSendAndReceive);
  CPPUNIT_TEST(testAckOverflows);
  CPPUNIT_TEST(testPauseAndResume);
  CPPUNIT_TEST(testQueueOverflow);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testSendAndReceive();
    void testAckOverflows();
    void testPauseAndResume();
    void testQueueOverflow();

    void VerifyResponse(
        ola::rdm::rdm_response_code expected_code,
        const RDMResponse *expected_response,
        vector<string> expected_packets,
        bool delete_response,
        ola::rdm::rdm_response_code code,
        const RDMResponse *response,
        const vector<string> &packets);

  private:
    RDMRequest *NewGetRequest(const UID &source,
                              const UID &destination);
};

CPPUNIT_TEST_SUITE_REGISTRATION(QueueingRDMControllerTest);

/**
 * The MockRDMController, used to verify the behaviour
 */
class MockRDMController: public ola::rdm::RDMControllerInterface {
  public:
    MockRDMController() {}
    ~MockRDMController() {}
    void SendRDMRequest(const RDMRequest *request, RDMCallback *on_complete);

    void AddExpectedCall(RDMRequest *request,
                         ola::rdm::rdm_response_code code,
                         RDMResponse *response,
                         const string &packet);

    void Verify();
  private:
    typedef struct {
      RDMRequest *request;
      ola::rdm::rdm_response_code code;
      RDMResponse *response;
      string packet;
    } expected_call;

    std::queue<expected_call> m_expected_calls;
};


void MockRDMController::SendRDMRequest(const RDMRequest *request,
                                       RDMCallback *on_complete) {
  CPPUNIT_ASSERT(m_expected_calls.size());
  expected_call call = m_expected_calls.front();
  m_expected_calls.pop();
  CPPUNIT_ASSERT((*call.request) == (*request));
  delete request;
  vector<string> packets;
  if (!call.packet.empty())
    packets.push_back(call.packet);
  on_complete->Run(call.code, call.response, packets);
}


void MockRDMController::AddExpectedCall(RDMRequest *request,
                                        ola::rdm::rdm_response_code code,
                                        RDMResponse *response,
                                        const string &packet) {
  expected_call call = {
    request,
    code,
    response,
    packet
  };
  m_expected_calls.push(call);
}


/**
 * Verify no expected calls remain
 */
void MockRDMController::Verify() {
  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), m_expected_calls.size());
}


void QueueingRDMControllerTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
}


/**
 * Verify a response
 */
void QueueingRDMControllerTest::VerifyResponse(
    ola::rdm::rdm_response_code expected_code,
    const RDMResponse *expected_response,
    vector<string> expected_packets,
    bool delete_response,
    ola::rdm::rdm_response_code code,
    const RDMResponse *response,
    const vector<string> &packets) {
  CPPUNIT_ASSERT_EQUAL(expected_code, code);
  if (expected_response)
    CPPUNIT_ASSERT((*expected_response) == (*response));
  else
    CPPUNIT_ASSERT_EQUAL(expected_response, response);

  CPPUNIT_ASSERT_EQUAL(expected_packets.size(), packets.size());
  for (unsigned int i = 0; i < packets.size(); i++)
    CPPUNIT_ASSERT_EQUAL(expected_packets[i], packets[i]);

  if (delete_response)
    delete response;
}


/**
 *
 */
RDMRequest *QueueingRDMControllerTest::NewGetRequest(const UID &source,
                                                     const UID &destination) {
  return  new ola::rdm::RDMGetRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // message count
      10,  // sub device
      296,  // param id
      NULL,  // data
      0);  // data length
}


/*
 * Simple commands work.
 */
void QueueingRDMControllerTest::testSendAndReceive() {
  UID source(1, 2);
  UID destination(3, 4);

  MockRDMController mock_controller;
  ola::rdm::QueueingRDMController controller(&mock_controller, 10);

  // test a simple request/response
  RDMRequest *get_request = NewGetRequest(source, destination);

  RDMGetResponse expected_command(destination,
                                  source,
                                  0,  // transaction #
                                  RDM_ACK,
                                  0,  // message count
                                  10,  // sub device
                                  296,  // param id
                                  NULL,  // data
                                  0);  // data length

  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  &expected_command,
                                  "foo");

  vector<string> packets;
  packets.push_back("foo");
  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_COMPLETED_OK,
          static_cast<const RDMResponse*>(&expected_command),
          packets,
          false));

  // check a response where we return ok, but pass a null pointer
  get_request = NewGetRequest(source, destination);
  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  NULL,
                                  "bar");
  packets[0] = "bar";
  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_INVALID_RESPONSE,
          static_cast<const RDMResponse*>(NULL),
          packets,
          false));

  // check a failed command
  get_request = NewGetRequest(source, destination);
  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_FAILED_TO_SEND,
                                  NULL,
                                  "");
  vector<string> empty_packets;
  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_FAILED_TO_SEND,
          static_cast<const RDMResponse*>(NULL),
          empty_packets,
          false));

  mock_controller.Verify();
}


/*
 * Check that overflow sequences work.
 */
void QueueingRDMControllerTest::testAckOverflows() {
  UID source(1, 2);
  UID destination(3, 4);

  MockRDMController mock_controller;
  ola::rdm::QueueingRDMController controller(&mock_controller, 10);

  // test an ack overflow sequence
  RDMRequest *get_request = NewGetRequest(source, destination);
  uint8_t data[] = {0xaa, 0xbb};

  RDMGetResponse *response1 = new RDMGetResponse(
      destination,
      source,
      0,  // transaction #
      ACK_OVERFLOW,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data,  // data
      1);  // data length

  RDMGetResponse *response2 = new RDMGetResponse(
      destination,
      source,
      0,  // transaction #
      RDM_ACK,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data + 1,  // data
      1);  // data length

  RDMGetResponse expected_response(
      destination,
      source,
      0,  // transaction #
      RDM_ACK,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data,  // data
      2);  // data length

  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  response1,
                                  "foo");
  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  response2,
                                  "bar");

  vector<string> packets;
  packets.push_back("foo");
  packets.push_back("bar");
  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_COMPLETED_OK,
          static_cast<const RDMResponse*>(&expected_response),
          packets,
          true));

  // now check an broken transaction. We first ACK, the return a timeout
  get_request = NewGetRequest(source, destination);

  response1 = new RDMGetResponse(
      destination,
      source,
      0,  // transaction #
      ACK_OVERFLOW,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data,  // data
      1);  // data length

  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  response1,
                                  "foo");
  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_TIMEOUT,
                                  NULL,
                                  "bar");

  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_TIMEOUT,
          static_cast<const RDMResponse*>(NULL),
          packets,
          false));

  // now test the case where the responses can't be combined
  get_request = NewGetRequest(source, destination);

  response1 = new RDMGetResponse(
      destination,
      source,
      0,  // transaction #
      ACK_OVERFLOW,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data,  // data
      1);  // data length

  response2 = new RDMGetResponse(
      source,
      source,
      0,  // transaction #
      RDM_ACK,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data + 1,  // data
      1);  // data length

  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  response1,
                                  "foo");
  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  response2,
                                  "bar");

  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_INVALID_RESPONSE,
          static_cast<const RDMResponse*>(NULL),
          packets,
          false));

  mock_controller.Verify();
}


/*
 * Verify that pausing works.
 */
void QueueingRDMControllerTest::testPauseAndResume() {
  UID source(1, 2);
  UID destination(3, 4);

  MockRDMController mock_controller;
  ola::rdm::QueueingRDMController controller(&mock_controller, 10);
  controller.Pause();

  RDMRequest *get_request = NewGetRequest(source, destination);

  RDMGetResponse expected_command(destination,
                                  source,
                                  0,  // transaction #
                                  RDM_ACK,  // response type
                                  0,  // message count
                                  10,  // sub device
                                  296,  // param id
                                  NULL,  // data
                                  0);  // data length

  // queue up two requests
  vector<string> packets;
  packets.push_back("foo");
  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_COMPLETED_OK,
          static_cast<const RDMResponse*>(&expected_command),
          packets,
          false));

  packets[0] = "bar";
  get_request = NewGetRequest(source, destination);
  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_COMPLETED_OK,
          static_cast<const RDMResponse*>(&expected_command),
          packets,
          false));

  // now resume
  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  &expected_command,
                                  "foo");
  mock_controller.AddExpectedCall(get_request,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  &expected_command,
                                  "bar");
  controller.Resume();
}


/*
 * Verify that overflowing the queue behaves
 */
void QueueingRDMControllerTest::testQueueOverflow() {
  UID source(1, 2);
  UID destination(3, 4);

  MockRDMController mock_controller;
  ola::rdm::QueueingRDMController *controller = new
      ola::rdm::QueueingRDMController(&mock_controller, 1);
  controller->Pause();

  RDMRequest *get_request = NewGetRequest(source, destination);
  vector<string> packets;
  controller->SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_FAILED_TO_SEND,
          static_cast<const RDMResponse*>(NULL),
          packets,
          false));

  // this one should overflow the queue
  get_request = NewGetRequest(source, destination);
  controller->SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_FAILED_TO_SEND,
          static_cast<const RDMResponse*>(NULL),
          packets,
          false));

  // now because we're paused the first should fail as well
  delete controller;
}
