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
 * RpcChannel.h
 * The RPC Channel
 * Copyright (C) 2005 Simon Newton
 */

#ifndef COMMON_RPC_RPCCHANNEL_H_
#define COMMON_RPC_RPCCHANNEL_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <stdint.h>
#include <google/protobuf/service.h>
#include <ola/Callback.h>
#include <ola/io/Descriptor.h>
#include <ola/util/SequenceNumber.h>
#include <memory>

#include "ola/ExportMap.h"

#include HASH_MAP_H

namespace ola {
namespace rpc {

class RpcMessage;
class RpcService;

/**
 * @brief The RPC channel used to communicate between the client and the
 * server.
 * This implementation runs over a ConnectedDescriptor which means it can be
 * used over TCP or pipes.
 */
class RpcChannel {
 public :
  /**
   * @brief The callback to run when the channel is closed.
   *
   * When run, the callback is passed the RpcSession associated with this
   * channel.
   */
  typedef SingleUseCallback1<void, class RpcSession*> CloseCallback;

    /**
     * @brief Create a new RpcChannel.
     * @param service the Service to use to handle incoming requests. Ownership
     *   is not transferred.
     * @param descriptor the descriptor to use for reading/writing data. The
     *   caller is responsible for registering the descriptor with the
     *   SelectServer. Ownership of the descriptor is not transferred.
     * @param export_map the ExportMap to use for stats
     */
    RpcChannel(RpcService *service,
               ola::io::ConnectedDescriptor *descriptor,
               ExportMap *export_map = NULL);

    /**
     * @brief Destructor
     */
    ~RpcChannel();

    /**
     * @brief Set the Service to use to handle incoming requests.
     * @param service the new Service to use, ownership is not transferred.
     */
    void SetService(RpcService *service) { m_service = service; }

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
     * The callback will be run from the call stack of the RpcChannel
     * object. This means you can't delete the RpcChannel object from
     * within the called, you'll need to queue it up and delete it later.
     */
    void SetChannelCloseHandler(CloseCallback *callback);

    /**
     * @brief Invoke an RPC method on this channel.
     */
    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    class RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    SingleUseCallback0<void> *done);

    /**
     * @brief Invoked by the RPC completion handler when the server side
     * response is ready.
     * @param request the OutstandingRequest that is now complete.
     */
    void RequestComplete(class OutstandingRequest *request);

    /**
     * @brief Return the RpcSession associated with this channel.
     * @returns the RpcSession associated with this channel.
     */
    RpcSession *Session();

    /**
     * @brief the RPC protocol version.
     */
    static const unsigned int PROTOCOL_VERSION = 1;

 private:
    typedef HASH_NAMESPACE::HASH_MAP_CLASS<int, class OutstandingResponse*>
      ResponseMap;

    std::unique_ptr<RpcSession> m_session;
    RpcService *m_service;  // service to dispatch requests to
    std::unique_ptr<CloseCallback> m_on_close;
    // the descriptor to read/write to.
    class ola::io::ConnectedDescriptor *m_descriptor;
    SequenceNumber<uint32_t> m_sequence;
    uint8_t *m_buffer;  // buffer for incoming msgs
    unsigned int m_buffer_size;  // size of the buffer
    unsigned int m_expected_size;  // the total size of the current msg
    unsigned int m_current_size;  // the amount of data read for the current msg
    HASH_NAMESPACE::HASH_MAP_CLASS<int, class OutstandingRequest*> m_requests;
    ResponseMap m_responses;
    ExportMap *m_export_map;
    UIntMap *m_recv_type_map;

    bool SendMsg(RpcMessage *msg);
    int AllocateMsgBuffer(unsigned int size);
    int ReadHeader(unsigned int *version, unsigned int *size) const;
    bool HandleNewMsg(uint8_t *buffer, unsigned int size);
    void HandleRequest(RpcMessage *msg);
    void HandleStreamRequest(RpcMessage *msg);

    // server end
    void SendRequestFailed(class OutstandingRequest *request);
    void SendNotImplemented(int msg_id);
    void DeleteOutstandingRequest(class OutstandingRequest *request);

    // client end
    void HandleResponse(RpcMessage *msg);
    void HandleFailedResponse(RpcMessage *msg);
    void HandleCanceledResponse(RpcMessage *msg);
    void HandleNotImplemented(RpcMessage *msg);

    void HandleChannelClose();

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
#endif  // COMMON_RPC_RPCCHANNEL_H_
