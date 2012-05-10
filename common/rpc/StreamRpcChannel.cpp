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
 * StreamRpcChannel.cpp
 * Interface for the UDP RPC Channel
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <errno.h>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <string>

#include "common/rpc/Rpc.pb.h"
#include "common/rpc/SimpleRpcController.h"
#include "common/rpc/StreamRpcChannel.h"
#include "ola/Callback.h"
#include "ola/Logging.h"


namespace ola {
namespace rpc {

using google::protobuf::ServiceDescriptor;

const char StreamRpcChannel::K_RPC_RECEIVED_TYPE_VAR[] = "rpc-received-type";
const char StreamRpcChannel::K_RPC_RECEIVED_VAR[] = "rpc-received";
const char StreamRpcChannel::K_RPC_SENT_ERROR_VAR[] = "rpc-send-errors";
const char StreamRpcChannel::K_RPC_SENT_VAR[] = "rpc-sent";
const char StreamRpcChannel::STREAMING_NO_RESPONSE[] = "STREAMING_NO_RESPONSE";

StreamRpcChannel::StreamRpcChannel(
    Service *service,
    ola::io::ConnectedDescriptor *descriptor,
    ExportMap *export_map)
    : m_service(service),
      m_on_close(NULL),
      m_descriptor(descriptor),
      m_seq(0),
      m_buffer(NULL),
      m_buffer_size(0),
      m_expected_size(0),
      m_current_size(0),
      m_export_map(export_map),
      m_recv_type_map(NULL) {
  descriptor->SetOnData(
      ola::NewCallback(this, &StreamRpcChannel::DescriptorReady));

  // init the counters
  const char *vars[] = {
    K_RPC_RECEIVED_VAR,
    K_RPC_SENT_ERROR_VAR,
    K_RPC_SENT_VAR,
  };

  if (m_export_map) {
    for (unsigned int i = 0; i < sizeof(vars) / sizeof(vars[0]); ++i)
      m_export_map->GetCounterVar(string(vars[i]));
    m_recv_type_map = m_export_map->GetUIntMapVar(K_RPC_RECEIVED_TYPE_VAR,
                                                  "type");
  }
}


StreamRpcChannel::~StreamRpcChannel() {
  if (m_on_close)
    delete m_on_close;
  free(m_buffer);
}


/*
 * Receive a message for this RPCChannel. Called when data is available on the
 * descriptor.
 */
void StreamRpcChannel::DescriptorReady() {
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
    m_current_size = 0;
    m_buffer_size = AllocateMsgBuffer(m_expected_size);

    if (m_buffer_size < m_expected_size) {
      OLA_WARN << "buffer size to small " << m_buffer_size << " < " <<
        m_expected_size;
      return;
    }
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


/*
 * Set the Closure to be called if a write on this channel fails. This is
 * different from the Descriptor on close handler which is called when reads hit
 * EOF/
 */
void StreamRpcChannel::SetOnClose(SingleUseCallback0<void> *closure) {
  if (closure != m_on_close) {
    delete m_on_close;
    m_on_close = closure;
  }
}


/*
 * Call a method with the given request and reply
 * TODO(simonn): reduce the number of copies here
 */
void StreamRpcChannel::CallMethod(
    const MethodDescriptor *method,
    RpcController *controller,
    const Message *request,
    Message *reply,
    google::protobuf::Closure *done) {
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
  message.set_id(m_seq++);
  message.set_name(method->name());

  request->SerializeToString(&output);
  message.set_buffer(output);
  bool r = SendMsg(&message);

  if (is_streaming)
    return;

  if (!r) {
    // send failed, call the handler now
    controller->SetFailed("Failed to send request");
    done->Run();
    return;
  }

  OutstandingResponse *response = GetOutstandingResponse(message.id());
  if (response) {
    // fail any outstanding response with the same id
    OLA_WARN << "response " << response->id << " already pending, failing " <<
      "now";
    response->controller->SetFailed("Duplicate request found");
    InvokeCallbackAndCleanup(response);
  }

  response = new OutstandingResponse();
  response->id = message.id();
  response->controller = controller;
  response->callback = done;
  response->reply = reply;
  m_responses[message.id()] = response;
}


/*
 * Called when a response is ready.
 */
void StreamRpcChannel::RequestComplete(OutstandingRequest *request) {
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


// private
//-----------------------------------------------------------------------------

/*
 * Write an RpcMessage to the write descriptor.
 */
bool StreamRpcChannel::SendMsg(RpcMessage *msg) {
  if (!m_descriptor->ValidReadDescriptor()) {
    OLA_WARN << "RPC descriptor closed, not sending messages";
    return false;
  }

  string output;
  msg->SerializeToString(&output);
  int length = output.length();
  uint32_t header;
  StreamRpcHeader::EncodeHeader(&header, PROTOCOL_VERSION, length);

  ssize_t ret = m_descriptor->Send(reinterpret_cast<const uint8_t*>(&header),
                                   sizeof(header));
  ret = m_descriptor->Send(reinterpret_cast<const uint8_t*>(output.data()),
                           length);

  if (ret != length) {
    if (ret == -1)
      OLA_WARN << "Send failed " << strerror(errno);
    else
      OLA_WARN << "Failed to send full datagram, closing channel";
    // At the point framing is screwed and we should shut the channel down
    m_descriptor->Close();
    if (m_on_close)
      m_on_close->Run();

    if (m_export_map)
      (*m_export_map->GetCounterVar(K_RPC_SENT_ERROR_VAR))++;
    return false;
  }

  if (m_export_map)
    (*m_export_map->GetCounterVar(K_RPC_SENT_VAR))++;
  return true;
}


/*
 * Allocate an incomming message buffer
 * @param size the size of the new buffer to allocate
 * @returns the size of the new buffer
 */
int StreamRpcChannel::AllocateMsgBuffer(unsigned int size) {
  unsigned int requested_size = size;
  uint8_t *new_buffer;

  if (size < m_buffer_size)
    return size;

  if (m_buffer_size == 0 && size < INITIAL_BUFFER_SIZE)
    requested_size = INITIAL_BUFFER_SIZE;

  if (requested_size > MAX_BUFFER_SIZE)
    return m_buffer_size;

  new_buffer = static_cast<uint8_t*>(realloc(m_buffer, requested_size));
  if (new_buffer < 0)
    return m_buffer_size;

  m_buffer = new_buffer;
  m_buffer_size = requested_size;
  return requested_size;
}


/*
 * Read 4 bytes and decode the header fields.
 * @returns: -1 if there is no data is available, version and size are 0
 */
int StreamRpcChannel::ReadHeader(unsigned int *version,
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

  StreamRpcHeader::DecodeHeader(header, version, size);
  return 0;
}


/*
 * Parse a new message and handle it.
 */
bool StreamRpcChannel::HandleNewMsg(uint8_t *data, unsigned int size) {
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
void StreamRpcChannel::HandleRequest(RpcMessage *msg) {
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

  OutstandingRequest *request = new OutstandingRequest();
  request->id = msg->id();
  request->controller = new SimpleRpcController();
  request->response = response_pb;

  if (m_requests.find(msg->id()) != m_requests.end()) {
    OLA_WARN << "dup sequence number for request " << msg->id();
    SendRequestFailed(m_requests[msg->id()]);
  }

  m_requests[msg->id()] = request;
  google::protobuf::Closure *callback = NewCallback(
      this, &StreamRpcChannel::RequestComplete, request);
  m_service->CallMethod(method, request->controller, request_pb, response_pb,
                        callback);
  delete request_pb;
}


/*
 * Handle a streaming RPC call. This doesn't return any response to the client.
 */
void StreamRpcChannel::HandleStreamRequest(RpcMessage *msg) {
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
    OLA_WARN << "Streaming request recieved for " << method->name() <<
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

  m_service->CallMethod(method, NULL, request_pb, NULL, NULL);
  delete request_pb;
}


// server side
/*
 * Notify the caller that the request failed.
 */
void StreamRpcChannel::SendRequestFailed(OutstandingRequest *request) {
  RpcMessage message;
  message.set_type(RESPONSE_FAILED);
  message.set_id(request->id);
  message.set_buffer(request->controller->ErrorText());
  SendMsg(&message);
  DeleteOutstandingRequest(request);
}


/*
 * Sent if we get a request for a non-existant method.
 */
void StreamRpcChannel::SendNotImplemented(int msg_id) {
  RpcMessage message;
  message.set_type(RESPONSE_NOT_IMPLEMENTED);
  message.set_id(msg_id);
  SendMsg(&message);
}


/*
 * Cleanup an outstanding request after the response has been returned
 */
void StreamRpcChannel::DeleteOutstandingRequest(OutstandingRequest *request) {
  m_requests.erase(request->id);
  delete request->controller;
  delete request->response;
  delete request;
}


// client side methods
/*
 * Handle a RPC response by invoking the callback.
 */
void StreamRpcChannel::HandleResponse(RpcMessage *msg) {
  OutstandingResponse *response = GetOutstandingResponse(msg->id());
  if (response) {
    response->reply->ParseFromString(msg->buffer());
    InvokeCallbackAndCleanup(response);
  }
}


/*
 * Handle a RPC response by invoking the callback.
 */
void StreamRpcChannel::HandleFailedResponse(RpcMessage *msg) {
  OutstandingResponse *response = GetOutstandingResponse(msg->id());
  if (response) {
    response->controller->SetFailed(msg->buffer());
    InvokeCallbackAndCleanup(response);
  }
}


/*
 * Handle a RPC response by invoking the callback.
 */
void StreamRpcChannel::HandleCanceledResponse(RpcMessage *msg) {
  OLA_INFO << "Received a canceled response";
  OutstandingResponse *response = GetOutstandingResponse(msg->id());
  if (response) {
    response->controller->SetFailed(msg->buffer());
    InvokeCallbackAndCleanup(response);
  }
}


/*
 * Handle a NOT_IMPLEMENTED by invoking the callback.
 */
void StreamRpcChannel::HandleNotImplemented(RpcMessage *msg) {
  OLA_INFO << "Received a non-implemented response";
  OutstandingResponse *response = GetOutstandingResponse(msg->id());
  if (response) {
    response->controller->SetFailed("Not Implemented");
    InvokeCallbackAndCleanup(response);
  }
}


/*
 * Find the outstanding response with id msg_id.
 */
OutstandingResponse *StreamRpcChannel::GetOutstandingResponse(int msg_id) {
  if (m_responses.find(msg_id) != m_responses.end()) {
    return m_responses[msg_id];
  }
  return NULL;
}


/*
 * Run the callback for a request.
 */
void StreamRpcChannel::InvokeCallbackAndCleanup(OutstandingResponse *response) {
  if (response) {
    int id = response->id;
    response->callback->Run();
    delete response;
    m_responses.erase(id);
  }
}


// StreamRpcHeader
//--------------------------------------------------------------

/**
 * Encode a header
 */
void StreamRpcHeader::EncodeHeader(uint32_t *header, unsigned int version,
                                   unsigned int size) {
  *header = (version << 28) & VERSION_MASK;
  *header |= size & SIZE_MASK;
}


/**
 * Decode a header
 */
void StreamRpcHeader::DecodeHeader(uint32_t header, unsigned int *version,
                                   unsigned int *size) {
  *version = (header & VERSION_MASK) >> 28;
  *size = header & SIZE_MASK;
}
}  // rpc
}  // ola
