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
# ResponderTest.py
# Copyright (C) 2010 Simon Newton

'''Automated testing for RDM responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'

import logging
from ola import PidStore
from ola.OlaClient import OlaClient, RDMNack
from ola.RDMAPI import RDMAPI


class Error(Exception):
  """The base error class."""


class DuplicatePropertyException(Error):
  """Raised if a property is declared in more than one test."""


class MissingPropertyException(Error):
  """Raised if a property was listed in a REQUIRES list but it didn't appear in
    any PROVIDES list.
  """


class CircularDepdendancyException(Error):
  """Raised if there is a circular depdendancy created by PROVIDES &
     REQUIRES statements.
  """


class UndeclaredPropertyException(Error):
  """Raised if a test attempts to get/set a property that it didn't declare."""


class ExpectedResult(object):
  """Create an expected result object.

  Args:
    pid_id: The pid id we expect
    response_code: The OLA RDM response code we expect
    response_type: The RDM response type we expect
    nack_reason: The nack reason we expect if response_type is NACK
    field_names: Check that these fields are present in the response
    field_dict: Check that fields & values are present in the response
    action: Run this action on match
    warning_message: Generates a warning message if this matches
    advisory_message: Generates an advisory messaeg if this matches
  """
  def __init__(self,
               pid_id,
               response_code,
               response_type = None,
               nack_reason = None,
               field_names = None,
               field_values = None,
               action = None,
               warning = None,
               advisory = None):
    self._pid = pid_id
    self._response_code = response_code
    self._response_type = response_type
    self._nack_reason = nack_reason
    self._field_names = field_names
    self._field_values = field_values
    self._action = action
    self._warning_messae = warning
    self._advisory_message = advisory

  @property
  def action(self):
    return self._action

  @property
  def warning(self):
    return self._warning_messae

  @property
  def advisory(self):
    return self._advisory_message

  def __str__(self):
    """Represent this object as a string for logging."""
    if self._response_code != OlaClient.RDM_COMPLETED_OK:
      return str(self._response_code)

    if self._response_type == OlaClient.RDM_ACK:
      return 'Pid 0x%04hx, ACK, fields %s, values %s' % (
          self._pid, self._field_names, self._field_values)
    elif self._response_type == OlaClient.RDM_ACK_TIMER:
      return 'Pid 0x%04hx, ACK TIMER' % self._pid
    else:
      return 'Pid 0x%04hx, NACK %s' % (self._pid, self._nack_reason)

  def Matches(self, status, pid, fields):
    """Check if the response we receieved matches this object.

    Args:
      status: An RDMRequestStatus object
      pid: The pid that was returned, or None if we didn't get a valid
        response.
      fields: A dict of field name : value mappings that were present in the
        response.
    """
    if self._response_code != status.response_code:
      return False

    if status.response_code != OlaClient.RDM_COMPLETED_OK:
      # for anything other than OK, this is all the checking we do
      return True

    if status.response_type != self._response_type:
      return False

    if pid != self._pid:
      return False

    if status.response_type == OlaClient.RDM_NACK_REASON:
      return status.nack_reason == self._nack_reason
    elif status.response_type == OlaClient.RDM_ACK_TIMER:
      print 'Got unexpected ack timer'
      return False

    # At this stage we know we got an ACK
    # fields may be either a list of dicts, or a dict
    if isinstance(fields, list):
      for item in fields:
        field_keys = set(item.keys())
        for field in self._field_names:
          if field not in field_keys:
            return False
    else:
      field_keys = set(fields.keys())
      for field in self._field_names:
        if field not in field_keys:
          return False

    for field, value in self._field_values.iteritems():
      if field not in fields:
        return False
      if value != fields[field]:
        return False
    return True

  # Helper methods to create expected responses
  @staticmethod
  def NackResponse(pid_id,
                   nack_reason,
                   action = None,
                   warning = None,
                   advisory = None):
    nack = RDMNack(nack_reason)
    return ExpectedResult(pid_id,
                          OlaClient.RDM_COMPLETED_OK,
                          OlaClient.RDM_NACK_REASON,
                          nack_reason = nack,
                          action = action,
                          warning = warning,
                          advisory = advisory)

  @staticmethod
  def BroadcastResponse(pid, nack_reason):
    return ExpectedResult(pid, OlaClient.RDM_WAS_BROADCAST)

  @staticmethod
  def AckResponse(pid_id,
                  field_names = [],
                  field_values = {},
                  action = None,
                  warning = None,
                  advisory = None):
    return ExpectedResult(pid_id,
                          OlaClient.RDM_COMPLETED_OK,
                          OlaClient.RDM_ACK,
                          field_names = field_names,
                          field_values = field_values,
                          action = action,
                          warning = warning,
                          advisory = advisory)


class TestCategory(object):
  """The category a test is part of."""
  def __init__(self, category):
    self._category = category

  def __str__(self):
    return self._category

  def __hash__(self):
    return hash(self._category)


# These correspond to categories in the E1.20 document
TestCategory.STATUS_COLLECTION = TestCategory('Status Collection')
TestCategory.RDM_INFORMATION = TestCategory('RDM Information')
TestCategory.PRODUCT_INFORMATION = TestCategory('Product Information')
TestCategory.DMX_SETUP = TestCategory('DMX512 Setup')
TestCategory.SENSORS = TestCategory('Sensors')
TestCategory.POWER_LAMP_SETTINGS = TestCategory('Power / Lamp Settings')
TestCategory.DISPLAY_SETTINGS = TestCategory('Display Settings')
TestCategory.CONFIGURATION = TestCategory('Configuration')
TestCategory.CONTROL = TestCategory('Control')

# And others for things that don't quite fit
TestCategory.CORE = TestCategory('Core Functionality')
TestCategory.ERROR_CONDITIONS = TestCategory('Error Conditions')
TestCategory.SUB_DEVICES = TestCategory('Sub Devices')
TestCategory.UNCLASSIFIED = TestCategory('Unclassified')


class TestState(object):
  """Represents the state of a test."""
  def __init__(self, state):
    self._state = state

  def __str__(self):
    return self._state

  def __cmp__(self, other):
    return cmp(self._state, other._state)

  def __hash__(self):
    return hash(self._state)

  def ColorString(self):
    strs = []
    if self == TestState.PASSED:
      strs.append('\x1b[32m')
    elif self == TestState.FAILED:
      strs.append('\x1b[31m')

    strs.append(str(self._state))
    strs.append('\x1b[0m')
    return ''.join(strs)

TestState.PASSED = TestState('Passed')
TestState.FAILED = TestState('Failed')
TestState.BROKEN = TestState('Broken')
TestState.NOT_RUN = TestState('Not Run')


class DeviceProperties(object):
  """Encapsulates the properties of a device."""
  def __init__(self, property_names):
    object.__setattr__(self, '_property_names', property_names)
    object.__setattr__(self, '_properties', {})

  def __str__(self):
    return str(self._properties)

  def __repr__(self):
    return self._properties

  def __getattr__(self, property):
    if property not in self._properties:
      raise AttributeError(property)
    return self._properties[property]

  def __setattr__(self, property, value):
    if property in self._properties:
      logging.warning('Multiple sets of property %s' % property)
    self._properties[property] = value


class ResponderTest(object):
  """The base responder test class, every test inherits from this."""
  CATEGORY = TestCategory.UNCLASSIFIED
  DEPS = []
  PROVIDES = []
  REQUIRES = []

  def __init__(self, device, universe, uid, pid_store, rdm_api, wrapper):
    self._device_properties = device
    self._universe = universe;
    self._uid = uid
    self._pid_store = pid_store
    self._api = rdm_api
    self._wrapper = wrapper
    self._status = None
    self._fields = None
    self._expected_results = []
    self._state = None
    self.pid = self.LookupPid(self.PID)
    self._warnings = []
    self._advisories = []
    self._should_run_wrapper = True
    self._in_reset_mode = False

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

  def AckResponse(self,
                  field_names = [],
                  field_values = {},
                  action = None,
                  warning = None,
                  advisory = None):
    """A helper method which returns a ACK ExpectedResult.

    Args:
      field_names: A list of fields we expect to see in the response.
      field_values: A dist of field_name : value mappings that we expect to see
        in the response.
      action: A function to run if this response matches
      warning: A string warning message to log if this response matches.
      advisory: A string advisory message to log if this response matches.

    Returns:
      A ExpectedResult object which expects a ACK for self.PID
    """
    return ExpectedResult.AckResponse(self.pid.value,
                                      field_names,
                                      field_values,
                                      action,
                                      warning,
                                      advisory)

  def NackResponse(self,
                   nack_reason,
                   action=None,
                   warning=None,
                   advisory=None):
    """A helper method which returns a NACK ExpectedResult.

    Args:
      nack_reason: One of the RDMNack codes.
      action: A function to run if this response matches
      warning: A string warning message to log if this response matches.
      advisory: A string advisory message to log if this response matches.

    Returns:
      A ExpectedResult object which expects a NACK for self.PID with a reason
      matching nack_reason.
    """
    nack = RDMNack(nack_reason)
    return ExpectedResult(self.pid.value,
                          OlaClient.RDM_COMPLETED_OK,
                          OlaClient.RDM_NACK_REASON,
                          nack,
                          action,
                          warning,
                          advisory)

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
    self.Stop()

  def ResetState(self):
    pass

  def VerifyResult(self, status, fields):
    """A hook to perform additional verification of data."""
    pass

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
    if self._should_run_wrapper:
      self._wrapper.Run()
    self._in_reset_mode = True
    self.ResetState()
    self._wrapper.Reset()

  def GetField(self, field):
    """Return the fields in the response."""
    if self._fields is not None:
      return self._fields.get(field)
    return None

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

  def Stop(self):
    self._should_run_wrapper = False
    self._wrapper.Stop()

  def AddExpectedResults(self, results):
    """Add a set of expected results."""
    if isinstance(results, list):
      self._expected_results = results
    else:
      self._expected_results = [results]

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

  def _HandleResponse(self, sub_device, status, command_class, pid, fields,
                      unpack_exception):
    """Handle a RDM response.

    Args:
      sub_device: the sub device this was for
      status: A RDMRequestStatus object
      pid: The pid in the response
      obj: A dict of fields
    """
    if not self._CheckState(sub_device, status, pid, fields, unpack_exception):
      return

    if self._in_reset_mode:
      self._wrapper.Stop()
      return

    for result in self._expected_results:
      if result.Matches(status, pid, fields):
        self._state = TestState.PASSED
        self._status = status
        self._fields = fields
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
        self._HandleResponse(sub_device, status, pid, fields, unpack_exception)
        self._GetQueuedMessage(sub_device)
      return

    # at this point we just have RDM_ACKs left
    logging.debug('pid 0x%hx' % pid)
    logging.debug( fields)
    if (fields.get('messages', None) == []):
      # this means we've run out of messages
      self.Stop()
    else:
      self._HandleResponse(sub_device, status, pid, fields, unpack_exception)
      # fetch the next one
      self._GetQueuedMessage(sub_device, pid)

  def _CheckState(self, sub_device, status, pid, fields, unpack_exception):
    """Check the state of a RDM response.

    Returns:
      True if processing should continue, False if there was a transport error
      or a ACK_TIMER was received.
    """
    if not status.Succeeded():
      # this indicated a transport error
      self.SetBroken(' Error: %s' % status.message)
      self.Stop()
      return False

    # handle the case of an ack timer
    if (status.response_code == OlaClient.RDM_COMPLETED_OK and
        status.response_type == OlaClient.RDM_ACK_TIMER):
      logging.debug('Got ACK TIMER set to %d ms' % status.ack_timer)
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
                              pid,
                              fields,
                              unpack_exception):
      self._HandleQueuedResponse(sub_device,
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


class SupportedParamResponderTest(ResponderTest):
  """A sub class of ResponderTest that alters behaviour if the PID isn't
     supported.
  """
  def Requires(self):
    return self.REQUIRES + ['supported_parameters']

  def PidSupported(self):
    return self.pid.value in self.Property('supported_parameters')

  def AddIfSupported(self, result):
    if not self.PidSupported():
      result = self.NackResponse(RDMNack.NR_UNKNOWN_PID)
    self.AddExpectedResults(result)


class TestRunner(object):
  """The Test Runner executes the tests."""
  def __init__(self, universe, uid, pid_store, wrapper):
    """Create a new TestRunner.

    Args:
      universe: The universe number to use
      uid: The UID object to test
      pid_store: A PidStore object
      wrapper: A ClientWrapper object
    """
    self._universe = universe;
    self._uid = uid
    self._pid_store = pid_store
    self._api = RDMAPI(wrapper.Client(), pid_store, strict_checks=False)
    self._wrapper = wrapper
    # A dict of class name to object mappings
    self._class_name_to_object = {}
    # dict in the form test_obj: first_order_deps
    self._deps_tree = {}

    # maps device properties to the tests that provide them
    self._property_map = {}
    self._all_tests = []  # list of all test classes

  def RegisterTest(self, test_class):
    """Register a test.

    This doesn't necessarily mean a test will be run as we may restrict which
    tests are executed.

    Args:
      test: A child class of ResponderTest.
    """
    for property in test_class.PROVIDES:
      if property in self._property_map:
        raise DuplicatePropertyException('%s is declared in more than one test'
                                         % property)
      self._property_map[property] = test_class
    self._all_tests.append(test_class)

  def RunTests(self, filter=None):
    """Run all the tests.

    Args:
      filter: If not None, limit the tests to those in the list and their
        dependancies.

    Returns:
      A tuple in the form (tests, device), where tests is a list of tests that
      exectuted, and device is an instance of DeviceProperties.
    """
    if filter is None:
      tests_to_run = self._all_tests
    else:
      tests_to_run = [test for test in self._all_tests
                      if test.__name__ in filter]

    device = DeviceProperties(self._property_map.keys())

    self._InstantiateTests(device, tests_to_run)
    tests = self._TopologicalSort(self._deps_tree.copy())

    logging.debug('Test order is %s' % tests)
    for test in tests:
      test.Run()
      logging.info('%s: %s' % (test, test.state.ColorString()))
    return tests, device

  def _InstantiateTests(self, device, tests_to_run):
    """Instantiate the required tests and calculate the dependancies.

    Args:
      device: A DeviceProperties object
      tests_to_run: The list of test class names to run
    """
    for test_class in tests_to_run:
      self._AddTest(device, test_class)

  def _AddTest(self, device, test_class, parents = []):
    """Add a test class, recursively adding all REQUIRES.
       This also checks for circular dependancies.

    Args:
      test_class: A class which sub classes ResponderTest.

    Returns:
      An instance of the test class.
    """
    if test_class in self._class_name_to_object:
      return self._class_name_to_object[test_class]

    self._class_name_to_object[test_class] = None
    test_obj = test_class(device,
                          self._universe,
                          self._uid,
                          self._pid_store,
                          self._api,
                          self._wrapper)

    new_parents = parents + [test_class]
    dep_classes = []
    for property in test_obj.Requires():
      if property not in self._property_map:
        raise MissingPropertyException(
            '%s not listed in any PROVIDES list.' % property)
      dep_classes.append(self._property_map[property])
    dep_classes.extend(test_class.DEPS)

    dep_objects = []
    for dep_class in dep_classes:
      if dep_class in new_parents:
        raise CircularDepdendancyException(
            'Circular depdendancy found %s in %s' % (dep_class, new_parents))
      obj = self._AddTest(device, dep_class, new_parents)
      dep_objects.append(obj)

    self._class_name_to_object[test_class] = test_obj
    self._deps_tree[test_obj] = set(dep_objects)
    return test_obj

  def _TopologicalSort(self, deps_dict):
    """Sort the tests according to the dep ordering.

    Args:
      A dict in the form test_name: [deps].
    """
    # The final order to run tests in
    tests = []

    remaining_tests = [
        test for test, deps in deps_dict.iteritems() if len(deps)]
    no_deps = set(
        test for test, deps in deps_dict.iteritems() if len(deps) == 0)

    while len(no_deps) > 0:
      current_test = no_deps.pop()
      tests.append(current_test)

      remove_list = []
      for test in remaining_tests:
        deps_dict[test].discard(current_test)
        if len(deps_dict[test]) == 0:
          no_deps.add(test)
          remove_list.append(test)

      for test in remove_list:
        remaining_tests.remove(test)
    return tests
