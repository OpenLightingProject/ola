#!/usr/bin/env python
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
# ola_send_dmx.py
# Copyright (C) 2005 Simon Newton

"""Send some DMX data."""

from __future__ import print_function

import array
import sys

from ola.ClientWrapper import ClientWrapper

__author__ = 'nomis52@gmail.com (Simon Newton)'

wrapper = None


def DmxSent(status):
  if status.Succeeded():
    print('Success!')
  else:
    print('Error: %s' % status.message, file=sys.stderr)

  global wrapper
  if wrapper:
    wrapper.Stop()


def main():
  universe = 1
  data = array.array('B')
  # append first dmx-value
  data.append(10)
  # append second dmx-value
  data.append(50)
  # append third dmx-value
  data.append(255)

  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()
  # send 1 dmx frame with values for channels 1-3
  client.SendDmx(universe, data, DmxSent)
  wrapper.Run()


if __name__ == '__main__':
  main()
