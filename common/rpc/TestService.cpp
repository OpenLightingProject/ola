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
 * TestService.cpp
 * The client and server implementation for the simple echo service.
 * Copyright (C) 2014 Simon Newton
 */

#include "common/rpc/TestService.h"

#include <string>
#include <memory>

#include "common/rpc/RpcController.h"
#include "common/rpc/RpcSession.h"
#include "common/rpc/TestServiceService.pb.h"
#include "ola/io/SelectServer.h"
#include "ola/testing/TestUtils.h"
#include "common/rpc/RpcChannel.h"

using ola::NewSingleCallback;
using ola::io::SelectServer;
using ola::rpc::EchoReply;
using ola::rpc::EchoRequest;
using ola::rpc::RpcChannel;
using ola::rpc::RpcController;
using ola::rpc::STREAMING_NO_RESPONSE;
using ola::rpc::TestService_Stub;
using ola::network::TCPSocket;
using ola::network::GenericSocketAddress;
using std::string;

const char TestClient::kTestData[] = "foo";

void TestServiceImpl::Echo(OLA_UNUSED RpcController* controller,
                           OLA_UNUSED const EchoRequest* request,
                           EchoReply* response,
                           CompletionCallback* done) {
  OLA_ASSERT_TRUE(request->has_session_ptr());
  uintptr_t expected_ptr = request->session_ptr();
  OLA_ASSERT_EQ(expected_ptr,
                reinterpret_cast<uintptr_t>(controller->Session()->GetData()));
  response->set_data(request->data());
  done->Run();
}

void TestServiceImpl::FailedEcho(RpcController* controller,
                                 OLA_UNUSED const EchoRequest* request,
                                 OLA_UNUSED EchoReply* response,
                                 CompletionCallback* done) {
  controller->SetFailed("Error");
  done->Run();
}

void TestServiceImpl::Stream(RpcController* controller,
                             const ::ola::rpc::EchoRequest* request,
                             STREAMING_NO_RESPONSE* response,
                             CompletionCallback* done) {
  OLA_ASSERT_NOT_NULL(controller);
  OLA_ASSERT_FALSE(response);
  OLA_ASSERT_FALSE(done);
  OLA_ASSERT_TRUE(request);
  OLA_ASSERT_EQ(string(TestClient::kTestData), request->data());
  m_ss->Terminate();
}


TestClient::TestClient(SelectServer *ss,
                       const GenericSocketAddress &server_addr)
    : m_ss(ss),
      m_server_addr(server_addr) {
}

TestClient::~TestClient() {
  m_ss->RemoveReadDescriptor(m_socket.get());
}

bool TestClient::Init() {
  m_socket.reset(TCPSocket::Connect(m_server_addr));
  OLA_ASSERT_NOT_NULL(m_socket.get());

  m_channel.reset(new RpcChannel(NULL, m_socket.get()));
  m_stub.reset(new TestService_Stub(m_channel.get()));

  m_ss->AddReadDescriptor(m_socket.get());
  return true;
}

void TestClient::CallEcho(void *session_ptr) {
  EchoRequest request;
  EchoReply reply;
  RpcController controller;

  request.set_data(kTestData);
  request.set_session_ptr(reinterpret_cast<uintptr_t>(session_ptr));
  m_stub->Echo(
      &controller, &request, &reply,
      NewSingleCallback(this, &TestClient::EchoComplete, &controller, &reply));
  m_ss->Run();
}

void TestClient::CallFailedEcho() {
  EchoRequest request;
  EchoReply reply;
  RpcController controller;

  request.set_data(kTestData);
  m_stub->FailedEcho(
      &controller, &request, &reply,
      NewSingleCallback(this, &TestClient::FailedEchoComplete, &controller,
                        &reply));
  m_ss->Run();
}

void TestClient::StreamMessage() {
  EchoRequest request;
  request.set_data(kTestData);
  m_stub->Stream(NULL, &request, NULL, NULL);
  m_ss->Run();
}

void TestClient::EchoComplete(RpcController *controller, EchoReply *reply) {
  OLA_ASSERT_FALSE(controller->Failed());
  OLA_ASSERT_EQ(string(kTestData), reply->data());
  m_ss->Terminate();
}

void TestClient::FailedEchoComplete(RpcController *controller,
                                    OLA_UNUSED EchoReply *reply) {
  OLA_ASSERT_TRUE(controller->Failed());
  m_ss->Terminate();
}
