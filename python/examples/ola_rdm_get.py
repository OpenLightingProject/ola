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
# ola_rdm_get.py
# Copyright (C) 2010 Simon Newton

'''Get a PID from a UID.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'

import getopt
import os.path
import sys
import textwrap
from ola import PidStore
from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OlaClient
from ola.RDMAPI import RDMAPI
from ola.UID import UID

def Usage():
  print textwrap.dedent("""\
  Usage: ola_rdm_get.py --universe <universe> --uid <uid> <pid>

  Get the value of a pid for a device.
  Use 'ola_rdm_get --list_pids' to get a list of pids.

    -d, --sub_device <device> target a particular sub device (default is 0)
    -h, --help                Display this help message and exit.
    -l, --list_pids           display a list of pids
    --pid_file                The PID data store to use.
    --uid                     The UID to send to.
    -u, --universe <universe> Universe number.""")

wrapper = None


def CheckStatus(status):
  if status.Succeeded():
    if status.response_code != OlaClient.RDM_COMPLETED_OK:
      print status.ResponseCodeAsString()
    elif status.response_type == OlaClient.RDM_NACK_REASON:
      print 'Got nack with reason: %s' % status.NackReasonAsString()
    elif status.response_type == OlaClient.RDM_ACK_TIMER:
      print 'Got ACK TIMER set to %d ms' % status.ack_timer
    else:
      # proper response
      return True
  return False


def RequestComplete(status, pid, thing):
  global wrapper
  wrapper.Stop()
  if CheckStatus(status):
    print 'PID: 0x%04hx' % pid
    print thing

def ListPids(uid):
  """List the pid available, taking into account manufacturer specific pids.

  Args:
    uid: Include manufacturer specific pids for this UID.
  """
  pid_store = PidStore.GetStore()
  names = []
  for pid in pid_store.Pids():
    names.append('%s (0x%04hx)' % (pid.name, pid.value))
  for pid in pid_store.ManufacturerPids(uid.manufacturer_id):
    names.append('%s (0x%04hx)' % (pid.name, pid.value))
  names.sort()
  print '\n'.join(names)

def ValidateArgs(pid, args, request_type):
  if not pid.RequestSupported(request_type):
    print >> sys.stderr, 'PID does not support command'
    return None

  return pid.CheckArgs(args, request_type)


def main():
  try:
    opts, args = getopt.getopt(sys.argv[1:], 'd:hlu:',
                               ['sub_device=', 'help', 'list_pids',
                                 'pid_file=', 'uid=', 'universe='])
  except getopt.GetoptError, err:
    print str(err)
    Usage()
    sys.exit(2)

  universe = None
  uid = None
  sub_device = 0
  list_pids = False
  pid_file = None
  for o, a in opts:
    if o in ('-d', '--sub_device'):
      sub_device = int(a)
    elif o in ('-h', '--help'):
      Usage()
      sys.exit()
    elif o in ('-l', '--list_pids'):
      list_pids = True
    elif o in ('--uid',):
      uid = UID.FromString(a)
    elif o in ('--pid_file',):
      pid_file = a
    elif o in ('-u', '--universe'):
      universe = int(a)

  pid_store = PidStore.GetStore(pid_file)

  if list_pids:
    ListPids(uid)
    sys.exit()

  if not universe or not uid or len(args) == 0:
    Usage()
    sys.exit()

  pid = None
  try:
    pid = pid_store.GetPid(int(args[0], 0), uid)
  except ValueError:
    pid = pid_store.GetName(args[0], uid)

  if not pid:
    ListPids(uid)
    sys.exit()

  global wrapper
  wrapper = ClientWrapper()
  client = wrapper.Client()
  rdm_api = RDMAPI(client, pid_store)

  request_type = PidStore.RDM_GET
  if os.path.basename(sys.argv[0]) == 'ola_rdm_set.py':
    request_type = PidStore.RDM_SET

  rdm_args = args[1:]
  santitized_args = ValidateArgs(pid, rdm_args, request_type)
  if santitized_args is None:
    # TODO(simon): print the format here
    sys.exit()

  if request_type == PidStore.RDM_SET:
    if rdm_api.Set(universe, uid, sub_device, pid, RequestComplete,
                   santitized_args):
      wrapper.Run()
  else:
    if rdm_api.Get(universe, uid, sub_device, pid, RequestComplete,
                   santitized_args):
      wrapper.Run()

if __name__ == '__main__':
  main()
