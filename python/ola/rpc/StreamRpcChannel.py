# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
# StreamRpcChannel.py
# Copyright (C) 2005 Simon Newton

import binascii
import logging
import struct

from google.protobuf import service
from ola.rpc import Rpc_pb2
from ola.rpc.SimpleRpcController import SimpleRpcController

from ola import ola_logger

"""A RpcChannel that works over a TCP socket."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class OutstandingRequest(object):
  """These represent requests on the server side that haven't completed yet."""
  def __init__(self, request_id, controller):
    self.controller = controller
    self.id = request_id


class OutstandingResponse(object):
  """These represent requests on the client side that haven't completed yet."""
  def __init__(self, response_id, controller, callback, reply_class):
    self.callback = callback
    self.controller = controller
    self.id = response_id
    self.reply = None
    self.reply_class = reply_class


class StreamRpcChannel(service.RpcChannel):
  """Implements a RpcChannel over a TCP socket."""
  PROTOCOL_VERSION = 1
  VERSION_MASK = 0xf0000000
  SIZE_MASK = 0x0fffffff
  RECEIVE_BUFFER_SIZE = 8192

  def __init__(self, socket, service_impl, close_callback=None):
    """Create a new StreamRpcChannel.

    Args:
      socket: the socket to communicate on
      service: the service to route incoming requests to
    """
    self._service = service_impl
    self._socket = socket
    self._sequence = 0
    self._outstanding_requests = {}
    self._outstanding_responses = {}
    self._buffer = []  # The received data
    self._expected_size = None  # The size of the message we're receiving
    self._skip_message = False  # Skip the current message
    self._close_callback = close_callback
    self._log_msgs = False  # set to enable wire message logging
    if self._log_msgs:
      logging.basicConfig(level=logging.DEBUG)

  def SocketReady(self):
    """Read data from the socket and handle when we get a full message.

    Returns:
      True if the socket remains connected, False if it was closed.
    """
    data = self._socket.recv(self.RECEIVE_BUFFER_SIZE)
    if not data:
      ola_logger.info('OLAD Server Socket closed')
      if self._close_callback is not None:
        self._close_callback()
      return False

    self._buffer.append(data)
    self._ProcessIncomingData()
    return True

  def CallMethod(self, method, controller, request, response_pb, done):
    """Call a method.

    Don't use this directly, use the Stub object.

    Args:
      method: The MethodDescriptor to call
      controller: An RpcController object
      request: The request message
      response: The response class
      done: A closure to call once complete.
    """
    message = Rpc_pb2.RpcMessage()
    message.type = Rpc_pb2.REQUEST
    message.id = self._sequence
    message.name = method.name
    message.buffer = request.SerializeToString()
    self._SendMessage(message)
    self._sequence += 1

    if message.id in self._outstanding_responses:
      # fail any outstanding response with the same id, not the best approach
      # but it'll do for now.
      ola_logger.warning('Response %d already pending, failing now', message.id)
      response = self._outstanding_responses[message.id]
      response.controller.SetFailed('Duplicate request found')
      self._InvokeCallback(response)

    response = OutstandingResponse(message.id, controller, done, response_pb)
    self._outstanding_responses[message.id] = response

  def RequestComplete(self, request, response):
    """This is called on the server side when a request has completed.

    Args:
      request: the OutstandingRequest object that has completed.
      response: the response to send to the client
    """
    message = Rpc_pb2.RpcMessage()

    if request.controller.Failed():
      self._SendRequestFailed(request)
      return

    message.type = Rpc_pb2.RESPONSE
    message.id = request.id
    message.buffer = response.SerializeToString()
    self._SendMessage(message)
    del self._outstanding_requests[request.id]

  def _EncodeHeader(self, size):
    """Encode a version and size into a header.

    Args:
      size: the size of the rpc message

    Returns:
      The header field
    """
    header = (self.PROTOCOL_VERSION << 28) & self.VERSION_MASK
    header |= size & self.SIZE_MASK
    return struct.pack('=L', header)

  def _DecodeHeader(self, header):
    """Decode a header into the version and size.

    Args:
      header: the received header

    Return:
      A tuple in the form (version, size)
    """
    return ((header & self.VERSION_MASK) >> 28, header & self.SIZE_MASK)

  def _SendMessage(self, message):
    """Send an RpcMessage.

    Args:
      message: An RpcMessage object.

    Returns:
      True if the send succeeded, False otherwise.
    """
    data = message.SerializeToString()
    # combine into one buffer to send so we avoid sending two packets
    data = self._EncodeHeader(len(data)) + data
    # this log is useful for building mock regression tests
    if self._log_msgs:
      logging.debug("send->" + str(binascii.hexlify(data)))

    sent_bytes = self._socket.send(data)
    if sent_bytes != len(data):
      ola_logger.warning('Failed to send full datagram')
      return False
    return True

  def _SendRequestFailed(self, request):
    """Send a response indicating this request failed.

    Args:
      request: An OutstandingRequest object.

    """
    message = Rpc_pb2.RpcMessage()
    message.type = Rpc_pb2.RESPONSE_FAILED
    message.id = request.id
    message.buffer = request.controller.ErrorText()
    self._SendMessage(message)
    del self._outstanding_requests[request.id]

  def _SendNotImplemented(self, message_id):
    """Send a not-implemented response.

    Args:
      message_id: The message number.
    """
    message = Rpc_pb2.RpcMessage
    message.type = Rpc_pb2.RESPONSE_NOT_IMPLEMENTED
    message.id = message_id
    self._SendMessage(message)

  def _GrabData(self, size):
    """Fetch the next N bytes of data from the buffer.

    Args:
      size: the amount of data to fetch.

    Returns:
      The next size bytes of data, or None if there isn't enough.
    """
    data_size = sum([len(s) for s in self._buffer])
    if data_size < size:
      return None

    data = []
    size_left = size
    while size_left > 0:
      chunk = self._buffer.pop(0)
      if len(chunk) > size_left:
        # we only want part of it
        data.append(chunk[0:size_left])
        self._buffer.insert(0, chunk[size_left:])
        size_left = 0
      else:
        data.append(chunk)
        size_left -= len(chunk)

    return b''.join(data)

  def _ProcessIncomingData(self):
    """Process the received data."""

    while True:
      if not self._expected_size:
        # this is a new msg
        raw_header = self._GrabData(4)
        if not raw_header:
          # not enough data yet
          return
        if self._log_msgs:
          logging.debug("recvhdr<-" + str(binascii.hexlify(raw_header)))
        header = struct.unpack('=L', raw_header)[0]
        version, size = self._DecodeHeader(header)

        if version != self.PROTOCOL_VERSION:
          ola_logger.warning('Protocol mismatch: %d != %d', version,
                             self.PROTOCOL_VERSION)
          self._skip_message = True
        self._expected_size = size

      data = self._GrabData(self._expected_size)
      if not data:
        # not enough data yet
        return

      if self._log_msgs:
        logging.debug("recvmsg<-" + str(binascii.hexlify(data)))
      if not self._skip_message:
        self._HandleNewMessage(data)
      self._expected_size = 0
      self._skip_message = 0

  def _HandleNewMessage(self, data):
    """Handle a new Message.

    Args:
      data: A chunk of data representing a RpcMessage
    """
    message = Rpc_pb2.RpcMessage()
    message.ParseFromString(data)

    if message.type in self.MESSAGE_HANDLERS:
      self.MESSAGE_HANDLERS[message.type](self, message)
    else:
      ola_logger.warning('Not sure of message type %d', message.type())

  def _HandleRequest(self, message):
    """Handle a Request message.

    Args:
      message: The RpcMessage object.
    """
    if not self._service:
      ola_logger.warning('No service registered')
      return

    descriptor = self._service.GetDescriptor()
    method = descriptor.FindMethodByName(message.name)
    if not method:
      ola_logger.warning('Failed to get method descriptor for %s', message.name)
      self._SendNotImplemented(message.id)
      return

    request_pb = self._service.GetRequestClass(method)()
    request_pb.ParseFromString(message.buffer)
    controller = SimpleRpcController()
    request = OutstandingRequest(message.id, controller)

    if message.id in self._outstanding_requests:
      ola_logger.warning('Duplicate request for %d', message.id)
      self._SendRequestFailed(message.id)

    self._outstanding_requests[message.id] = request
    self._service.CallMethod(method, request.controller, request_pb,
                             lambda x: self.RequestComplete(request, x))

  def _HandleResponse(self, message):
    """Handle a Response message.

    Args:
      message: The RpcMessage object.
    """
    response = self._outstanding_responses.get(message.id, None)
    if response:
      response.reply = response.reply_class()
      response.reply.ParseFromString(message.buffer)
      self._InvokeCallback(response)

  def _HandleCanceledResponse(self, message):
    """Handle a Not Implemented message.

    Args:
      message: The RpcMessage object.
    """
    ola_logger.warning('Received a canceled response')
    response = self._outstanding_responses.get(message.id, None)
    if response:
      response.controller.SetFailed(message.buffer)
      self._InvokeCallback(response)

  def _HandleFailedReponse(self, message):
    """Handle a Failed Response message.

    Args:
      message: The RpcMessage object.
    """
    response = self._outstanding_responses.get(message.id, None)
    if response:
      response.controller.SetFailed(message.buffer)
      self._InvokeCallback(response)

  def _HandleNotImplemented(self, message):
    """Handle a Not Implemented message.

    Args:
      message: The RpcMessage object.
    """
    ola_logger.warning('Received a non-implemented response')
    response = self._outstanding_responses.get(message.id, None)
    if response:
      response.controller.SetFailed('Not Implemented')
      self._InvokeCallback(response)

  def _InvokeCallback(self, response):
    """Run the callback and delete the outstanding response.

    Args:
      response: the resoponse to run.
    """
    response.callback(response.controller, response.reply)
    del self._outstanding_responses[response.id]

  MESSAGE_HANDLERS = {
      Rpc_pb2.REQUEST: _HandleRequest,
      Rpc_pb2.RESPONSE: _HandleResponse,
      Rpc_pb2.RESPONSE_CANCEL: _HandleCanceledResponse,
      Rpc_pb2.RESPONSE_FAILED: _HandleFailedReponse,
      Rpc_pb2.RESPONSE_NOT_IMPLEMENTED: _HandleNotImplemented,
  }
