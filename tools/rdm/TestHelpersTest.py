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
# TestHelpersTest.py
# Copyright (C) 2021 Peter Newman

import unittest

# Keep this import relative to simplify the testing
from TestHelpers import ContainsUnprintable

"""Test cases for TestHelpers utilities."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class TestHelpersContainsUnprintableTest(unittest.TestCase):
  def testContainsUnprintable(self):
    self.assertFalse(ContainsUnprintable("foo"))
    self.assertFalse(ContainsUnprintable("bar"))
    self.assertFalse(ContainsUnprintable("bar[]"))
    self.assertFalse(ContainsUnprintable(u'foo-bar'))
    self.assertTrue(ContainsUnprintable("foo\x00bar"))
    # TODO(Peter): How do these interact with the E1.20 Unicode flag?
    self.assertTrue(ContainsUnprintable(u'caf\xe9'))
    self.assertTrue(ContainsUnprintable(u'c\xc0f\xe9'))
    self.assertTrue(ContainsUnprintable(u'foo\u2014bar'))

    with self.assertRaises(TypeError):
      result = ContainsUnprintable(42)
      self.assertNone(result)


if __name__ == '__main__':
  unittest.main()
