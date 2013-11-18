#  This program is free software; you can redistribute it and/or modify
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
# MACAddress.py
# Copyright (C) 2013 Peter Newman

"""The MACAddress class."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

MAC_ADDRESS_LENGTH = 6;

class Error(Exception):
  """Base Error Class."""


class MACAddress(object):
  """Represents a MAC Address."""
  def __init__(self, mac_address):
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

  def __cmp__(self, other):
    if other is None:
      return 1
    return cmp(self.mac_address, other.mac_address)

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
