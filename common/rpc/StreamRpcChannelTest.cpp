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
 * StreamRpcChannelTest.cpp
 * Test fixture for the StreamRpcChannel class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include <cppunit/extensions/HelperMacros.h>
#include <google/protobuf/stubs/common.h>
#include <lla/network/SelectServer.h>
#include <lla/network/Socket.h>
#include "StreamRpcChannel.h"
#include "SimpleRpcController.h"
#include "TestServiceImpl.h"
#include "TestService.pb.h"

using namespace std;
using namespace lla::rpc;
using namespace lla::network;
using namespace google::protobuf;


class StreamRpcChannelTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StreamRpcChannelTest);
  CPPUNIT_TEST(testEcho);
  CPPUNIT_TEST(testFailedEcho);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testEcho();
    void testFailedEcho();
    void EchoComplete();
    void FailedEchoComplete();

  private:
    int m_fd_pair[2];
    SimpleRpcController m_controller;
    EchoRequest m_request;
    EchoReply m_reply;
    TestService_Stub *m_stub;
    lla::network::SelectServer m_ss;
    TestServiceImpl m_service;
    StreamRpcChannel *m_channel;
    LoopbackSocket *m_socket;
};


CPPUNIT_TEST_SUITE_REGISTRATION(StreamRpcChannelTest);


void StreamRpcChannelTest::setUp() {
  m_socket = new LoopbackSocket();
  m_socket->Init();

  m_channel = new StreamRpcChannel(&m_service, m_socket);
  m_ss.AddSocket(m_socket);
  m_stub = new TestService_Stub(m_channel);
}


void StreamRpcChannelTest::tearDown() {
  m_ss.RemoveSocket(m_socket);
  delete m_socket;
  delete m_stub;
  delete m_channel;
}


void StreamRpcChannelTest::EchoComplete() {
  m_ss.Terminate();
  CPPUNIT_ASSERT(!m_controller.Failed());
  CPPUNIT_ASSERT_EQUAL(m_reply.data(), m_request.data());
}


void StreamRpcChannelTest::FailedEchoComplete() {
  m_ss.Terminate();
  CPPUNIT_ASSERT(m_controller.Failed());
}


void StreamRpcChannelTest::testEcho() {
  /*
   * Check that we can call the echo method in the TestServiceImpl.
   */
  m_request.set_data("foo");
  m_stub->Echo(&m_controller,
               &m_request,
               &m_reply,
               NewCallback(this, &StreamRpcChannelTest::EchoComplete));

  m_ss.Run();
}


void StreamRpcChannelTest::testFailedEcho() {
  /*
   * Check that method that fail return correctly
   */
  m_request.set_data("foo");
  m_stub->FailedEcho(&m_controller,
                     &m_request,
                     &m_reply,
                     NewCallback(this, &StreamRpcChannelTest::FailedEchoComplete));
  m_ss.Run();
}
