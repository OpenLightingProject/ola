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
# StringUtilsTest.py
# Copyright (C) 2022 Peter Newman

import unittest

from ola.StringUtils import StringEscape

"""Test cases for StringUtils utilities."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class StringUtilsTest(unittest.TestCase):
  def testStringEscape(self):
    # Test we escape properly
    self.assertEqual('foo', StringEscape("foo"))
    self.assertEqual('bar', StringEscape("bar"))
    self.assertEqual('bar[]', StringEscape("bar[]"))
    self.assertEqual('foo-bar', StringEscape(u'foo-bar'))
    self.assertEqual('foo\\x00bar', StringEscape("foo\x00bar"))
    # TODO(Peter): How does this interact with the E1.20 Unicode flag?
    self.assertEqual('caf\\xe9', StringEscape(u'caf\xe9'))
    self.assertEqual('foo\\u2014bar', StringEscape(u'foo\u2014bar'))

    # Test that we display nicely in a string
    self.assertEqual('foo', ("%s" % StringEscape("foo")))
    self.assertEqual('bar[]', ("%s" % StringEscape("bar[]")))
    self.assertEqual('foo-bar', ("%s" % StringEscape(u'foo-bar')))
    self.assertEqual('foo\\x00bar', ("%s" % StringEscape("foo\x00bar")))
    # TODO(Peter): How does this interact with the E1.20 Unicode flag?
    self.assertEqual('caf\\xe9', ("%s" % StringEscape(u'caf\xe9')))
    self.assertEqual('foo\\u2014bar', ("%s" % StringEscape(u'foo\u2014bar')))

    # Confirm we throw an exception if we pass in a number or something else
    # that's not a string
    with self.assertRaises(TypeError):
      result = StringEscape(42)
      self.assertNone(result)


if __name__ == '__main__':
  unittest.main()
