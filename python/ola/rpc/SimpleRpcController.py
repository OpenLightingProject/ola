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
# SimpleRpcController.py
# Copyright (C) 2005 Simon Newton

"""An implementation of the RpcController interface."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class SimpleRpcController(object):
  """Implements the protocol buffer RpcController interface.

  The generic service module was removed from protobuf 6.30.  RpcController is
  an interface, so consumers only require the methods implemented below and do
  not require the deprecated base class.
  """

  def __init__(self):
    self.Reset()

  def Reset(self):
    self._failed = False
    self._cancelled = False
    self._error = None
    self._callback = None

  def Failed(self):
    return self._failed

  def ErrorText(self):
    return self._error

  def StartCancel(self):
    self._cancelled = True
    if self._callback:
      self._callback()

  def SetFailed(self, reason):
    self._failed = True
    self._error = reason

  def IsCanceled(self):
    return self._cancelled

  def NotifyOnCancel(self, callback):
    self._callback = callback
