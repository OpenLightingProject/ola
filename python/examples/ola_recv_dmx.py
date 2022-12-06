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
# ola_recv_dmx.py
# Copyright (C) 2005 Simon Newton

"""Receive DMX data."""

import getopt
import sys
import textwrap

from ola.ClientWrapper import ClientWrapper

__author__ = 'nomis52@gmail.com (Simon Newton)'


def NewData(data):
  print(data)


def Usage():
  print(textwrap.dedent("""
  Usage: ola_recv_dmx.py --universe <universe>

  Display the DXM512 data for the universe.

  -h, --help                Display this help message and exit.
  -u, --universe <universe> Universe number."""))


def main():
  try:
      opts, args = getopt.getopt(sys.argv[1:], "hu:", ["help", "universe="])
  except getopt.GetoptError as err:
    print(str(err))
    Usage()
    sys.exit(2)

  universe = 1
  for o, a in opts:
    if o in ("-h", "--help"):
      Usage()
      sys.exit()
    elif o in ("-u", "--universe"):
      universe = int(a)

  wrapper = ClientWrapper()
  client = wrapper.Client()
  client.RegisterUniverse(universe, client.REGISTER, NewData)
  wrapper.Run()


if __name__ == '__main__':
  main()
