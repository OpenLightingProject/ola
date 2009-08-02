#!/usr/bin/python
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
# client_wrapper.py
# Copyright (C) 2005-2009 Simon Newton

"""A simple client wrapper for the OlaClient."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import socket
import select
from ola.OlaClient import OlaClient, Universe


class ClientWrapper(object):
  def __init__(self):
    self._quit = False
    self._sock = socket.socket()
    self._sock.connect(('localhost', 9010))
    self._client = OlaClient(self._sock)

  def Stop(self):
    self._quit = True

  def Client(self):
    return self._client

  def Run(self):
    while not self._quit:
      i, o, e = select.select([self._sock], [], [])
      if self._sock in i:
        self._client.SocketReady()
