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
 * RpcChannelTest.cpp
 * Test fixture for the RpcChannel class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <google/protobuf/stubs/common.h>
#include <memory>
#include <string>

#include "common/rpc/RpcChannel.h"
#include "common/rpc/RpcController.h"
#include "common/rpc/TestService.h"
#include "common/rpc/TestService.pb.h"
#include "common/rpc/TestServiceService.pb.h"
#include "ola/Callback.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/testing/TestUtils.h"


using ola::NewSingleCallback;
using ola::io::LoopbackDescriptor;
using ola::io::SelectServer;
using ola::rpc::EchoReply;
using ola::rpc::EchoRequest;
using ola::rpc::RpcChannel;
using ola::rpc::RpcController;
using ola::rpc::STREAMING_NO_RESPONSE;
using ola::rpc::RpcController;
using ola::rpc::TestService;
using ola::rpc::TestService_Stub;
using std::auto_ptr;
using std::string;

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
  RpcController m_controller;
  EchoRequest m_request;
  EchoReply m_reply;
  SelectServer m_ss;

  auto_ptr<TestServiceImpl> m_service;
  auto_ptr<RpcChannel> m_channel;
  auto_ptr<TestService_Stub> m_stub;
  auto_ptr<LoopbackDescriptor> m_socket;
};


CPPUNIT_TEST_SUITE_REGISTRATION(RpcChannelTest);

void RpcChannelTest::setUp() {
  m_socket.reset(new LoopbackDescriptor());
  m_socket->Init();

  m_service.reset(new TestServiceImpl(&m_ss));
  m_channel.reset(new RpcChannel(m_service.get(), m_socket.get()));
  m_ss.AddReadDescriptor(m_socket.get());
  m_stub.reset(new TestService_Stub(m_channel.get()));
}

void RpcChannelTest::tearDown() {
  m_ss.RemoveReadDescriptor(m_socket.get());
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
  m_request.set_session_ptr(0);
  m_stub->Echo(&m_controller,
               &m_request,
               &m_reply,
               NewSingleCallback(this, &RpcChannelTest::EchoComplete));

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
      NewSingleCallback(this, &RpcChannelTest::FailedEchoComplete));
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
