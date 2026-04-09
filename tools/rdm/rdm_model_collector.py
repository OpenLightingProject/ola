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
# rdm_model_collector.py
# Copyright (C) 2011 Simon Newton

from __future__ import print_function

import getopt
import logging
import pprint
import sys
import textwrap

from ola.ClientWrapper import ClientWrapper
from ola.testing.rdm.ModelCollector import ModelCollector

from ola import PidStore

'''Quick script to collect information about responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


def Usage():
  print(textwrap.dedent("""\
  Usage: rdm_model_collector.py --universe <universe>

  Collect information about responders attached to a universe and output in a
  format that can be imported into the RDM manufacturer index
  (http://rdm.openlighting.org/)

    -d, --debug               Print extra debug info.
    -h, --help                Display this help message and exit.
    -p, --pid-location        The directory to read PID definitions from.
    --skip-queued-messages    Don't attempt to fetch queued messages for the
                              device.
    -u, --universe <universe> Universe number."""))


def main():
  try:
    opts, args = getopt.getopt(
        sys.argv[1:],
        'dhp:u:',
        ['debug', 'help', 'skip-queued-messages', 'pid-location=', 'universe='])
  except getopt.GetoptError as e:
    print(str(e))
    Usage()
    sys.exit(2)

  universe = None
  pid_location = None
  level = logging.INFO
  skip_queued_messages = False
  for o, a in opts:
    if o in ('-d', '--debug'):
      level = logging.DEBUG
    elif o in ('-h', '--help'):
      Usage()
      sys.exit()
    elif o in ('--skip-queued-messages'):
      skip_queued_messages = True
    elif o in ('-p', '--pid-location',):
      pid_location = a
    elif o in ('-u', '--universe'):
      universe = int(a)

  if universe is None:
    Usage()
    sys.exit()

  logging.basicConfig(
      level=level,
      format='%(message)s')

  client_wrapper = ClientWrapper()
  pid_store = PidStore.GetStore(pid_location)
  controller = ModelCollector(client_wrapper, pid_store)
  data = controller.Run(universe, skip_queued_messages)
  pprint.pprint(data)


if __name__ == '__main__':
  main()
