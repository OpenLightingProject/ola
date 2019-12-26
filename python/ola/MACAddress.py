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
# MACAddress.py
# Copyright (C) 2013 Peter Newman

"""The MACAddress class."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

MAC_ADDRESS_LENGTH = 6


class Error(Exception):
  """Base Error Class."""


class MACAddress(object):
  """Represents a MAC Address."""

  def __init__(self, mac_address):
    """Create a new MAC Address object.

    Args:
      mac_address: The byte array representation of the MAC Address, e.g.
      bytearray([0x01, 0x23, 0x45, 0x67, 0x89, 0xab]).
    """
    self._mac_address = mac_address

  @property
  def mac_address(self):
    return self._mac_address

  def __str__(self):
    return ':'.join(format(x, '02x') for x in self._mac_address)

  def __hash__(self):
    return hash(str(self))

  def __repr__(self):
    return self.__str__()

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self.mac_address == other.mac_address

  def __lt__(self, other):
    if other is None:
      return False
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self.mac_address < other.mac_address

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if other is None:
      return False
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if other is None:
      return True
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if other is None:
      return True
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other

  @staticmethod
  def FromString(mac_address_str):
    """Create a new MAC Address from a string.

    Args:
      mac_address_str: The string representation of the MAC Address, e.g.
      01:23:45:67:89:ab or 98.76.54.fe.dc.ba.
    """
    parts = mac_address_str.split(':')
    if len(parts) != MAC_ADDRESS_LENGTH:
      parts = mac_address_str.split('.')
      if len(parts) != MAC_ADDRESS_LENGTH:
        return None
    try:
      address = bytearray([int(parts[0], 16),
                           int(parts[1], 16),
                           int(parts[2], 16),
                           int(parts[3], 16),
                           int(parts[4], 16),
                           int(parts[5], 16)])
    except ValueError:
      return None

    return MACAddress(address)
