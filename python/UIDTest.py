#!/usr/bin/python
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# SimpleRpcControllerTest.py
# Copyright (C) 2005-2009 Simon Newton

"""Test cases for the UID class."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import unittest
from UID import UID

class UIDTest(unittest.TestCase):

  def testBasic(self):
    uid = UID(0x707a, 0x12345678)
    self.assertEquals(0x707a, uid.manufacturer_id)
    self.assertEquals(0x12345678, uid.device_id)
    self.assertEquals('707a:12345678', str(uid))

    self.assertTrue(uid > None)
    uid2 = UID(0x707a, 0x12345679)
    self.assertTrue(uid2 > uid)
    uid3 = UID(0x7079, 0x12345678)
    self.assertTrue(uid > uid3)
    uids = [uid, uid2, uid3]
    self.assertEquals([uid3, uid, uid2], sorted(uids))

    vendorcast_uid = UID.AllManufacturerDevices(0x707a)
    self.assertTrue(vendorcast_uid.IsBroadcast())
    broadcast_uid = UID.AllDevices()
    self.assertTrue(broadcast_uid.IsBroadcast())

  def testFromString(self):
    self.assertEquals(None, UID.FromString(''))
    self.assertEquals(None, UID.FromString('abc'))
    self.assertEquals(None, UID.FromString(':'))
    self.assertEquals(None, UID.FromString('0:1:2'))
    self.assertEquals(None, UID.FromString('12345:1234'))

    uid = UID.FromString('00a0:12345678')
    self.assertTrue(uid)
    self.assertEquals(0x00a0, uid.manufacturer_id)
    self.assertEquals(0x12345678, uid.device_id)
    self.assertEquals('00a0:12345678', str(uid))

  def testSorting(self):
    u1 = UID(0x4845, 0xfffffffe)
    u2 = UID(0x4845, 0x0000022e)
    u3 = UID(0x4844, 0x0000022e)
    u4 = UID(0x4846, 0x0000022e)
    uids = [u1, u2, u3, u4]
    uids.sort()
    self.assertEquals([u3, u2, u1, u4], uids)

if __name__ == '__main__':
  unittest.main()
