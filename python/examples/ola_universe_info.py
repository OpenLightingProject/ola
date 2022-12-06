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
# ola_universe_info.py
# Copyright (C) 2005 Simon Newton

"""Lists the active universes."""

from __future__ import print_function

import sys

from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import Universe

__author__ = 'nomis52@gmail.com (Simon Newton)'

wrapper = None


def Universes(status, universes):
  if status.Succeeded():
    for uni in universes:
      print('Universe %d' % uni.id)
      print('  - Name: %s' % uni.name)
      print('  - Merge mode: %s' % (
            ('LTP' if uni.merge_mode == Universe.LTP else 'HTP')))

      if len(uni.input_ports) > 0:
        print('  - Input ports:')
        for p in uni.input_ports:
          print('    - %s' % p)

      if len(uni.output_ports) > 0:
        print('  - Output ports:')
        for p in uni.output_ports:
          print('    - %s' % p)
  else:
    print('Error: %s' % status.message, file=sys.stderr)

  global wrapper
  if wrapper:
    wrapper.Stop()


def main():
  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()
  client.FetchUniverses(Universes)
  wrapper.Run()


if __name__ == '__main__':
  main()
