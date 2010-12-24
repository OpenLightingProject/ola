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

  def Get(self, universe, uid, sub_device, pid, callback):
    """Send a RDM Get message.

    Args:
      universe: The universe to send the request on.
      uid: The UID to address the request to.
      sub_device: The Sub Device to send the request to.
      pid: A PID object that describes the format of the request.
      callback: The callback to run when the request completes.

    Return:
      True if sent ok, False otherwise.
    """
    if self._strict_checks:
      if uid.IsBroadcast():
        print >> sys.stderr, "Can't send GET to broadcast address %s" % uid
        return False

      args = {
        'uid': uid,
        'sub_device': sub_device,
      }
      if not pid.ValidateGet(args):
        return False

    return self._client.RDMGet(
        universe,
        uid,
        sub_device,
        pid.value,
        self._CreateCallback(callback))

  def Set(self, universe, uid, sub_device, pid, callback):
    """Send a RDM Set message.

    Args:
      universe: The universe to send the request on.
      uid: The UID to address the request to.
      sub_device: The Sub Device to send the request to.
      pid: A PID object that describes the format of the request.
      callback: The callback to run when the request completes.

    Return:
      True if sent ok, False otherwise.
    """
    if self._strict_checks:
      args = {
        'uid': uid,
        'sub_device': sub_device,
      }
      if not pid.ValidateSet(args):
        return False

    return self._client.RDMSet(
        universe,
        uid,
        sub_device,
        pid.value,
        self._CreateCallback(callback, is_set = True))

  def _GenericHandler(self, callback, is_set, status, pid, data,
                      unused_raw_data):
    obj = None
    pid_descriptor = self._pid_store.get(pid)
    if status.WasSuccessfull():
      if pid_descriptor:
        obj = pid_descriptor.Unpack(data, is_set)
        if obj is None:
          status.response_code = RDMAPI.RDM_INVALID_RESPONSE
      else:
        obj = data
    callback(status, pid, obj)

  def _CreateCallback(self, callback, is_set = False):
    return lambda s, p, d, r: self._GenericHandler(
        callback,
        is_set,
        s, p, d, r)
