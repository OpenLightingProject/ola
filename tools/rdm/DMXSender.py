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
# DMXSenderThread.py
# Copyright (C) 2011 Simon Newton

__author__ = 'nomis52@gmail.com (Simon Newton)'

import array
import logging

class DMXSender(object):
  def __init__(self, ola_wrapper, universe, frame_rate):
    """Create a new DMXSender:

    Args:
      ola_wrapper: the ClientWrapper to use
      universe: universe number to send on
      frame_rate: frames per second
    """
    self._wrapper = ola_wrapper
    self._universe = universe
    self._data = array.array('B')
    self._data.append(0)
    self._frame_count = 0

    if (frame_rate > 0):
      logging.info('Sending %d fps of DMX data' % frame_rate)
      self._frame_interval = 1000 / frame_rate
      self.SendDMXFrame()

  def SendDMXFrame(self):
    """Send the next DMX Frame."""
    self._data[0] = self._frame_count % 255
    self._frame_count += 1
    self._wrapper.Client().SendDmx(self._universe,
                                   self._data,
                                   self.SendComplete)
    self._wrapper.AddEvent(self._frame_interval, self.SendDMXFrame)

  def SendComplete(self, state):
    """Called when the DMX send completes."""
