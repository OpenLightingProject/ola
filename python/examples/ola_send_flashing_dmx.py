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
# ola_send_flashing_dmx.py
# Copyright (C) 2014 Sean Sill

"""Send some DMX data including the first one flashing."""

import array
from array import *
from ola.ClientWrapper import ClientWrapper

UPDATE_INTERVAL = 500 # In ms
DMX_DATA_SIZE = 100

universe = 1
data = array ('B', [])
for i in range(0, DMX_DATA_SIZE):
  data.append(255)

wrapper = ClientWrapper()
client = wrapper.Client()

def NewDmx():
  """
  This function gets called periodically based on UPDATE_INTERVAL
  """
  if (data[0] == 0):
    data[0] = 255
  else:
    data[0] = 0
  # Send the DMX data
  client.SendDmx(universe, data)
  # For more information on AddEvent
  # https://github.com/OpenLightingProject/ola/blob/master/python/ola/ClientWrapper.py#L282
  wrapper.AddEvent(UPDATE_INTERVAL, NewDmx) # Add our event here again so it gets called again

# Call it initially
wrapper.AddEvent(UPDATE_INTERVAL, NewDmx)
# Start the wrapper
wrapper.Run()
