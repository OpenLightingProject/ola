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

import datetime
import heapq
import socket
import select
from ola.OlaClient import OlaClient, Universe


class Event(object):
  def __init__(self, time_ms, callback):
    self._run_at = (datetime.datetime.now() +
                    datetime.timedelta(milliseconds = time_ms))
    self._callback = callback

  def __cmp__(self, other):
    return cmp(self._run_at, other._run_at)

  def TimeLeft(self, now):
    """Returns the number of seconds until this event."""
    time_delta = self._run_at - now
    seconds =  time_delta.seconds + time_delta.days * 24 * 3600
    seconds += time_delta.microseconds / 10.0**6
    return seconds

  def HasExpired(self, now):
    return self._run_at < now

  def Run(self):
    self._callback()


class ClientWrapper(object):
  def __init__(self):
    self._quit = False
    self._sock = socket.socket()
    self._sock.connect(('localhost', 9010))
    self._client = OlaClient(self._sock)

    self._events = []
    heapq.heapify(self._events)

  def Stop(self):
    self._quit = True

  def StopIfNoEvents(self):
    if len(self._events) == 0:
      self._quit = True

  def Reset(self):
    self._quit = False

  def Client(self):
    return self._client

  def Run(self):
    self._quit = False
    while not self._quit:

      # default to 1s sleep
      sleep_time = 1
      now = datetime.datetime.now()
      self.CheckTimeouts(now)
      if len(self._events):
        sleep_time = min(1.0, self._events[0].TimeLeft(now))

      i, o, e = select.select([self._sock], [], [], sleep_time)
      now = datetime.datetime.now()
      self.CheckTimeouts(now)
      if self._sock in i:
        self._client.SocketReady()

  def AddEvent(self, time_in_ms, callback):
    """Schedule an event to run in the future.

    Args:
      time_in_ms: An interval in milliseconds when this should run.
      callback: The function to run.
    """
    event = Event(time_in_ms, callback)
    heapq.heappush(self._events, event)

  def CheckTimeouts(self, now):
    while len(self._events):
      event = self._events[0]
      if event.HasExpired(now):
        event.Run()
      else:
        break
      heapq.heappop(self._events)
