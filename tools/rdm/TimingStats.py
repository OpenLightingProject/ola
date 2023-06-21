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
# TimingStats.py
# Copyright (C) 2015 Simon Newton

import logging

import numpy
from ola.OlaClient import OlaClient

__author__ = 'nomis52@gmail.com (Simon Newton)'


class FrameTypeStats(object):
  """Holds timing stats for a particular class of response."""
  def __init__(self):
    self._count = 0
    self._response_times = []
    self._break_times = []
    self._mark_times = []
    self._data_times = []

  def RecordFrame(self, frame):
    self._count += 1
    if frame.response_delay:
      self._response_times.append(frame.response_delay)

    if frame.break_time:
      self._break_times.append(frame.break_time)

    if frame.mark_time:
      self._mark_times.append(frame.mark_time)

    if frame.data_time:
      self._data_times.append(frame.data_time)

  def Count(self):
    return self._count

  def ResponseTime(self):
    return self._BuildStats(self._response_times)

  def Break(self):
    return self._BuildStats(self._break_times)

  def Mark(self):
    return self._BuildStats(self._mark_times)

  def Data(self):
    return self._BuildStats(self._data_times)

  def _BuildStats(self, data):
    # convert to numpy.array here
    if not data:
      # reset 0 to stats
      data = [0]

    array = numpy.array([x / 1000.0 for x in data])
    return {
        'max': numpy.amax(array),
        'mean': numpy.mean(array),
        'median': numpy.median(array),
        'min': numpy.amin(array),
        'std': numpy.std(array),
        '99': numpy.percentile(array, 99),
    }


class TimingStats(object):
  """Holds the timing stats for all frame types."""
  GET, SET, DISCOVERY, DUB = range(4)

  def __init__(self):
    self._stats_by_type = {}
    frame_types = [self.GET, self.SET, self.DISCOVERY, self.DUB]
    for frame_type in frame_types:
      self._stats_by_type[frame_type] = FrameTypeStats()

  def GetStatsForType(self, frame_type):
    return self._stats_by_type.get(frame_type)

  def RecordFrame(self, frame_type, frame):
    stats = self._stats_by_type.get(frame_type)
    if stats:
      stats.RecordFrame(frame)
    else:
      logging.error('Unknown frame type %s' % frame_type)

  @staticmethod
  def FrameTypeFromCommandClass(command_class):
    types = {
      OlaClient.RDM_GET_RESPONSE: TimingStats.GET,
      OlaClient.RDM_SET_RESPONSE: TimingStats.SET,
      OlaClient.RDM_DISCOVERY_RESPONSE: TimingStats.DISCOVERY,
    }
    return types.get(command_class)
