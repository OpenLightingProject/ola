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
 * RpcChannelTest.cpp
 * Test fixture for the RpcChannel class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <google/protobuf/stubs/common.h>
#include <string>

#include "common/rpc/RpcChannel.h"
#include "common/rpc/SimpleRpcController.h"
#include "common/rpc/TestService.pb.h"
#include "common/rpc/TestServiceService.pb.h"
#include "ola/Callback.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/testing/TestUtils.h"


using ola::NewCallback;
using ola::io::LoopbackDescriptor;
using ola::io::SelectServer;
using ola::rpc::EchoReply;
using ola::rpc::EchoRequest;
using ola::rpc::RpcChannel;
using ola::rpc::RpcController;
using ola::rpc::STREAMING_NO_RESPONSE;
using ola::rpc::SimpleRpcController;
using ola::rpc::TestService;
using ola::rpc::TestService_Stub;
using std::string;

/*
 * Our test implementation
 */
class TestServiceImpl: public TestService {
  public:
    explicit TestServiceImpl(SelectServer *ss): m_ss(ss) {}
    ~TestServiceImpl() {}

    void Echo(RpcController* controller,
              const EchoRequest* request,
              EchoReply* response,
              CompletionCallback* done);

    void FailedEcho(RpcController* controller,
                    const EchoRequest* request,
                    EchoReply* response,
                    CompletionCallback* done);

    void Stream(RpcController* controller,
                const ::ola::rpc::EchoRequest* request,
                STREAMING_NO_RESPONSE* response,
                CompletionCallback* done);

  private:
    SelectServer *m_ss;
};


class RpcChannelTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RpcChannelTest);
  CPPUNIT_TEST(testEcho);
  CPPUNIT_TEST(testFailedEcho);
  CPPUNIT_TEST(testStreamRequest);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testEcho();
    void testFailedEcho();
    void testStreamRequest();
    void EchoComplete();
    void FailedEchoComplete();

  private:
    SimpleRpcController m_controller;
    EchoRequest m_request;
    EchoReply m_reply;
    TestService_Stub *m_stub;
    SelectServer m_ss;
    TestServiceImpl *m_service;
    RpcChannel *m_channel;
    LoopbackDescriptor *m_socket;
};


CPPUNIT_TEST_SUITE_REGISTRATION(RpcChannelTest);


void TestServiceImpl::Echo(RpcController* controller,
                           const EchoRequest* request,
                           EchoReply* response,
                           CompletionCallback* done) {
  response->set_data(request->data());
  done->Run();
  (void) controller;
  (void) request;
}


void TestServiceImpl::FailedEcho(RpcController* controller,
                                 const EchoRequest* request,
                                 EchoReply* response,
                                 CompletionCallback* done) {
  controller->SetFailed("Error");
  done->Run();
  (void) request;
  (void) response;
}

void TestServiceImpl::Stream(RpcController* controller,
                             const ::ola::rpc::EchoRequest* request,
                             STREAMING_NO_RESPONSE* response,
                             CompletionCallback* done) {
  OLA_ASSERT_FALSE(controller);
  OLA_ASSERT_FALSE(response);
  OLA_ASSERT_FALSE(done);
  OLA_ASSERT_TRUE(request);
  OLA_ASSERT_EQ(string("foo"), request->data());
  m_ss->Terminate();
}


void RpcChannelTest::setUp() {
  m_socket = new LoopbackDescriptor();
  m_socket->Init();

  m_service = new TestServiceImpl(&m_ss);
  m_channel = new RpcChannel(m_service, m_socket);
  m_ss.AddReadDescriptor(m_socket);
  m_stub = new TestService_Stub(m_channel);
}


void RpcChannelTest::tearDown() {
  m_ss.RemoveReadDescriptor(m_socket);
  delete m_socket;
  delete m_stub;
  delete m_channel;
  delete m_service;
}


void RpcChannelTest::EchoComplete() {
  m_ss.Terminate();
  OLA_ASSERT_FALSE(m_controller.Failed());
  OLA_ASSERT_EQ(m_reply.data(), m_request.data());
}


void RpcChannelTest::FailedEchoComplete() {
  m_ss.Terminate();
  OLA_ASSERT_TRUE(m_controller.Failed());
}


/*
 * Check that we can call the echo method in the TestServiceImpl.
 */
void RpcChannelTest::testEcho() {
  m_request.set_data("foo");
  m_stub->Echo(&m_controller,
               &m_request,
               &m_reply,
               NewCallback(this, &RpcChannelTest::EchoComplete));

  m_ss.Run();
}


/*
 * Check that method that fail return correctly
 */
void RpcChannelTest::testFailedEcho() {
  m_request.set_data("foo");
  m_stub->FailedEcho(
      &m_controller,
      &m_request,
      &m_reply,
      NewCallback(this, &RpcChannelTest::FailedEchoComplete));
  m_ss.Run();
}

/*
 * Check stream requests work
 */
void RpcChannelTest::testStreamRequest() {
  m_request.set_data("foo");
  m_stub->Stream(NULL, &m_request, NULL, NULL);
  m_ss.Run();
}
