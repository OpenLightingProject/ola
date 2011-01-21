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


import cmd
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



class InteractiveModeController(cmd.Cmd):
  """Interactive mode!"""
  def __init__(self, universe, uid, sub_device, pid_file):
    """Create a new InteractiveModeController.

    Args:
      universe:
      uid:
      sub_device:
      pid_file:
    """
    cmd.Cmd.__init__(self)
    self._universe = universe
    self._uid = uid
    self._sub_device = sub_device

    self.pid_store = PidStore.GetStore(pid_file)
    self.wrapper = ClientWrapper()
    self.client = self.wrapper.Client()
    self.rdm_api = RDMAPI(self.client, self.pid_store)
    self._uids = []

    self.prompt = '> '

  def emptyline(self):
    pass

  def do_exit(self, s):
    """Exit the intrepreter."""
    return True

  def do_EOF(self, s):
    print ''
    return self.do_exit('')

  def do_uid(self, line):
    """Sets the active UID."""
    args = line.split()
    if len(args) != 1:
      print '*** Requires a single UID argument'
      return

    uid = UID.FromString(args[0])
    if uid is None:
      print '*** Invalid UID'
      return

    self._uid = uid

  def complete_uid(self, text, line, start_index, end_index):
    uids = [str(uid) for uid in self._uids if str(uid).startswith(text)]
    return uids

  def do_subdevice(self, line):
    """Sets the sub device."""
    args = line.split()
    if len(args) != 1:
      print '*** Requires a single int argument'
      return

    try:
      sub_device = int(args[0])
    except ValueError:
      print '*** Requires a single int argument'
      return

    if sub_device < 0 or sub_device > PidStore.ALL_SUB_DEVICES:
      print ('*** Argument must be between 0 and 0x%hx' %
             PidStore.ALL_SUB_DEVICES)
      return
    self._sub_device = sub_device

  def do_print(self, l):
    """Prints the current universe, UID and sub device."""
    print textwrap.dedent("""\
      Universe: %d
      UID: %s
      Sub Device: %d""" % (
        self._universe,
        self._uid,
        self._sub_device))

  def do_uids(self, l):
    """List the UIDs for this universe."""
    self.client.FetchUIDList(self._universe, self._DisplayUids)
    self.wrapper.Run()

  def do_discover(self, l):
    """Run RDM discovery for this universe."""
    self.client.RunRDMDiscovery(self._universe, self._DiscoveryDone)
    self.wrapper.Run()

  def do_list(self, l):
    """List the pids available."""
    names = []
    for pid in self.pid_store.Pids():
      names.append('%s (0x%04hx)' % (pid.name.lower(), pid.value))
    if self._uid:
      for pid in self.pid_store.ManufacturerPids(self._uid.manufacturer_id):
        names.append('%s (0x%04hx)' % (pid.name.lower(), pid.value))
    names.sort()
    print '\n'.join(names)

  def do_get(self, l):
    """Send a GET command."""
    self.GetOrSet(PidStore.RDM_GET, l)

  def complete_get(self, text, line, start_index, end_index):
    return self.CompleteGetOrSet(PidStore.RDM_GET, text, line)

  def do_set(self, l):
    """Send a SET command."""
    self.GetOrSet(PidStore.RDM_SET, l)

  def complete_set(self, text, line, start_index, end_index):
    return self.CompleteGetOrSet(PidStore.RDM_SET, text, line)

  def CompleteGetOrSet(self, request_type, text, line):
    if len(line.split(' ')) > 2:
      return []
    pids = [pid for pid in self.pid_store.Pids()
            if pid.name.lower().startswith(text)]
    if self._uid:
      for pid in self.pid_store.ManufacturerPids(self._uid.manufacturer_id):
        if pid.name.lower().startswith(text):
          pids.append(pid)

    # now check if this type of request is supported
    pid_names = [pid.name.lower() for pid in pids
                 if pid.RequestSupported(request_type)]

    pid_names.sort()
    return pid_names

  def GetOrSet(self, request_type, l):
    if self._uid is None:
      print '*** No UID selected, use the uid command'
      return

    args = l.split()
    command = 'get'
    if request_type == PidStore.RDM_SET:
      command = 'set'
    if len(args) < 1:
      print '%s <pid> [args]' % command
      return

    pid = None
    try:
      pid = self.pid_store.GetPid(int(args[0], 0),
                                  self._uid.manufacturer_id)
    except ValueError:
      pid = self.pid_store.GetName(args[0].upper(),
                                   self._uid.manufacturer_id)
    if pid is None:
      print '*** Unknown pid %s' % args[0]
      return

    rdm_args = args[1:]
    if not pid.RequestSupported(request_type):
      print '*** PID does not support command'
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
                self._RDMRequestComplete,
                args[1:]):
        self.wrapper.Run()
    except PidStore.ArgsValidationError, e:
      # TODO(simon): print the format here
      args, help_string = pid.GetRequestDescription(request_type)
      print 'Usage: %s %s %s' % (command, pid.name.lower(), args)
      print help_string
      print ''
      print '*** %s' % e
      return

  def _DisplayUids(self, state, uids):
    self._uids = []
    if state.Succeeded():
      for uid in uids:
        self._uids.append(uid)
        print str(uid)
    self.wrapper.Stop()

  def _DiscoveryDone(self, state):
    self.wrapper.Stop()

  def _RDMRequestComplete(self, status, pid_value, response_data,
                          unpack_exception):
    self.wrapper.Stop()
    if self._CheckStatus(status, unpack_exception):
      pid = self.pid_store.GetPid(pid_value,
                                  self._uid.manufacturer_id)
      if pid is None:
        print 'PID: 0x%04hx' % pid
      else:
        print pid
      for key, value in response_data.iteritems():
        print '%s: %s' % (key, value)

  def _CheckStatus(self, status, unpack_exception):
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

  if not universe and not list_pids:
    Usage()
    sys.exit()

  if not uid and not list_pids and not interactive_mode:
    Usage()
    sys.exit()

  controller = InteractiveModeController(universe, uid, sub_device, pid_file)
  if interactive_mode:
    sys.stdout.write('Available Uids:\n')
    controller.onecmd('uids')
    controller.cmdloop()
    sys.exit()

  if list_pids:
    controller.onecmd('list')
    sys.exit()

  if len(args) == 0:
    Usage()
    sys.exit()

  request_type = 'get'
  if os.path.basename(sys.argv[0]) == 'ola_rdm_set.py':
    request_type = 'set'
  controller.onecmd('%s %s' % (request_type, ' '.join(args)))


if __name__ == '__main__':
  main()
