# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
# ClientWrapper.py
# Copyright (C) 2005 Simon Newton

import array
import datetime
import fcntl
import heapq
import logging
import os
import select
import termios
import threading
import traceback

from ola.OlaClient import OlaClient

"""A simple client wrapper for the OlaClient."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class _Event(object):
  """An _Event represents a timer scheduled to expire in the future.

  Args:
    delay: datetime.timedelta or number of ms before this event fires
    callback: the callable to run
  """
  def __init__(self, delay, callback):
    self._run_at = (datetime.datetime.now() +
                    (delay if isinstance(delay, datetime.timedelta)
                     else datetime.timedelta(milliseconds=delay)))
    self._callback = callback

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self._run_at == other._run_at and self._callback == other._callback

  def __lt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    if self._run_at != other._run_at:
      return self._run_at < other._run_at
    return self._callback.__name__ < other._callback.__name__

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other

  def __hash__(self):
    return hash((self._run_at, self._callback))

  def TimeLeft(self, now):
    """Get the time remaining before this event triggers.

    Returns:
      The number of seconds (as a float) before this event fires.
    """
    time_delta = self._run_at - now
    seconds = time_delta.seconds + time_delta.days * 24 * 3600
    seconds += time_delta.microseconds / 10.0 ** 6
    return seconds

  def HasExpired(self, now):
    """Return true if this event has expired."""
    return self._run_at < now

  def Run(self):
    """Run the callback."""
    self._callback()


class SelectServer(object):
  """Similar to include/ola/io/SelectServer.h

  Manages I/O and Events.
  This class isn't thread safe, apart from Execute and Stop.
  """
  def __init__(self):
    self._quit = False
    self._ss_thread_id = self._GetThreadID()
    # heap of _Event objects, ordered by time-to-expiry
    self._events = []
    heapq.heapify(self._events)
    # sockets to track
    self._read_descriptors = {}
    self._write_descriptors = {}
    self._error_descriptors = {}
    # functions to run in the SelectServer
    self._functions = []
    self._function_list_lock = threading.Lock()
    # the pipe used to wake up select() from other threads
    self._local_socket = os.pipe()

  def __del__(self):
    os.close(self._local_socket[0])
    os.close(self._local_socket[1])

  def Execute(self, f):
    """Execute a function from within the SelectServer.
       Can be called from any thread.

    Args:
      f: The callable to run
    """
    self._function_list_lock.acquire()
    self._functions.append(f)
    # can write anything here, this wakes up the select() call
    os.write(self._local_socket[1], b'a')
    self._function_list_lock.release()

  def Terminate(self):
    """Terminate this SelectServer. Can be called from any thread."""
    if self._ss_thread_id == self._GetThreadID():
      self._Stop()
    else:
      self.Execute(self._Stop)

  def Reset(self):
    self._quit = False

  def StopIfNoEvents(self):
    if len(self._events) == 0:
      self._quit = True

  def AddReadDescriptor(self, fd, callback):
    """Add a descriptor to the read FD Set.

    Args:
      fd: the descriptor to add
      callback: the callback to run when this descriptor is ready.
    """
    self._read_descriptors[fd] = callback

  def RemoveReadDescriptor(self, fd):
    """Remove a socket from the read FD Set.

    Args:
      fd: the descriptor to remove
    """
    if fd in self._read_descriptors:
      del self._read_descriptors[fd]

  def AddWriteDescriptor(self, fd, callback):
    """Add a socket to the write FD Set.

    Args:
      fd: the descriptor to add
      callback: the callback to run when this descriptor is ready.
    """
    self._write_descriptors[fd] = callback

  def RemoveWriteDescriptor(self, fd):
    """Remove a socket from the write FD Set.

    Args:
      fd: the descriptor to remove
    """
    if fd in self._write_descriptors:
      del self._write_descriptors[fd]

  def AddErrorDescriptor(self, fd, callback):
    """Add a descriptor to the error FD Set.

    Args:
      fd: the descriptor to add
      callback: the callback to run when this descriptor is ready.
    """
    self._error_descriptors[fd] = callback

  def Run(self):
    """Run the SelectServer. This doesn't return until Terminate() is called.


    Returns:
      False if the calling thread isn't the one that created the select server.
    """
    if self._ss_thread_id != self._GetThreadID():
      logging.critical(
         'SelectServer called in a thread other than the owner. '
         'Owner %d, caller %d' % (self._ss_thread_id, self._GetThreadID()))
      traceback.print_stack()

    # Add the internal descriptor, see comments below
    self.AddReadDescriptor(self._local_socket[0], self._DrainAndExecute)
    self._quit = False
    while not self._quit:
      # default to 1s sleep
      sleep_time = 1
      now = datetime.datetime.now()
      self._CheckTimeouts(now)
      if self._quit:
        sleep_time = 0
      if len(self._events):
        sleep_time = min(sleep_time, self._events[0].TimeLeft(now))

      i, o, e = select.select(self._read_descriptors.keys(),
                              self._write_descriptors.keys(),
                              self._error_descriptors.keys(),
                              sleep_time)
      now = datetime.datetime.now()
      self._CheckTimeouts(now)
      self._CheckDescriptors(i, self._read_descriptors)
      self._CheckDescriptors(o, self._write_descriptors)
      self._CheckDescriptors(e, self._error_descriptors)

    # remove the internal socket from the read set to avoid a circular
    # reference which in turn breaks garbage collection (and leaks the socket
    # descriptors).
    self.RemoveReadDescriptor(self._local_socket[0])

  def AddEvent(self, delay, callback):
    """Schedule an event to run in the future.

    Args:
      delay: timedelta or number of ms before this event fires
      callback: The function to run.
    """
    event = _Event(delay, callback)
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

  def _CheckDescriptors(self, ready_set, all_descriptors):
    runnables = []
    for fd, runnable in all_descriptors.items():
      if fd in ready_set:
        runnables.append(runnable)
    for runnable in runnables:
      runnable()

  def _GetThreadID(self):
    return threading.current_thread().ident

  def _Stop(self):
    self._quit = True

  def _DrainAndExecute(self):
    "Run all the queued functions."""
    # drain socket
    buf_ = array.array('i', [0])
    if fcntl.ioctl(self._local_socket[0], termios.FIONREAD, buf_, 1) == -1:
      return
    os.read(self._local_socket[0], buf_[0])

    self._function_list_lock.acquire()
    functions = list(self._functions)
    self._functions = []
    self._function_list_lock.release()

    for f in functions:
      f()


class ClientWrapper(object):
  def __init__(self, socket=None):
    self._ss = SelectServer()
    self._client = OlaClient(socket)
    self._ss.AddReadDescriptor(self._client.GetSocket(),
                               self._client.SocketReady)

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
