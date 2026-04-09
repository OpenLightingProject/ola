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
# TestStateTest.py
# Copyright (C) 2019 Bruce Lowekamp

import unittest

from ola.TestUtils import allHashNotEqual, allNotEqual
# Keep this import relative to simplify the testing
from TestState import TestState

"""Test cases for sorting TestState."""

__author__ = 'bruce@lowekamp.net (Bruce Lowekamp)'


class TestStateCmpTest(unittest.TestCase):
  def testCmp(self):
    self.assertEqual(TestState.PASSED, TestState.PASSED)

    states = sorted([TestState.PASSED, TestState.FAILED,
                     TestState.BROKEN, TestState.NOT_RUN])
    self.assertEqual(states,
                     [TestState.BROKEN, TestState.FAILED,
                      TestState.NOT_RUN, TestState.PASSED])
    allNotEqual(self, states)
    allHashNotEqual(self, states)


if __name__ == '__main__':
  unittest.main()
