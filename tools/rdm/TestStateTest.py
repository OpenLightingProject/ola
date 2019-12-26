#!/usr/bin/env python
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
# TestStateTest.py

import itertools
import unittest
from TestState import TestState

"""Test cases for sorting TestState."""

__author__ = 'bruce@lowekamp.net (Bruce Lowekamp)'


class TestStateCmpTest(unittest.TestCase):
  def allNotEqual(self, t):
    for pair in itertools.combinations(t, 2):
      self.assertNotEqual(pair[0], pair[1])

  def allHashNotEqual(self, t):
    h = map(hash, t)
    for pair in itertools.combinations(h, 2):
      self.assertNotEqual(pair[0], pair[1])

  def testCmp(self):
    self.assertEqual(TestState.PASSED, TestState.PASSED)

    states = sorted([TestState.PASSED, TestState.FAILED,
                     TestState.BROKEN, TestState.NOT_RUN])
    self.assertEqual(states,
                              [TestState.BROKEN, TestState.FAILED,
                               TestState.NOT_RUN, TestState.PASSED])
    self.allNotEqual(states)
    self.allHashNotEqual(states)


if __name__ == '__main__':
  unittest.main()
