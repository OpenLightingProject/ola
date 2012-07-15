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
# UID.py
# Copyright (C) 2010 Simon Newton

"""The UID class."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class Error(Exception):
  """Base Error Class."""


class UIDOutOfRangeException(Error):
  """Returned when a UID would be out of range."""


class UID(object):
  """Represents a UID."""
  def __init__(self, manufacturer_id, device_id):
    self._manufacturer_id = manufacturer_id
    self._device_id = device_id

  @property
  def manufacturer_id(self):
    return self._manufacturer_id

  @property
  def device_id(self):
    return self._device_id

  def IsBroadcast(self):
    return self._device_id == 0xffffffff

  def __str__(self):
    return '%04x:%08x' % (self._manufacturer_id, self._device_id)

  def __repr__(self):
    return self.__str__()

  def __cmp__(self, other):
    if other is None:
      return 1
    if self._manufacturer_id == other._manufacturer_id:
      return cmp(self._device_id, other._device_id)
    return cmp(self.manufacturer_id, other.manufacturer_id)

  @staticmethod
  def AllDevices():
    return UID(0xffff, 0xffffffff)

  @staticmethod
  def AllManufacturerDevices(manufacturer_id):
    return UID(manufacturer_id, 0xffffffff)

  @staticmethod
  def FromString(uid_str):
    """Create a new UID from a string.

    Args:
      uid_str: The string representation of the UID, e.g. 00f0:12345678.
    """
    parts = uid_str.split(':')
    if len(parts) != 2:
      return None
    try:
      manufacturer_id = int(parts[0], 16)
      device_id = int(parts[1], 16)
    except ValueError:
      return None

    if manufacturer_id > 0xffff or device_id > 0xffffffff:
      return None
    return UID(manufacturer_id, device_id)

  @staticmethod
  def NextUID(uid):
    if uid == UID.AllDevices():
      raise UIDOutOfRangeException(uid)

    if uid.IsBroadcast():
      return UID(uid.manufacturer_id + 1, 0)
    else:
      return UID(uid.manufacturer_id, uid.device_id + 1)

  @staticmethod
  def PreviousUID(uid):
    if uid == UID(0, 0):
      raise UIDOutOfRangeException(uid)

    if uid.device_id == 0:
      return UID(uid.manufacturer_id - 1, 0xffffffff)
    else:
      return UID(uid.manufacturer_id, uid.device_id - 1)
