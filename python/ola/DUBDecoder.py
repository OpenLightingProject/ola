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
# DUBDecoder.py
# Copyright (C) 2012 Simon Newton

import itertools

from ola.UID import UID

"""Decodes a DUB response."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


def DecodeResponse(data):
  """Decode a DUB response.

  Args:
    data: an iterable of data like a bytearray

  Returns:
    The UID that responded, or None if the response wasn't valid.
  """
  # min length is 18 bytes
  if len(data) < 18:
    return None

  # must start with 0xfe
  if data[0] != 0xfe:
    return None

  data = list(itertools.dropwhile(lambda x: x == 0xfe, data))

  if len(data) < 17 or data[0] != 0xaa:
    return None

  data = data[1:]

  checksum = 0
  for b in data[0:12]:
    checksum += b

  packet_checksum = (
      (data[12] & data[13]) << 8 |
      (data[14] & data[15])
  )

  if checksum != packet_checksum:
    return None

  manufacturer_id = (
      (data[0] & data[1]) << 8 |
      (data[2] & data[3])
  )

  device_id = (
      (data[4] & data[5]) << 24 |
      (data[6] & data[7]) << 16 |
      (data[8] & data[9]) << 8 |
      (data[10] & data[11])
  )

  return UID(manufacturer_id, device_id)
