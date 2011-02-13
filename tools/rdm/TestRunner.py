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
# TestRunner.py
# Copyright (C) 2011 Simon Newton

__author__ = 'nomis52@gmail.com (Simon Newton)'

import logging
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
    self._universe = universe
    self._uid = uid
    self._pid_store = pid_store
    self._api = RDMAPI(wrapper.Client(), pid_store, strict_checks=False)
    self._wrapper = wrapper

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
        raise DuplicatePropertyException(
            '%s is declared in more than one test' % property)
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
    device = DeviceProperties(self._property_map.keys())
    if filter is None:
      tests_to_run = self._all_tests
    else:
      tests_to_run = [test for test in self._all_tests
                      if test.__name__ in filter]

    deps_map = self._InstantiateTests(device, tests_to_run)
    tests = self._TopologicalSort(deps_map)

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

    Returns:
      A dict mapping each test object to the set of test objects it depends on.
    """
    class_name_to_object = {}
    deps_map = {}
    for test_class in tests_to_run:
      self._AddTest(device, class_name_to_object, deps_map, test_class)
    return deps_map

  def _AddTest(self, device, class_name_to_object, deps_map, test_class,
               parents = []):
    """Add a test class, recursively adding all REQUIRES.
       This also checks for circular dependancies.

    Args:
      device: A DeviceProperties object which is passed to each test.
      class_name_to_object: A dict of class names to objects.
      deps_map: A dict mapping each test object to the set of test objects it
        depends on.
      test_class: A class which sub classes ResponderTest.
      parents: The parents for the current class.

    Returns:
      An instance of the test class.
    """
    if test_class in class_name_to_object:
      return class_name_to_object[test_class]

    class_name_to_object[test_class] = None
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
      obj = self._AddTest(device,
                          class_name_to_object,
                          deps_map,
                          dep_class,
                          new_parents)
      dep_objects.append(obj)

    class_name_to_object[test_class] = test_obj
    deps_map[test_obj] = set(dep_objects)
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
