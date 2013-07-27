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
 * StreamRpcChannel.h
 * Interface for the Stream RPC Channel
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef COMMON_RPC_STREAMRPCCHANNEL_H_
#define COMMON_RPC_STREAMRPCCHANNEL_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdint.h>
#include <google/protobuf/service.h>
#include <ola/Callback.h>
#include <ola/io/Descriptor.h>
#include <ola/io/SelectServer.h>
#include <ola/util/SequenceNumber.h>
#include <memory>

#include "ola/ExportMap.h"

#include HASH_MAP_H

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


/**
 * @brief The RPC channel used to communicate between the client and the
 * server.
 * This implementation runs over a ConnectedDescriptor which means it can be
 * used over TCP or pipes.
 */
class StreamRpcChannel: public RpcChannel {
  public :
    /**
     * @@brief Create a new StreamRpcChannel.
     * @param service the Service to use to handle incoming requests. Ownership
     *   is not transferred.
     * @param descriptor the descriptor to use for reading/writing data. The
     *   caller is responsible for registering the descriptor with the
     *   SelectServer. Ownership of the descriptor is not transferred.
     * @param export_map the ExportMap to use for stats
     */
    StreamRpcChannel(Service *service,
                     ola::io::ConnectedDescriptor *descriptor,
                     ExportMap *export_map = NULL);

    /**
     * @brief Destructor
     */
    ~StreamRpcChannel();

    /**
     * @brief Set the Service to use to handle incoming requests.
     * @param service the new Service to use, ownership is not transferred.
     */
    void SetService(Service *service) { m_service = service; }

    /**
     * @brief Check if there are any pending RPCs on the channel.
     * Pending RPCs are those where a request has been sent, but no reply has
     * been received.
     * @returns true if there is one or more pending RPCs.
     */
    bool PendingRPCs() const { return !m_requests.empty(); }

    /**
     * @brief Called when new data arrives on the descriptor.
     */
    void DescriptorReady();

    /**
     * @brief Set the Callback to be run when the channel fails.
     * The callback will be invoked if the descriptor is closed, or if writes
     * to the descriptor fail.
     * @param callback the callback to run when the channel fails.
     *
     * @note
     * The callback will be run from the call stack of the StreamRpcChannel
     * object. This means you can't delete the StreamRpcChannel object from
     * within the called, you'll need to queue it up an delete it later.
     */
    void SetChannelCloseHandler(SingleUseCallback0<void> *callback);

    /**
     * @brief Invoke an RPC method on this channel.
     */
    void CallMethod(
        const MethodDescriptor *method,
        RpcController *controller,
        const Message *request,
        Message *response,
        google::protobuf::Closure *done);

    /**
     * @brief Invoked by the RPC completion handler when the server side
     * response is ready.
     * @param request the OutstandingRequest that is now complete.
     */
    void RequestComplete(OutstandingRequest *request);

    /**
     * @brief the RPC protocol version.
     */
    static const unsigned int PROTOCOL_VERSION = 1;

  private:
    typedef HASH_NAMESPACE::HASH_MAP_CLASS<int, OutstandingResponse*>
      ResponseMap;

    bool SendMsg(RpcMessage *msg);
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

    void HandleChannelClose();

    Service *m_service;  // service to dispatch requests to
    std::auto_ptr<SingleUseCallback0<void> > m_on_close;
    // the descriptor to read/write to.
    class ola::io::ConnectedDescriptor *m_descriptor;
    SequenceNumber<uint32_t> m_sequence;
    uint8_t *m_buffer;  // buffer for incomming msgs
    unsigned int m_buffer_size;  // size of the buffer
    unsigned int m_expected_size;  // the total size of the current msg
    unsigned int m_current_size;  // the amount of data read for the current msg
    HASH_NAMESPACE::HASH_MAP_CLASS<int, OutstandingRequest*> m_requests;
    ResponseMap m_responses;
    ExportMap *m_export_map;
    UIntMap *m_recv_type_map;

    static const char K_RPC_RECEIVED_TYPE_VAR[];
    static const char K_RPC_RECEIVED_VAR[];
    static const char K_RPC_SENT_ERROR_VAR[];
    static const char K_RPC_SENT_VAR[];
    static const char *K_RPC_VARIABLES[];
    static const char STREAMING_NO_RESPONSE[];
    static const unsigned int INITIAL_BUFFER_SIZE = 1 << 11;  // 2k
    static const unsigned int MAX_BUFFER_SIZE = 1 << 20;  // 1M
};
}  // namespace rpc
}  // namespace ola
#endif  // COMMON_RPC_STREAMRPCCHANNEL_H_
