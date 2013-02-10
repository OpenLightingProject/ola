#!/usr/bin/python
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# model_collector.py
# Copyright (C) 2011 Simon Newton

'''Quick script to collect information about responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


import getopt
import logging
import pprint
import sys
import textwrap
from ola import PidStore
from ola.testing.rdm.ModelCollector import ModelCollector
from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OlaClient, RDMNack
from ola.RDMAPI import RDMAPI
from ola.UID import UID

def Usage():
  print textwrap.dedent("""\
  Usage: rdm_model_collector.py --universe <universe>

  Collect information about responders attached to a universe and output in a
  format that can be imported into the RDM manufacturer index
  (http://rdm.openlighting.org/)

    -d, --debug               Print extra debug info.
    -h, --help                Display this help message and exit.
    --pid_file                The PID data store to use.
    --skip_queued_messages    Don't attempt to fetch queued messages for the
                              device.
    -u, --universe <universe> Universe number.""")


def main():
  try:
    opts, args = getopt.getopt(
        sys.argv[1:],
        'dhp:u:',
        ['debug', 'help', 'skip_queued_messages', 'pid_file=', 'universe='])
  except getopt.GetoptError, err:
    print str(err)
    Usage()
    sys.exit(2)

  universe = None
  pid_file = None
  level = logging.INFO
  skip_queued_messages = False
  for o, a in opts:
    if o in ('-d', '--debug'):
      level = logging.DEBUG
    elif o in ('-h', '--help'):
      Usage()
      sys.exit()
    elif o in ('--skip_queued_messages'):
      skip_queued_messages = True
    elif o in ('--pid_file',):
      pid_file = a
    elif o in ('-u', '--universe'):
      universe = int(a)

  if universe is None:
    Usage()
    sys.exit()

  logging.basicConfig(
      level=level,
      format='%(message)s')

  client_wrapper = ClientWrapper()
  pid_store = PidStore.GetStore(pid_file)
  controller = ModelCollector(client_wrapper, pid_store)
  data = controller.Run(universe, skip_queued_messages)
  pprint.pprint(data)

if __name__ == '__main__':
  main()
