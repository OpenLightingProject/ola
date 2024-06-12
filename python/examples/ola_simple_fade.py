#!/usr/bin/env python
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# ola_simple_fade.py
# Copyright (C) 2014 Sean Sill

from array import array

from ola.ClientWrapper import ClientWrapper
from ola.DMXConstants import (DMX_MAX_SLOT_VALUE, DMX_MIN_SLOT_VALUE,
                              DMX_UNIVERSE_SIZE)

__author__ = 'Sean Sill'

"""
This script fades DMX_DATA_SIZE channels from 0 to 255. It serves as an example
of how to use AddEvent to schedule dmx data updates from python

To view data, use the web interface or patch an output device to the same
universe
"""


UPDATE_INTERVAL = 25  # In ms, this comes about to ~40 frames a second
SHUTDOWN_INTERVAL = 10000  # in ms, This is 10 seconds
DMX_DATA_SIZE = 100
UNIVERSE = 1


class SimpleFadeController(object):
  def __init__(self, universe, update_interval, client_wrapper,
               dmx_data_size=DMX_UNIVERSE_SIZE):
    dmx_data_size = min(dmx_data_size, DMX_UNIVERSE_SIZE)
    self._universe = universe
    self._update_interval = update_interval
    self._data = array('B', [DMX_MIN_SLOT_VALUE] * dmx_data_size)
    self._wrapper = client_wrapper
    self._client = client_wrapper.Client()
    self._wrapper.AddEvent(self._update_interval, self.UpdateDmx)

  def UpdateDmx(self):
    """
    This function gets called periodically based on UPDATE_INTERVAL
    """
    for i in range(len(self._data)):
      self._data[i] = (self._data[i] + 1) % DMX_MAX_SLOT_VALUE
    # Send the DMX data
    self._client.SendDmx(self._universe, self._data)
    # For more information on Add Event, reference the OlaClient
    # Add our event again so it becomes periodic
    self._wrapper.AddEvent(self._update_interval, self.UpdateDmx)


if __name__ == '__main__':
  wrapper = ClientWrapper()
  controller = SimpleFadeController(UNIVERSE, UPDATE_INTERVAL, wrapper,
                                    DMX_DATA_SIZE)
  # Call it initially
  wrapper.AddEvent(SHUTDOWN_INTERVAL, wrapper.Stop)
  # Start the wrapper
  wrapper.Run()
