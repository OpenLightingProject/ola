#!/usr/bin/python
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
# PidDataTest.py
# Copyright (C) 2013 Simon Newton

"""Test to check we can load the Pid data in Python."""

import os
import unittest

from ola import PidStore

__author__ = 'nomis52@gmail.com (Simon Newton)'


class PidDataTest(unittest.TestCase):
  # Implement assertIsNotNone for Python runtimes < 2.7 or < 3.1
  # Copied from:
  # https://github.com/nvie/rq/commit/f5951900c8e79a116c59470b8f5b2f544000bf1f
  if not hasattr(unittest.TestCase, 'assertIsNotNone'):
    def assertIsNotNone(self, value, *args):
      self.assertNotEqual(value, None, *args)

  def testLoad(self):
    store = PidStore.GetStore(os.environ['PIDDATA'])
    self.assertIsNotNone(store)

    pids = store.Pids()
    self.assertNotEqual(0, len(pids))


if __name__ == '__main__':
  unittest.main()
