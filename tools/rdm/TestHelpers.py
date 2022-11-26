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
# TestHelpers.py
# Copyright (C) 2013 Peter Newman

import sys
from ola.StringUtils import StringEscape

if sys.version_info >= (3, 0):
  try:
    unicode
  except NameError:
    unicode = str

__author__ = 'nomis52@gmail.com (Simon Newton)'


def ContainsUnprintable(s):
  """Check if a string s contain unprintable characters."""
  # TODO(Peter): How does this interact with the E1.20 Unicode flag?
  # We don't use sys.version_info.major to support Python 2.6.
  if sys.version_info[0] == 2 and (type(s) == str or type(s) == unicode):
    return s != StringEscape(s)
  elif type(s) == str:
    # All strings in Python 3 are unicode
    return s.encode() != StringEscape(s)
  else:
    return False
