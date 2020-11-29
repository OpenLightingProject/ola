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
# TestUtils.py
# Copyright (C) 2020 Bruce Lowekamp

import itertools
import struct
import sys

"""Common utils for ola python tests"""

__author__ = 'bruce@lowekamp.net (Bruce Lowekamp)'


def allNotEqual(testCase, t):
  for pair in itertools.combinations(t, 2):
    testCase.assertNotEqual(pair[0], pair[1])


def allHashNotEqual(testCase, t):
  h = map(hash, t)
  for pair in itertools.combinations(h, 2):
    testCase.assertNotEqual(pair[0], pair[1])


def handleRPCByteOrder(expected):
  # The RPC header (version and size) is encoded in native format, so flip that
  # part of the expected data where necessary (as our expected is from a
  # little endian source)
  if sys.byteorder == 'big':
    expected = (struct.pack('=L', struct.unpack_from('<L', expected)[0]) +
                expected[4:])
  return expected
