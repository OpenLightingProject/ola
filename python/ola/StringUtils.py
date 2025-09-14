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
# StringUtils.py
# Copyright (C) 2022 Peter Newman

"""Common utils for OLA Python string handling"""

import sys

if sys.version_info >= (3, 0):
  try:
    unicode
  except NameError:
    unicode = str

__author__ = 'nomis52@gmail.com (Simon Newton)'


def StringEscape(s):
  """Escape unprintable characters in a string."""
  # TODO(Peter): How does this interact with the E1.20 Unicode flag?
  # We don't use sys.version_info.major to support Python 2.6.
  if sys.version_info[0] == 2 and isinstance(s, str):
    return s.encode('string-escape')
  elif sys.version_info[0] == 2 and isinstance(s, unicode):
    return s.encode('unicode-escape')
  elif isinstance(s, str):
    # All strings in Python 3 are unicode
    # This encode/decode pair gets us an escaped string
    return s.encode('unicode-escape').decode(encoding="ascii",
                                             errors="backslashreplace")
  else:
    raise TypeError('Only strings are supported not %s' % type(s))
