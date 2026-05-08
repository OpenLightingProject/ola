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
 * QueueingRDMControllerTest.cpp
 * Test fixture for the QueueingRDMController
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/Callback.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/QueueingRDMController.h"
#include "ola/testing/TestUtils.h"

using ola::NewSingleCallback;
using ola::rdm::ACK_OVERFLOW;
using ola::rdm::RDMCallback;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMFrame;
using ola::rdm::RDMFrames;
using ola::rdm::RDMGetResponse;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDM_ACK;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::unique_ptr;
using std::string;
using std::vector;

RDMRequest *NewGetRequest(const UID &source, const UID &destination) {
  return new ola::rdm::RDMGetRequest(source,
                                     destination,
                                     0,  // transaction #
                                     1,  // port id
                                     10,  // sub device
                                     296,  // param id
                                     NULL,  // data
                                     0);  // data length
}

RDMResponse *NewGetResponse(const UID &source, const UID &destination) {
  return new RDMGetResponse(source,
                            destination,
                            0,  // transaction #
                            RDM_ACK,
                            0,  // message count
                            10,  // sub device
                            296,  // param id
                            NULL,  // data
                            0);  // data length
}

class QueueingRDMControllerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(QueueingRDMControllerTest);
  CPPUNIT_TEST(testSendAndReceive);
  CPPUNIT_TEST(testDelayedSendAndReceive);
  CPPUNIT_TEST(testAckOverflows);
  CPPUNIT_TEST(testPauseAndResume);
  CPPUNIT_TEST(testQueueOverflow);
  CPPUNIT_TEST(testDiscovery);
  CPPUNIT_TEST(testMultipleDiscovery);
  CPPUNIT_TEST(testReentrantDiscovery);
  CPPUNIT_TEST(testRequestAndDiscovery);
  CPPUNIT_TEST_SUITE_END();

 public:
  QueueingRDMControllerTest()
      : m_source(1, 2),
        m_destination(3, 4),
        m_discovery_complete_count(0) {
  }

  void testSendAndReceive();
  void testDelayedSendAndReceive();
  void testAckOverflows();
  void testPauseAndResume();
  void testQueueOverflow();
  void testDiscovery();
  void testMultipleDiscovery();
  void testReentrantDiscovery();
  void testRequestAndDiscovery();

  void VerifyResponse(RDMReply *expected_reply, RDMReply *reply) {
    OLA_ASSERT_EQ(*expected_reply, *reply);
  }

  void VerifyDiscoveryComplete(UIDSet *expected_uids, const UIDSet &uids) {
    OLA_ASSERT_EQ(*expected_uids, uids);
    m_discovery_complete_count++;
  }

  void ReentrantDiscovery(
      ola::rdm::DiscoverableQueueingRDMController *controller,
      UIDSet *expected_uids,
      const UIDSet &uids);

 private:
  UID m_source;
  UID m_destination;
  int m_discovery_complete_count;

  static const uint8_t MOCK_FRAME_DATA[];
  static const uint8_t MOCK_FRAME_DATA2[];
};

const uint8_t QueueingRDMControllerTest::MOCK_FRAME_DATA[] = { 1, 2, 3, 4 };
const uint8_t QueueingRDMControllerTest::MOCK_FRAME_DATA2[] = { 5, 6, 7, 8 };

CPPUNIT_TEST_SUITE_REGISTRATION(QueueingRDMControllerTest);


/**
 * The MockRDMController, used to verify the behaviour
 */
class MockRDMController: public ola::rdm::DiscoverableRDMControllerInterface {
 public:
    MockRDMController()
        : m_rdm_callback(NULL),
          m_discovery_callback(NULL) {
    }

    void SendRDMRequest(RDMRequest *request, RDMCallback *on_complete);

    void ExpectCallAndCapture(RDMRequest *request);

    /*
     * Takes ownership of reply.
     */
    void ExpectCallAndReplyWith(RDMRequest *request,
                                RDMReply *reply);

    void AddExpectedDiscoveryCall(bool full, const UIDSet *uids);

    void RunFullDiscovery(RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(RDMDiscoveryCallback *callback);

    void RunRDMCallback(RDMReply *reply);

    void RunDiscoveryCallback(const UIDSet &uids);
    void Verify();

 private:
    typedef struct {
      RDMRequest *request;
      RDMReply *reply;
      bool run_callback;
    } expected_call;

    typedef struct {
      bool full;
      const UIDSet *uids;
    } expected_discovery_call;

    std::queue<expected_call> m_expected_calls;
    std::queue<expected_discovery_call> m_expected_discover_calls;
    RDMCallback *m_rdm_callback;
    RDMDiscoveryCallback *m_discovery_callback;
};


void MockRDMController::SendRDMRequest(RDMRequest *request,
                                       RDMCallback *on_complete) {
  OLA_ASSERT_TRUE(m_expected_calls.size());
  expected_call call = m_expected_calls.front();
  m_expected_calls.pop();
  OLA_ASSERT_TRUE(*call.request == *request);
  delete request;

  if (call.run_callback) {
    on_complete->Run(call.reply);
    delete call.reply;
  } else {
    OLA_ASSERT_FALSE(m_rdm_callback);
    m_rdm_callback = on_complete;
  }
}


void MockRDMController::RunFullDiscovery(RDMDiscoveryCallback *callback) {
  OLA_ASSERT_TRUE(m_expected_discover_calls.size());
  expected_discovery_call call = m_expected_discover_calls.front();
  m_expected_discover_calls.pop();
  OLA_ASSERT_TRUE(call.full);

  if (call.uids) {
    callback->Run(*call.uids);
  } else {
    OLA_ASSERT_FALSE(m_discovery_callback);
    m_discovery_callback = callback;
  }
}


void MockRDMController::RunIncrementalDiscovery(
    RDMDiscoveryCallback *callback) {
  OLA_ASSERT_TRUE(m_expected_discover_calls.size());
  expected_discovery_call call = m_expected_discover_calls.front();
  m_expected_discover_calls.pop();
  OLA_ASSERT_FALSE(call.full);

  if (call.uids) {
    callback->Run(*call.uids);
  } else {
    OLA_ASSERT_FALSE(m_discovery_callback);
    m_discovery_callback = callback;
  }
}


void MockRDMController::ExpectCallAndCapture(RDMRequest *request) {
  expected_call call = {
    request,
    NULL,
    false,
  };
  m_expected_calls.push(call);
}

void MockRDMController::ExpectCallAndReplyWith(RDMRequest *request,
                                                 RDMReply *reply) {
  expected_call call = {
    request,
    reply,
    true,
  };
  m_expected_calls.push(call);
}

void MockRDMController::AddExpectedDiscoveryCall(bool full,
                                                 const UIDSet *uids) {
  expected_discovery_call call = {
    full,
    uids,
  };
  m_expected_discover_calls.push(call);
}


/**
 * Run the current RDM callback
 */
void MockRDMController::RunRDMCallback(RDMReply *reply) {
  OLA_ASSERT_TRUE(m_rdm_callback);
  RDMCallback *callback = m_rdm_callback;
  m_rdm_callback = NULL;
  callback->Run(reply);
}


/**
 * Run the current discovery callback
 */
void MockRDMController::RunDiscoveryCallback(const UIDSet &uids) {
  OLA_ASSERT_TRUE(m_discovery_callback);
  RDMDiscoveryCallback *callback = m_discovery_callback;
  m_discovery_callback = NULL;
  callback->Run(uids);
}


/**
 * Verify no expected calls remain
 */
void MockRDMController::Verify() {
  OLA_ASSERT_EQ(static_cast<size_t>(0), m_expected_calls.size());
  OLA_ASSERT_EQ(static_cast<size_t>(0),
                m_expected_discover_calls.size());
}


void QueueingRDMControllerTest::ReentrantDiscovery(
    ola::rdm::DiscoverableQueueingRDMController *controller,
    UIDSet *expected_uids,
    const UIDSet &uids) {
  OLA_ASSERT_EQ(*expected_uids, uids);
  m_discovery_complete_count++;
  controller->RunFullDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          expected_uids));
}

/*
 * Check that sending RDM commands work.
 * This runs the RDMCallback immediately.
 */
void QueueingRDMControllerTest::testSendAndReceive() {
  MockRDMController mock_controller;
  ola::rdm::QueueingRDMController controller(&mock_controller, 10);

  // Some mock frame data.
  RDMFrame frame(MOCK_FRAME_DATA, arraysize(MOCK_FRAME_DATA));
  RDMFrames frames;
  frames.push_back(frame);

  // test a simple request/response
  RDMRequest *get_request = NewGetRequest(m_source, m_destination);
  RDMReply *expected_reply = new RDMReply(
      ola::rdm::RDM_COMPLETED_OK,
      NewGetResponse(m_destination, m_source),
      frames);
  mock_controller.ExpectCallAndReplyWith(get_request, expected_reply);

  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          expected_reply));

  // check a response where we return ok, but pass a null pointer
  get_request = NewGetRequest(m_source, m_destination);
  expected_reply = new RDMReply(ola::rdm::RDM_COMPLETED_OK,
                                NULL,
                                frames);
  mock_controller.ExpectCallAndReplyWith(get_request, expected_reply);

  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          expected_reply));

  // check a failed command
  get_request = NewGetRequest(m_source, m_destination);
  expected_reply = new RDMReply(ola::rdm::RDM_FAILED_TO_SEND);
  mock_controller.ExpectCallAndReplyWith(get_request, expected_reply);

  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          expected_reply));

  mock_controller.Verify();
}


/*
 * Check that sending RDM commands work.
 * This all defer the running of the RDMCallback.
 */
void QueueingRDMControllerTest::testDelayedSendAndReceive() {
  MockRDMController mock_controller;
  ola::rdm::QueueingRDMController controller(&mock_controller, 10);

  // Some mock frame data.
  RDMFrame frame(MOCK_FRAME_DATA, arraysize(MOCK_FRAME_DATA));
  RDMFrames frames;
  frames.push_back(frame);

  // test a simple request/response
  RDMRequest *get_request = NewGetRequest(m_source, m_destination);
  mock_controller.ExpectCallAndCapture(get_request);

  RDMReply expected_reply(
      ola::rdm::RDM_COMPLETED_OK,
      NewGetResponse(m_destination, m_source),
      frames);

  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          &expected_reply));

  // now run the callback
  mock_controller.RunRDMCallback(&expected_reply);
  mock_controller.Verify();
}

/*
 * Check that overflow sequences work.
 */
void QueueingRDMControllerTest::testAckOverflows() {
  MockRDMController mock_controller;
  ola::rdm::QueueingRDMController controller(&mock_controller, 10);

  // Some mock frame data.
  RDMFrame frame1(MOCK_FRAME_DATA, arraysize(MOCK_FRAME_DATA));
  RDMFrame frame2(MOCK_FRAME_DATA2, arraysize(MOCK_FRAME_DATA2));

  // test an ack overflow sequence
  RDMRequest *get_request = NewGetRequest(m_source, m_destination);
  uint8_t data[] = {0xaa, 0xbb};

  RDMGetResponse *response1 = new RDMGetResponse(
      m_destination,
      m_source,
      0,  // transaction #
      ACK_OVERFLOW,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data,  // data
      1);  // data length

  RDMGetResponse *response2 = new RDMGetResponse(
      m_destination,
      m_source,
      0,  // transaction #
      RDM_ACK,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data + 1,  // data
      1);  // data length

  RDMGetResponse *expected_response = new RDMGetResponse(
      m_destination,
      m_source,
      0,  // transaction #
      RDM_ACK,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data,  // data
      2);  // data length

  RDMFrames frames1;
  frames1.push_back(frame1);

  RDMFrames frames2;
  frames2.push_back(frame2);

  mock_controller.ExpectCallAndReplyWith(
      get_request,
      new RDMReply(ola::rdm::RDM_COMPLETED_OK, response1, frames1));
  mock_controller.ExpectCallAndReplyWith(
      get_request,
      new RDMReply(ola::rdm::RDM_COMPLETED_OK, response2, frames2));

  RDMFrames expected_frames;
  expected_frames.push_back(frame1);
  expected_frames.push_back(frame2);

  RDMReply expected_reply(
      ola::rdm::RDM_COMPLETED_OK,
      expected_response,
      expected_frames);
  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          &expected_reply));

  // now check an broken transaction. We first ACK, the return a timeout
  get_request = NewGetRequest(m_source, m_destination);
  RDMReply timeout_reply(ola::rdm::RDM_TIMEOUT);

  response1 = new RDMGetResponse(
      m_destination,
      m_source,
      0,  // transaction #
      ACK_OVERFLOW,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data,  // data
      1);  // data length

  mock_controller.ExpectCallAndReplyWith(
      get_request,
      new RDMReply(ola::rdm::RDM_COMPLETED_OK, response1));
  mock_controller.ExpectCallAndReplyWith(
      get_request,
      new RDMReply(ola::rdm::RDM_TIMEOUT));

  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          &timeout_reply));

  // now test the case where the responses can't be combined
  get_request = NewGetRequest(m_source, m_destination);

  response1 = new RDMGetResponse(
      m_destination,
      m_source,
      0,  // transaction #
      ACK_OVERFLOW,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data,  // data
      1);  // data length

  response2 = new RDMGetResponse(
      m_source,
      m_source,
      0,  // transaction #
      RDM_ACK,  // response type
      0,  // message count
      10,  // sub device
      296,  // param id
      data + 1,  // data
      1);  // data length

  mock_controller.ExpectCallAndReplyWith(
      get_request,
      new RDMReply(ola::rdm::RDM_COMPLETED_OK, response1));
  mock_controller.ExpectCallAndReplyWith(
      get_request,
      new RDMReply(ola::rdm::RDM_COMPLETED_OK, response2));

  RDMReply invalid_reply(ola::rdm::RDM_INVALID_RESPONSE);
  controller.SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          &invalid_reply));

  mock_controller.Verify();
}

/*
 * Verify that pausing works.
 */
void QueueingRDMControllerTest::testPauseAndResume() {
  MockRDMController mock_controller;
  ola::rdm::QueueingRDMController controller(&mock_controller, 10);
  controller.Pause();



  // queue up two requests
  RDMRequest *get_request1 = NewGetRequest(m_source, m_destination);
  RDMRequest *get_request2 = NewGetRequest(m_source, m_destination);
  RDMReply *expected_reply1 = new RDMReply(
      ola::rdm::RDM_COMPLETED_OK,
      NewGetResponse(m_destination, m_source));
  RDMReply *expected_reply2 = new RDMReply(
      ola::rdm::RDM_FAILED_TO_SEND);


  // queue up two requests
  controller.SendRDMRequest(
      get_request1,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          expected_reply1));
  controller.SendRDMRequest(
      get_request2,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          expected_reply2));

  // now resume
  mock_controller.ExpectCallAndReplyWith(get_request1, expected_reply1);
  mock_controller.ExpectCallAndReplyWith(get_request2, expected_reply2);
  controller.Resume();
}

/*
 * Verify that overflowing the queue behaves
 */
void QueueingRDMControllerTest::testQueueOverflow() {
  MockRDMController mock_controller;
  unique_ptr<ola::rdm::QueueingRDMController> controller(
      new ola::rdm::QueueingRDMController(&mock_controller, 1));
  controller->Pause();

  RDMRequest *get_request = NewGetRequest(m_source, m_destination);
  RDMReply failed_to_send_reply(ola::rdm::RDM_FAILED_TO_SEND);

  controller->SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          &failed_to_send_reply));

  // this one should overflow the queue
  get_request = NewGetRequest(m_source, m_destination);
  controller->SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          &failed_to_send_reply));

  // now because we're paused the first should fail as well when the control
  // goes out of scope
}

/*
 * Verify discovery works
 */
void QueueingRDMControllerTest::testDiscovery() {
  MockRDMController mock_controller;
  unique_ptr<ola::rdm::DiscoverableQueueingRDMController> controller(
      new ola::rdm::DiscoverableQueueingRDMController(&mock_controller, 1));

  UIDSet uids, uids2;
  UID uid1(2, 3);
  UID uid2(10, 11);
  UID uid3(20, 22);
  UID uid4(65, 45);
  uids.AddUID(uid1);
  uids.AddUID(uid2);
  uids2.AddUID(uid3);
  uids2.AddUID(uid4);

  // trigger discovery, in this case the callback runs immediately.
  mock_controller.AddExpectedDiscoveryCall(true, &uids);
  controller->RunFullDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          &uids));
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  m_discovery_complete_count = 0;
  mock_controller.Verify();

  // now test incremental discovery
  // trigger discovery, in this case the callback runs immediately.
  mock_controller.AddExpectedDiscoveryCall(false, &uids);
  controller->RunIncrementalDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          &uids));
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  m_discovery_complete_count = 0;
  mock_controller.Verify();

  // now check the deferred case
  mock_controller.AddExpectedDiscoveryCall(true, NULL);
  controller->RunFullDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          &uids2));
  mock_controller.Verify();
  OLA_ASSERT_FALSE(m_discovery_complete_count);

  // now run the callback
  mock_controller.RunDiscoveryCallback(uids2);
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  m_discovery_complete_count = 0;
  mock_controller.Verify();

  // now try an incremental that deferrs the callback
  mock_controller.AddExpectedDiscoveryCall(false, NULL);
  controller->RunIncrementalDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          &uids2));
  mock_controller.Verify();
  OLA_ASSERT_FALSE(m_discovery_complete_count);

  // now run the callback
  mock_controller.RunDiscoveryCallback(uids2);
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  m_discovery_complete_count = 0;
  mock_controller.Verify();
}

/*
 * Check that attempting a discovery while another is running fails.
 */
void QueueingRDMControllerTest::testMultipleDiscovery() {
  MockRDMController mock_controller;
  unique_ptr<ola::rdm::DiscoverableQueueingRDMController> controller(
      new ola::rdm::DiscoverableQueueingRDMController(&mock_controller, 1));

  UIDSet uids, uids2;
  UID uid1(2, 3);
  UID uid2(10, 11);
  UID uid3(20, 22);
  UID uid4(65, 45);
  uids.AddUID(uid1);
  uids.AddUID(uid2);
  uids2.AddUID(uid3);
  uids2.AddUID(uid4);

  // trigger discovery, this doesn't run the callback immediately
  mock_controller.AddExpectedDiscoveryCall(true, NULL);
  controller->RunFullDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          &uids));
  mock_controller.Verify();
  OLA_ASSERT_FALSE(m_discovery_complete_count);

  // trigger discovery again, this should queue the discovery request
  controller->RunIncrementalDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          &uids2));
  mock_controller.Verify();

  // and again
  controller->RunIncrementalDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          &uids2));
  mock_controller.Verify();

  // return from the first one, this will trigger the second discovery call
  mock_controller.AddExpectedDiscoveryCall(false, NULL);
  mock_controller.RunDiscoveryCallback(uids);
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  m_discovery_complete_count = 0;
  mock_controller.Verify();

  // now return from the second one, which should complete the 2nd and 3rd
  // requests
  mock_controller.RunDiscoveryCallback(uids2);
  OLA_ASSERT_EQ(2, m_discovery_complete_count);
  m_discovery_complete_count = 0;
  mock_controller.Verify();
}

/*
 * Verify reentrant discovery works
 */
void QueueingRDMControllerTest::testReentrantDiscovery() {
  MockRDMController mock_controller;
  unique_ptr<ola::rdm::DiscoverableQueueingRDMController> controller(
      new ola::rdm::DiscoverableQueueingRDMController(&mock_controller, 1));

  UIDSet uids;
  UID uid1(2, 3);
  UID uid2(10, 11);
  uids.AddUID(uid1);
  uids.AddUID(uid2);

  // trigger discovery, the ReentrantDiscovery starts a new discovery from
  // within the callback of the first.
  mock_controller.AddExpectedDiscoveryCall(true, NULL);
  controller->RunFullDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::ReentrantDiscovery,
          controller.get(),
          &uids));
  mock_controller.Verify();

  // this will finish the first discovery attempt, and start the second
  mock_controller.AddExpectedDiscoveryCall(true, NULL);
  mock_controller.RunDiscoveryCallback(uids);
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  m_discovery_complete_count = 0;
  mock_controller.Verify();

  // now unblock the second
  mock_controller.RunDiscoveryCallback(uids);
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  m_discovery_complete_count = 0;
  mock_controller.Verify();
}

/*
 * Check that interleaving requests and discovery commands work.
 */
void QueueingRDMControllerTest::testRequestAndDiscovery() {
  MockRDMController mock_controller;
  unique_ptr<ola::rdm::DiscoverableQueueingRDMController> controller(
      new ola::rdm::DiscoverableQueueingRDMController(&mock_controller, 1));

  UIDSet uids;
  UID uid1(2, 3);
  UID uid2(10, 11);
  uids.AddUID(uid1);
  uids.AddUID(uid2);

  // Send a request, but don't run the RDM request callback
  RDMRequest *get_request = NewGetRequest(m_source, m_destination);
  mock_controller.ExpectCallAndCapture(get_request);

  RDMReply expected_reply(
      ola::rdm::RDM_COMPLETED_OK,
      NewGetResponse(m_destination, m_source));

  controller->SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          &expected_reply));

  // now queue up a discovery request
  controller->RunFullDiscovery(
      NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyDiscoveryComplete,
          &uids));
  mock_controller.Verify();
  OLA_ASSERT_FALSE(m_discovery_complete_count);

  // now run the RDM callback, this should unblock the discovery process
  mock_controller.AddExpectedDiscoveryCall(true, NULL);
  mock_controller.RunRDMCallback(&expected_reply);
  mock_controller.Verify();

  // now queue another RDM request
  RDMRequest *get_request2 = NewGetRequest(m_source, m_destination);
  RDMReply *expected_reply2 = new RDMReply(
      ola::rdm::RDM_COMPLETED_OK,
      NewGetResponse(m_destination, m_source));
  mock_controller.ExpectCallAndReplyWith(get_request2, expected_reply2);

  // discovery is still running so this won't send the request just yet.
  controller->SendRDMRequest(
      get_request2,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          expected_reply2));

  // now finish the discovery
  mock_controller.RunDiscoveryCallback(uids);
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  mock_controller.Verify();
}
