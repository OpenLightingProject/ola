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
# StringUtilsTest.py
# Copyright (C) 2022 Peter Newman

import unittest
from StringUtils import StringEscape

"""Test cases for StringUtils utilities."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class StringUtilsTest(unittest.TestCase):
  def testStringEscape(self):
    self.assertEqual(b'foo', StringEscape("foo"))
    self.assertEqual(b'bar', StringEscape("bar"))
    self.assertEqual(b'bar[]', StringEscape("bar[]"))
    self.assertEqual(b'foo-bar', StringEscape(u'foo-bar'))
    self.assertEqual(b'foo\\x00bar', StringEscape("foo\x00bar"))
    # TODO(Peter): How does this interact with the E1.20 Unicode flag?
    self.assertEqual(b'caf\\xe9', StringEscape(u'caf\xe9'))
    self.assertEqual(b'foo\\u2014bar', StringEscape(u'foo\u2014bar'))


if __name__ == '__main__':
  unittest.main()
