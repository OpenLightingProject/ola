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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# MACAddressTest.py
# Copyright (C) 2013 Peter Newman

"""Test cases for the MACAddress class."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import unittest
from ola.MACAddress import MACAddress

class MACAddressTest(unittest.TestCase):

  def testBasic(self):
    mac = MACAddress(bytearray([0x01, 0x23, 0x45, 0x67, 0x89, 0xab]))
    self.assertEquals(b'\x01\x23\x45\x67\x89\xab', bytes(mac.mac_address))
    self.assertEquals('01:23:45:67:89:ab', str(mac))

#TODO(Peter): Do these
#    self.assertTrue(uid > None)
#    uid2 = UID(0x707a, 0x12345679)
#    self.assertTrue(uid2 > uid)
#    uid3 = UID(0x7079, 0x12345678)
#    self.assertTrue(uid > uid3)
#    uids = [uid, uid2, uid3]
#    self.assertEquals([uid3, uid, uid2], sorted(uids))

  def testFromString(self):
    self.assertEquals(None, MACAddress.FromString(''))
    self.assertEquals(None, MACAddress.FromString('abc'))
    self.assertEquals(None, MACAddress.FromString(':'))
    self.assertEquals(None, MACAddress.FromString('0:1:2'))
    self.assertEquals(None, MACAddress.FromString('12345:1234'))

    mac = MACAddress.FromString('01:23:45:67:89:ab')
    self.assertTrue(mac)
    self.assertEquals(b'\x01\x23\x45\x67\x89\xab', bytes(mac.mac_address))
    self.assertEquals('01:23:45:67:89:ab', str(mac))

    mac2 = MACAddress.FromString('98.76.54.fe.dc.ba')
    self.assertTrue(mac2)
    self.assertEquals(b'\x98\x76\x54\xfe\xdc\xba', bytes(mac2.mac_address))
    self.assertEquals('98:76:54:fe:dc:ba', str(mac2))

#TODO(Peter): Do these
#  def testSorting(self):
#    u1 = UID(0x4845, 0xfffffffe)
#    u2 = UID(0x4845, 0x0000022e)
#    u3 = UID(0x4844, 0x0000022e)
#    u4 = UID(0x4846, 0x0000022e)
#    uids = [u1, u2, u3, u4]
#    uids.sort()
#    self.assertEquals([u3, u2, u1, u4], uids)

if __name__ == '__main__':
  unittest.main()
