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
import readline
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
    -i, --interactive         Interactive mode
    -l, --list_pids           display a list of pids
    --pid_file                The PID data store to use.
    --uid                     The UID to send to.
    -u, --universe <universe> Universe number.""")

wrapper = None


def CheckStatus(status, unpack_exception):
  if status.Succeeded():
    if status.response_code != OlaClient.RDM_COMPLETED_OK:
      print status.ResponseCodeAsString()
    elif status.response_type == OlaClient.RDM_NACK_REASON:
      print 'Got nack with reason: %s' % status.nack_reason
    elif status.response_type == OlaClient.RDM_ACK_TIMER:
      print 'Got ACK TIMER set to %d ms' % status.ack_timer
    else:
      # proper response
      return True
  else:
    print unpack_exception
  return False


def RequestComplete(status, pid, response_data, unpack_exception):
  global wrapper
  wrapper.Stop()
  if CheckStatus(status, unpack_exception):
    print 'PID: 0x%04hx' % pid
    for key, value in response_data.iteritems():
      print '%s: %s' % (key, value)


def ListPids(uid):
  """List the pid available, taking into account manufacturer specific pids.

  Args:
    uid: Include manufacturer specific pids for this UID.
  """
  pid_store = PidStore.GetStore()
  names = []
  for pid in pid_store.Pids():
    names.append('%s (0x%04hx)' % (pid.name.lower(), pid.value))
  if uid:
    for pid in pid_store.ManufacturerPids(uid.manufacturer_id):
      names.append('%s (0x%04hx)' % (pid.name.lower(), pid.value))
  names.sort()
  print '\n'.join(names)


class InteractiveModeController(object):
  """Interactive mode!"""
  def __init__(self, universe, uid, sub_device, pid_file):
    """Create a new InteractiveModeController.

    Args:
      uid
      sub_device:
      pid_file:
    """
    self._universe = universe
    self._uid = uid
    self._sub_device = sub_device

    self.pid_store = PidStore.GetStore(pid_file)

    self.wrapper = ClientWrapper()
    self.client = self.wrapper.Client()
    self.rdm_api = RDMAPI(self.client, self.pid_store)

  def SetSubDevice(self, args):
    """Set the sub device."""
    if len(args) != 1:
      print 'Requires a single int argument'
      return

    try:
      sub_device = int(args[0])
    except ValueError:
      print 'Requires a single int argument'
      return

    if sub_device < 0 or sub_device > PidStore.ALL_SUB_DEVICES:
      print 'Argument must be between 0 and 0x%hx' % PidStore.ALL_SUB_DEVICES
      return
    self._sub_device = sub_device

  def ShowHelp(self):
    print textwrap.dedent("""\
      Commands:
        help              Show this help message
        list              List available pids
        print             Print universe, UID & sub device
        subdevice <int>   Set the sub device
        get <PID> [args]  Get a PID
        set <PID> [args]  Set a PID
        quit              Exit""")

  def PrintState(self):
    print textwrap.dedent("""\
      Universe: %d
      UID: %s
      Sub Device: %d""" % (
        self._universe,
        self._uid,
        self._sub_device))

  def ListPids(self):
    """List the pids available."""
    names = []
    for pid in self.pid_store.Pids():
      names.append('%s (0x%04hx)' % (pid.name.lower(), pid.value))
    if self._uid:
      for pid in self.pid_store.ManufacturerPids(self._uid.manufacturer_id):
        names.append('%s (0x%04hx)' % (pid.name.lower(), pid.value))
    names.sort()
    print '\n'.join(names)

  def _RequestComplete(self, status, pid, response_data, unpack_exception):
    self.wrapper.Stop()
    if CheckStatus(status):
      print 'PID: 0x%04hx' % pid
      for key, value in response_data.iteritems():
        print '%s: %s' % (key, value)

  def GetOrSet(self, request_type, args):
    if len(args) < 1:
      print 'get <pid> [args]'
      return

    pid = None
    try:
      pid = self.pid_store.GetPid(int(args[0], 0),
                                  self._uid.manufacturer_id)
    except ValueError:
      pid = self.pid_store.GetName(args[0].upper(),
                                   self._uid.manufacturer_id)
    if not pid:
      'Unknown pid'
      return

    rdm_args = args[1:]
    if not pid.RequestSupported(request_type):
      print 'PID does not support command'
      return

    if request_type == PidStore.RDM_SET:
      method = self.rdm_api.Set
    else:
      method = self.rdm_api.Get

    try:
      if method(self._universe,
                self._uid,
                self._sub_device,
                pid,
                self._RequestComplete,
                args[1:]):
        self.wrapper.Reset()
        self.wrapper.Run()
    except PidStore.ArgsValidationError, e:
      # TODO(simon): print the format here
      print e
      return

  def Run(self):
    """Run the interactive mode."""
    while True:
      """
      sys.stdout.write('> ')
      sys.stdout.flush()
      """
      try:
        input = raw_input('> ')
      except EOFError:
        print ''
        return

      if input.strip().lower() == 'quit':
        return

      input = input.strip()
      if input == '':
        continue

      commands = input.split()
      main_command = commands[0]
      args = commands[1:]
      if main_command == 'help':
        self.ShowHelp()
      elif main_command == 'get':
        self.GetOrSet(PidStore.RDM_GET, args)
      elif main_command == 'list':
        self.ListPids()
      elif main_command == 'print':
        self.PrintState()
      elif main_command == 'set':
        self.GetOrSet(PidStore.RDM_SET, args)
      elif main_command == 'subdevice':
        self.SetSubDevice(args)
      else:
        print "Unknown command, type 'help'"


def main():
  try:
    opts, args = getopt.getopt(sys.argv[1:], 'd:hilu:',
                               ['sub_device=', 'help', 'interactive',
                                 'list_pids', 'pid_file=', 'uid=',
                                 'universe='])
  except getopt.GetoptError, err:
    print str(err)
    Usage()
    sys.exit(2)

  universe = None
  uid = None
  sub_device = 0
  list_pids = False
  pid_file = None
  interactive_mode = False
  for o, a in opts:
    if o in ('-d', '--sub_device'):
      sub_device = int(a)
    elif o in ('-h', '--help'):
      Usage()
      sys.exit()
    elif o in ('-i', '--interactive'):
      interactive_mode = True
    elif o in ('-l', '--list_pids'):
      list_pids = True
    elif o in ('--uid',):
      uid = UID.FromString(a)
    elif o in ('--pid_file',):
      pid_file = a
    elif o in ('-u', '--universe'):
      universe = int(a)

  if not universe:
    Usage()
    sys.exit()

  if not uid and not list_pids:
    Usage()
    sys.exit()

  if interactive_mode:
    controller = InteractiveModeController(universe, uid, sub_device, pid_file)
    controller.Run()
    return

  pid_store = PidStore.GetStore(pid_file)

  if list_pids:
    ListPids(uid)
    sys.exit()

  if len(args) == 0:
    Usage()
    sys.exit()

  pid = None
  try:
    pid = pid_store.GetPid(int(args[0], 0), uid.manufacturer_id)
  except ValueError:
    pid = pid_store.GetName(args[0].upper(), uid.manufacturer_id)

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
  if not pid.RequestSupported(request_type):
    print >> sys.stderr, 'PID does not support command'
    return None

  if request_type == PidStore.RDM_SET:
    method = rdm_api.Set
  else:
    method = rdm_api.Get

  try:
    if method(universe, uid, sub_device, pid, RequestComplete, rdm_args):
      wrapper.Run()
  except PidStore.ArgsValidationError, e:
    # TODO(simon): print the format here
    print >> sys.stderr, e
    return None


if __name__ == '__main__':
  main()
