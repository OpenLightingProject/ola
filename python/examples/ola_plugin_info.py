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
# ola_plugin_info.py
# Copyright (C) 2005 Simon Newton

"""Lists the loaded plugins."""

from __future__ import print_function

import getopt
import sys
import textwrap

from ola.ClientWrapper import ClientWrapper

__author__ = 'nomis52@gmail.com (Simon Newton)'

wrapper = None


def Usage():
  print(textwrap.dedent("""
  Usage: ola_plugin_info.py [--plugin <plugin_id>]

  Display a list of plugins, or a description for a particular plugin.

  -h, --help                Display this help message and exit.
  -p, --plugin <plugin_id>  Plugin ID number."""))


def Plugins(status, plugins):
  if status.Succeeded():
    for plugin in plugins:
      print('%d %s' % (plugin.id, plugin.name))
  else:
    print('Error: %s' % status.message, file=sys.stderr)

  global wrapper
  if wrapper:
    wrapper.Stop()


def PluginDescription(status, description):
  if status.Succeeded():
    print(description)
  else:
    print('Error: %s' % status.message, file=sys.stderr)

  global wrapper
  if wrapper:
    wrapper.Stop()


def main():
  try:
      opts, args = getopt.getopt(sys.argv[1:], "hp:", ["help", "plugin="])
  except getopt.GetoptError as err:
    print(str(err))
    Usage()
    sys.exit(2)

  plugin = None
  for o, a in opts:
    if o in ("-h", "--help"):
      Usage()
      sys.exit()
    elif o in ("-p", "--plugin"):
      plugin = int(a)

  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()

  if plugin is not None:
    client.PluginDescription(PluginDescription, plugin)
  else:
    client.FetchPlugins(Plugins)

  wrapper.Run()


if __name__ == '__main__':
  main()
