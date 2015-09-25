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

__author__ = 'nomis52@gmail.com (Simon Newton)'

from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import Universe


def Universes(state, universes):
  for uni in universes:
    print('Universe {}'.format(uni.id))
    print('  - Name: {}'.format(uni.name))
    print('  - Merge mode: {}'.format(
        'LTP' if uni.merge_mode == Universe.LTP else 'HTP'))

    if len(uni.input_ports) > 0:
      print('  - Input ports:')
      for p in uni.input_ports:
        print('    - {}'.format(p))

    if len(uni.output_ports) > 0:
      print('  - Output ports:')
      for p in uni.output_ports:
        print('    - {}'.format(p))

  wrapper.Stop()

wrapper = ClientWrapper()
client = wrapper.Client()
client.FetchUniverses(Universes)
wrapper.Run()
