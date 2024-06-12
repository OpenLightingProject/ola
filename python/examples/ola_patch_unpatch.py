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
# ola_patch_unpatch.py
# Copyright (C) 2015 Simon Marchi

"""Patch and unpatch ports."""

from __future__ import print_function

import argparse
import sys

from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OlaClient

__author__ = 'simon.marchi@polymtl.ca (Simon Marchi)'

wrapper = None


def ParseArgs():
  description = 'Patch or unpatch an OLA port.'
  argparser = argparse.ArgumentParser(description=description)
  argparser.add_argument('--device', '-d',
                         metavar='DEV',
                         type=int,
                         required=True)
  argparser.add_argument('--port', '-p',
                         metavar='PORT',
                         type=int,
                         required=True)
  argparser.add_argument('--universe', '-u',
                         metavar='UNI',
                         type=int,
                         required=True)
  argparser.add_argument('--mode', '-m',
                         choices=['input', 'output'],
                         required=True)
  argparser.add_argument('--action', '-a',
                         choices=['patch', 'unpatch'],
                         required=True)

  return argparser.parse_args()


def PatchPortCallback(status):
  if status.Succeeded():
    print('Success!')
  else:
    print('Error: %s' % status.message, file=sys.stderr)
  wrapper.Stop()


def main():
  args = ParseArgs()

  device = args.device
  port = args.port
  is_output = args.mode == 'output'
  action = OlaClient.PATCH if args.action == 'patch' else OlaClient.UNPATCH
  universe = args.universe

  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()
  client.PatchPort(device, port, is_output, action, universe, PatchPortCallback)
  wrapper.Run()


if __name__ == '__main__':
  main()
