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
# DUBDecoderTest.py
# Copyright (C) Simon Newton

import unittest

from ola.DUBDecoder import DecodeResponse

"""Test cases for the DUBDecoder class."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class UIDTest(unittest.TestCase):
  TEST_DATA = [
      0xfe, 0xfe, 0xfe, 0xfe,
      0xaa, 0xaa, 0x55, 0xab,
      0xf5, 0xaa, 0x55, 0xaa,
      0x57, 0xaa, 0x55, 0xaa,
      0x75, 0xae, 0x57, 0xbf,
      0xfd
  ]

  TEST_BAD_DATA = [
      0xfe, 0xfe, 0xfe, 0xfe,
      0xaa, 0xaa, 0x55, 0xab,
      0xf5, 0xaa, 0x55, 0xaa,
      0x57, 0xaa, 0x55, 0xaa,
      0x75, 0xae, 0x57, 0xbf,
      0xff  # invalid checksum
  ]

  def testInvalid(self):
    # we stick to methods in 2.6 for now
    self.assertEqual(None, DecodeResponse(bytearray()))
    self.assertEqual(None, DecodeResponse([0]))
    self.assertEqual(None, DecodeResponse([0, 0, 0]))
    self.assertEqual(None, DecodeResponse(self.TEST_DATA[0:-1]))
    self.assertEqual(None, DecodeResponse(self.TEST_DATA[4:]))
    self.assertEqual(None, DecodeResponse(self.TEST_BAD_DATA))

  def testValidResponse(self):
    uid = DecodeResponse(self.TEST_DATA)
    self.assertNotEqual(None, uid)
    self.assertEqual(0x00a1, uid.manufacturer_id)
    self.assertEqual(0x00020020, uid.device_id)


if __name__ == '__main__':
  unittest.main()
