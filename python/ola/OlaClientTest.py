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
# OlaClientTest.py
# Copyright (C) 2019 Bruce Lowekamp

import unittest

from ola.OlaClient import Device, Plugin, Port, RDMNack, Universe
from ola.TestUtils import allHashNotEqual, allNotEqual

"""Test cases for data structures of OlaClient.
   SendDMX is tested with ClientWrapper."""

__author__ = 'bruce@lowekamp.net (Bruce Lowekamp)'


class OlaClientTest(unittest.TestCase):

  def testPlugin(self):
    # hash and eq only on id (1)
    a = Plugin(1, "a", False, False)
    aeq = Plugin(1, "az", True, True)
    b = Plugin(2, "b", False, False)
    c = Plugin(3, "b", False, False)

    self.assertEqual(a, aeq)
    self.assertEqual(hash(a), hash(aeq))
    allNotEqual(self, [a, b, c])
    allHashNotEqual(self, [a, b, c])

    s = sorted([c, a, b])
    self.assertEqual([a, b, c], s)

    self.assertEqual(a.__lt__("hello"), NotImplemented)
    self.assertNotEqual(a, "hello")

  def testDevice(self):
    # only eq on alias (2)
    a = Device(1, 2, "a", 4, [1, 2], [3, 4])
    aeq = Device(0, 2, "za", 54, [0, 1, 2, 3], [2, 3, 4])
    b = Device(2, 3, "b", 4, [1, 2], [3, 4])
    c = Device(2, 4, "b", 4, [1, 2], [3, 4])

    self.assertEqual(a, aeq)
    allNotEqual(self, [a, b, c])

    s = sorted([b, a, c])
    self.assertEqual([a, b, c], s)

    self.assertEqual(a.__lt__("hello"), NotImplemented)
    self.assertNotEqual(a, "hello")

  def testPort(self):
    # hash and eq only on id (1)
    a = Port(1, 1, False, "a", False)
    aeq = Port(1, 2, True, "xa", True)
    b = Port(2, 2, False, "b", False)
    c = Port(3, 3, False, "b", False)

    self.assertEqual(a, aeq)
    self.assertEqual(hash(a), hash(aeq))
    allNotEqual(self, [a, b, c])
    allHashNotEqual(self, [a, b, c])

    s = sorted([c, a, b])
    self.assertEqual([a, b, c], s)

    self.assertEqual(a.__lt__("hello"), NotImplemented)
    self.assertNotEqual(a, "hello")

  def testUniverse(self):
    # universe doesn't have hash and implements eq and < only on id
    a = Universe(1, 2, Universe.LTP, [1, 2], [3, 4])
    aeq = Universe(1, 3, Universe.HTP, [1, 2, 3], [3, 4, 5])
    b = Universe(2, 2, Universe.LTP, [1, 2], [3, 4])
    c = Universe(3, 2, Universe.HTP, [1, 2], [3, 4])

    self.assertEqual(a, aeq)
    allNotEqual(self, [a, b, c])

    s = sorted([c, b, a])
    self.assertEqual([a, b, c], s)

    self.assertEqual(a.__lt__("hello"), NotImplemented)
    self.assertNotEqual(a, "hello")

  def testRDMNack(self):
    # hash and eq only on value (1)
    a = RDMNack(1, "a")
    aeq = RDMNack(1, "also a")
    b = RDMNack(2, "b")
    c = RDMNack(3, "c")

    self.assertEqual(a, aeq)
    self.assertEqual(hash(a), hash(aeq))
    allNotEqual(self, [a, b, c])
    allHashNotEqual(self, [a, b, c])

    s = sorted([c, a, b])
    self.assertEqual([a, b, c], s)

    self.assertEqual(a.__lt__("hello"), NotImplemented)
    self.assertNotEqual(a, "hello")


if __name__ == '__main__':
  unittest.main()
