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
# ResponderTestTest.py
# Copyright (C) 2019 Bruce Lowekamp

import unittest

from ola.testing.rdm.ResponderTest import ResponderTestFixture, TestFixture

"""Test cases for sorting TestFixtures."""

__author__ = 'bruce@lowekamp.net (Bruce Lowekamp)'


class ATestFixture(TestFixture):
  pass


class ZTestFixture(TestFixture):
  pass


class TestFixtureTest(unittest.TestCase):
  def testCmp(self):
    base = TestFixture({}, 2, 123, None)
    base2 = TestFixture({}, 3, 456, None)

    a = ATestFixture({}, 2, 123, None)
    z = ZTestFixture({}, 2, 123, None)

    self.assertEqual(base, base2)
    self.assertNotEqual(base, a)
    self.assertTrue(a < base)
    self.assertTrue(z > base)
    self.assertTrue(a <= base)
    self.assertTrue(base2 <= base)
    self.assertTrue(z >= base)
    self.assertTrue(base >= base)
    self.assertNotEqual(a, z)
    self.assertTrue(a < z)
    self.assertTrue(z > a)


class ResponderTestFixtureTest(unittest.TestCase):
  def testEscapeData(self):
    rtf = ResponderTestFixture(None, 0, None, None, None, None, 0, None)

    # TODO(Peter): How does this interact with the E1.20 Unicode flag?
    # We probably still want to escape it regardless
    self.assertEqual(rtf._EscapeData("foo"), "foo")
    self.assertEqual(rtf._EscapeData("bar"), "bar")
    self.assertEqual(rtf._EscapeData("bar[]"), "bar[]")
    self.assertEqual(rtf._EscapeData(u'foo-bar'), "foo-bar")
    self.assertEqual(rtf._EscapeData("foo\x00bar"), "foo\\x00bar")
    self.assertEqual(rtf._EscapeData(u'caf\xe9'), "caf\\xe9")
    self.assertEqual(rtf._EscapeData(u'foo\u2014bar'), "foo\\u2014bar")

    self.assertEqual('%s' % rtf._EscapeData("foo"), "foo")
    self.assertEqual('%s' % rtf._EscapeData("bar"), "bar")
    self.assertEqual('%s' % rtf._EscapeData("bar[]"), "bar[]")
    self.assertEqual('%s' % rtf._EscapeData(u'foo-bar'), "foo-bar")
    self.assertEqual('%s' % rtf._EscapeData("foo\x00bar"), "foo\\x00bar")
    self.assertEqual('%s' % rtf._EscapeData(u'caf\xe9'), "caf\\xe9")
    self.assertEqual('%s' % rtf._EscapeData(u'foo\u2014bar'), "foo\\u2014bar")

    self.assertEqual('%s' % rtf._EscapeData(None), "None")

    self.assertEqual('%s' % rtf._EscapeData(0), "0")
    self.assertEqual('%s' % rtf._EscapeData(1), "1")

    self.assertEqual('%s' % rtf._EscapeData([]), "[]")
    self.assertEqual('%s' % rtf._EscapeData([0]), "[0]")
    self.assertEqual('%s' % rtf._EscapeData([0, 1]), "[0, 1]")
    self.assertEqual('%s' % rtf._EscapeData(['a']), "['a']")
    self.assertEqual('%s' % rtf._EscapeData(["foo", 'a']), "['foo', 'a']")
    self.assertEqual('%s' % rtf._EscapeData(['caf\xe9']), "['caf\\\\xe9']")
    self.assertEqual('%s' % rtf._EscapeData(["foo", u'foo\u2014bar']),
                     "['foo', 'foo\\\\u2014bar']")

    self.assertEqual('%s' % rtf._EscapeData({"a": 0}), "{'a': 0}")
    # TODO(Peter): The tests below that are commented out are non-deterministic
    # on Python 3. We might be able to get round it with a sort or something? We
    # don't actually care about the order...
#    self.assertEqual('%s' % rtf._EscapeData({'a': 0, 'bar': 1}),
#                     "{'a': 0, 'bar': 1}")
    self.assertEqual('%s' % rtf._EscapeData({"a": "bar"}), "{'a': 'bar'}")
#    self.assertEqual('%s' % rtf._EscapeData({'a': 'foo', 'bar': "baz"}),
#                     "{'a': 'foo', 'bar': 'baz'}")
    self.assertEqual('%s' % rtf._EscapeData({"a": "caf\xe9"}),
                     "{'a': 'caf\\\\xe9'}")
#    self.assertEqual('%s' % rtf._EscapeData({'a': 'foo',
#                                             'bar': u'foo\u2014bar'}),
#                     "{'a': 'foo', 'bar': 'foo\\\\u2014bar'}")
#    self.assertEqual('%s' % rtf._EscapeData({"caf\xe9": "bar"}),
#                     "{'caf\xe9': 'bar'}")
#    self.assertEqual('%s' % rtf._EscapeData({'a': 'foo',
#                                             'foo\u2014bar': "baz"}),
#                     "{'a': 'foo', 'foo\\\\u2014bar': 'baz'}")


if __name__ == '__main__':
  unittest.main()
