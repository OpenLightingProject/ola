#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# OlaClient.py
# Copyright (C) 2005-2009 Simon Newton

"""The client used to communicate with the Ola Server."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import array
import struct
from ola.rpc.StreamRpcChannel import StreamRpcChannel
from ola.rpc.SimpleRpcController import SimpleRpcController
from ola import Ola_pb2
from ola.UID import UID

"""The port that the OLA server listens on."""
OLA_PORT = 9010

class Plugin(object):
  """Represents a plugin.

  Attributes:
    id: the id of this plugin
    name: the name of this plugin
  """
  def __init__(self, plugin_id, name):
    self._id = plugin_id
    self._name = name

  @property
  def id(self):
    return self._id

  @property
  def name(self):
    return self._name

  def __cmp__(self, other):
    return cmp(self._id, other._id)

  def __str__(self):
    return 'Plugin %d (%s)' % (self._id, self,_name)


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

  def __cmp__(self, other):
    return cmp(self._alias, other._alias)


class Port(object):
  """Represents a port.

  Attributes:
    id: the unique id of this port
    universe: the universe that this port belongs to
    active: True if this port is active
    description: the description of the port
  """
  def __init__(self, port_id, universe, active, description):
    self._id = port_id
    self._universe = universe
    self._active = active
    self._description = description

  @property
  def id(self):
    return self._id

  @property
  def universe(self):
    return self._universe

  @property
  def action(self):
    return self._active

  @property
  def description(self):
    return self._description

  def __cmp__(self, other):
    return cmp(self._id, other._id)


class Universe(object):
  """Represents a universe.

  Attributes:
    id: the integer universe id
    name: the name of this universe
    merge_mode: the merge mode this universe is using
  """

  LTP = Ola_pb2.LTP
  HTP = Ola_pb2.HTP

  def __init__(self, universe_id, name, merge_mode):
    self._id = universe_id
    self._name = name
    self._merge_mode = merge_mode

  @property
  def id(self):
    return self._id

  @property
  def name(self):
    return self._name

  @property
  def merge_mode(self):
    return self._merge_mode

  def __cmp__(self, other):
    return cmp(self._id, other._id)


class RequestStatus(object):
  """Represents the status of an reqeust.

  Attributes:
    state: the state of the operation
    message: an error message if it failed
  """
  SUCCESS, FAILED, CANCELLED = range(3)

  def __init__(self, controller):
    if controller.Failed():
      self._state = self.FAILED

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
  NR_UNKNOWN_PID = 0
  NR_FORMAT_ERROR = 1
  NR_HARDWARE_FAULT = 2
  NR_PROXY_REJECT = 3
  NR_WRITE_PROTECT = 4
  NR_UNSUPPORTED_COMMAND_CLASS = 5
  NR_DATA_OUT_OF_RANGE = 6
  NR_BUFFER_FULL = 7
  NR_PACKET_SIZE_UNSUPPORTED = 8
  NR_SUB_DEVICE_OUT_OF_RANGE = 9
  NR_PROXY_BUFFER_FULL = 10

  NACK_REASONS_TO_STRING = {
      NR_UNKNOWN_PID: 'Unknown PID',
      NR_FORMAT_ERROR: 'Format error',
      NR_HARDWARE_FAULT: 'Hardware fault',
      NR_PROXY_REJECT: 'Proxy reject',
      NR_WRITE_PROTECT: 'Write protect',
      NR_UNSUPPORTED_COMMAND_CLASS: 'Unsupported command class',
      NR_DATA_OUT_OF_RANGE: 'Data out of range',
      NR_BUFFER_FULL: 'Buffer full',
      NR_PACKET_SIZE_UNSUPPORTED: 'Packet size unsupported',
      NR_SUB_DEVICE_OUT_OF_RANGE: 'Sub device out of range',
      NR_PROXY_BUFFER_FULL: 'Proxy buffer full'
  }

  def __init__(self, nack_value):
    self._value = nack_value

  @property
  def value(self):
    return self._value

  def __str__(self):
    return self.NACK_REASONS_TO_STRING.get(self.value, 'Unknown')

  def __cmp__(self, other):
    return cmp(self.value, other.value)


class RDMRequestStatus(RequestStatus):
  """Represents the status of a RDM request.

  Attributes:
    state: the state of the operation
    message: an error message if it failed
    rdm_ResponseCode: The response code for the RDM request
    rdm_response_type: The response type (ACK, ACK_TIMER, NACK_REASON) for
      the request.
    nack_reason: If the response type was NACK_REASON, this is the reason for
      the NACK.
    ack_timer: If the response type was ACK_TIMER, this is the number of ms to
      wait before checking for queued messages.
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
      Ola_pb2.RDM_DEVICE_MISMATCH: 'Device mismatch',
  }

  def __init__(self, controller, response):
    super(RDMRequestStatus, self).__init__(controller)
    self._ResponseCode = response.response_code
    self._response_type = response.response_type
    self._queued_messages = response.message_count
    self._nack_reason = None
    self._ack_timer = None

    if self.Succeeded() and self.response_code == Ola_pb2.RDM_COMPLETED_OK:
      # check for ack timer or nack
      if self.response_type == Ola_pb2.RDM_NACK_REASON:
        nack_value = self._get_short_from_data(response.data)
        if nack_value is None:
          self.response_code = Ola_pb2.RDM_INVALID_RESPONSE
        else:
          self._nack_reason = RDMNack(nack_value)
      elif self.response_type == Ola_pb2.RDM_ACK_TIMER:
        self._ack_timer = self._get_short_from_data(response.data)
        if self._ack_timer is None:
          self.response_code = Ola_pb2.RDM_INVALID_RESPONSE

  def ResponseCode(self):
    return self._ResponseCode

  def SetResponseCode(self, code):
    self._ResponseCode = code

  response_code = property(ResponseCode, SetResponseCode, None)

  def ResponseCodeAsString(self):
    return self.RESPONSE_CODES_TO_STRING.get(self._ResponseCode,
                                             'Unknown')

  @property
  def response_type(self):
    return self._response_type

  @property
  def queued_messages(self):
    return self._queued_messages

  @property
  def nack_reason(self):
    return self._nack_reason

  def WasSuccessfull(self):
    """Returns true if this RDM request returns a ACK response."""
    return (self.Succeeded() and
            self.response_code == OlaClient.RDM_COMPLETED_OK and
            self.response_type == OlaClient.RDM_ACK)

  @property
  def ack_timer(self):
    return 100 * self._ack_timer

  def __str__(self):
    if self.response_code != Ola_pb2.RDM_COMPLETED_OK:
      return 'RDMRequestStatus: %s' % self.ResponseCodeAsString()

    if self.response_type == OlaClient.RDM_ACK:
      return 'RDMRequestStatus: ACK' % self._pid
    elif self.response_type == OlaClient.RDM_ACK_TIMER:
      return 'RDMRequestStatus: ACK TIMER, %d ms' % self.ack_timer
    else:
      return 'RDMRequestStatus:, NACK %s' % self.nack_reason

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


class OlaClient(Ola_pb2.OlaClientService):
  """The client used to communicate with olad."""
  def __init__(self, socket):
    """Create a new client.

    Args:
      socket: the socket to use for communications.
    """
    self._socket = socket
    self._channel = StreamRpcChannel(socket, self)
    self._stub = Ola_pb2.OlaServerService_Stub(self._channel)
    self._universe_callbacks = {}

  def SocketReady(self):
    """Called when the socket has new data."""
    self._channel.SocketReady()

  def FetchPlugins(self, callback):
    """Fetch the list of plugins.

    Args:
      callback: the function to call once complete, takes two arguments, a
        RequestStatus object and a list of Plugin objects
    """
    controller = SimpleRpcController()
    request = Ola_pb2.PluginListRequest()
    done = lambda x, y: self._GetPluginsComplete(callback, x, y)
    self._stub.GetPlugins(controller, request, done)

  def PluginDescription(self, callback, plugin_id):
    """Fetch the list of plugins.

    Args:
      callback: the function to call once complete, takes two arguments, a
        RequestStatus object and a list of Plugin objects
      plugin_id: the id of the plugin
    """
    controller = SimpleRpcController()
    request = Ola_pb2.PluginDescriptionRequest()
    request.plugin_id = plugin_id
    done = lambda x, y: self._PluginDescriptionComplete(callback, x, y)
    self._stub.GetPluginDescription(controller, request, done)

  def FetchDevices(self, callback, plugin_filter=Plugin.OLA_PLUGIN_ALL):
    """Fetch a list of devices from the server.

    Args:
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and a list of Device objects.
      filter: a plugin id to filter by
    """
    controller = SimpleRpcController()
    request = Ola_pb2.DeviceInfoRequest()
    request.plugin_id = plugin_filter
    done = lambda x, y: self._DeviceInfoComplete(callback, x, y)
    self._stub.GetDeviceInfo(controller, request, done)

  def FetchUniverses(self, callback):
    """Fetch a list of universes from the server

    Args:
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and a list of Universe objects.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.OptionalUniverseRequest()
    done = lambda x, y: self._UniverseInfoComplete(callback, x, y)
    self._stub.GetUniverseInfo(controller, request, done)

  def FetchDmx(self, universe, callback):
    """Fetch a list of universes from the server

    Args:
      universe: the universe to fetch the data for
      callback: The function to call once complete, takes three arguments, a
        RequestStatus object, a universe number and a list of dmx data.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.UniverseRequest()
    request.universe = universe
    done = lambda x, y: self._GetDmxComplete(callback, x, y)
    self._stub.GetDmx(controller, request, done)

  def SendDmx(self, universe, data, callback=None):
    """Send DMX data to the server

    Args:
      universe: the universe to fetch the data for
      data: An array object with the DMX data
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.DmxData()
    request.universe = universe
    request.data = data.tostring()
    done = lambda x, y: self._AckMessageComplete(callback, x, y)
    self._stub.UpdateDmxData(controller, request, done)

  def SetUniverseName(self, universe, name, callback=None):
    """Set the name of a universe.

    Args:
      universe: the universe to set the name of
      name: the new name for the universe
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.UniverseNameRequest()
    request.universe = universe
    request.name = name
    done = lambda x, y: self._AckMessageComplete(callback, x, y)
    self._stub.SetUniverseName(controller, request, done)

  def SetUniverseMergeMode(self, universe, merge_mode, callback=None):
    """Set the merge_mode of a universe.

    Args:
      universe: the universe to set the name of
      merge_mode: either Universe.HTP or Universe.LTP
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.MergeModeRequest()
    request.universe = universe
    request.merge_mode = merge_mode
    done = lambda x, y: self._AckMessageComplete(callback, x, y)
    self._stub.SetMergeMode(controller, request, done)

  def RegisterUniverse(self, universe, action, data_callback, callback=None):
    """Register to receive dmx updates for a universe.

    Args:
      universe: the universe to set the name of
      action: OlaClient.REGISTER or OlaClient.UNREGISTER
      data_callback: the function to be called when there is new data, passed
        a single argument of type array.
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.RegisterDmxRequest()
    request.universe = universe
    request.action = action
    done = lambda x, y: self._AckMessageComplete(callback, x, y)
    self._stub.RegisterForDmx(controller, request, done)
    if action == self.PATCH:
      self._universe_callbacks[universe] = data_callback
    elif universe in self._universe_callbacks:
      del self._universe_callbacks[universe]

  def PatchPort(self, device_alias, port, action, universe, callback=None):
    """Patch a port to a universe.

    Args:
      device_alias: the alias of the device to configure
      port: the id of the port
      action: OlaClient.PATCH or OlcClient.UNPATCH
      universe: the universe to set the name of
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.PatchPortRequest()
    request.device_alias = device_alias
    request.port_id = port
    request.action = action
    request.universe = universe
    done = lambda x, y: self._AckMessageComplete(callback, x, y)
    self._stub.PatchPort(controller, request, done)

  def ConfigureDevice(self, device_alias, request_data, callback):
    """Send a device config request.

    Args:
      device_alias: the alias of the device to configure
      request_data: the request to send to the device
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and a response.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.DeviceConfigRequest()
    request.device_alias = device_alias
    request.data = request_data
    done = lambda x, y: self._ConfigureDeviceComplete(callback, x, y)
    self._stub.ConfigureDevice(controller, request, done)

  def UpdateDmxData(self, controller, request, callback):
    """Called when we receive new DMX data.

    Args:
      controller: An RpcController object
      reqeust: A DmxData message
      callback: The callback to run once complete
    """
    if request.universe in self._universe_callbacks:
      data = array.array('B')
      data.fromstring(request.data)
      self._universe_callbacks[request.universe](data)
    response = Ola_pb2.Ack()
    callback(response)

  def FetchUIDList(self, universe, callback):
    """Used to get a list of UIDs for a particular universe.

    Args:
      universe: The universe to get the UID list for.
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and a iterable of UIDs.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.UniverseRequest()
    request.universe = universe
    done = lambda x, y: self._FetchUIDsComplete(callback, x, y)
    self._stub.GetUIDs(controller, request, done)

  def RunRDMDiscovery(self, universe, callback):
    """Triggers RDM discovery for a universe.

    Args:
      universe: The universe to run discovery for.
      callback: The function to call once complete, takes one argument, a
        RequestStatus object.
    """
    controller = SimpleRpcController()
    request = Ola_pb2.UniverseRequest()
    request.universe = universe
    done = lambda x, y: self._AckMessageComplete(callback, x, y)
    self._stub.ForceDiscovery(controller, request, done)

  def RDMGet(self, universe, uid, sub_device, param_id, callback, data = ''):
    """Send an RDM get command.

    Args:
      universe: The universe to get the UID list for.
      uid: A UID object
      sub_device: The sub device index
      param_id: the param ID
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and
      data: the data to send
    """
    return self._RDMMessage(universe, uid, sub_device, param_id, callback,
                            data);

  def RDMSet(self, universe, uid, sub_device, param_id, callback, data = ''):
    """Send an RDM set command.

    Args:
      universe: The universe to get the UID list for.
      uid: A UID object
      sub_device: The sub device index
      param_id: the param ID
      callback: The function to call once complete, takes two arguments, a
        RequestStatus object and
      data: the data to send
    """
    return self._RDMMessage(universe, uid, sub_device, param_id, callback,
                            data, set = True);

  def _RDMMessage(self, universe, uid, sub_device, param_id, callback, data,
                  set = False):
    controller = SimpleRpcController()
    request = Ola_pb2.RDMRequest()
    request.universe = universe
    request.uid.esta_id = uid.manufacturer_id
    request.uid.device_id = uid.device_id
    request.sub_device = sub_device
    request.param_id = param_id
    request.data = data
    request.is_set = set
    done = lambda x, y: self._RDMCommandComplete(callback, x, y)
    self._stub.RDMCommand(controller, request, done)
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
    if not status.Succeeded():
      return

    plugins = [Plugin(p.plugin_id, p.name) for p in response.plugin]
    plugins.sort(key=lambda x: x.id)
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
    if not status.Succeeded():
      return
    callback(status, response.description)

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
    if not status.Succeeded():
      return

    devices = []
    for device in response.device:
      input_ports = []
      output_ports = []
      for port in device.input_port:
        input_ports.append(Port(port.port_id,
                                port.universe,
                                port.active,
                                port.description))

      for port in device.output_port:
        output_ports.append(Port(port.port_id,
                                 port.universe,
                                 port.active,
                                 port.description))

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
    if not status.Succeeded():
      return

    universes = [Universe(u.universe, u.name, u.merge_mode) for u in
        response.universe]
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
    if not status.Succeeded():
      return

    data = array.array('B')
    data.fromstring(response.data)
    callback(status, response.universe, data)

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
    if not status.Succeeded():
      return
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
    if not status.Succeeded():
      return
    callback(status, response.data)

  def _FetchUIDsComplete(self, callback, controller, response):
    """Called when a FetchUIDList request completes.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: an DeviceConfigReply message.
    """
    if not callback:
      return
    status = RequestStatus(controller)
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
      response: an DeviceConfigReply message.
    """
    if not callback:
      return

    status = RDMRequestStatus(controller, response)
    callback(status, response.param_id, response.data, response.raw_response)

# Populate the patch & register actions
for value in Ola_pb2._PATCHACTION.values:
  setattr(OlaClient, value.name, value.number)
for value in Ola_pb2._REGISTERACTION.values:
  setattr(OlaClient, value.name, value.number)

# populate the RDM response codes & types
for value in Ola_pb2._RDMRESPONSECODE.values:
  setattr(OlaClient, value.name, value.number)
for value in Ola_pb2._RDMRESPONSETYPE.values:
  setattr(OlaClient, value.name, value.number)
