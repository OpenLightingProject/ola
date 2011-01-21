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

from ola import PidStore
from ola.OlaClient import OlaClient, RDMNack
from ola.RDMAPI import RDMAPI


class ExpectedResult(object):
  """Create an expected result object.

  Args:
    pid: The pid we expect
    response_code: The OLA RDM response code we expect
    response_type: The RDM response type we expect
    nack_reason: The nack reason we expect if response_type is NACK
    action: Run this action on match
    field_names: Check that these fields are present in the response
    field_dict: Check that fields & values are present in the response
  """
  def __init__(self,
               pid,
               response_code,
               response_type = None,
               nack_reason = None,
               field_names = None,
               field_values = None,
               action = None):
    self._pid = pid
    self._response_code = response_code
    self._response_type = response_type
    self._nack_reason = nack_reason
    self._field_names = field_names
    self._field_values = field_values
    self._action = action

  @property
  def action(self):
    return self._action

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
  def NackResponse(pid, nack_reason, action = None):
    nack = RDMNack(nack_reason)
    return ExpectedResult(pid,
                          OlaClient.RDM_COMPLETED_OK,
                          OlaClient.RDM_NACK_REASON,
                          nack_reason = nack,
                          action = action)

  @staticmethod
  def BroadcastResponse(pid, nack_reason):
    return ExpectedResult(pid, OlaClient.RDM_WAS_BROADCAST)

  @staticmethod
  def AckResponse(pid, field_names = [], field_values = {}, action = None):
    return ExpectedResult(pid,
                          OlaClient.RDM_COMPLETED_OK,
                          OlaClient.RDM_ACK,
                          field_names = field_names,
                          field_values = field_values,
                          action = action)


class TestCategory(object):
  """The category a test is part of."""
  def __init__(self, category):
    self._category = category

  def __str__(self):
    return self._category

  def __hash__(self):
    return hash(self._category)


# These correspond to categories in the E1.20 document
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


class ResponderTest(object):
  """The base responder test class, every test inherits from this."""
  DEPS = []
  CATEGORY = TestCategory.UNCLASSIFIED

  def __init__(self, universe, uid, pid_store, rdm_api, wrapper, logger, deps):
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
    self._deps = {}
    self._warnings = []
    self._advisories = []
    self._logger = logger
    self._should_run_wrapper = True

    for dep in deps:
      self._deps[dep.__class__.__name__] = dep

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
    self._logger.debug('Warning: %s' % message)
    self._warnings.append(message)

  # Advisories are logged independently of errors. They should be used to
  # indicate conditions that while aren't covered by the standard, should still
  # be fixed.
  @property
  def advisories(self):
    """Non-fatal advisories message."""
    return self._advisories

  def AddAdvisory(self, message):
    self._logger.debug('Advisory: %s' % message)
    self._advisories.append(message)

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

  def PreCondition(self):
    """Return True if this test should run."""
    return True

  def VerifyResult(self, status, fields):
    """A hook to perform additional verification of data."""
    pass

  def Run(self):
    """Run the test if all deps passed and the pre condition is true."""
    self._logger.debug('%s: %s' % (self, self.__doc__))
    for dep in self._deps.values():
      if dep.state != TestState.PASSED:
        self._state = TestState.NOT_RUN
        self._logger.debug(' Dep: %s %s, not running' % (dep, dep.state))
        return

    if not self.PreCondition():
      self._logger.debug(' Preconditions not met, test will not be run')
      self._state = TestState.NOT_RUN
      return

    self.Test()
    if self._should_run_wrapper:
      self._wrapper.Run()
    self._wrapper.Reset()

  def Deps(self, dep_class):
    """Lookup an object that this test depends on.

    Args:
      dep_class: The class to lookup.
    """
    return self._deps[dep_class.__name__]

  def GetField(self, field):
    """Return the fields in the response."""
    if self._fields is not None:
      return self._fields.get(field)
    return None

  def SetNotRun(self):
    self._state = TestState.NOT_RUN

  def SetBroken(self, message):
    self._logger.debug(' Broken: %s' % message)
    self._state = TestState.BROKEN

  def SetFailed(self, message):
    self._logger.debug(' Failed: %s' % message)
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
    self._logger.debug(' GET: pid = %s, sub device = %d, args = %s' %
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
    self._logger.debug(' GET: pid = %s, sub device = %d, data = %s' %
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
    self._logger.debug(' SET: pid = %s, sub device = %d, args = %s' %
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
    self._logger.debug(' SET: pid = %s, sub device = %d, data = %s' %
        (pid, sub_device, data))
    return self._api.RawSet(self._universe,
                            self._uid,
                            sub_device,
                            pid,
                            self._BuildResponseHandler(sub_device),
                            data)

  def _HandleResponse(self, sub_device, status, pid, fields, unpack_exception):
    """Handle a RDM response.

    Args:
      sub_device: the sub device this was for
      status: A RDMRequestStatus object
      pid: The pid in the response
      obj: A dict of fields
    """
    if not self._CheckState(sub_device, status, pid, fields, unpack_exception):
      return

    for result in self._expected_results:
      if result.Matches(status, pid, fields):
        self._state = TestState.PASSED
        self._status = status
        self._fields = fields
        self.VerifyResult(status, fields)
        if result.action is not None:
          result.action()
        else:
          self._wrapper.Stop()

        return

    # nothing matched
    self.SetFailed('expected one of:')

    for result in self._expected_results:
      self._logger.debug('  %s' % result)
    self.Stop()

  def _HandleQueuedResponse(self, sub_device, status, pid, fields,
                            unpack_exception):
    if not self._CheckState(sub_device, status, pid, fields, unpack_exception):
      return

    if fields['messages'] == []:
      # this means we've run out of messages
      self.Stop()
    else:
      self._HandleResponse(sub_device, status, pid, fields, unpack_exception)
      # fetch the next one
      self._GetQueuedMessage(sub_device)

  def _CheckState(self, sub_device, status, pid, fields, unpack_exception):
    """Check the state of a RDM response."""
    if not status.Succeeded():
      # this indicated a transport error
      self.SetBroken(' Error: %s' % status.message)
      self.Stop()
      return False

    # handle the case of an ack timer
    if (status.response_code == OlaClient.RDM_COMPLETED_OK and
        status.response_type == OlaClient.RDM_ACK_TIMER):
      self._logger.debug('Got ACK TIMER set to %d ms' % status.ack_timer)
      # mark as failed, if we get a message that matches we'll set it to PASSED
      self.SetFailed('Queued Messages failed to return the expected message')
      self._wrapper.AddEvent(
          status.ack_timer,
          lambda: self._GetQueuedMessage(sub_device))

      return False

    if status.WasSuccessfull():
      self._logger.debug(' Response: %s, PID = 0x%04hx, data = %s' %
                         (status, pid, fields))
    else:
      if unpack_exception:
        self._logger.debug(' Response: %s, PID = 0x%04hx, Error: %s' %
                           (status, pid, unpack_exception))
      else:
        self._logger.debug(' Response: %s, PID = 0x%04hx' % (status, pid))

    return True

  def _GetQueuedMessage(self, sub_device):
    """Fetch queued messages."""
    pid = self.LookupPid('QUEUED_MESSAGE')
    data = ['error']
    self._logger.debug(' GET: pid = %s, sub device = %d, data = %s' %
        (pid, sub_device, data))

    def QueuedResponseHandler(status,
                              pid,
                              fields,
                              unpack_exception):
      self._HandleQueuedResponse(sub_device,
                                 status,
                                 pid,
                                 fields,
                                 unpack_exception)
    return self._api.Get(self._universe,
                         self._uid,
                         sub_device,
                         pid,
                         QueuedResponseHandler,
                         data)

  def _BuildResponseHandler(self, sub_device):
    return lambda s, p, f, e: self._HandleResponse(sub_device,
                                                   s, p, f, e)


class TestRunner(object):
  """The Test Runner executes the tests."""
  def __init__(self, universe, uid, pid_store, wrapper, results_logger, tests):
    self._universe = universe;
    self._uid = uid
    self._pid_store = pid_store
    self._api = RDMAPI(wrapper.Client(), pid_store, strict_checks=False)
    self._wrapper = wrapper
    self._logger = results_logger
    self._tests_filter = tests
    # A dict of class name to object mappings
    self._class_name_to_object = {}
    # dict in the form test_obj: first_order_deps
    self._deps_tree = {}
    self._tests = []

  def AddTest(self, test):
    """Add a test to be run.

    Args:
      test: A child class of ResponderTest.

    Returns:
      True if added, False if there was an error.
    """
    if (self._tests_filter is not None and
        test.__name__ not in self._tests_filter):
      return True

    test_obj = self._AddTest(test)
    return test_obj is not None

  def RunTests(self):
    """Run all the tests."""
    test_order = self._TopologicalSort(self._deps_tree.copy())
    self._logger.debug('Test order is %s' % test_order)
    for test in test_order:
      self._tests.append(test)
      test.Run()
      self._logger.info('%s: %s' % (test, test.state.ColorString()))

  @property
  def all_tests(self):
    """Returns the list of tests in the order they were executed.

    Returns:
      A list of test objects
    """
    return self._tests

  def _AddTest(self, test, parents = []):
    """Add a test class, recursively adding all deps and checking for circular
    dependancies.

    Args:
      test: A class which sub classes ResponderTest.

    Returns:
      An instance of the test class or None if an error occured.
    """
    if test in self._class_name_to_object:
      return self._class_name_to_object[test]

    self._class_name_to_object[test] = None

    new_parents = parents + [test]
    dep_objects = []
    for dep_class in test.DEPS:
      if dep_class in new_parents:
        self._logger.error('Circular depdendancy found %s in %s' %
                           (dep_class, new_parents))
        return None
      obj = self._AddTest(dep_class, new_parents)
      if obj is None:
        return None
      dep_objects.append(obj)

    test_obj = test(self._universe,
                    self._uid,
                    self._pid_store,
                    self._api,
                    self._wrapper,
                    self._logger,
                    dep_objects)
    self._class_name_to_object[test] = test_obj
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
