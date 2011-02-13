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
# ResponderTestFixture.py
# Copyright (C) 2010 Simon Newton
#
# The test classes are broken down as follows:
#
# TestFixture - The base test class, defines common behaviour
#  ResponderTestFixture - A test which involves sending one or more RDM
#                         commands to a responder
#   QueuedMessageTestFixture - A test which handles ACK_TIMER and fetches the
#                              appropriate queued message.
#    OptionalParameterTestFixture - A test fixture that changes the expected
#                                   result based on the output of
#                                   SUPPORTED_PARAMETERS
#
# NOTE: Almost all tests that involve RDM commands should inherit from
# QueuedMessageTestFixture. This ensures that by the time the test is run, we
# know if QUEUED_MESSAGE is supported which determines if an ACK_TIMER is a
# valid response.

'''Automated testing for RDM responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'

import logging
from ExpectedResults import *
from TestCategory import TestCategory
from TestState import TestState
from ola import PidStore
from ola.OlaClient import OlaClient, RDMNack
from ola.RDMAPI import RDMAPI


class Error(Exception):
  """The base error class."""


class UndeclaredPropertyException(Error):
  """Raised if a test attempts to get/set a property that it didn't declare."""


class TestFixture(object):
  """The base responder test class, every test inherits from this."""
  CATEGORY = TestCategory.UNCLASSIFIED
  DEPS = []
  PROVIDES = []
  REQUIRES = []

  def __init__(self, device, uid, pid_store):
    self._device_properties = device
    self._uid = uid
    self._pid_store = pid_store
    self._status = None
    self._state = TestState.NOT_RUN
    self.pid = self.LookupPid(self.PID)
    self._warnings = []
    self._advisories = []

  def __hash__(self):
    return hash(self.__class__.__name__)

  def __str__(self):
    return self.__class__.__name__

  def __repr__(self):
    return self.__class__.__name__

  def __cmp__(self, other):
    return cmp(self.__class__.__name__, other.__class__.__name__)

  def LookupPid(self, pid_name):
    return self._pid_store.GetName(pid_name, self._uid)

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
    logging.debug('Warning: %s' % message)
    self._warnings.append(message)

  # Advisories are logged independently of errors. They should be used to
  # indicate conditions that while aren't covered by the standard, should still
  # be fixed.
  @property
  def advisories(self):
    """Non-fatal advisories message."""
    return self._advisories

  def AddAdvisory(self, message):
    """Add an advisory message.

    Args:
      message: The text of the advisory message.
    """
    logging.debug('Advisory: %s' % message)
    self._advisories.append(message)

  def Property(self, property):
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
    return getattr(self._device_properties, property)

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

  def SetPropertyFromDict(self, dictionary, property):
    """Set a property to the value of the same name in a dictionary.

    Often it's useful to set a property to a value in a dictionary where the
    key has the same name.

    Args:
      dictionary: A dictionary that has a value for the property key
      property: the name of the property.
    """
    self.SetProperty(property, dictionary[property])

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
    """Run the test if all the required properties are available."""
    logging.debug('%s: %s' % (self, self.__doc__))

    for property in self.Requires():
      try:
        getattr(self._device_properties, property)
      except AttributeError:
        logging.debug(' Property: %s not found, skipping test.' % property)
        return

    self.Test()

  def SetNotRun(self, message=None):
    self._state = TestState.NOT_RUN
    if message:
      logging.debug(message)

  def SetBroken(self, message):
    logging.debug(' Broken: %s' % message)
    self._state = TestState.BROKEN

  def SetFailed(self, message):
    logging.debug(' Failed: %s' % message)
    self._state = TestState.FAILED


class ResponderTestFixture(TestFixture):
  """A Test that that sends one or more messages to a responder."""
  def __init__(self,
               device,
               universe,
               uid,
               pid_store,
               rdm_api,
               wrapper,
               check_for_queued_support=False):
    super(ResponderTestFixture, self).__init__(device, uid, pid_store)
    self._api = rdm_api
    self._expected_results = []
    self._in_reset_mode = False
    self._should_run_wrapper = True
    self._universe = universe
    self._wrapper = wrapper
    self._check_for_queued_support = check_for_queued_support

  def Run(self):
    """Call the test method and then start running the loop wrapper."""
    # the super call invokes self.Test()
    super(ResponderTestFixture, self).Run()

    if self.state == TestState.BROKEN:
      return

    if self._should_run_wrapper:
      self._wrapper.Run()
    self._in_reset_mode = True
    self.ResetState()
    self._wrapper.Reset()

  def VerifyResult(self, status, fields):
    """A hook to perform additional verification of data."""
    pass

  def ResetState(self):
    pass

  def Stop(self):
    self._should_run_wrapper = False
    self._wrapper.Stop()

  def AddExpectedResults(self, results):
    """Add a set of expected results."""
    if isinstance(results, list):
      self._expected_results = results
    else:
      self._expected_results = [results]

  def NackGetResult(self, nack_reason, **kwargs):
    """A helper method which returns a NackGetResult for the current PID."""
    return NackGetResult(self.pid.value, nack_reason, **kwargs)

  def NackSetResult(self, nack_reason, **kwargs):
    """A helper method which returns a NackSetResult for the current PID."""
    return NackSetResult(self.pid.value, nack_reason, **kwargs)

  def AckGetResult(self, **kwargs):
    """A helper method which returns an AckGetResult for the current PID."""
    return AckGetResult(self.pid.value, **kwargs)

  def AckSetResult(self, **kwargs):
    """A helper method which returns an AckSetResult for the current PID."""
    return AckSetResult(self.pid.value, **kwargs)

  def SendGet(self, sub_device, pid, args = []):
    """Send a GET request using the RDM API.

    Args:
      sub_device: The sub device
      pid: A PID object
      args: A list of arguments
    """
    logging.debug(' GET: pid = %s, sub device = %d, args = %s' %
        (pid, sub_device, args))
    return self._api.Get(self._universe,
                         self._uid,
                         sub_device,
                         pid,
                         self._BuildResponseHandler(sub_device),
                         args)

  def SendRawGet(self, sub_device, pid, data = ""):
    """Send a raw GET request.

    Args:
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    logging.debug(' GET: pid = %s, sub device = %d, data = %s' %
        (pid, sub_device, data))
    return self._api.RawGet(self._universe,
                            self._uid,
                            sub_device,
                            pid,
                            self._BuildResponseHandler(sub_device),
                            data)

  def SendSet(self, sub_device, pid, args = []):
    """Send a SET request using the RDM API.

    Args:
      sub_device: The sub device
      pid: A PID object
      args: A list of arguments
    """
    logging.debug(' SET: pid = %s, sub device = %d, args = %s' %
        (pid, sub_device, args))
    return self._api.Set(self._universe,
                         self._uid,
                         sub_device,
                         pid,
                         self._BuildResponseHandler(sub_device),
                         args)

  def SendRawSet(self, sub_device, pid, data = ""):
    """Send a raw SET request.

    Args:
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    logging.debug(' SET: pid = %s, sub device = %d, data = %s' %
        (pid, sub_device, data))
    return self._api.RawSet(self._universe,
                            self._uid,
                            sub_device,
                            pid,
                            self._BuildResponseHandler(sub_device),
                            data)

  def _HandleResponse(self,
                      sub_device,
                      status,
                      command_class,
                      pid,
                      fields,
                      unpack_exception):
    """Handle a RDM response.

    Args:
      sub_device: the sub device this was for
      status: A RDMRequestStatus object
      command_class: RDM_GET or RDM_SET
      pid: The pid in the response
      obj: A dict of fields
    """
    if not self._CheckState(sub_device, status, pid, fields, unpack_exception):
      return

    if self._in_reset_mode:
      self._wrapper.Stop()
      return

    for result in self._expected_results:
      if result.Matches(status, command_class, pid, fields):
        self._state = TestState.PASSED
        self._status = status
        self.VerifyResult(status, fields)
        if result.warning is not None:
          self.AddWarning(result.warning)
        if result.advisory is not None:
          self.AddAdvisory(result.advisory)
        if result.action is not None:
          result.action()
        else:
          self._wrapper.Stop()

        return

    # nothing matched
    self.SetFailed('expected one of:')

    for result in self._expected_results:
      logging.debug('  %s' % result)
    self.Stop()

  def _HandleQueuedResponse(self,
                            sub_device,
                            command_class,
                            target_pid,
                            status,
                            pid,
                            fields,
                            unpack_exception):
    if not self._CheckState(sub_device, status, pid, fields, unpack_exception):
      return

    if status.response_code != OlaClient.RDM_COMPLETED_OK:
      self.SetFailed(' Get Queued message returned: %s' %
                     status.ResponseCodeAsString())
      self.Stop()
      return

    # handle nacks
    if status.response_type == OlaClient.RDM_NACK_REASON:
      queued_message_pid = self.LookupPid('QUEUED_MESSAGE')
      if pid == queued_message_pid.value:
        # a nack for this is is fatal
        self.SetFailed(' Get Queued message returned nack: %s' %
                       status.nack_reason)
      elif pid != target_pid:
        # this is a nack for something else
        self._HandleResponse(sub_device, status, command_class, pid, fields,
                             unpack_exception)
        self._GetQueuedMessage(sub_device, target_pid)
      return

    # at this point we just have RDM_ACKs left
    logging.debug('pid 0x%hx' % pid)
    logging.debug(fields)
    if (fields.get('messages', None) == []):
      # this means we've run out of messages
      self.Stop()
    else:
      self._HandleResponse(sub_device, status, command_class, pid, fields,
                           unpack_exception)
      # fetch the next one
      self._GetQueuedMessage(sub_device, target_pid)

  def _CheckState(self, sub_device, status, pid, fields, unpack_exception):
    """Check the state of a RDM response.

    Returns:
      True if processing should continue, False if there was a transport error
      or a ACK_TIMER was received.
    """
    if not status.Succeeded():
      # this indicates a transport error
      self.SetBroken(' Error: %s' % status.message)
      self.Stop()
      return False

    # handle the case of an ack timer
    if (status.response_code == OlaClient.RDM_COMPLETED_OK and
        status.response_type == OlaClient.RDM_ACK_TIMER):
      logging.debug('Got ACK TIMER set to %d ms' % status.ack_timer)
      if (self._check_for_queued_support and
          not self.Property('supports_queued_messages')):
        # TODO(simon): this is wrong because we could have proxies on the line
        self.SetFailed(
            'ACK_TIMER received but device doesn\'t support QUEUED_MESSAGE')
        self.Stop()
      else:
        # mark as failed, if we get a message that matches we'll set it to PASSED
        self.SetFailed('Queued Messages failed to return the expected message')
        self._wrapper.AddEvent(
            status.ack_timer,
            lambda: self._GetQueuedMessage(sub_device, pid))
      return False

    if status.WasSuccessfull():
      logging.debug(' Response: %s, PID = 0x%04hx, data = %s' %
                    (status, pid, fields))
    else:
      if unpack_exception:
        logging.debug(' Response: %s, PID = 0x%04hx, Error: %s' %
                      (status, pid, unpack_exception))
      else:
        logging.debug(' Response: %s, PID = 0x%04hx' % (status, pid))

    return True

  def _GetQueuedMessage(self, sub_device, target_pid):
    """Fetch queued messages.

    Args:
      sub_device: The sub device to fetch messages for
      target_pid: The pid to look for in the response.
    """
    queued_message_pid = self.LookupPid('QUEUED_MESSAGE')
    data = ['error']
    logging.debug(' GET: pid = %s, sub device = %d, data = %s' %
        (queued_message_pid, sub_device, data))

    def QueuedResponseHandler(status,
                              command_class,
                              pid,
                              fields,
                              unpack_exception):
      self._HandleQueuedResponse(sub_device,
                                 command_class,
                                 target_pid,
                                 status,
                                 pid,
                                 fields,
                                 unpack_exception)
    return self._api.Get(self._universe,
                         self._uid,
                         sub_device,
                         queued_message_pid,
                         QueuedResponseHandler,
                         data)

  def _BuildResponseHandler(self, sub_device):
    return lambda s, c, p, f, e: self._HandleResponse(sub_device,
                                                      s, c, p, f, e)


class QueuedMessageTestFixture(ResponderTestFixture):
  """A test that depends on supports_queued_messages.

  All tests that communicate with a responder should inherit from this test.
  This ensures that QueuedMessageTest runs before anything else which allows us
  to know if ACK_TIMER responses are valid (If a device ever returns an
  ACK_TIMER it MUST support queued messages).
  """
  def __init__(self, device, universe, uid, pid_store, rdm_api, wrapper):
    super(QueuedMessageTestFixture, self).__init__(device,
                                  universe,
                                  uid,
                                  pid_store,
                                  rdm_api,
                                  wrapper,
                                  check_for_queued_support=True)

  def Requires(self):
    return (super(QueuedMessageTestFixture, self).Requires() + ['supports_queued_messages'])


class OptionalParameterTestFixture(QueuedMessageTestFixture):
  """A sub class of ResponderTestFixture that alters behaviour if the PID isn't
     supported.
  """
  def Requires(self):
    return (super(OptionalParameterTestFixture, self).Requires() +
            ['supported_parameters'])

  def PidSupported(self):
    return self.pid.value in self.Property('supported_parameters')

  def AddIfGetSupported(self, result):
    if not self.PidSupported():
      result = self.NackGetResult(RDMNack.NR_UNKNOWN_PID)
    self.AddExpectedResults(result)

  def AddIfSetSupported(self, result):
    if not self.PidSupported():
      result = self.NackSetResult(RDMNack.NR_UNKNOWN_PID)
    self.AddExpectedResults(result)
