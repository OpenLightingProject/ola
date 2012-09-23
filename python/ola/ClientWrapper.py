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

import array
import datetime
import fcntl
import heapq
import logging
import os
import select
import socket
import termios
import threading
import traceback
from ola.OlaClient import OLADNotRunningException, OlaClient, Universe


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


class SelectServer(object):
  """Similar to include/ola/io/SelectServer.h

  This class isn't thread safe, apart from Execute and Stop.
  """
  def __init__(self):
    self._quit = False
    self._client = OlaClient()
    self._events = []
    heapq.heapify(self._events)
    # sockets
    self._sockets = {}

    # functions to run in the SelectServer
    self._ss_thread_id = self._GetThreadID()
    self._functions = []
    self._function_list_lock = threading.Lock()
    self._local_socket = os.pipe()
    self.AddSocket(self._local_socket[0], self._DrainAndExecute)

  def Execute(self, function):
    """Execute a function from within the SelectServer.
       Can be called from any thread.
    """
    self._function_list_lock.acquire()
    self._functions.append(function)
    # can write anything here
    os.write(self._local_socket[1], 'a')
    self._function_list_lock.release()

  def Terminate(self):
    if self._ss_thread_id == self._GetThreadID():
      self._Stop()
    else:
      self.Execute(self._Stop)

  def Reset(self):
    self._quit = False

  def StopIfNoEvents(self):
    if len(self._events) == 0:
      self._quit = True

  def AddSocket(self, sd, callback):
    """Add a socket to the socket set.

    Args:
      sd: the socket to add
      callback: the callback to run when this socket is ready.
    """
    self._sockets[sd] = callback

  def Run(self):
    if self._ss_thread_id != self._GetThreadID():
      logging.critical(
         'SelectServer called in a thread other than the owner. '
         'Owner %d, caller %d' % (self._ss_thread_id, self._GetThreadID()))
      traceback.print_stack()
    self._quit = False
    while self._client.GetSocket() is not None and not self._quit:
      # default to 1s sleep
      sleep_time = 1
      now = datetime.datetime.now()
      self._CheckTimeouts(now)
      if len(self._events):
        sleep_time = min(1.0, self._events[0].TimeLeft(now))

      i, o, e = select.select(self._sockets.keys(), [], [], sleep_time)
      now = datetime.datetime.now()
      self._CheckTimeouts(now)
      for s, c in self._sockets.iteritems():
        if s in i:
          c()

  def AddEvent(self, time_in_ms, callback):
    """Schedule an event to run in the future.

    Args:
      time_in_ms: An interval in milliseconds when this should run.
      callback: The function to run.
    """
    event = Event(time_in_ms, callback)
    heapq.heappush(self._events, event)

  def _CheckTimeouts(self, now):
    """Execute any expired timeouts."""
    while len(self._events):
      event = self._events[0]
      if event.HasExpired(now):
        event.Run()
      else:
        break
      heapq.heappop(self._events)

  def _GetThreadID(self):
    return threading.currentThread().ident

  def _Stop(self):
    self._quit = True

  def _DrainAndExecute(self):
    "Run all the queued functions."""
    # drain socket
    buf_ = array.array('i', [0])
    if fcntl.ioctl(self._local_socket[0], termios.FIONREAD, buf_, 1) == -1:
      return
    b = os.read(self._local_socket[0], buf_[0])

    self._function_list_lock.acquire()
    functions = list(self._functions)
    self._functions = []
    self._function_list_lock.release()

    for f in functions:
      f()


class ClientWrapper(object):
  def __init__(self):
    self._ss = SelectServer()
    self._client = OlaClient()
    self._ss.AddSocket(self._client.GetSocket(), self._client.SocketReady)

  def Stop(self):
    self._ss.Terminate()

  def StopIfNoEvents(self):
    self._ss.StopIfNoEvents()

  def Reset(self):
    self._ss.Reset()

  def Client(self):
    return self._client

  def Run(self):
    self._ss.Run()

  def AddEvent(self, time_in_ms, callback):
    """Schedule an event to run in the future.

    Args:
      time_in_ms: An interval in milliseconds when this should run.
      callback: The function to run.
    """
    self._ss.AddEvent(time_in_ms, callback)
