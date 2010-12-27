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
      print 'response code mis match %s' % status.ResponseCodeAsString()
      return False

    if status.response_code != OlaClient.RDM_COMPLETED_OK:
      # for anything other than OK, this is all the checking we do
      return True

    if status.response_type != self._response_type:
      return False

    if pid != self._pid:
      print 'pid mismatch'
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
        fields_keys = set(item.keys())
        for field in self._field_names:
          if field not in field_keys:
            print 'missing %s' % field
            return False
    else:
      field_keys = set(fields.keys())
      for field in self._field_names:
        if field not in field_keys:
          print 'missing %s' % field
          return False

    for field, value in self._field_values.iteritems():
      if field not in fields:
        print '%s missing' % field
        return False
      if value != fields[field]:
        print 'Field %s, %s != %s' % (field, value, fields[field])
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


class ResponderTest(object):
  """The base responder test class, every test inherits from this."""
  PASSED, FAILED, BROKEN, NOT_RUN = range(4)

  STATE_TO_STRING = {
    PASSED: 'Passed',
    FAILED: 'Failed',
    BROKEN: 'Broken',
    NOT_RUN: 'Not Run',
  }

  DEPS = []

  def __init__(self, universe, uid, pid_store, rdm_api, wrapper, deps):
    self._universe = universe;
    self._uid = uid
    self._pid_store = pid_store
    self._api = rdm_api
    self._wrapper = wrapper
    self._status = None
    self._fields = None
    self._expected_results = []
    self._state = None
    self.pid = self.GetPid(self.PID)
    self._deps = {}
    self._error = None
    self._warnings = []

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

  @property
  def error_message(self):
    """An error string if the test failed."""
    return self._error

  @property
  def warnings(self):
    """Non-fatal warnings."""
    return self._warnings

  def GetPid(self, pid_name):
    """Return the pid object that this test case uses.

    Args:
      pid_name: The name of this pid in the PID Store

    Returns:
      A instance of a PID object or None if not found.
    """
    return self._pid_store.GetName(pid_name, self._uid)

  # Sub class can override these
  def test(self):
    self.SetBroken('test method not defined')

  def PreCondition(self):
    """Return True if this test should run."""
    return True

  def VerifyResult(self, status, fields):
    """A hook to perform additional verification of data."""
    pass

  def Run(self):
    """Run the test if all deps passed and the pre condition is true."""
    for dep in self._deps.values():
      if dep.State()[0] != self.PASSED:
        self._state = self.NOT_RUN
        return

    if not self.PreCondition():
      self._state = self.NOT_RUN
      return

    self.Test()
    self._wrapper.Run()
    self._wrapper.Reset()

  def State(self):
    """Returns the state of the test.

    Returns:
      PASSED, FAILED, BROKEN or None
    """
    return self._state, self.STATE_TO_STRING.get(
        self._state,
        'Unknown was %s' % self._state)

  def Deps(self, dep_class):
    """Lookup an object that this test depends on.

    Args:
      dep_class: The class to lookup.
    """
    return self._deps[dep_class.__name__]

  def GetField(self, field):
    """Return the fields in the response."""
    return self._fields.get(field)

  def SetNotRun(self):
    self._state = self.NOT_RUN

  def SetBroken(self, message):
    self._error = message
    self._state = self.BROKEN

  def SetFailed(self, message):
    self._error = message
    self._state = self.FAILED

  def Stop(self):
    self._wrapper.Stop()

  def AddWarning(self, message):
    self._warnings.append(message)

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
    return self._api.Get(self._universe,
                         self._uid,
                         sub_device,
                         pid,
                         self._HandleResponse,
                         args)

  def SendRawGet(self, sub_device, pid, data = ""):
    """Send a raw GET request.

    Args:
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    return wrapper.Client().RDMGet(
        self._universe,
        self._uid,
        sub_device,
        pid,
        self._HandleRawResponse,
        data)

  def SendSet(self, sub_device, pid, args = []):
    """Send a SET request using the RDM API.

    Args:
      sub_device: The sub device
      pid: A PID object
      args: A list of arguments
    """
    return self._api.Set(self._universe,
                         self._uid,
                         sub_device,
                         pid,
                         self._HandleResponse,
                         args)

  def SendRawSet(self, sub_device, pid, data = ""):
    """Send a raw SET request.

    Args:
      sub_device: The sub device
      pid: The pid value
      data: The param data
    """
    return self._wrapper.Client().RDMSet(
        self._universe,
        self._uid,
        sub_device,
        pid,
        self._HandleRawResponse,
        data)

  def _HandleResponse(self, status, pid, fields):
    """Handle a RDM response."""
    if not self._CheckState(status):
      return

    for result in self._expected_results:
      if result.Matches(status, pid, fields):
        self._state = self.PASSED
        self._status = status
        self._fields = fields
        self.VerifyResult(status, fields)
        if result.action is not None:
          result.action()
        else:
          self._wrapper.Stop()

        return

    # nothing matched
    print "nothing matched"
    self.SetFailed('nothing matched')
    self.Stop()

  def _HandleRawResponse(self, status, pid, data, unused_raw_data):
    """Handle a raw RDM response."""
    if not self._CheckState(status):
      return

    for result in self._expected_results:
      if result.Matches(status, pid, {}):
        self._state = self.PASSED
        if result.action is not None:
          result.action()
        else:
          self._wrapper.Stop()
        return

    # nothing matched
    print "nothing matched"
    print status.response_type
    print status.nack_reason
    self.SetFailed('nothing matched')
    self.Stop()

  def _CheckState(self, status):
    """Check the state of a RDM response."""
    if not status.Succeeded():
      # this indicated a transport error
      self.SetBroken('transport error')
      self.Stop()
      return False

    # handle the case of an ack timer
    if (status.response_code == OlaClient.RDM_COMPLETED_OK and
        status.response_type == OlaClient.RDM_ACK_TIMER):
      print 'Got ACK TIMER set to %d ms' % status.ack_timer
      # TODO: schedule queued message fetch

      return False
    return True


class TestRunner(object):
  """The Test Runner executes the tests."""
  def __init__(self, universe, uid, pid_store, rdm_api, wrapper, test_logger):
    self._universe = universe;
    self._uid = uid
    self._pid_store = pid_store
    self._api = rdm_api
    self._wrapper = wrapper
    self._logger = test_logger
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
    test_obj = self._AddTest(test)
    return test_obj is not None

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

    logging.debug('Adding %s' % test.__name__)
    self._class_name_to_object[test] = None

    new_parents = parents + [test]
    dep_objects = []
    logging.debug(' Deps are %s' % test.DEPS)
    for dep_class in test.DEPS:
      if dep_class in parents:
        logging.error('Circular depdendancy found %s' % parents)
        return None
      obj = self._AddTest(dep_class, new_parents)
      if obj is None:
        return None
      dep_objects.append(obj)

    logging.debug('Creating %s with dep objects %s' % (test.__name__, dep_objects))
    test_obj = test(self._universe,
                    self._uid,
                    self._pid_store,
                    self._api,
                    self._wrapper,
                    dep_objects)
    self._class_name_to_object[test] = test_obj
    self._deps_tree[test_obj] = set(dep_objects)
    return test_obj

  def RunTests(self):
    """Run all the tests."""
    test_order = self._TopologicalSort(self._deps_tree.copy())
    logging.info('Test order is %s' % test_order)
    for test in test_order:
      test.Run()
      self._tests.append(test)

  def GetTests(self):
    """Returns the list of tests in the order they were executed.

    Returns:
      A list of test objects
    """
    return self._tests

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
