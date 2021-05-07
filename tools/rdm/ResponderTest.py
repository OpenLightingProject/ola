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
# ResponderTest.py
# Copyright (C) 2010 Simon Newton
#
# The test classes are broken down as follows:
#
# TestFixture - The base test class, defines common behaviour
#  ResponderTestFixture - A test which involves sending one or more RDM
#                         commands to a responder
#   ParamDescriptionTestFixture - A test fixture that changes the expected
#                                 result based on if PARAMETER_DESCRIPTION
#                                 is supported
#   OptionalParameterTestFixture - A test fixture that changes the expected
#                                  result based on the output of
#                                  SUPPORTED_PARAMETERS

import logging
import time
from ExpectedResults import (AckDiscoveryResult, AckGetResult, AckSetResult,
                             NackDiscoveryResult, NackGetResult, NackSetResult)
from TestCategory import TestCategory
from TestState import TestState
from TimingStats import TimingStats
from ola import PidStore
from ola.OlaClient import OlaClient, RDMNack

'''Automated testing for RDM responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


class Error(Exception):
  """The base error class."""


class UndeclaredPropertyException(Error):
  """Raised if a test attempts to get/set a property that it didn't declare."""


class TestFixture(object):
  """The base responder test class, every test inherits from this."""
  PID = None
  CATEGORY = TestCategory.UNCLASSIFIED
  DEPS = []
  PROVIDES = []
  REQUIRES = []

  def __init__(self, device, universe, uid, pid_store, *args, **kwargs):
    self._warnings = []
    self._advisories = []
    self._debug = []
    self._device_properties = device
    self._uid = uid
    self._pid_store = pid_store
    self._state = TestState.NOT_RUN
    try:
      self.pid = self.LookupPid(self.PID)
    except AttributeError:
      self.pid = None
    if self.PidRequired() and self.pid is None:
      self.SetBroken("%s: Couldn't find PID from %s" %
                     (self.__class__.__name__, self.PID))

  def __hash__(self):
    return hash(self.__class__.__name__)

  def __str__(self):
    return self.__class__.__name__

  def __repr__(self):
    return self.__class__.__name__

  # TestFixture and its subclasses are equal/ordered based on comparing
  # class name.  Two TestFixtures are equal if they are the same class,
  # and ordering is based on comparing __class__.__name__
  def __eq__(self, other):
    return other.__class__ is self.__class__

  def __lt__(self, other):
    if not issubclass(other.__class__, TestFixture):
      return NotImplemented
    return self.__class__.__name__ < other.__class__.__name__

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not issubclass(other.__class__, TestFixture):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not issubclass(other.__class__, TestFixture):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not issubclass(other.__class__, TestFixture):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other

  def PidRequired(self):
    """Whether a valid PID is required for this test"""
    return True

  def LookupPid(self, pid_name):
    return self._pid_store.GetName(pid_name, self._uid)

  def LookupPidValue(self, pid_value):
    return self._pid_store.GetPid(pid_value)

  def Requires(self):
    """Returns a list of the properties this test requires to run."""
    return self.REQUIRES

  @property
  def category(self):
    """The category this test belongs to."""
    return self.CATEGORY

  # Warnings are logged independently of errors.
  # They should be used to indicate deviations from the standard that will not
  # cause interoperability issues
  @property
  def warnings(self):
    """Non-fatal warnings."""
    return self._warnings

  def AddWarning(self, message):
    """Add an warning message.

    Args:
      message: The text of the warning message.
    """
    self.LogDebug(' Warning: %s' % message)
    self._warnings.append(message)

  # Advisories are logged independently of errors. They should be used to
  # indicate conditions that while not covered by the standard, should still be
  # fixed.
  @property
  def advisories(self):
    """Non-fatal advisories message."""
    return self._advisories

  def AddAdvisory(self, message):
    """Add an advisory message.

    Args:
      message: The text of the advisory message.
    """
    self.LogDebug(' Advisory: %s' % message)
    self._advisories.append(message)

  @property
  def debug(self):
    """Return a list of debug lines."""
    return self._debug

  def LogDebug(self, string):
    """Append debugging information."""
    logging.debug(string)
    self._debug.append(string)

  def Property(self, property, default=None):
    """Lookup a device property.

    Args:
      property: the name of the property.

    Returns:
      The value of the property.
    """
    if property not in self.Requires():
      raise UndeclaredPropertyException(
        '%s attempted to get %s which wasn\'t declared' %
        (self.__class__.__name__, property))
    return getattr(self._device_properties, property, default)

  def SetProperty(self, property, value):
    """Set a device property.

    Args:
      property: the name of the property.
      value: the value of the property.
    """
    if property not in self.PROVIDES:
      raise UndeclaredPropertyException(
        '%s attempted to set %s which wasn\'t declared' %
        (self.__class__.__name__, property))
    setattr(self._device_properties, property, value)

  def SetPropertyFromDict(self, dictionary, property, default=None):
    """Set a property to the value of the same name in a dictionary.

    Often it's useful to set a property to a value in a dictionary where the
    key has the same name.

    Args:
      dictionary: A dictionary that has a value for the property key
      property: the name of the property.
    """
    self.SetProperty(property, dictionary.get(property, default))

  @property
  def state(self):
    """Returns the state of the test.

    Returns:
      An instance of TestState
    """
    return self._state

  # Sub classes can override these
  def Test(self):
    self.SetBroken('test method not defined')

  def Run(self):
    """Run the test."""
    # Try and fail early
    if self.state == TestState.BROKEN:
      return

    self.Test()

  def Stop(self):
    self.SetBroken('stop method not defined')

  def SetNotRun(self, message=None):
    """Set the state of the test to NOT_RUN."""
    self._state = TestState.NOT_RUN
    if message:
      self.LogDebug(' ' + message)

  def SetBroken(self, message):
    """Set the state of the test to BROKEN."""
    self.LogDebug(' Broken: %s' % message)
    self._state = TestState.BROKEN

  def SetFailed(self, message):
    """Set the state of the test to FAILED."""
    self.LogDebug(' Failed: %s' % message)
    self._state = TestState.FAILED

  def SetPassed(self):
    """Set the state of the test to PASSED."""
    self._state = TestState.PASSED


class ResponderTestFixture(TestFixture):
  """A Test that that sends one or more messages to a responder."""
  def __init__(self,
               device,
               universe,
               uid,
               pid_store,
               rdm_api,
               wrapper,
               broadcast_write_delay,
               timing_stats):
    super(ResponderTestFixture, self).__init__(device, universe, uid, pid_store)
    self._api = rdm_api
    self._expected_results = []
    self._in_reset_mode = False
    self._should_run_wrapper = True
    self._universe = universe
    self._wrapper = wrapper
    self._broadcast_write_delay_s = broadcast_write_delay / 1000.0
    self._timing_stats = timing_stats

    # This is set to the tuple of (sub_device, command_class, pid) when we send
    # a message. It's used to identify the response if we get an ACK_TIMER and
    # use QUEUED_MESSAGEs
    self._outstanding_request = None

  @property
  def uid(self):
    return self._uid

  def PidSupported(self):
    # By default all PIDs are supported, overridden in subclasses
    return True

  def SleepAfterBroadcastSet(self):
    if self._broadcast_write_delay_s:
      self.LogDebug('Sleeping after broadcast...')
    time.sleep(self._broadcast_write_delay_s)

  def Run(self):
    """Call the test method and then start running the loop wrapper."""
    # Try and fail early
    if self.state == TestState.BROKEN:
      return

    # the super call invokes self.Test()
    super(ResponderTestFixture, self).Run()

    # Check if we've broken during self.Test()
    if self.state == TestState.BROKEN:
      return

    if self._should_run_wrapper:
      self._wrapper.Run()
    if self.state != TestState.NOT_RUN:
      self._in_reset_mode = True
      self.ResetState()
    self._wrapper.Reset()

  def VerifyResult(self, status, fields):
    """A hook to perform additional verification of data."""
    pass

  def ResetState(self):
    """A hook to reset the responder after the test has run."""
    pass

  def Stop(self):
    self._should_run_wrapper = False
    self._wrapper.Stop()

  def SetNotRun(self, message=None):
    """Set the state of the test to NOT_RUN and stop further processing."""
    super(ResponderTestFixture, self).SetNotRun(message)
    self.Stop()

  def SetBroken(self, message):
    """Set the state of the test to BROKEN and stop further processing."""
    super(ResponderTestFixture, self).SetBroken(message)
    self.Stop()

  def SetFailed(self, message):
    """Set the state of the test to FAILED and stop further processing."""
    super(ResponderTestFixture, self).SetFailed(message)
    self.Stop()

  def SetPassed(self):
    """Set the state of the test to PASSED and stop further processing."""
    super(ResponderTestFixture, self).SetPassed()
    self.Stop()

  def AddExpectedResults(self, results):
    """Add a set of expected results."""
    if isinstance(results, list):
      self._expected_results = results
    else:
      self._expected_results = [results]

  def AddIfGetSupported(self, result):
    """Add a set of expected GET results if the PID is supported"""
    if not self.PidSupported():
      result = self.NackGetResult(RDMNack.NR_UNKNOWN_PID)
    self.AddExpectedResults(result)

  def AddIfSetSupported(self, result):
    """Add a set of expected SET results if the PID is supported"""
    # The lock modes means that some sets may return NR_WRITE_PROTECT. Account
    # for that here.
    if not self.PidSupported():
      expected_results = self.NackSetResult(RDMNack.NR_UNKNOWN_PID)
    else:
      expected_results = [
        self.NackSetResult(
          RDMNack.NR_WRITE_PROTECT,
          advisory='SET %s was write protected, try changing the lock mode if '
                   'enabled' % self.pid.name)
      ]
      if isinstance(result, list):
        expected_results.extend(result)
      else:
        expected_results.append(result)

    self.AddExpectedResults(expected_results)

  def NackDiscoveryResult(self, nack_reason, **kwargs):
    """A helper method which returns a NackDiscoveryResult for the current
       PID.
    """
    return NackDiscoveryResult(self.pid.value, nack_reason, **kwargs)

  def NackGetResult(self, nack_reason, **kwargs):
    """A helper method which returns a NackGetResult for the current PID."""
    return NackGetResult(self.pid.value, nack_reason, **kwargs)

  def NackSetResult(self, nack_reason, **kwargs):
    """A helper method which returns a NackSetResult for the current PID."""
    return NackSetResult(self.pid.value, nack_reason, **kwargs)

  def AckDiscoveryResult(self, **kwargs):
    """A helper method which returns an AckDiscoveryResult for the current
      PID.
    """
    return AckDiscoveryResult(self.pid.value, **kwargs)

  def AckGetResult(self, **kwargs):
    """A helper method which returns an AckGetResult for the current PID."""
    return AckGetResult(self.pid.value, **kwargs)

  def AckSetResult(self, **kwargs):
    """A helper method which returns an AckSetResult for the current PID."""
    return AckSetResult(self.pid.value, **kwargs)

  def SendDiscovery(self, sub_device, pid, args=[]):
    """Send a raw Discovery request.

    Args:
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    return self.SendDirectedDiscovery(self._uid, sub_device, pid, args)

  def SendDirectedDiscovery(self, uid, sub_device, pid, args=[]):
    """Send a raw Discovery request.

    Args:
      uid: The uid to send the GET to
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    self.LogDebug(' DISCOVERY: uid: %s, pid: %s, sub device: %d, args: %s' %
                  (uid, pid, sub_device, args))
    self._MakeRequestKey(sub_device, PidStore.RDM_DISCOVERY, pid.value)
    return self._api.Discovery(self._universe,
                               uid,
                               sub_device,
                               pid,
                               self._HandleResponse,
                               args,
                               include_frames=True)

  def SendRawDiscovery(self, sub_device, pid, data=""):
    """Send a raw Discovery request.

    Args:
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    self.LogDebug(' DISCOVERY: pid: %s, sub device: %d, data: %r' %
                  (pid, sub_device, data))
    self._MakeRequestKey(sub_device, PidStore.RDM_DISCOVERY, pid.value)
    return self._api.RawDiscovery(self._universe,
                                  self._uid,
                                  sub_device,
                                  pid,
                                  self._HandleResponse,
                                  data,
                                  include_frames=True)

  def SendGet(self, sub_device, pid, args=[]):
    """Send a GET request using the RDM API.

    Args:
      sub_device: The sub device
      pid: A PID object
      args: A list of arguments
    """
    return self.SendDirectedGet(self._uid, sub_device, pid, args)

  def SendDirectedGet(self, uid, sub_device, pid, args=[]):
    """Send a GET request using the RDM API.

    Args:
      uid: The uid to send the GET to
      sub_device: The sub device
      pid: A PID object
      args: A list of arguments
    """
    self.LogDebug(' GET: uid: %s, pid: %s, sub device: %d, args: %s' %
                  (uid, pid, sub_device, args))
    self._MakeRequestKey(sub_device, PidStore.RDM_GET, pid.value)
    ret_code = self._api.Get(self._universe,
                             uid,
                             sub_device,
                             pid,
                             self._HandleResponse,
                             args,
                             include_frames=True)
    return ret_code

  def SendRawGet(self, sub_device, pid, data=""):
    """Send a raw GET request.

    Args:
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    self.LogDebug(' GET: uid: %s, pid: %s, sub device: %d, data: %r' %
                  (self._uid, pid, sub_device, data))
    self._MakeRequestKey(sub_device, PidStore.RDM_GET, pid.value)
    return self._api.RawGet(self._universe,
                            self._uid,
                            sub_device,
                            pid,
                            self._HandleResponse,
                            data,
                            include_frames=True)

  def SendSet(self, sub_device, pid, args=[]):
    """Send a SET request using the RDM API.

    Args:
      sub_device: The sub device
      pid: A PID object
      args: A list of arguments
    """
    return self.SendDirectedSet(self._uid, sub_device, pid, args)

  def SendDirectedSet(self, uid, sub_device, pid, args=[]):
    """Send a SET request using the RDM API.

    Args:
      uid: The uid to send the GET to
      sub_device: The sub device
      pid: A PID object
      args: A list of arguments
    """
    self.LogDebug(' SET: uid: %s, pid: %s, sub device: %d, args: %s' %
                  (uid, pid, sub_device, self._EscapeData(args)))
    self._MakeRequestKey(sub_device, PidStore.RDM_SET, pid.value)
    ret_code = self._api.Set(self._universe,
                             uid,
                             sub_device,
                             pid,
                             self._HandleResponse,
                             args,
                             include_frames=True)
    if uid.IsBroadcast():
      self.SleepAfterBroadcastSet()
    return ret_code

  def SendRawSet(self, sub_device, pid, data=""):
    """Send a raw SET request.

    Args:
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    self.LogDebug(' SET: pid: %s, sub device: %d, data: %r' %
                  (pid, sub_device, data))
    self._MakeRequestKey(sub_device, PidStore.RDM_SET, pid.value)
    return self._api.RawSet(self._universe,
                            self._uid,
                            sub_device,
                            pid,
                            self._HandleResponse,
                            data,
                            include_frames=True)

  def _HandleResponse(self, response, unpacked_data, unpack_exception):
    """Handle a RDM response.

    Args:
      response: A RDMResponse object
      unpacked_data: The unpacked data structure for this pid
      unpack_exception: The exception if the data could not be unpacked
    """
    if not self._CheckForAckOrNack(response, unpacked_data, unpack_exception):
      return

    self._PerformMatching(response, unpacked_data, unpack_exception)

  def _MakeRequestKey(self, sub_device, command_class, pid):
    # If the request was sent to the all-subdevice, the response should come
    # from the root.
    if sub_device == PidStore.ALL_SUB_DEVICES:
      sub_device = PidStore.ROOT_DEVICE
    self._outstanding_request = (sub_device, command_class, pid)

  def _HandleQueuedResponse(self, response, unpacked_data, unpack_exception):
    """Handle a response to a get QUEUED_MESSAGE request.

    Args:
      response: A RDMResponse object
      unpacked_data: The unpacked data structure for this pid
      unpack_exception: The exception if the data could not be unpacked
    """
    if not self._CheckForAckOrNack(response, unpacked_data, unpack_exception):
      return

    if response.response_code != OlaClient.RDM_COMPLETED_OK:
      self.SetFailed('Request failed: %s' % response.ResponseCodeAsString())
      self.Stop()
      return False

    # At this stage we have NACKs and ACKs left
    request_key = (response.sub_device, response.command_class, response.pid)
    if (self._outstanding_request == request_key):
      # This is what we've been waiting for
      self._PerformMatching(response, unpacked_data, unpack_exception)
      return

    queued_message_pid = self.LookupPid('QUEUED_MESSAGE')
    status_messages_pid = self.LookupPid('STATUS_MESSAGES')
    if (response.pid == queued_message_pid.value and
        response.response_type == OlaClient.RDM_NACK_REASON):
        # A Nack here is fatal because if we get an ACK_TIMER, QUEUED_MESSAGE
        # should be supported.
        self.SetFailed(' Get Queued message returned nack: %s' %
                       response.nack_reason)
        self.Stop()
        return
    elif (response.pid == status_messages_pid.value and
          response.response_type != OlaClient.RDM_NACK_REASON and
          unpacked_data.get('messages', None) == []):
        # This means we've run out of messages
        if self._state == TestState.NOT_RUN:
          self.SetFailed('ACK_TIMER issued but the response was never queued')
        self.Stop()
        return
    elif (response.pid == status_messages_pid.value and
          response.response_type == OlaClient.RDM_NACK_REASON):
        self.LogDebug('Status message returned nack, fetching next message: %s'
                      % response.nack_reason)

    # Otherwise fetch the next one
    self._GetQueuedMessage()

  def _CheckForAckOrNack(self, response, unpacked_data, unpack_exception):
    """Check the state of a RDM response.

    Returns:
      True if processing should continue, False if there was a transport error
      or a ACK_TIMER was received.
    """
    if not response.status.Succeeded():
      # This indicates a transport error
      self.SetBroken(' Error: %s' % response.status.message)
      self.Stop()
      return False

    if response.response_code != OlaClient.RDM_COMPLETED_OK:
      self.LogDebug(' Response status: %s' % response.ResponseCodeAsString())
      if response.response_code == OlaClient.RDM_DUB_RESPONSE:
        # track timing for DUB responses.
        self._RecordFrameTiming(response, TimingStats.DUB)
        self._LogFrameTiming(response)
      return True

    self._RecordFrameTiming(response)

    # Handle the case of an ack timer
    if response.response_type == OlaClient.RDM_ACK_TIMER:
      self.LogDebug(' Received ACK TIMER set to %d ms' % response.ack_timer)
      self._LogFrameTiming(response)
      self._wrapper.AddEvent(response.ack_timer, self._GetQueuedMessage)
      return False

    # Now log the result
    if response.WasAcked():
      if unpack_exception:
        self.LogDebug(' Response: %s, PID: 0x%04hx, TN: %d, Error: %s' %
                      (response, response.pid, response.transaction_number,
                       unpack_exception))
      else:
        escaped_string = '%s' % self._EscapeData(unpacked_data)
        self.LogDebug(' Response: %s, PID: 0x%04hx, TN: %d, PDL: %d, data: %s'
                      % (response, response.pid, response.transaction_number,
                         len(response.data), escaped_string))
    else:
      self.LogDebug(' Response: %s, PID: 0x%04hx, TN: %d' %
                    (response, response.pid, response.transaction_number))

    self._LogFrameTiming(response)
    return True

  def _EscapeData(self, data):
    if type(data) == list:
      return [self._EscapeData(i) for i in data]
    elif type(data) == dict:
      d = {}
      for k, v in data.iteritems():
        d[k] = self._EscapeData(v)
      return d
    elif type(data) == str:
      return data.encode('string-escape')
    elif type(data) == unicode:
      return data.encode('unicode-escape')
    else:
      return data

  def _PerformMatching(self, response, unpacked_data, unpack_exception):
    """Check if this response matched any of the expected values.

    Args:
      response: A RDMResponse object
      unpacked_data: The unpacked data structure for this pid
      unpack_exception: The exception if the data could not be unpacked
    """
    if response.WasAcked():
      if unpack_exception:
        self.SetFailed('Invalid Param data: %s' % unpack_exception)
        self.Stop()
        return

    if self._in_reset_mode:
      self._wrapper.Stop()
      return

    for result in self._expected_results:
      if result.Matches(response, unpacked_data):
        self._state = TestState.PASSED
        self.VerifyResult(response, unpacked_data)
        if result.warning is not None:
          self.AddWarning(result.warning)
        if result.advisory is not None:
          self.AddAdvisory(result.advisory)
        if result.action is not None:
          result.action()
        else:
          self._wrapper.Stop()

        return

    # Nothing matched
    self.SetFailed('expected one of:')
    for result in self._expected_results:
      self.LogDebug('  %s' % result)
    self.Stop()

  def _GetQueuedMessage(self):
    """Fetch queued messages."""
    queued_message_pid = self.LookupPid('QUEUED_MESSAGE')
    data = ['error']
    self.LogDebug(' GET: pid: %s, data: %s' % (queued_message_pid, data))

    self._api.Get(self._universe,
                  self._uid,
                  PidStore.ROOT_DEVICE,
                  queued_message_pid,
                  self._HandleQueuedResponse,
                  data,
                  include_frames=True)

  def _RecordFrameTiming(self, response, override_type=None):
    for frame in response.frames:
      frame_type = override_type
      if not frame_type:
        frame_type = TimingStats.FrameTypeFromCommandClass(
            response.command_class)
      self._timing_stats.RecordFrame(frame_type, frame)

  def _LogFrameTiming(self, response):
    for frame in response.frames:
      stats = []
      if frame.response_delay:
        stats.append('Response Delay: %.1fus' %
                     self._NanoSecondsToMicroSeconds(frame.response_delay))
      if frame.break_time:
        stats.append('Break: %.1fus' %
                     self._NanoSecondsToMicroSeconds(frame.break_time))
      if frame.mark_time:
        stats.append('Mark: %.1fus' %
                     self._NanoSecondsToMicroSeconds(frame.mark_time))
      if frame.data_time:
        stats.append('Data: %.1fus' %
                     self._NanoSecondsToMicroSeconds(frame.data_time))

      if stats:
        self.LogDebug('    ' + ', '.join(stats))

  def _NanoSecondsToMicroSeconds(self, nano_seconds):
    return nano_seconds / 1000.0


class ParamDescriptionTestFixture(ResponderTestFixture):
  """A sub class of ResponderTestFixture that alters behaviour if the
     PARAMETER_DESCRIPTION PID shouldn't be supported.
  """
  def Requires(self):
    return (super(ParamDescriptionTestFixture, self).Requires() +
            ['manufacturer_parameters'])

  def PidSupported(self):
    return self.Property('manufacturer_parameters')


class OptionalParameterTestFixture(ResponderTestFixture):
  """A sub class of ResponderTestFixture that alters behaviour if the PID isn't
     supported.
  """
  def Requires(self):
    return (super(OptionalParameterTestFixture, self).Requires() +
            ['supported_parameters'])

  def PidSupported(self):
    return self.pid.value in self.Property('supported_parameters')
