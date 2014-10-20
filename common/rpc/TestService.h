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
 * TestService.h
 * The client and server implementation for the simple echo service.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef COMMON_RPC_TESTSERVICE_H_
#define COMMON_RPC_TESTSERVICE_H_

#include <memory>

#include "common/rpc/RpcController.h"
#include "common/rpc/TestServiceService.pb.h"
#include "ola/network/TCPSocket.h"
#include "ola/io/SelectServer.h"
#include "common/rpc/RpcChannel.h"

class TestServiceImpl: public ola::rpc::TestService {
 public:
  explicit TestServiceImpl(ola::io::SelectServer *ss) : m_ss(ss) {}
  ~TestServiceImpl() {}

  void Echo(ola::rpc::RpcController* controller,
            const ola::rpc::EchoRequest* request,
            ola::rpc::EchoReply* response,
            CompletionCallback* done);

  void FailedEcho(ola::rpc::RpcController* controller,
                  const ola::rpc::EchoRequest* request,
                  ola::rpc::EchoReply* response,
                  CompletionCallback* done);

  void Stream(ola::rpc::RpcController* controller,
              const ola::rpc::EchoRequest* request,
              ola::rpc::STREAMING_NO_RESPONSE* response,
              CompletionCallback* done);
 private:
  ola::io::SelectServer *m_ss;
};


class TestClient {
 public:
  TestClient(ola::io::SelectServer *ss,
             const ola::network::GenericSocketAddress &server_addr);
  ~TestClient();

  bool Init();

  // These block until the transaction completes.
  void CallEcho(void *session_ptr);
  void CallFailedEcho();
  void StreamMessage();

  // Completion handlers.
  void EchoComplete(ola::rpc::RpcController *controller,
                    ola::rpc::EchoReply *reply);
  void FailedEchoComplete(ola::rpc::RpcController *controller,
                          ola::rpc::EchoReply *reply);

  static const char kTestData[];

 private:
  ola::io::SelectServer *m_ss;
  const ola::network::GenericSocketAddress m_server_addr;
  std::auto_ptr<ola::network::TCPSocket> m_socket;
  std::auto_ptr<ola::rpc::TestService_Stub> m_stub;
  std::auto_ptr<ola::rpc::RpcChannel> m_channel;
};
#endif  // COMMON_RPC_TESTSERVICE_H_
