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
# MACAddressTest.py
# Copyright (C) 2013 Peter Newman

import sys
import unittest

from ola.MACAddress import MACAddress
from ola.TestUtils import allHashNotEqual, allNotEqual

"""Test cases for the MACAddress class."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class MACAddressTest(unittest.TestCase):

  def testBasic(self):
    mac = MACAddress(bytearray([0x01, 0x23, 0x45, 0x67, 0x89, 0xab]))
    self.assertEqual(b'\x01\x23\x45\x67\x89\xab', bytes(mac.mac_address))
    self.assertEqual('01:23:45:67:89:ab', str(mac))

    # Python 3 does not allow sorting of incompatible types.
    # We don't use sys.version_info.major to support Python 2.6.
    if sys.version_info[0] == 2:
        self.assertTrue(mac > None)

    mac2 = MACAddress(bytearray([0x01, 0x23, 0x45, 0x67, 0x89, 0xcd]))
    self.assertTrue(mac2 > mac)
    mac3 = MACAddress(bytearray([0x01, 0x23, 0x45, 0x67, 0x88, 0xab]))
    self.assertTrue(mac > mac3)
    macs = [mac, mac2, mac3]
    self.assertEqual([mac3, mac, mac2], sorted(macs))

  def testFromString(self):
    self.assertEqual(None, MACAddress.FromString(''))
    self.assertEqual(None, MACAddress.FromString('abc'))
    self.assertEqual(None, MACAddress.FromString(':'))
    self.assertEqual(None, MACAddress.FromString('0:1:2'))
    self.assertEqual(None, MACAddress.FromString('12345:1234'))

    mac = MACAddress.FromString('01:23:45:67:89:ab')
    self.assertTrue(mac)
    self.assertEqual(b'\x01\x23\x45\x67\x89\xab', bytes(mac.mac_address))
    self.assertEqual('01:23:45:67:89:ab', str(mac))

    mac2 = MACAddress.FromString('98.76.54.fe.dc.ba')
    self.assertTrue(mac2)
    self.assertEqual(b'\x98\x76\x54\xfe\xdc\xba', bytes(mac2.mac_address))
    self.assertEqual('98:76:54:fe:dc:ba', str(mac2))

  def testSorting(self):
    m1 = MACAddress(bytearray([0x48, 0x45, 0xff, 0xff, 0xff, 0xfe]))
    m2 = MACAddress(bytearray([0x48, 0x45, 0x00, 0x00, 0x02, 0x2e]))
    m3 = MACAddress(bytearray([0x48, 0x44, 0x00, 0x00, 0x02, 0x2e]))
    m4 = MACAddress(bytearray([0x48, 0x46, 0x00, 0x00, 0x02, 0x2e]))
    macs = sorted([m1, m2, None, m3, m4])
    self.assertEqual([None, m3, m2, m1, m4], macs)
    allNotEqual(self, macs)
    allHashNotEqual(self, macs)

  def testEquals(self):
    m1 = MACAddress(bytearray([0x48, 0x45, 0xff, 0xff, 0xff, 0xfe]))
    m2 = MACAddress(bytearray([0x48, 0x45, 0xff, 0xff, 0xff, 0xfe]))
    self.assertEqual(m1, m2)

  def testCmp(self):
    m2 = MACAddress(bytearray([0x48, 0x45, 0x00, 0x00, 0x02, 0x2e]))
    m3 = MACAddress(bytearray([0x48, 0x44, 0x00, 0x00, 0x02, 0x2e]))
    m3a = MACAddress(bytearray([0x48, 0x44, 0x00, 0x00, 0x02, 0x2e]))

    self.assertEqual(m3, m3a)
    self.assertTrue(m3 <= m3a)
    self.assertTrue(m3 >= m3a)

    self.assertTrue(m3 < m2)
    self.assertTrue(m2 > m3)
    self.assertTrue(m3 <= m2)
    self.assertTrue(m2 >= m3)
    self.assertTrue(m3 != m2)

    self.assertFalse(m3 > m2)
    self.assertFalse(m2 < m3)
    self.assertFalse(m3 >= m2)
    self.assertFalse(m2 <= m3)
    self.assertFalse(m3 == m2)

    self.assertEqual(m2.__lt__("hello"), NotImplemented)
    self.assertNotEqual(m2, "hello")

    # None case
    self.assertFalse(m3 < None)
    self.assertTrue(m3 > None)
    self.assertFalse(m3 <= None)
    self.assertTrue(m3 >= None)
    self.assertTrue(m3 is not None)
    self.assertFalse(m3 is None)


if __name__ == '__main__':
  unittest.main()
