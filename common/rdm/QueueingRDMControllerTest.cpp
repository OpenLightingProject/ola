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
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "ola/testing/TestUtils.h"

#include "ola/Logging.h"
#include "ola/Callback.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/QueueingRDMController.h"

using ola::NewSingleCallback;
using ola::rdm::ACK_OVERFLOW;
using ola::rdm::RDMCallback;
using ola::rdm::RDMDiscoveryCallback;
using ola::rdm::RDMGetResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDM_ACK;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::auto_ptr;
using std::string;
using std::vector;

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
    void setUp();

    void testSendAndReceive();
    void testDelayedSendAndReceive();
    void testAckOverflows();
    void testPauseAndResume();
    void testQueueOverflow();
    void testDiscovery();
    void testMultipleDiscovery();
    void testReentrantDiscovery();
    void testRequestAndDiscovery();

    void VerifyResponse(
        ola::rdm::rdm_response_code expected_code,
        const RDMResponse *expected_response,
        vector<string> expected_packets,
        bool delete_response,
        ola::rdm::rdm_response_code code,
        const RDMResponse *response,
        const vector<string> &packets);

    void VerifyDiscoveryComplete(UIDSet *expected_uids,
                                 const UIDSet &uids);
    void ReentrantDiscovery(
        ola::rdm::DiscoverableQueueingRDMController *controller,
        UIDSet *expected_uids,
        const UIDSet &uids);

  private:
    int m_discovery_complete_count;

    RDMRequest *NewGetRequest(const UID &source,
                              const UID &destination);
};

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
    ~MockRDMController() {}
    void SendRDMRequest(const RDMRequest *request, RDMCallback *on_complete);

    void AddExpectedCall(RDMRequest *request,
                         ola::rdm::rdm_response_code code,
                         RDMResponse *response,
                         const string &packet,
                         bool run_callback = true);

    void AddExpectedDiscoveryCall(bool full, const UIDSet *uids);

    void RunFullDiscovery(RDMDiscoveryCallback *callback);
    void RunIncrementalDiscovery(RDMDiscoveryCallback *callback);

    void RunRDMCallback(ola::rdm::rdm_response_code code,
                        RDMResponse *response,
                        const string &packet);
    void RunDiscoveryCallback(const UIDSet &uids);
    void Verify();

  private:
    typedef struct {
      RDMRequest *request;
      ola::rdm::rdm_response_code code;
      RDMResponse *response;
      string packet;
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


void MockRDMController::SendRDMRequest(const RDMRequest *request,
                                       RDMCallback *on_complete) {
  OLA_ASSERT_TRUE(m_expected_calls.size());
  expected_call call = m_expected_calls.front();
  m_expected_calls.pop();
  OLA_ASSERT_TRUE((*call.request) == (*request));
  delete request;
  vector<string> packets;
  if (!call.packet.empty())
    packets.push_back(call.packet);
  if (call.run_callback) {
    on_complete->Run(call.code, call.response, packets);
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


void MockRDMController::AddExpectedCall(RDMRequest *request,
                                        ola::rdm::rdm_response_code code,
                                        RDMResponse *response,
                                        const string &packet,
                                        bool run_callback) {
  expected_call call = {
    request,
    code,
    response,
    packet,
    run_callback
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
void MockRDMController::RunRDMCallback(ola::rdm::rdm_response_code code,
                                       RDMResponse *response,
                                       const string &packet) {
  vector<string> packets;
  if (!packet.empty())
    packets.push_back(packet);
  OLA_ASSERT_TRUE(m_rdm_callback);
  RDMCallback *callback = m_rdm_callback;
  m_rdm_callback = NULL;
  callback->Run(code, response, packets);
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


void QueueingRDMControllerTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_discovery_complete_count = 0;
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
  OLA_ASSERT_EQ(expected_code, code);
  if (expected_response)
    OLA_ASSERT_TRUE((*expected_response) == (*response));
  else
    OLA_ASSERT_EQ(expected_response, response);

  OLA_ASSERT_EQ(expected_packets.size(), packets.size());
  for (unsigned int i = 0; i < packets.size(); i++)
    OLA_ASSERT_EQ(expected_packets[i], packets[i]);

  if (delete_response)
    delete response;
}


/**
 * Verify the two UIDSets match
 */
void QueueingRDMControllerTest::VerifyDiscoveryComplete(
    UIDSet *expected_uids,
    const UIDSet &uids) {
  OLA_ASSERT_EQ(*expected_uids, uids);
  m_discovery_complete_count++;
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
 * Check that sending RDM commands work.
 * This all run the RDMCallback immediately.
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
 * Check that sending RDM commands work.
 * This all defer the running of the RDMCallback.
 */
void QueueingRDMControllerTest::testDelayedSendAndReceive() {
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
                                  NULL,
                                  "",
                                  false);

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

  // now run the callback
  mock_controller.RunRDMCallback(ola::rdm::RDM_COMPLETED_OK,
                                 &expected_command,
                                 "foo");
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
  auto_ptr<ola::rdm::QueueingRDMController> controller(
      new ola::rdm::QueueingRDMController(&mock_controller, 1));
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

  // now because we're paused the first should fail as well when the control
  // goes out of scope
}


/**
 * Verify discovery works
 */
void QueueingRDMControllerTest::testDiscovery() {
  MockRDMController mock_controller;
  auto_ptr<ola::rdm::DiscoverableQueueingRDMController> controller(
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


/**
 * Check that attempting a discovery while another is running fails.
 */
void QueueingRDMControllerTest::testMultipleDiscovery() {
  MockRDMController mock_controller;
  auto_ptr<ola::rdm::DiscoverableQueueingRDMController> controller(
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


/**
 * Verify reentrant discovery works
 */
void QueueingRDMControllerTest::testReentrantDiscovery() {
  MockRDMController mock_controller;
  auto_ptr<ola::rdm::DiscoverableQueueingRDMController> controller(
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


/**
 * Check that interleaving requests and discovery commands work.
 */
void QueueingRDMControllerTest::testRequestAndDiscovery() {
  MockRDMController mock_controller;
  auto_ptr<ola::rdm::DiscoverableQueueingRDMController> controller(
      new ola::rdm::DiscoverableQueueingRDMController(&mock_controller, 1));

  UIDSet uids;
  UID uid1(2, 3);
  UID uid2(10, 11);
  uids.AddUID(uid1);
  uids.AddUID(uid2);

  UID source(1, 2);
  UID destination(3, 4);

  // Send a request, but don't run the RDM request callback
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
                                  NULL,
                                  "",
                                  false);

  vector<string> packets;
  packets.push_back("foo");
  controller->SendRDMRequest(
      get_request,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_COMPLETED_OK,
          static_cast<const RDMResponse*>(&expected_command),
          packets,
          false));

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
  mock_controller.RunRDMCallback(ola::rdm::RDM_COMPLETED_OK,
                                 &expected_command,
                                 "foo");
  mock_controller.Verify();

  // now queue another RDM request
  RDMRequest *get_request2 = NewGetRequest(source, destination);

  RDMGetResponse expected_command2(destination,
                                   source,
                                   0,  // transaction #
                                   RDM_ACK,
                                   0,  // message count
                                   10,  // sub device
                                   296,  // param id
                                   NULL,  // data
                                   0);  // data length

  controller->SendRDMRequest(
      get_request2,
      ola::NewSingleCallback(
          this,
          &QueueingRDMControllerTest::VerifyResponse,
          ola::rdm::RDM_COMPLETED_OK,
          static_cast<const RDMResponse*>(&expected_command2),
          packets,
          false));
  // discovery is still running so this won't send the request
  mock_controller.Verify();

  // now finish the discovery
  mock_controller.AddExpectedCall(get_request2,
                                  ola::rdm::RDM_COMPLETED_OK,
                                  &expected_command,
                                  "foo");
  mock_controller.RunDiscoveryCallback(uids);
  OLA_ASSERT_TRUE(m_discovery_complete_count);
  mock_controller.Verify();
}
