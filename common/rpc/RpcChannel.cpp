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
 * RpcChannel.cpp
 * Interface for the UDP RPC Channel
 * Copyright (C) 2005 Simon Newton
 */

#include "common/rpc/RpcChannel.h"

#include <errno.h>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <string>

#include "common/rpc/Rpc.pb.h"
#include "common/rpc/RpcSession.h"
#include "common/rpc/RpcController.h"
#include "common/rpc/RpcHeader.h"
#include "common/rpc/RpcService.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace rpc {

using google::protobuf::Message;
using google::protobuf::MethodDescriptor;
using google::protobuf::ServiceDescriptor;
using std::auto_ptr;
using std::string;

const char RpcChannel::K_RPC_RECEIVED_TYPE_VAR[] = "rpc-received-type";
const char RpcChannel::K_RPC_RECEIVED_VAR[] = "rpc-received";
const char RpcChannel::K_RPC_SENT_ERROR_VAR[] = "rpc-send-errors";
const char RpcChannel::K_RPC_SENT_VAR[] = "rpc-sent";
const char RpcChannel::STREAMING_NO_RESPONSE[] = "STREAMING_NO_RESPONSE";

const char *RpcChannel::K_RPC_VARIABLES[] = {
  K_RPC_RECEIVED_VAR,
  K_RPC_SENT_ERROR_VAR,
  K_RPC_SENT_VAR,
};

class OutstandingRequest {
  /*
   * These are requests on the server end that haven't completed yet.
   */
 public:
  OutstandingRequest(int id,
                     RpcSession *session,
                     google::protobuf::Message *response)
      : id(id),
        controller(new RpcController(session)),
        response(response) {
  }
  ~OutstandingRequest() {
    if (controller) {
      delete controller;
    }
    if (response) {
      delete response;
    }
  }

  int id;
  RpcController *controller;
  google::protobuf::Message *response;
};


class OutstandingResponse {
  /*
   * These are Requests on the client end that haven't completed yet.
   */
 public:
  OutstandingResponse(int id,
                      RpcController *controller,
                      SingleUseCallback0<void> *callback,
                      Message *reply)
      : id(id),
        controller(controller),
        callback(callback),
        reply(reply) {
  }

  int id;
  RpcController *controller;
  SingleUseCallback0<void> *callback;
  Message *reply;
};

RpcChannel::RpcChannel(
    RpcService *service,
    ola::io::ConnectedDescriptor *descriptor,
    ExportMap *export_map)
    : m_session(new RpcSession(this)),
      m_service(service),
      m_descriptor(descriptor),
      m_buffer(NULL),
      m_buffer_size(0),
      m_expected_size(0),
      m_current_size(0),
      m_export_map(export_map),
      m_recv_type_map(NULL) {
  if (descriptor) {
    descriptor->SetOnData(
        ola::NewCallback(this, &RpcChannel::DescriptorReady));
    descriptor->SetOnClose(
        ola::NewSingleCallback(this, &RpcChannel::HandleChannelClose));
  }

  if (m_export_map) {
    for (unsigned int i = 0; i < arraysize(K_RPC_VARIABLES); ++i) {
      m_export_map->GetCounterVar(string(K_RPC_VARIABLES[i]));
    }
    m_recv_type_map = m_export_map->GetUIntMapVar(K_RPC_RECEIVED_TYPE_VAR,
                                                  "type");
  }
}

RpcChannel::~RpcChannel() {
  free(m_buffer);
}

void RpcChannel::DescriptorReady() {
  if (!m_expected_size) {
    // this is a new msg
    unsigned int version;
    if (ReadHeader(&version, &m_expected_size) < 0)
      return;

    if (!m_expected_size)
      return;

    if (version != PROTOCOL_VERSION) {
      OLA_WARN << "protocol mismatch " << version << " != " <<
        PROTOCOL_VERSION;
      return;
    }

    if (m_expected_size > MAX_BUFFER_SIZE) {
      OLA_WARN << "Incoming message size " << m_expected_size
                << " is larger than MAX_BUFFER_SIZE: " << MAX_BUFFER_SIZE;
      m_descriptor->Close();
      return;
    }

    m_current_size = 0;
    m_buffer_size = AllocateMsgBuffer(m_expected_size);

    if (m_buffer_size < m_expected_size) {
      OLA_WARN << "buffer size to small " << m_buffer_size << " < " <<
        m_expected_size;
      return;
    }
  }

  if (!m_descriptor) {
    return;
  }

  unsigned int data_read;
  if (m_descriptor->Receive(m_buffer + m_current_size,
                            m_expected_size - m_current_size,
                            data_read) < 0) {
    OLA_WARN << "something went wrong in descriptor recv\n";
    return;
  }

  m_current_size += data_read;

  if (m_current_size == m_expected_size) {
    // we've got all of this message so parse it.
    if (!HandleNewMsg(m_buffer, m_expected_size)) {
      // this probably means we've messed the framing up, close the channel
      OLA_WARN << "Errors detected on RPC channel, closing";
      m_descriptor->Close();
    }
    m_expected_size = 0;
  }
  return;
}

void RpcChannel::SetChannelCloseHandler(CloseCallback *callback) {
  m_on_close.reset(callback);
}

void RpcChannel::CallMethod(const MethodDescriptor *method,
                            RpcController *controller,
                            const Message *request,
                            Message *reply,
                            SingleUseCallback0<void> *done) {
  // TODO(simonn): reduce the number of copies here
  string output;
  RpcMessage message;
  bool is_streaming = false;

  // Streaming methods are those with a reply set to STREAMING_NO_RESPONSE and
  // no controller, request or closure provided
  if (method->output_type()->name() == STREAMING_NO_RESPONSE) {
    if (controller || reply || done) {
      OLA_FATAL << "Calling streaming method " << method->name() <<
        " but a controller, reply or closure in non-NULL";
      return;
    }
    is_streaming = true;
  }

  message.set_type(is_streaming ? STREAM_REQUEST : REQUEST);
  message.set_id(m_sequence.Next());
  message.set_name(method->name());

  request->SerializeToString(&output);
  message.set_buffer(output);
  bool r = SendMsg(&message);

  if (is_streaming)
    return;

  if (!r) {
    // Send failed, call the handler now.
    controller->SetFailed("Failed to send request");
    done->Run();
    return;
  }

  OutstandingResponse *response = new OutstandingResponse(
      message.id(), controller, done, reply);

  auto_ptr<OutstandingResponse> old_response(
      STLReplacePtr(&m_responses, message.id(), response));

  if (old_response.get()) {
    // fail any outstanding response with the same id
    OLA_WARN << "response " << old_response->id << " already pending, failing "
             << "now";
    response->controller->SetFailed("Duplicate request found");
    response->callback->Run();
  }
}

void RpcChannel::RequestComplete(OutstandingRequest *request) {
  string output;
  RpcMessage message;

  if (request->controller->Failed()) {
    SendRequestFailed(request);
    return;
  }

  message.set_type(RESPONSE);
  message.set_id(request->id);
  request->response->SerializeToString(&output);
  message.set_buffer(output);
  SendMsg(&message);
  DeleteOutstandingRequest(request);
}

RpcSession *RpcChannel::Session() {
  return m_session.get();
}

// private
//-----------------------------------------------------------------------------

/*
 * Write an RpcMessage to the write descriptor.
 */
bool RpcChannel::SendMsg(RpcMessage *msg) {
  if (!(m_descriptor && m_descriptor->ValidReadDescriptor())) {
    OLA_WARN << "RPC descriptor closed, not sending messages";
    return false;
  }

  uint32_t header;
  // reserve the first 4 bytes for the header
  string output(sizeof(header), 0);
  msg->AppendToString(&output);
  int length = output.size();

  RpcHeader::EncodeHeader(&header, PROTOCOL_VERSION,
                                length - sizeof(header));
  output.replace(
      0, sizeof(header),
      reinterpret_cast<const char*>(&header), sizeof(header));

  ssize_t ret = m_descriptor->Send(
      reinterpret_cast<const uint8_t*>(output.data()), length);

  if (ret != length) {
    OLA_WARN << "Failed to send full RPC message, closing channel";

    if (m_export_map) {
      (*m_export_map->GetCounterVar(K_RPC_SENT_ERROR_VAR))++;
    }

    // At this point there is no point using the descriptor since framing has
    // probably been messed up.
    // TODO(simon): consider if it's worth leaving the descriptor open for
    // reading.
    m_descriptor = NULL;

    HandleChannelClose();
    return false;
  }

  if (m_export_map) {
    (*m_export_map->GetCounterVar(K_RPC_SENT_VAR))++;
  }
  return true;
}


/*
 * Allocate an incoming message buffer
 * @param size the size of the new buffer to allocate
 * @returns the size of the new buffer
 */
int RpcChannel::AllocateMsgBuffer(unsigned int size) {
  unsigned int requested_size = size;
  uint8_t *new_buffer;

  if (size < m_buffer_size)
    return size;

  if (m_buffer_size == 0 && size < INITIAL_BUFFER_SIZE)
    requested_size = INITIAL_BUFFER_SIZE;

  if (requested_size > MAX_BUFFER_SIZE) {
    OLA_WARN << "Incoming message size " << requested_size
              << " is larger than MAX_BUFFER_SIZE: " << MAX_BUFFER_SIZE;
    return m_buffer_size;
  }

  new_buffer = static_cast<uint8_t*>(realloc(m_buffer, requested_size));
  if (!new_buffer)
    return m_buffer_size;

  m_buffer = new_buffer;
  m_buffer_size = requested_size;
  return requested_size;
}


/*
 * Read 4 bytes and decode the header fields.
 * @returns: -1 if there is no data is available, version and size are 0
 */
int RpcChannel::ReadHeader(unsigned int *version,
                                 unsigned int *size) const {
  uint32_t header;
  unsigned int data_read = 0;
  *version = *size = 0;

  if (m_descriptor->Receive(reinterpret_cast<uint8_t*>(&header),
                            sizeof(header), data_read)) {
    OLA_WARN << "read header error: " << strerror(errno);
    return -1;
  }

  if (!data_read)
    return 0;

  RpcHeader::DecodeHeader(header, version, size);
  return 0;
}


/*
 * Parse a new message and handle it.
 */
bool RpcChannel::HandleNewMsg(uint8_t *data, unsigned int size) {
  RpcMessage msg;
  if (!msg.ParseFromArray(data, size)) {
    OLA_WARN << "Failed to parse RPC";
    return false;
  }

  if (m_export_map)
    (*m_export_map->GetCounterVar(K_RPC_RECEIVED_VAR))++;

  switch (msg.type()) {
    case REQUEST:
      if (m_recv_type_map)
        (*m_recv_type_map)["request"]++;
      HandleRequest(&msg);
      break;
    case RESPONSE:
      if (m_recv_type_map)
        (*m_recv_type_map)["response"]++;
      HandleResponse(&msg);
      break;
    case RESPONSE_CANCEL:
      if (m_recv_type_map)
        (*m_recv_type_map)["cancelled"]++;
      HandleCanceledResponse(&msg);
      break;
    case RESPONSE_FAILED:
      if (m_recv_type_map)
        (*m_recv_type_map)["failed"]++;
      HandleFailedResponse(&msg);
      break;
    case RESPONSE_NOT_IMPLEMENTED:
      if (m_recv_type_map)
        (*m_recv_type_map)["not-implemented"]++;
      HandleNotImplemented(&msg);
      break;
    case STREAM_REQUEST:
      if (m_recv_type_map)
        (*m_recv_type_map)["stream_request"]++;
      HandleStreamRequest(&msg);
      break;
    default:
      OLA_WARN << "not sure of msg type " << msg.type();
      break;
  }
  return true;
}


/*
 * Handle a new RPC method call.
 */
void RpcChannel::HandleRequest(RpcMessage *msg) {
  if (!m_service) {
    OLA_WARN << "no service registered";
    return;
  }

  const ServiceDescriptor *service = m_service->GetDescriptor();
  if (!service) {
    OLA_WARN << "failed to get service descriptor";
    return;
  }
  const MethodDescriptor *method = service->FindMethodByName(msg->name());
  if (!method) {
    OLA_WARN << "failed to get method descriptor";
    SendNotImplemented(msg->id());
    return;
  }

  Message* request_pb = m_service->GetRequestPrototype(method).New();
  Message* response_pb = m_service->GetResponsePrototype(method).New();

  if (!request_pb || !response_pb) {
    OLA_WARN << "failed to get request or response objects";
    return;
  }

  if (!request_pb->ParseFromString(msg->buffer())) {
    OLA_WARN << "parsing of request pb failed";
    return;
  }

  OutstandingRequest *request = new OutstandingRequest(
      msg->id(), m_session.get(), response_pb);

  if (m_requests.find(msg->id()) != m_requests.end()) {
    OLA_WARN << "dup sequence number for request " << msg->id();
    SendRequestFailed(m_requests[msg->id()]);
  }

  m_requests[msg->id()] = request;
  SingleUseCallback0<void> *callback = NewSingleCallback(
      this, &RpcChannel::RequestComplete, request);
  m_service->CallMethod(method, request->controller, request_pb, response_pb,
                        callback);
  delete request_pb;
}


/*
 * Handle a streaming RPC call. This doesn't return any response to the client.
 */
void RpcChannel::HandleStreamRequest(RpcMessage *msg) {
  if (!m_service) {
    OLA_WARN << "no service registered";
    return;
  }

  const ServiceDescriptor *service = m_service->GetDescriptor();
  if (!service) {
    OLA_WARN << "failed to get service descriptor";
    return;
  }
  const MethodDescriptor *method = service->FindMethodByName(msg->name());
  if (!method) {
    OLA_WARN << "failed to get method descriptor";
    SendNotImplemented(msg->id());
    return;
  }

  if (method->output_type()->name() != STREAMING_NO_RESPONSE) {
    OLA_WARN << "Streaming request received for " << method->name() <<
      ", but the output type isn't STREAMING_NO_RESPONSE";
    return;
  }

  Message* request_pb = m_service->GetRequestPrototype(method).New();

  if (!request_pb) {
    OLA_WARN << "failed to get request or response objects";
    return;
  }

  if (!request_pb->ParseFromString(msg->buffer())) {
    OLA_WARN << "parsing of request pb failed";
    return;
  }

  RpcController controller(m_session.get());
  m_service->CallMethod(method, &controller, request_pb, NULL, NULL);
  delete request_pb;
}


// server side
/*
 * Notify the caller that the request failed.
 */
void RpcChannel::SendRequestFailed(OutstandingRequest *request) {
  RpcMessage message;
  message.set_type(RESPONSE_FAILED);
  message.set_id(request->id);
  message.set_buffer(request->controller->ErrorText());
  SendMsg(&message);
  DeleteOutstandingRequest(request);
}


/*
 * Sent if we get a request for a non-existent method.
 */
void RpcChannel::SendNotImplemented(int msg_id) {
  RpcMessage message;
  message.set_type(RESPONSE_NOT_IMPLEMENTED);
  message.set_id(msg_id);
  SendMsg(&message);
}


/*
 * Cleanup an outstanding request after the response has been returned
 */
void RpcChannel::DeleteOutstandingRequest(OutstandingRequest *request) {
  STLRemoveAndDelete(&m_requests, request->id);
}


// client side methods
/*
 * Handle a RPC response by invoking the callback.
 */
void RpcChannel::HandleResponse(RpcMessage *msg) {
  auto_ptr<OutstandingResponse> response(
      STLLookupAndRemovePtr(&m_responses, msg->id()));
  if (response.get()) {
    if (!response->reply->ParseFromString(msg->buffer())) {
      OLA_WARN << "Failed to parse response proto for "
               << response->reply->GetTypeName();
    }
    response->callback->Run();
  }
}


/*
 * Handle a RPC response by invoking the callback.
 */
void RpcChannel::HandleFailedResponse(RpcMessage *msg) {
  auto_ptr<OutstandingResponse> response(
      STLLookupAndRemovePtr(&m_responses, msg->id()));
  if (response.get()) {
    response->controller->SetFailed(msg->buffer());
    response->callback->Run();
  }
}


/*
 * Handle a RPC response by invoking the callback.
 */
void RpcChannel::HandleCanceledResponse(RpcMessage *msg) {
  OLA_INFO << "Received a canceled response";
  auto_ptr<OutstandingResponse> response(
      STLLookupAndRemovePtr(&m_responses, msg->id()));
  if (response.get()) {
    response->controller->SetFailed(msg->buffer());
    response->callback->Run();
  }
}


/*
 * Handle a NOT_IMPLEMENTED by invoking the callback.
 */
void RpcChannel::HandleNotImplemented(RpcMessage *msg) {
  OLA_INFO << "Received a non-implemented response";
  auto_ptr<OutstandingResponse> response(
      STLLookupAndRemovePtr(&m_responses, msg->id()));
  if (response.get()) {
    response->controller->SetFailed("Not Implemented");
    response->callback->Run();
  }
}

/*
 * Invoke the Channel close handler/
 */
void RpcChannel::HandleChannelClose() {
  if (m_on_close.get()) {
    m_on_close.release()->Run(m_session.get());
  }
}
}  // namespace rpc
}  // namespace ola
