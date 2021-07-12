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
# DMXSender.py
# Copyright (C) 2011 Simon Newton

import array
import logging

__author__ = 'nomis52@gmail.com (Simon Newton)'


class DMXSender(object):
  def __init__(self, ola_wrapper, universe, frame_rate, slot_count):
    """Create a new DMXSender:

    Args:
      ola_wrapper: the ClientWrapper to use
      universe: universe number to send on
      frame_rate: frames per second
      slot_count: number of slots to send
    """
    self._wrapper = ola_wrapper
    self._universe = universe
    self._data = array.array('B')
    self._frame_count = 0
    self._slot_count = max(0, min(int(slot_count), 512))
    self._send = True

    if (frame_rate > 0 and slot_count > 0):
      logging.info('Sending %d FPS of DMX data with %d slots' %
                   (frame_rate, self._slot_count))
      for i in range(0, self._slot_count):
        self._data.append(0)
      self._frame_interval = 1000 / frame_rate
      self.SendDMXFrame()

  def Stop(self):
    self._send = False

  def SendDMXFrame(self):
    """Send the next DMX Frame."""
    for i in range(0, self._slot_count):
      self._data[i] = self._frame_count % 255
    self._frame_count += 1
    self._wrapper.Client().SendDmx(self._universe,
                                   self._data,
                                   self.SendComplete)
    if self._send:
      self._wrapper.AddEvent(self._frame_interval, self.SendDMXFrame)

  def SendComplete(self, state):
    """Called when the DMX send completes."""
