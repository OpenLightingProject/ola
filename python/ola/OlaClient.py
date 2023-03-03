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
# OlaClient.py
# Copyright (C) 2005 Simon Newton

import array
import socket
import struct
import sys

from ola.rpc.SimpleRpcController import SimpleRpcController
from ola.rpc.StreamRpcChannel import StreamRpcChannel
from ola.UID import UID

from ola import Ola_pb2

"""The client used to communicate with the Ola Server."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


"""The port that the OLA server listens on."""
OLA_PORT = 9010


class Error(Exception):
  """The base error class."""


class OLADNotRunningException(Error):
  """Thrown if we try to connect and olad isn't running."""


class Plugin(object):
  """Represents a plugin.

  Attributes:
    id: the id of this plugin
    name: the name of this plugin
    active: whether this plugin is active
    enabled: whether this plugin is enabled
  """
  def __init__(self, plugin_id, name, active, enabled):
    self._id = plugin_id
    self._name = name
    self._active = active
    self._enabled = enabled

  @property
  def id(self):
    return self._id

  @property
  def name(self):
    return self._name

  @property
  def active(self):
    return self._active

  @property
  def enabled(self):
    return self._enabled

  @staticmethod
  def FromProtobuf(plugin_pb):
    return Plugin(plugin_pb.plugin_id,
                  plugin_pb.name,
                  plugin_pb.active,
                  plugin_pb.enabled)

  def __repr__(self):
    s = 'Plugin(id={id}, name="{name}", active={active}, enabled={enabled})'
    return s.format(id=self.id,
                    name=self.name,
                    active=self.active,
                    enabled=self.enabled)

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self.id == other.id

  def __lt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self.id < other.id

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other

  def __hash__(self):
    return hash(self._id)


# Populate the Plugin class attributes from the protobuf
for value in Ola_pb2._PLUGINIDS.values:
  setattr(Plugin, value.name, value.number)


class Device(object):
  """Represents a device.

  Attributes:
    id: the unique id of this device
    alias: the integer alias for this device
    name: the name of this device
    plugin_id: the plugin that this device belongs to
    input_ports: a list of Input Port objects
    output_ports: a list of Output Port objects
  """
  def __init__(self, device_id, alias, name, plugin_id, input_ports,
               output_ports):
    self._id = device_id
    self._alias = alias
    self._name = name
    self._plugin_id = plugin_id
    self._input_ports = sorted(input_ports)
    self._output_ports = sorted(output_ports)

  @property
  def id(self):
    return self._id

  @property
  def alias(self):
    return self._alias

  @property
  def name(self):
    return self._name

  @property
  def plugin_id(self):
    return self._plugin_id

  @property
  def input_ports(self):
    return self._input_ports

  @property
  def output_ports(self):
    return self._output_ports

  @staticmethod
  def FromProtobuf(device_pb):
    input_ports = [Port.FromProtobuf(x) for x in device_pb.input_port]
    output_ports = [Port.FromProtobuf(x) for x in device_pb.output_port]

    return Device(device_pb.device_id,
                  device_pb.device_alias,
                  device_pb.device_name,
                  device_pb.plugin_id,
                  input_ports,
                  output_ports)

  def __repr__(self):
    s = 'Device(id="{id}", alias={alias}, name="{name}", ' \
        'plugin_id={plugin_id}, {nr_inputs} inputs, {nr_outputs} outputs)'
    return s.format(id=self.id,
                    alias=self.alias,
                    name=self.name,
                    plugin_id=self.plugin_id,
                    nr_inputs=len(self.input_ports),
                    nr_outputs=len(self.output_ports))

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self.alias == other.alias

  def __lt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self.alias < other.alias

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other


class Port(object):
  """Represents a port.

  Attributes:
    id: the unique id of this port
    universe: the universe that this port belongs to
    active: True if this port is active
    description: the description of the port
    supports_rdm: if the port supports RDM
  """
  def __init__(self, port_id, universe, active, description, supports_rdm):
    self._id = port_id
    self._universe = universe
    self._active = active
    self._description = description
    self._supports_rdm = supports_rdm

  @property
  def id(self):
    return self._id

  @property
  def universe(self):
    return self._universe

  @property
  def active(self):
    return self._active

  @property
  def description(self):
    return self._description

  @property
  def supports_rdm(self):
    return self._supports_rdm

  @staticmethod
  def FromProtobuf(port_pb):
    universe = port_pb.universe if port_pb.HasField('universe') else None
    return Port(port_pb.port_id,
                universe,
                port_pb.active,
                port_pb.description,
                port_pb.supports_rdm)

  def __repr__(self):
    s = 'Port(id={id}, universe={universe}, active={active}, ' \
        'description="{desc}", supports_rdm={supports_rdm})'
    return s.format(id=self.id,
                    universe=self.universe,
                    active=self.active,
                    desc=self.description,
                    supports_rdm=self.supports_rdm)

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self.id == other.id

  def __lt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self.id < other.id

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other

  def __hash__(self):
    return hash(self._id)


class Universe(object):
  """Represents a universe.

  Attributes:
    id: the integer universe id
    name: the name of this universe
    merge_mode: the merge mode this universe is using
  """

  LTP = Ola_pb2.LTP
  HTP = Ola_pb2.HTP

  def __init__(self, universe_id, name, merge_mode, input_ports, output_ports):
    self._id = universe_id
    self._name = name
    self._merge_mode = merge_mode
    self._input_ports = sorted(input_ports)
    self._output_ports = sorted(output_ports)

  @property
  def id(self):
    return self._id

  @property
  def name(self):
    return self._name

  @property
  def merge_mode(self):
    return self._merge_mode

  @property
  def input_ports(self):
    return self._input_ports

  @property
  def output_ports(self):
    return self._output_ports

  @staticmethod
  def FromProtobuf(universe_pb):
    input_ports = [Port.FromProtobuf(x) for x in universe_pb.input_ports]
    output_ports = [Port.FromProtobuf(x) for x in universe_pb.output_ports]

    return Universe(universe_pb.universe,
                    universe_pb.name,
                    universe_pb.merge_mode,
                    input_ports,
                    output_ports)

  def __repr__(self):
    merge_mode = 'LTP' if self.merge_mode == Universe.LTP else 'HTP'
    s = 'Universe(id={id}, name="{name}", merge_mode={merge_mode})'
    return s.format(id=self.id,
                    name=self.name,
                    merge_mode=merge_mode)

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self.id == other.id

  def __lt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self.id < other.id

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other


class RequestStatus(object):
  """Represents the status of an request.

  Attributes:
    state: the state of the operation
    message: an error message if it failed
  """
  SUCCESS, FAILED, CANCELLED = range(3)

  def __init__(self, controller):
    if controller.Failed():
      self._state = self.FAILED
      self._message = controller.ErrorText()
    elif controller.IsCanceled():
      self._state = self.CANCELLED
      self._message = controller.ErrorText()
    else:
      self._state = self.SUCCESS
      self._message = None

  def Succeeded(self):
    """Returns true if this request succeeded."""
    return self._state == self.SUCCESS

  @property
  def state(self):
    return self._state

  @property
  def message(self):
    return self._message


class RDMNack(object):
  """Nack response to a request.

      Individual NACK response reasons can be access as attrs, e.g.
      RMDNack.NR_FORMAT_ERROR
      """

  NACK_SYMBOLS_TO_VALUES = {
    'NR_UNKNOWN_PID': (0, 'Unknown PID'),
    'NR_FORMAT_ERROR': (1, 'Format Error'),
    'NR_HARDWARE_FAULT': (2, 'Hardware fault'),
    'NR_PROXY_REJECT': (3, 'Proxy reject'),
    'NR_WRITE_PROTECT': (4, 'Write protect'),
    'NR_UNSUPPORTED_COMMAND_CLASS': (5, 'Unsupported command class'),
    'NR_DATA_OUT_OF_RANGE': (6, 'Data out of range'),
    'NR_BUFFER_FULL': (7, 'Buffer full'),
    'NR_PACKET_SIZE_UNSUPPORTED': (8, 'Packet size unsupported'),
    'NR_SUB_DEVICE_OUT_OF_RANGE': (9, 'Sub device out of range'),
    'NR_PROXY_BUFFER_FULL': (10, 'Proxy buffer full'),
    'NR_ACTION_NOT_SUPPORTED': (11, 'Action not supported'),
    'NR_ENDPOINT_NUMBER_INVALID': (12, 'Endpoint number invalid'),
    'NR_INVALID_ENDPOINT_MODE': (13, 'Invalid endpoint mode'),
    'NR_UNKNOWN_UID': (14, 'Unknown UID'),
    'NR_UNKNOWN_SCOPE': (15, 'Unknown scope'),
    'NR_INVALID_STATIC_CONFIG_TYPE': (16, 'Invalid static config type'),
    'NR_INVALID_IPV4_ADDRESS': (17, 'Invalid IPv4 address'),
    'NR_INVALID_IPV6_ADDRESS': (18, 'Invalid IPv6 address'),
    'NR_INVALID_PORT': (19, 'Invalid port'),
  }

  # this is populated below
  _CODE_TO_OBJECT = {}

  def __init__(self, nack_value, description):
    self._value = nack_value
    self._description = description

  @property
  def value(self):
    return self._value

  @property
  def description(self):
      return self._description

  def __repr__(self):
    s = 'RDMNack(value={value}, desc="{desc}")'
    return s.format(value=self.value,
                    desc=self.description)

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self.value == other.value

  def __lt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self.value < other.value

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other

  def __hash__(self):
    return hash(self._value)

  @classmethod
  def LookupCode(cls, code):
    obj = cls._CODE_TO_OBJECT.get(code, None)
    if not obj:
      obj = RDMNack(code, 'Unknown')
    return obj


for symbol, (value, description) in RDMNack.NACK_SYMBOLS_TO_VALUES.items():
  nack = RDMNack(value, description)
  setattr(RDMNack, symbol, nack)
  RDMNack._CODE_TO_OBJECT[value] = nack


class RDMFrame(object):
  """The raw data in an RDM frame.

  The timing attributes may be 0 if the plugin does not provide timing
  information. All timing data is in nano-seconds.

  Attributes:
    data: The raw byte data.
    response_delay: The time between the request and the response.
    break_time: The break duration.
    mark_time: The mark duration.
    data_time: The data time.
  """
  def __init__(self, frame):
    self._data = frame.raw_response
    self._response_delay = frame.timing.response_delay
    self._break_time = frame.timing.break_time
    self._mark_time = frame.timing.mark_time
    self._data_time = frame.timing.data_time

  @property
  def data(self):
    return self._data

  @property
  def response_delay(self):
    return self._response_delay

  @property
  def break_time(self):
    return self._break_time

  @property
  def mark_time(self):
    return self._mark_time

  @property
  def data_time(self):
    return self._data_time


class RDMResponse(object):
  """Represents a RDM Response.

  Failures can occur at many layers, the recommended way for dealing with
  responses is:
    Check .status.Succeeded(), if not true this indicates a rpc or server
      error.
    Check .response_code, if not RDM_COMPLETED_OK, it indicates a problem with
      the RDM transport layer or malformed response.
    If .response_code is RDM_COMPLETED_OK, .sub_device, .command_class, .pid,
    .queued_messages hold the properties of the response.
    Then check .response_type:
    if .response_type is ACK:
      .data holds the param data of the response.
    If .response_type is ACK_TIMER:
      .ack_timer: holds the number of ms before the response should be
      available.
    If .response_type is NACK_REASON:
      .nack_reason holds the reason for nack'ing

  Attributes:
    status: The RequestStatus object for this request / response
    response_code: The response code for the RDM request
    response_type: The response type (ACK, ACK_TIMER, NACK_REASON) for
      the request.
    sub_device: The sub device that sent the response
    command_class:
    pid:
    data:
    queued_messages: The number of queued messages the remain.
    nack_reason: If the response type was NACK_REASON, this is the reason for
      the NACK.
    ack_timer: If the response type was ACK_TIMER, this is the number of ms to
      wait before checking for queued messages.
    transaction_number:
    frames: A list of RDM frames that made up this response.
  """

  RESPONSE_CODES_TO_STRING = {
      Ola_pb2.RDM_COMPLETED_OK: 'Ok',
      Ola_pb2.RDM_WAS_BROADCAST: 'Request was broadcast',
      Ola_pb2.RDM_FAILED_TO_SEND: 'Failed to send request',
      Ola_pb2.RDM_TIMEOUT: 'Response Timeout',
      Ola_pb2.RDM_INVALID_RESPONSE: 'Invalid Response',
      Ola_pb2.RDM_UNKNOWN_UID: 'Unknown UID',
      Ola_pb2.RDM_CHECKSUM_INCORRECT: 'Incorrect Checksum',
      Ola_pb2.RDM_TRANSACTION_MISMATCH: 'Transaction number mismatch',
      Ola_pb2.RDM_SUB_DEVICE_MISMATCH: 'Sub device mismatch',
      Ola_pb2.RDM_SRC_UID_MISMATCH: 'Source UID in response doesn\'t match',
      Ola_pb2.RDM_DEST_UID_MISMATCH: (
          'Destination UID in response doesn\'t match'),
      Ola_pb2.RDM_WRONG_SUB_START_CODE: 'Incorrect sub start code',
      Ola_pb2.RDM_PACKET_TOO_SHORT: (
          'RDM response was smaller than the minimum size'),
      Ola_pb2.RDM_PACKET_LENGTH_MISMATCH: (
          'The length field of packet didn\'t match length received'),
      Ola_pb2.RDM_PARAM_LENGTH_MISMATCH: (
          'The parameter length exceeds the remaining packet size'),
      Ola_pb2.RDM_INVALID_COMMAND_CLASS: (
          'The command class was not one of GET_RESPONSE or SET_RESPONSE'),
      Ola_pb2.RDM_COMMAND_CLASS_MISMATCH: (
          'The command class didn\'t match the request'),
      Ola_pb2.RDM_INVALID_RESPONSE_TYPE: (
          'The response type was not ACK, ACK_OVERFLOW, ACK_TIMER or NACK'),
      Ola_pb2.RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED: (
          'The DISCOVERY Command Class is not supported by this controller'),
      Ola_pb2.RDM_DUB_RESPONSE: (
          'Discovery Unique Branch response')
  }

  def __init__(self, controller, response):
    """
    Create a new RDMResponse object.

    Args:
      controller: The RpcController
      response: A RDMResponse proto message.
    """
    self._frames = []

    self.status = RequestStatus(controller)
    if self.status.Succeeded() and response is not None:
      self._response_code = response.response_code
      self._response_type = response.response_type
      self._queued_messages = response.message_count
      self._transaction_number = response.transaction_number
      self.sub_device = response.sub_device
      self.command_class = response.command_class
      self.pid = response.param_id
      self.data = response.data

      for frame in response.raw_frame:
        self._frames.append(RDMFrame(frame))

    # we populate these below if required
    self._nack_reason = None
    self._ack_timer = None

    if (self.status.Succeeded() and
        self._response_code == Ola_pb2.RDM_COMPLETED_OK):
      # check for ack timer or nack
      if self._response_type == Ola_pb2.RDM_NACK_REASON:
        nack_value = self._get_short_from_data(response.data)
        if nack_value is None:
          self._response_code = Ola_pb2.RDM_INVALID_RESPONSE
        else:
          self._nack_reason = RDMNack.LookupCode(nack_value)
      elif self._response_type == Ola_pb2.RDM_ACK_TIMER:
        self._ack_timer = self._get_short_from_data(response.data)
        if self._ack_timer is None:
          self._response_code = Ola_pb2.RDM_INVALID_RESPONSE

  @property
  def response_code(self):
    return self._response_code

  def ResponseCodeAsString(self):
    return self.RESPONSE_CODES_TO_STRING.get(self._response_code, 'Unknown')

  @property
  def response_type(self):
    return self._response_type

  @property
  def queued_messages(self):
    return self._queued_messages

  @property
  def nack_reason(self):
    return self._nack_reason

  @property
  def transaction_number(self):
    return self._transaction_number

  @property
  def frames(self):
    return self._frames

  @property
  def raw_response(self):
    """The list of byte strings in the response packets."""
    data = []
    for frame in self._frames:
      data.append(frame.data)
    return data

  def WasAcked(self):
    """Returns true if this RDM request returned a ACK response."""
    return (self.status.Succeeded() and
            self.response_code == OlaClient.RDM_COMPLETED_OK and
            self.response_type == OlaClient.RDM_ACK)

  @property
  def ack_timer(self):
    return 100 * self._ack_timer

  def __repr__(self):
    if self.response_code != Ola_pb2.RDM_COMPLETED_OK:
      s = 'RDMResponse(error="{error}")'
      return s.format(error=self.ResponseCodeAsString())

    if self.response_type == OlaClient.RDM_ACK:
      s = 'RDMResponse(type=ACK, command_class={cmd})'
      return s.format(cmd=self._command_class())
    elif self.response_type == OlaClient.RDM_ACK_TIMER:
      s = 'RDMResponse(type=ACK_TIMER, ack_timer={timer} ms, ' \
          'command_class={cmd})'
      return s.format(timer=self.ack_timer,
                      cmd=self._command_class())
    elif self.response_type == OlaClient.RDM_NACK_REASON:
      s = 'RDMResponse(type=NACK, reason="{reason}")'
      return s.format(reason=self.nack_reason.description)
    else:
      s = 'RDMResponse(type="Unknown")'
      return s

  def _get_short_from_data(self, data):
    """Try to unpack the binary data into a short.

    Args:
      data: the binary data

    Returns:
      value: None if the unpacking failed
    """
    try:
      return struct.unpack('!h', data)[0]
    except struct.error:
      return None

  def _command_class(self):
    if self.command_class == OlaClient.RDM_GET_RESPONSE:
      return 'GET'
    elif self.command_class == OlaClient.RDM_SET_RESPONSE:
      return 'SET'
    elif self.command_class == OlaClient.RDM_DISCOVERY_RESPONSE:
      return 'DISCOVERY'
    else:
      return "UNKNOWN_CC"


class OlaClient(Ola_pb2.OlaClientService):
  """The client used to communicate with olad."""
  def __init__(self, our_socket=None, close_callback=None):
    """Create a new client.

    Args:
      socket: the socket to use for communications, if not provided one is
        created.
      close_callback: A callable to run if the socket is closed
    """
    self._close_callback = close_callback
    self._socket = our_socket

    if self._socket is None:
      self._socket = socket.socket()
      try:
        self._socket.connect(('localhost', OLA_PORT))
      except socket.error:
        raise OLADNotRunningException('Failed to connect to olad')

    self._channel = StreamRpcChannel(self._socket, self, self._SocketClosed)
    self._stub = Ola_pb2.OlaServerService_Stub(self._channel)
    self._universe_callbacks = {}

  def __del__(self):
    self._SocketClosed()

  def GetSocket(self):
    """Returns the socket used to communicate with the server."""
    return self._socket

  def SocketReady(self):
    """Called when the socket has new data."""
    self._channel.SocketReady()

  def _SocketClosed(self):
    """Called by the RPCChannel if the socket is closed."""
    try:
      self._socket.shutdown(socket.SHUT_RDWR)
    except socket.error:
      pass
    self._socket.close()
    self._socket = None

    if self._close_callback:
      self._close_callback()

  def FetchPlugins(self, callback):
    """Fetch the list of plugins.

    Args:
      callback: the function to call once complete, takes two arguments, a
        RequestStatus object and a list of Plugin objects

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.PluginListRequest()
    try:
      self._stub.GetPlugins(
          controller, request,
          lambda x, y: self._GetPluginsComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def PluginDescription(self, callback, plugin_id):
    """Fetch the description of a plugin.

    Args:
      callback: the function to call once complete, takes two arguments, a
        RequestStatus object and the plugin description text.
      plugin_id: the id of the plugin

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.PluginDescriptionRequest()
    request.plugin_id = plugin_id
    try:
      self._stub.GetPluginDescription(
          controller, request,
          lambda x, y: self._PluginDescriptionComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def FetchDevices(self, callback, plugin_filter=Plugin.OLA_PLUGIN_ALL):
    """Fetch a list of devices from the server.

    Args:
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and a list of Device objects.
      filter: a plugin id to filter by

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.DeviceInfoRequest()
    request.plugin_id = plugin_filter
    try:
      self._stub.GetDeviceInfo(
          controller, request,
          lambda x, y: self._DeviceInfoComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def FetchUniverses(self, callback):
    """Fetch a list of universes from the server

    Args:
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and a list of Universe objects.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.OptionalUniverseRequest()
    try:
      self._stub.GetUniverseInfo(
          controller, request,
          lambda x, y: self._UniverseInfoComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def FetchDmx(self, universe, callback):
    """Fetch DMX data from the server

    Args:
      universe: the universe to fetch the data for
      callback: The function to call once complete, takes three arguments, a
        RequestStatus object, a universe number and a list of dmx data.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.UniverseRequest()
    request.universe = universe
    try:
      self._stub.GetDmx(controller, request,
                        lambda x, y: self._GetDmxComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def SendDmx(self, universe, data, callback=None):
    """Send DMX data to the server

    Args:
      universe: the universe to send the data for
      data: An array object with the DMX data
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.DmxData()
    request.universe = universe
    if sys.version_info >= (3, 2):
      request.data = data.tobytes()
    else:
      request.data = data.tostring()
    try:
      self._stub.UpdateDmxData(
          controller, request,
          lambda x, y: self._AckMessageComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def SetUniverseName(self, universe, name, callback=None):
    """Set the name of a universe.

    Args:
      universe: the universe to set the name of
      name: the new name for the universe
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.UniverseNameRequest()
    request.universe = universe
    request.name = name
    try:
      self._stub.SetUniverseName(
          controller, request,
          lambda x, y: self._AckMessageComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def SetUniverseMergeMode(self, universe, merge_mode, callback=None):
    """Set the merge mode of a universe.

    Args:
      universe: the universe to set the merge mode of
      merge_mode: either Universe.HTP or Universe.LTP
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.MergeModeRequest()
    request.universe = universe
    request.merge_mode = merge_mode
    try:
      self._stub.SetMergeMode(
          controller, request,
          lambda x, y: self._AckMessageComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def RegisterUniverse(self, universe, action,
                       data_callback=None, callback=None):
    """Register to receive dmx updates for a universe.

    Args:
      universe: the universe to register to
      action: OlaClient.REGISTER or OlaClient.UNREGISTER
      data_callback: the function to be called when there is new data, passed
        a single argument of type array.
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.

    Returns:
      True if the request was sent, False otherwise.
    """

    if data_callback is None and action == self.REGISTER:
      raise TypeError("data_callback is None and action is REGISTER")

    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.RegisterDmxRequest()
    request.universe = universe
    request.action = action
    try:
      self._stub.RegisterForDmx(
          controller, request,
          lambda x, y: self._AckMessageComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    if action == self.REGISTER:
      self._universe_callbacks[universe] = data_callback
    elif universe in self._universe_callbacks:
      del self._universe_callbacks[universe]
    return True

  def PatchPort(self, device_alias, port, is_output, action, universe,
                callback=None):
    """Patch a port to a universe.

    Args:
      device_alias: the alias of the device of which to patch a port
      port: the id of the port
      is_output: select the input or output port
      action: OlaClient.PATCH or OlaClient.UNPATCH
      universe: the universe to set the name of
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.PatchPortRequest()
    request.device_alias = device_alias
    request.port_id = port
    request.action = action
    request.is_output = is_output
    request.universe = universe
    try:
      self._stub.PatchPort(
          controller, request,
          lambda x, y: self._AckMessageComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def ConfigureDevice(self, device_alias, request_data, callback):
    """Send a device config request.

    Args:
      device_alias: the alias of the device to configure
      request_data: the request to send to the device
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and a response.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.DeviceConfigRequest()
    request.device_alias = device_alias
    request.data = request_data
    try:
      self._stub.ConfigureDevice(
          controller, request,
          lambda x, y: self._ConfigureDeviceComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def SendTimeCode(self,
                   time_code_type,
                   hours,
                   minutes,
                   seconds,
                   frames,
                   callback=None):
    """Send Time Code Data.

    Args:
      time_code_type: One of OlaClient.TIMECODE_FILM, OlaClient.TIMECODE_EBU,
        OlaClient.TIMECODE_DF or OlaClient.TIMECODE_SMPTE
      hours: the hours
      minutes: the minutes
      seconds: the seconds
      frames: the frame count
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.TimeCode()
    request.type = time_code_type
    request.hours = hours
    request.minutes = minutes
    request.seconds = seconds
    request.frames = frames
    try:
      self._stub.SendTimeCode(
          controller, request,
          lambda x, y: self._AckMessageComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def UpdateDmxData(self, controller, request, callback):
    """Called when we receive new DMX data.

    Args:
      controller: An RpcController object
      request: A DmxData message
      callback: The callback to run once complete

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    if request.universe in self._universe_callbacks:
      data = array.array('B', request.data)
      self._universe_callbacks[request.universe](data)
    response = Ola_pb2.Ack()
    callback(response)
    return True

  def FetchUIDList(self, universe, callback):
    """Used to get a list of UIDs for a particular universe.

    Args:
      universe: The universe to get the UID list for.
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and a iterable of UIDs.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.UniverseRequest()
    request.universe = universe
    try:
      self._stub.GetUIDs(controller, request,
                         lambda x, y: self._FetchUIDsComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def RunRDMDiscovery(self, universe, full, callback):
    """Triggers RDM discovery for a universe.

    Args:
      universe: The universe to run discovery for.
      full: true to use full discovery, false for incremental (if supported)
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.DiscoveryRequest()
    request.universe = universe
    request.full = full
    try:
      self._stub.ForceDiscovery(
          controller, request,
          lambda x, y: self._FetchUIDsComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def RDMGet(self, universe, uid, sub_device, param_id, callback, data=b'',
             include_frames=False):
    """Send an RDM get command.

    Args:
      universe: The universe to get the UID list for.
      uid: A UID object
      sub_device: The sub device index
      param_id: the param ID
      callback: The function to call once complete, takes a RDMResponse object
      data: the data to send
      include_frames: True if the response should include the raw frame data.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    return self._RDMMessage(universe, uid, sub_device, param_id, callback,
                            data, include_frames)

  def RDMSet(self, universe, uid, sub_device, param_id, callback, data=b'',
             include_frames=False):
    """Send an RDM set command.

    Args:
      universe: The universe to get the UID list for.
      uid: A UID object
      sub_device: The sub device index
      param_id: the param ID
      callback: The function to call once complete, takes a RDMResponse object
      data: the data to send
      include_frames: True if the response should include the raw frame data.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    return self._RDMMessage(universe, uid, sub_device, param_id, callback,
                            data, include_frames, set=True)

  def SendRawRDMDiscovery(self,
                          universe,
                          uid,
                          sub_device,
                          param_id,
                          callback,
                          data=b'',
                          include_frames=False):
    """Send an RDM Discovery command. Unless you're writing RDM tests you
      shouldn't need to use this.

    Args:
      universe: The universe to get the UID list for.
      uid: A UID object
      sub_device: The sub device index
      param_id: the param ID
      callback: The function to call once complete, takes a RDMResponse object
      data: the data to send
      include_frames: True if the response should include the raw frame data.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.RDMDiscoveryRequest()
    request.universe = universe
    request.uid.esta_id = uid.manufacturer_id
    request.uid.device_id = uid.device_id
    request.sub_device = sub_device
    request.param_id = param_id
    request.data = data
    request.include_raw_response = True
    request.include_raw_response = include_frames
    try:
      self._stub.RDMDiscoveryCommand(
          controller, request,
          lambda x, y: self._RDMCommandComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def GetCandidatePorts(self, callback, universe=None):
    """Send a GetCandidatePorts request. The result is similar to FetchDevices
    (GetDeviceInfo), except that returned devices will only contain ports
    available for patching to the given universe. If universe is None, then the
    devices will list their ports available for patching to a potential new
    universe.

    Args:
      callback: The function to call once complete, takes a RequestStatus
        object and a list of Device objects.
      universe: The universe to get the candidate ports for. If unspecified,
        return the candidate ports for a new universe.

    Returns:
      True if the request was sent, False otherwise.
    """
    if self._socket is None:
      return False

    controller = SimpleRpcController()
    request = Ola_pb2.OptionalUniverseRequest()

    if universe is not None:
      request.universe = universe

    try:
      # GetCandidatePorts works very much like GetDeviceInfo, so we can re-use
      # its complete method.
      self._stub.GetCandidatePorts(
          controller, request,
          lambda x, y: self._DeviceInfoComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()

    return True

  def _RDMMessage(self, universe, uid, sub_device, param_id, callback, data,
                  include_frames, set=False):
    controller = SimpleRpcController()
    request = Ola_pb2.RDMRequest()
    request.universe = universe
    request.uid.esta_id = uid.manufacturer_id
    request.uid.device_id = uid.device_id
    request.sub_device = sub_device
    request.param_id = param_id
    request.data = data
    request.is_set = set
    request.include_raw_response = include_frames
    try:
      self._stub.RDMCommand(
          controller, request,
          lambda x, y: self._RDMCommandComplete(callback, x, y))
    except socket.error:
      raise OLADNotRunningException()
    return True

  def _GetPluginsComplete(self, callback, controller, response):
    """Called when the list of plugins is returned.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: a PluginInfoReply message.
    """
    if not callback:
      return

    status = RequestStatus(controller)
    plugins = None

    if status.Succeeded():
      plugins = sorted([Plugin.FromProtobuf(p) for p in response.plugin])

    callback(status, plugins)

  def _PluginDescriptionComplete(self, callback, controller, response):
    """Called when the plugin description is returned.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: a PluginInfoReply message.
    """
    if not callback:
      return

    status = RequestStatus(controller)
    description = None

    if status.Succeeded():
      description = response.description

    callback(status, description)

  def _DeviceInfoComplete(self, callback, controller, response):
    """Called when the Device info request returns.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: a DeviceInfoReply message.
    """
    if not callback:
      return
    status = RequestStatus(controller)
    devices = None

    if status.Succeeded():
      devices = []
      for device in response.device:
        input_ports = []
        output_ports = []
        for port in device.input_port:
          input_ports.append(Port.FromProtobuf(port))

        for port in device.output_port:
          output_ports.append(Port.FromProtobuf(port))

        devices.append(Device(device.device_id,
                              device.device_alias,
                              device.device_name,
                              device.plugin_id,
                              input_ports,
                              output_ports))
    callback(status, devices)

  def _UniverseInfoComplete(self, callback, controller, response):
    """Called when the Universe info request returns.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: a UniverseInfoReply message.
    """
    if not callback:
      return
    status = RequestStatus(controller)
    universes = None

    if status.Succeeded():
      universes = [Universe.FromProtobuf(u) for u in response.universe]

    callback(status, universes)

  def _GetDmxComplete(self, callback, controller, response):
    """Called when the Universe info request returns.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: a UniverseInfoReply message.
    """
    if not callback:
      return
    status = RequestStatus(controller)
    data = None
    universe = None

    if status.Succeeded():
      data = array.array('B', response.data)
      universe = response.universe

    callback(status, universe, data)

  def _AckMessageComplete(self, callback, controller, response):
    """Called when an rpc that returns an Ack completes.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: an Ack message.
    """
    if not callback:
      return
    status = RequestStatus(controller)
    callback(status)

  def _ConfigureDeviceComplete(self, callback, controller, response):
    """Called when a ConfigureDevice request completes.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: an DeviceConfigReply message.
    """
    if not callback:
      return

    status = RequestStatus(controller)
    data = None

    if status.Succeeded():
      data = response.data

    callback(status, data)

  def _FetchUIDsComplete(self, callback, controller, response):
    """Called when a FetchUIDList request completes.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: an UIDListReply message.
    """
    if not callback:
      return

    status = RequestStatus(controller)
    uids = None

    if status.Succeeded():
      uids = []
      for uid in response.uid:
        uids.append(UID(uid.esta_id, uid.device_id))
      uids.sort()

    callback(status, uids)

  def _RDMCommandComplete(self, callback, controller, response):
    """Called when a RDM request completes.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: an RDMResponse message.
    """
    if not callback:
      return
    callback(RDMResponse(controller, response))


# Populate the patch & register actions
for value in Ola_pb2._PATCHACTION.values:
  setattr(OlaClient, value.name, value.number)
for value in Ola_pb2._REGISTERACTION.values:
  setattr(OlaClient, value.name, value.number)

# populate time code enums
for value in Ola_pb2._TIMECODETYPE.values:
  setattr(OlaClient, value.name, value.number)

# populate the RDM response codes & types
for value in Ola_pb2._RDMRESPONSECODE.values:
  setattr(OlaClient, value.name, value.number)
for value in Ola_pb2._RDMRESPONSETYPE.values:
  setattr(OlaClient, value.name, value.number)
for value in Ola_pb2._RDMCOMMANDCLASS.values:
  setattr(OlaClient, value.name, value.number)
