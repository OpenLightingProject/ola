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
#
#

__author__ = 'Sean Sill'

"""
This script fades DMX_DATA_SIZE channels from 0 to 255. It serves as an example
of how to use AddEvent to schedule dmx data updates from python

To view data, use the Web browser on universe script or patch an output device to
universe specified in the script
"""

from array import *
from ola.ClientWrapper import ClientWrapper

UPDATE_INTERVAL = 25 # In ms, this comes about to ~40 frames a second
SHUTDOWN_INTERVAL = 10000 # in ms, This is 10 seconds
DMX_DATA_SIZE = 100
UNIVERSE = 1
MAX_DMX_VALUE = 255

class SimpleFadeController(object):
  def __init__(self, universe, update_interval, client_wrapper, dmx_data_size=512):
    self._universe = universe
    self._update_interval = update_interval
    self._data = array ('B', [])
    for i in range(0, dmx_data_size):
      self._data.append(0)
    self._wrapper = client_wrapper
    self._client = client_wrapper.Client()
    self._wrapper.AddEvent(self._update_interval, lambda: self.UpdateDmx())
    
  def UpdateDmx(self):
    """
    This function gets called periodically based on UPDATE_INTERVAL
    """
    for i in range(len(self._data)):
      if self._data[i] < MAX_DMX_VALUE:
        self._data[i] = self._data[i] + 1
      else:
        self._data[i] = 0
    # Send the DMX data
    self._client.SendDmx(self._universe, self._data)
    # For more information on AddEvent
    # https://github.com/OpenLightingProject/ola/blob/master/python/ola/ClientWrapper.py#L282
    self._wrapper.AddEvent(self._update_interval, lambda: self.UpdateDmx()) # Add our event here again
                                                    # so it gets called again

if __name__ == '__main__':
  wrapper = ClientWrapper()
  controller = SimpleFadeController(UNIVERSE, UPDATE_INTERVAL, wrapper, DMX_DATA_SIZE) 
  # Call it initially
  wrapper.AddEvent(SHUTDOWN_INTERVAL, lambda: wrapper.Stop())
  # Start the wrapper
  wrapper.Run()

