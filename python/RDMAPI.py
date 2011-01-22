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
# RDMAPI.py
# Copyright (C) 2010 Simon Newton

"""The Python RDM API."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import sys
from ola.UID import UID
from ola.OlaClient import OlaClient
from ola import PidStore


class RDMAPI(object):
  """The RDM API."""
  def __init__(self, client, pid_store, strict_checks = True):
    """Create a new RDM API.

    Args:
      client: A OlaClient object.
      pid_store: A PidStore instance
      strict_checks: Enable strict checking
    """
    self._client = client
    self._pid_store = pid_store
    self._strict_checks = strict_checks

  def Get(self, universe, uid, sub_device, pid, callback, args = []):
    """Send a RDM Get message, packing the arguments into a message.

    Args:
      universe: The universe to send the request on.
      uid: The UID to address the request to.
      sub_device: The Sub Device to send the request to.
      pid: A PID object that describes the format of the request.
      callback: The callback to run when the request completes.
      args: The args to pack into the param data section.

    Return:
      True if sent ok, False otherwise.
    """
    if self._strict_checks and uid.IsBroadcast():
      print >> sys.stderr, "Can't send GET to broadcast address %s" % uid
      return False

    return self._SendRequest(universe, uid, sub_device, pid, callback, args,
                             PidStore.RDM_GET)

  def RawGet(self, universe, uid, sub_device, pid, callback, data):
    """Send a RDM Get message with the raw data supplied.

    Args:
      universe: The universe to send the request on.
      uid: The UID to address the request to.
      sub_device: The Sub Device to send the request to.
      pid: A PID object that describes the format of the request.
      callback: The callback to run when the request completes.
      data: The param data

    Return:
      True if sent ok, False otherwise.
    """
    if self._strict_checks and uid.IsBroadcast():
      print >> sys.stderr, "Can't send GET to broadcast address %s" % uid
      return False

    return self._SendRawRequest(universe, uid, sub_device, pid, callback, data,
                                PidStore.RDM_GET)

  def Set(self, universe, uid, sub_device, pid, callback, args = []):
    """Send a RDM Set message.

    Args:
      universe: The universe to send the request on.
      uid: The UID to address the request to.
      sub_device: The Sub Device to send the request to.
      pid: A PID object that describes the format of the request.
      callback: The callback to run when the request completes.
      args: The args to pack into the param data section.

    Return:
      True if sent ok, False otherwise.
    """
    return self._SendRequest(universe, uid, sub_device, pid, callback, args,
                             PidStore.RDM_SET)

  def RawSet(self, universe, uid, sub_device, pid, callback, args = []):
    """Send a RDM Set message with the raw data supplied.

    Args:
      universe: The universe to send the request on.
      uid: The UID to address the request to.
      sub_device: The Sub Device to send the request to.
      pid: A PID object that describes the format of the request.
      callback: The callback to run when the request completes.
      data: The param data

    Return:
      True if sent ok, False otherwise.
    """
    return self._SendRawRequest(universe, uid, sub_device, pid, callback,
                                    args, PidStore.RDM_SET)

  def _SendRequest(self, universe, uid, sub_device, pid, callback, args,
                   request_type):
    """Send a RDM Request.

    Args:
      universe: The universe to send the request on.
      uid: The UID to address the request to.
      sub_device: The Sub Device to send the request to.
      pid: A PID object that describes the format of the request.
      callback: The callback to run when the request completes.
      args: The args to pack into the param data section.
      request_type: PidStore.RDM_GET or PidStore.RDM_SET

    Return:
      True if sent ok, False otherwise.
    """
    data = pid.Pack(args, request_type)
    if data is None:
      print >> sys.stderr, 'Could not pack data'
      return False

    return self._SendRawRequest(universe, uid, sub_device, pid, callback, data,
                                request_type)

  def _SendRawRequest(self, universe, uid, sub_device, pid, callback, data,
                      request_type):
    """Send a RDM Request.

    Args:
      universe: The universe to send the request on.
      uid: The UID to address the request to.
      sub_device: The Sub Device to send the request to.
      pid: A PID object that describes the format of the request.
      callback: The callback to run when the request completes.
      data: The param data.
      request_type: PidStore.RDM_GET or PidStore.RDM_SET

    Return:
      True if sent ok, False otherwise.
    """
    if self._strict_checks:
      request_params = {
        'uid': uid,
        'sub_device': sub_device,
      }
      if not pid.ValidateAddressing(request_params, request_type):
        return False

    if request_type == PidStore.RDM_SET:
      method = self._client.RDMSet
    else:
      method = self._client.RDMGet

    return method(
        universe,
        uid,
        sub_device,
        pid.value,
        self._CreateCallback(callback, request_type, uid),
        data)

  def _GenericHandler(self, callback, request_type, uid, status, pid, data,
                      unused_raw_data):
    """

    Args:
      callback: the function to run
      request_type: PidStore.RDM_GET or PidStore.RDM_SET
      uid: The uid the request was for
      status: A RDMRequestStatus object
      pid: The pid in the response
      data: The parameter data
      unused_raw_data: A list of raw packets that make up the response
    """
    obj = None
    unpack_exception = None
    if status.WasSuccessfull():
      pid_descriptor = self._pid_store.GetPid(pid, uid.manufacturer_id)
      if pid_descriptor:
        try:
          obj = pid_descriptor.Unpack(data, request_type)
        except PidStore.UnpackException, e:
          obj = None
          unpack_exception = e
          status.response_code = OlaClient.RDM_INVALID_RESPONSE
      else:
        obj = data
    callback(status, pid, obj, unpack_exception)

  def _CreateCallback(self, callback, request_type, uid):
    return lambda s, p, d, r: self._GenericHandler(
        callback,
        request_type,
        uid,
        s, p, d, r)
