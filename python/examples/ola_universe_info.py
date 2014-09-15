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
    print '%d %s %r' % (uni.id, uni.name, uni.merge_mode == Universe.LTP)
  wrapper.Stop()

wrapper = ClientWrapper()
client = wrapper.Client()
client.FetchUniverses(Universes)
wrapper.Run()
