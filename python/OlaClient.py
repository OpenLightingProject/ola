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

import socket
from rpc.StreamRpcChannel import StreamRpcChannel
from rpc.SimpleRpcController import SimpleRpcController
import Ola_pb2


class Plugin(object):
  """Represents a plugin.

  Attributes:
    id: the id of this plugin
    name: the name of this plugin
    description: the description of this plugin
  """
  def __init__(self, id, name, description):
    self.id = id
    self.name = name
    self.description = description

  def __cmp__(self, other):
    return cmp(self.id, other.id)


class Device(object):
  """Represents a device.

  Attributes:
    id: the unique id of this device
    alias: the integer alias for this device
    name: the name of this device
    plugin_id: the plugin that this device belongs to
    ports: a list of Port objects
  """
  def __init__(self, id, alias, name, plugin_id, ports):
    self.id = id;
    self.alias = alias
    self.name = name
    self.plugin_id = plugin_id
    self.ports = sorted(ports)

  def __cmp__(self, other):
    return cmp(self.alias, other.alias)


class Port(object):
  """Represents a port.

  Attributes:
    id: the unique id of this port
    capability:
    universe: the universe that this port belongs to
    active:
    description: the description of the port
  """
  def __init__(self, id, capability, universe, active, description):
    self.id = id
    self.capability = capability
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
  def __init__(self, id, name, merge_mode):
    self.id = id
    self.name = name
    self.merge_mode = merge_mode

  def __cmp__(self, other):
    return cmp(self.id, other.id)


class Status(object):
  """Represents the status of the operation.

  Attributes:
    state: the state of the operation
    message: an error message if it failed
  """
  SUCCESS, FAILED, CANCELLED = range(3)

  def __init__(self, state=SUCCESS, message=None):
    self.state = state
    self.message = message


class OlaClient(object):
  """The client used to communicate with olad."""
  def __init__(self, socket):
    """Create a new client.

    Args:
      socket: the socket to use for communications.
    """
    self._socket = socket
    self._channel = StreamRpcChannel(socket, None)
    self._stub = Ola_pb2.OlaServerService_Stub(self._channel)

  def SocketReady(self):
    """Called when the socket has new data."""
    self._channel.SocketReady()

  def FetchPluginInfo(self, callback, filter, include_description=False):
    """Fetch the list of plugins.

    Args:
      callback: the function to call with the list
      filter: the id of the plugin if you want to filter the results
      include_description: whether to include the plugin description or not
    """
    controller = SimpleRpcController()
    request = Ola_pb2.PluginInfoRequest()
    done = lambda x, y: self._PluginInfoComplete(callback, x, y)
    self._stub.GetPluginInfo(controller, request, done)

  def FetchDeviceInfo(self, callback, filter):
    """Fetch a list of devices from the server.

    Args:
      callback: the function to call with the list
      filter: a plugin id to filter by
    """
    controller = SimpleRpcController()
    request = Ola_pb2.DeviceInfoRequest()
    done = lambda x, y: self._PluginInfoComplete(callback, x, y)
    self._stub.GetPluginInfo(controller, request, done)



  def FetchUniverseInfo(self, callback):

  def FetchDmx(self, callback, universe):

  def SendDmx(self, universe, data):

  def SetUniverseName(self, universe, name):

  def SetUniverseMergeMode(self, universe, merge_mode):

  def RegisterUniverse(self, universe, action):

  def Patch(self, device_alias, port, action, universe):

  def ConfigureDevice(self, callback, device_alias, request):


  def _PluginInfoComplete(self, callback, controller, response):
    """Called when the list of plugins is returned.

    Args:
      controller: an RpcController
      response: a PluginInfoReply message.
    """
    if not callback:
      return

    state = Status()
    if controller.Failed():
      print 'failed'
      return
    if controller.IsCanceled():
      print 'request was cancelled'
      return

    plugins = [Plugin(p.plugin_id, p.name, p.description) for p in
        response.plugins]
    callback(status, plugins)

  def _DeviceInfoComplete(self, callback, controller, response):
    """Called when the Device info request returns.

    Args:
      controller: an RpcController
      response: a DeviceInfoReply message.
    """
