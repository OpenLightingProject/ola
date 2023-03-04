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
# ola_candidate_ports.py
# Copyright (C) 2015 Simon Marchi

"""List candidate ports for patching."""

from __future__ import print_function

import argparse
import sys

from ola.ClientWrapper import ClientWrapper

__author__ = 'simon.marchi@polymtl.ca (Simon Marchi)'

wrapper = None


def ParseArgs():
  desc = 'Show the candidate ports to patch to a universe.'
  argparser = argparse.ArgumentParser(description=desc)
  argparser.add_argument('--universe', '-u',
                         type=int,
                         help='Universe for which to get the candidates.')
  return argparser.parse_args()


def GetCandidatePortsCallback(status, devices):
  if status.Succeeded():
    for device in devices:
      print('Device {d.alias}: {d.name}'.format(d=device))
      print('Candidate input ports:')
      for port in device.input_ports:
        s = '  port {p.id}, {p.description}, supports RDM: ' \
            '{p.supports_rdm}'
        print(s.format(p=port))
      print('Candidate output ports:')
      for port in device.output_ports:
        s = '  port {p.id}, {p.description}, supports RDM: ' \
            '{p.supports_rdm}'
        print(s.format(p=port))
  else:
    print('Error: %s' % status.message, file=sys.stderr)

  global wrapper
  if wrapper:
    wrapper.Stop()


def main():
  args = ParseArgs()
  universe = args.universe

  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()
  client.GetCandidatePorts(GetCandidatePortsCallback, universe)
  wrapper.Run()


if __name__ == '__main__':
  main()
