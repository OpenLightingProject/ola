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
# ola_devices.py
# Copyright (C) 2005 Simon Newton

"""Lists the devices / ports."""

from __future__ import print_function

import sys

from ola.ClientWrapper import ClientWrapper

__author__ = 'nomis52@gmail.com (Simon Newton)'

wrapper = None


def RDMString(port):
  if port.supports_rdm:
    return ', RDM supported'
  return ''


def Devices(status, devices):
  if status.Succeeded():
    for device in sorted(devices):
      print('Device %d: %s' % (device.alias, device.name))
      print('Input ports:')
      for port in device.input_ports:
        print('  port %d, %s%s' % (port.id, port.description, RDMString(port)))
      print('Output ports:')
      for port in device.output_ports:
        print('  port %d, %s%s' % (port.id, port.description, RDMString(port)))
  else:
    print('Error: %s' % status.message, file=sys.stderr)

  global wrapper
  if wrapper:
    wrapper.Stop()


def main():
  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()
  client.FetchDevices(Devices)
  wrapper.Run()


if __name__ == '__main__':
  main()
