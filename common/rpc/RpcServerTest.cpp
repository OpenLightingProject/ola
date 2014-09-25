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
 * RpcServerTest.cpp
 * Test fixture for the RpcServer class
 * Copyright (C) 2014 Simon Newton
 */

#include <memory>

#include "common/rpc/RpcServer.h"
#include "common/rpc/RpcSession.h"
#include "common/rpc/TestService.h"
#include "common/rpc/TestServiceService.pb.h"
#include "ola/io/SelectServer.h"
#include "ola/rpc/RpcSessionHandler.h"
#include "ola/testing/TestUtils.h"

using ola::io::SelectServer;
using ola::rpc::RpcSession;
using ola::rpc::RpcServer;
using std::auto_ptr;

class RpcServerTest: public CppUnit::TestFixture,
                     public ola::rpc::RpcSessionHandlerInterface {
  CPPUNIT_TEST_SUITE(RpcServerTest);
  CPPUNIT_TEST(testEcho);
  CPPUNIT_TEST(testFailedEcho);
  CPPUNIT_TEST(testStreamRequest);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testEcho();
  void testFailedEcho();
  void testStreamRequest();

  void setUp();

  void NewClient(RpcSession *session);
  void ClientRemoved(RpcSession *session);

 private:
  SelectServer m_ss;
  auto_ptr<TestServiceImpl> m_service;
  auto_ptr<RpcServer> m_server;
  auto_ptr<TestClient> m_client;

  uint8_t ptr_data;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RpcServerTest);

void RpcServerTest::setUp() {
  m_service.reset(new TestServiceImpl(&m_ss));
  m_server.reset(new RpcServer(
      &m_ss, m_service.get(), this, RpcServer::Options()));

  OLA_ASSERT_TRUE(m_server->Init());
  m_client.reset(new TestClient(&m_ss, m_server->ListenAddress()));
  OLA_ASSERT_TRUE(m_client->Init());
}

void RpcServerTest::NewClient(RpcSession *session) {
  session->SetData(&ptr_data);
}

void RpcServerTest::ClientRemoved(RpcSession *session) {
  OLA_ASSERT_EQ(reinterpret_cast<void*>(&ptr_data), session->GetData());
}

void RpcServerTest::testEcho() {
  m_client->CallEcho(&ptr_data);
}

void RpcServerTest::testFailedEcho() {
  m_client->CallFailedEcho();
}

void RpcServerTest::testStreamRequest() {
  m_client->StreamMessage();
}
