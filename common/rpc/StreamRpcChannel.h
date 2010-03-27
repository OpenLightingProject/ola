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
 * StreamRpcChannel.h
 * Interface for the Stream RPC Channel
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef COMMON_RPC_STREAMRPCCHANNEL_H_
#define COMMON_RPC_STREAMRPCCHANNEL_H_

#include <stdint.h>
#include <ext/hash_map>
#include <google/protobuf/service.h>
#include <ola/network/Socket.h>
#include <ola/network/SelectServer.h>
#include <ola/Closure.h>
#include "ola/ExportMap.h"

namespace ola {
namespace rpc {

using google::protobuf::Message;
using google::protobuf::MethodDescriptor;
using google::protobuf::RpcChannel;
using google::protobuf::RpcController;
using google::protobuf::Service;

class RpcMessage;

class OutstandingRequest {
  /*
   * These are requests on the server end that haven't completed yet.
   */
  public:
    OutstandingRequest() {}
    ~OutstandingRequest() {}

    int id;
    RpcController *controller;
    Message *response;
};

class OutstandingResponse {
  /*
   * These are Requests on the client end that haven't completed yet.
   */
  public:
    OutstandingResponse() {}
    ~OutstandingResponse() {}

    int id;
    RpcController *controller;
    google::protobuf::Closure *callback;
    Message *reply;
};


class StreamRpcHeader {
  /*
   * The first 4 bytes are the header which contains the RPC protocol version
   * (this is separate from the protobuf version) and the size of the protobuf.
   */
  public:
    static void EncodeHeader(uint32_t *header, unsigned int version,
                             unsigned int size);
    static void DecodeHeader(uint32_t header, unsigned int *version,
                             unsigned int *size);
  private:
    static const unsigned int VERSION_MASK = 0xf0000000;
    static const unsigned int SIZE_MASK = 0x0fffffff;
};


class StreamRpcChannel: public RpcChannel {
  /*
   * Implements a RpcChannel over a pipe or socket.
   */
  public :
    StreamRpcChannel(Service *service,
                     ola::network::ConnectedSocket *socket,
                     ExportMap *export_map=NULL);
    ~StreamRpcChannel();

    int SocketReady();

    void CallMethod(
        const MethodDescriptor *method,
        RpcController *controller,
        const Message *request,
        Message *response,
        google::protobuf::Closure *done);

    void RequestComplete(OutstandingRequest *request);
    void SetService(Service *service) { m_service = service; }
    static const unsigned int PROTOCOL_VERSION = 1;

  private:
    int SendMsg(RpcMessage *msg);
    int AllocateMsgBuffer(unsigned int size);
    int ReadHeader(unsigned int *version, unsigned int *size) const;
    bool HandleNewMsg(uint8_t *buffer, unsigned int size);
    void HandleRequest(RpcMessage *msg);
    void HandleStreamRequest(RpcMessage *msg);

    // server end
    void SendRequestFailed(OutstandingRequest *request);
    void SendNotImplemented(int msg_id);
    void DeleteOutstandingRequest(OutstandingRequest *request);

    // client end
    void HandleResponse(RpcMessage *msg);
    void HandleFailedResponse(RpcMessage *msg);
    void HandleCanceledResponse(RpcMessage *msg);
    void HandleNotImplemented(RpcMessage *msg);
    OutstandingResponse *GetOutstandingResponse(int msg_id);
    void InvokeCallbackAndCleanup(OutstandingResponse *response);

    Service *m_service;  // service to dispatch requests to
    // the socket to read/write to.
    class ola::network::ConnectedSocket *m_socket;
    uint32_t m_seq;  // sequence number
    uint8_t *m_buffer;  // buffer for incomming msgs
    unsigned int m_buffer_size;  // size of the buffer
    unsigned int m_expected_size;  // the total size of the current msg
    unsigned int m_current_size;  // the amount of data read for the current msg
    __gnu_cxx::hash_map<int, OutstandingRequest*> m_requests;
    __gnu_cxx::hash_map<int, OutstandingResponse*> m_responses;
    ExportMap *m_export_map;
    UIntMap *m_recv_type_map;

    static const char K_RPC_RECEIVED_TYPE_VAR[];
    static const char K_RPC_RECEIVED_VAR[];
    static const char K_RPC_SENT_ERROR_VAR[];
    static const char K_RPC_SENT_VAR[];
    static const char STREAMING_NO_RESPONSE[];
    static const unsigned int INITIAL_BUFFER_SIZE = 1 << 11;  // 2k
    static const unsigned int MAX_BUFFER_SIZE = 1 << 20;  // 1M
};
}  // rpc
}  // ola

#endif  // COMMON_RPC_STREAMRPCCHANNEL_H_
