#!/usr/bin/python
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# ola_send_dmx.py
# Copyright (C) 2005-2009 Simon Newton

"""Send some DMX data."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import array
from ola.ClientWrapper import ClientWrapper

def DmxSent(state):
  wrapper.Stop()

universe = 1
data = array.array('B')
data.append(10)
data.append(50)
data.append(255)

wrapper = ClientWrapper()
client = wrapper.Client()
client.SendDmx(universe, data, DmxSent)
wrapper.Run()
