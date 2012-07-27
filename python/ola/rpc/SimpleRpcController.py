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
# SimpleRpcController.py
# Copyright (C) 2005-2009 Simon Newton

"""An implementation of the RpcController interface."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

from google.protobuf import service

class SimpleRpcController(service.RpcController):
  """See google.protobuf.service.RpcController for documentation."""
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
