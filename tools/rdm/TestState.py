# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# TestState.py
# Copyright (C) 2011 Simon Newton

__author__ = 'nomis52@gmail.com (Simon Newton)'


class TestState(object):
  """Represents the state of a test."""
  def __init__(self, state):
    self._state = state

  def __str__(self):
    return self._state

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self._state == other._state

  def __lt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self._state < other._state

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other

  def __hash__(self):
    return hash(self._state)

  def ColorString(self):
    strs = []
    if self == TestState.PASSED:
      strs.append('\x1b[32m')
    elif self == TestState.FAILED:
      strs.append('\x1b[31m')
    elif self == TestState.BROKEN:
      strs.append('\x1b[33m')

    strs.append(str(self._state))
    strs.append('\x1b[0m')
    return ''.join(strs)


TestState.PASSED = TestState('Passed')
TestState.FAILED = TestState('Failed')
TestState.BROKEN = TestState('Broken')
TestState.NOT_RUN = TestState('Not Run')
