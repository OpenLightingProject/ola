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
from ola.rpc.StreamRpcChannel import StreamRpcChannel
from ola.rpc.SimpleRpcController import SimpleRpcController
from ola import Ola_pb2

"""The port that the OLA server listens on."""
OLA_PORT = 9010

class Plugin(object):
  """Represents a plugin.

  Attributes:
    id: the id of this plugin
    name: the name of this plugin
    description: the description of this plugin
  """
  def __init__(self, plugin_id, name, description):
    self.id = plugin_id
    self.name = name
    self.description = description

  def __cmp__(self, other):
    return cmp(self.id, other.id)

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
    self.id = device_id
    self.alias = alias
    self.name = name
    self.plugin_id = plugin_id
    self.input_ports = sorted(input_ports)
    self.output_ports = sorted(output_ports)

  def __cmp__(self, other):
    return cmp(self.alias, other.alias)


class Port(object):
  """Represents a port.

  Attributes:
    id: the unique id of this port
    universe: the universe that this port belongs to
    active: True if this port is active
    description: the description of the port
  """
  def __init__(self, port_id, universe, active, description):
    self.id = port_id
    self.universe = universe
    self.active = active
    self.description = description

  def __cmp__(self, other):
    return cmp(self.id, other.id)


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
    self.id = universe_id
    self.name = name
    self.merge_mode = merge_mode

  def __cmp__(self, other):
    return cmp(self.id, other.id)


class RequestStatus(object):
  """Represents the status of an reqeust.

  Attributes:
    state: the state of the operation
    message: an error message if it failed
  """
  SUCCESS, FAILED, CANCELLED = range(3)

  def __init__(self, state=SUCCESS, message=None):
    self.state = state
    self.message = message

  def Succeeded(self):
    """Returns true if this request succeeded."""
    return self.state == self.SUCCESS


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

  def FetchPlugins(self, callback,
                   plugin_filter=Plugin.OLA_PLUGIN_ALL,
                   include_description=False):
    """Fetch the list of plugins.

    Args:
      callback: the function to call once complete, takes two arguments, a
        RequestStatus object and a list of Plugin objects
      filter: the id of the plugin if you want to filter the results
      include_description: whether to include the plugin description or not
    """
    controller = SimpleRpcController()
    request = Ola_pb2.PluginInfoRequest()
    request.plugin_id = plugin_filter
    request.include_description = include_description
    done = lambda x, y: self._PluginInfoComplete(callback, x, y)
    self._stub.GetPluginInfo(controller, request, done)

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
    request = Ola_pb2.UniverseInfoRequest()
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
    request = Ola_pb2.UniverseInfoRequest()
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

  def _CreateStateFromController(self, controller):
    """Return a Status object given a RpcController object.

    Args:
      controller: An RpcController object.

    Returns:
      A RequestStatus object.
    """
    if controller.Failed():
      return RequestStatus(RequestStatus.FAILED, controller.ErrorText())
    elif controller.IsCanceled():
      return RequestStatus(RequestStatus.CANCELLED, controller.ErrorText())
    else:
      return RequestStatus()

  def _PluginInfoComplete(self, callback, controller, response):
    """Called when the list of plugins is returned.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: a PluginInfoReply message.
    """
    if not callback:
      return
    status = self._CreateStateFromController(controller)
    if not status.Succeeded():
      return

    plugins = [Plugin(p.plugin_id, p.name, p.description) for p in
        response.plugin]
    callback(status, plugins)

  def _DeviceInfoComplete(self, callback, controller, response):
    """Called when the Device info request returns.

    Args:
      callback: the callback to run
      controller: an RpcController
      response: a DeviceInfoReply message.
    """
    if not callback:
      return
    status = self._CreateStateFromController(controller)
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
    status = self._CreateStateFromController(controller)
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
    status = self._CreateStateFromController(controller)
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
    status = self._CreateStateFromController(controller)
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
    status = self._CreateStateFromController(controller)
    if not status.Succeeded():
      return
    callback(status, response.data)

# Populate the patch & register actions
for value in Ola_pb2._PATCHACTION.values:
  setattr(OlaClient, value.name, value.number)
for value in Ola_pb2._REGISTERACTION.values:
  setattr(OlaClient, value.name, value.number)
