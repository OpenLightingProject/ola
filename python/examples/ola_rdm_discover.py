#!/usr/bin/python
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# ola_rdm_discover.py
# Copyright (C) 2010 Simon Newton

'''Show the UIDs for a universe.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'

import getopt
import textwrap
import sys
import client_wrapper

def usage():
  print textwrap.dedent("""\
  Usage: ola_rdm_discover.py --universe <universe> [--force_discovery]

  Fetch the UID list for a universe.

    -h, --help                Display this help message and exit.
    -f, --force_discovery     Force RDM Discovery for this universe
    -u, --universe <universe> Universe number.""")


def main():
  try:
      opts, args = getopt.getopt(sys.argv[1:], 'hfu:',
                                 ['help', 'force_discovery', 'universe='])
  except getopt.GetoptError, err:
    print str(err)
    Usage()
    sys.exit(2)

  universe = None
  run_discovery = False
  for o, a in opts:
    if o in ('-h', '--help'):
      usage()
      sys.exit()
    elif o in ('-f', '--force_discovery'):
      run_discovery = True
    elif o in ('-u', '--universe'):
      universe = int(a)

  if not universe:
    usage()
    sys.exit()

  wrapper = client_wrapper.ClientWrapper()
  client = wrapper.Client()

  def show_uids(state, uids):
    if state.Succeeded():
      for uid in uids:
        print str(uid)
    wrapper.Stop()

  def discovery_done(state):
    wrapper.Stop()

  if run_discovery:
    client.RunRDMDiscovery(universe, discovery_done)
  else:
    client.FetchUIDList(universe, show_uids)
  wrapper.Run()

if __name__ == '__main__':
  main()
