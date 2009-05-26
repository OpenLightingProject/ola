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

#ifndef STREAMRPCCHANNEL_H
#define STREAMRPCCHANNEL_H

#include <stdint.h>
#include <ext/hash_map>
#include <google/protobuf/service.h>
#include <lla/network/Socket.h>
#include <lla/network/SelectServer.h>
#include <lla/Closure.h>

namespace lla {
namespace rpc {

using namespace google::protobuf;

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
    static void EncodeHeader(uint32_t &header, unsigned int version,
                             unsigned int size);
    static void DecodeHeader(uint32_t header, unsigned int &version,
                             unsigned int &size);
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
                     lla::network::ConnectedSocket *socket);
    ~StreamRpcChannel();

    int SocketReady();

    void CallMethod(
        const MethodDescriptor * method,
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
    int ReadHeader(unsigned int &version, unsigned int &size) const;
    void HandleNewMsg(uint8_t *buffer, unsigned int size);
    void HandleRequest(RpcMessage *msg);

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

    Service *m_service; // service to dispatch requests to
    class lla::network::ConnectedSocket *m_socket; // the socket to read/write to.
    uint32_t m_seq; // sequence number
    uint8_t *m_buffer; // buffer for incomming msgs
    unsigned int m_buffer_size; // size of the buffer
    unsigned int m_expected_size; // the total size of the current msg
    unsigned int m_current_size; // the amount of data read for the current msg
    __gnu_cxx::hash_map<int, OutstandingRequest*> m_requests;
    __gnu_cxx::hash_map<int, OutstandingResponse*> m_responses;

    static const unsigned int INITIAL_BUFFER_SIZE = 1 << 11; // 2k
    static const unsigned int MAX_BUFFER_SIZE = 1 << 20; // 1M
};

} // rpc
} // lla

#endif
