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
# ola_artnet_params.py
# Copyright (C) 2005 Simon Newton

"""Fetch some ArtNet parameters."""

from __future__ import print_function

import sys

from ola.ClientWrapper import ClientWrapper

from ola import ArtNetConfigMessages_pb2

__author__ = 'nomis52@gmail.com (Simon Newton)'


def ArtNetConfigureReply(status, response):
  if status.Succeeded():
    reply = ArtNetConfigMessages_pb2.Reply()
    reply.ParseFromString(response)
    print('Short Name: %s' % reply.options.short_name)
    print('Long Name: %s' % reply.options.long_name)
    print('Subnet: %d' % reply.options.subnet)
  else:
    print('Error: %s' % status.message, file=sys.stderr)

  global wrapper
  if wrapper:
    wrapper.Stop()


def main():
  # Set this appropriately
  device_alias = 1
  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()
  artnet_request = ArtNetConfigMessages_pb2.Request()
  artnet_request.type = artnet_request.ARTNET_OPTIONS_REQUEST
  client.ConfigureDevice(device_alias, artnet_request.SerializeToString(),
                         ArtNetConfigureReply)
  wrapper.Run()


if __name__ == '__main__':
  main()
