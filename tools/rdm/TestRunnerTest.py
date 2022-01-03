#!/usr/bin/env python
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
# TestRunnerTest.py
# Copyright (C) 2022 Peter Newman

import unittest
from TestRunner import GetTestClasses
from ola.testing.rdm import TestDefinitions

"""Test cases for TestRunner utilities."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class TestRunnerGetTestClasses(unittest.TestCase):
  def testGetTestClasses(self):
    self.assertTrue(len(GetTestClasses(TestDefinitions)) > 0,
                    "Didn't find any test classes")
    self.assertTrue(len(GetTestClasses(TestDefinitions)) > 100,
                    "Didn't find a realistic number of test classes")
    # Check for a common test
    self.assertTrue("GetDeviceInfo" in GetTestClasses(TestDefinitions))
    # Check we don't contain the base classes:
    for classname in ["OptionalParameterTestFixture",
                      "ResponderTestFixture",
                      "TestFixture",
                      "ResponderTest.OptionalParameterTestFixture",
                      "ResponderTest.ResponderTestFixture",
                      "ResponderTest.TestFixture"]:
      self.assertTrue(classname not in GetTestClasses(TestDefinitions),
                      "Class %s found in list of test classes" % classname)


if __name__ == '__main__':
  unittest.main()
