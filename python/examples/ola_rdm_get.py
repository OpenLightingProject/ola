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
# ola_rdm_get.py
# Copyright (C) 2010 Simon Newton

'''Get a PID from a UID.'''

from __future__ import print_function
import cmd
import getopt
import os.path
import readline
import sys
import textwrap
from ola import PidStore
from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OlaClient, RDMNack
from ola.RDMAPI import RDMAPI
from ola.UID import UID

__author__ = 'nomis52@gmail.com (Simon Newton)'


def Usage():
  print(textwrap.dedent("""\
  Usage: ola_rdm_get.py --universe <universe> --uid <uid> <pid>

  Get the value of a pid for a device.
  Use 'ola_rdm_get --list-pids' to get a list of pids.

    -d, --sub-device <device> target a particular sub device (default is 0)
    -h, --help                display this help message and exit.
    -i, --interactive         interactive mode
    -l, --list-pids           display a list of pids
    -p, --pid-location        the directory to read PID definitions from
    --uid                     the UID to send to
    -u, --universe <universe> Universe number."""))


wrapper = None


class ResponsePrinter(object):
  def __init__(self):
    self.pid_store = PidStore.GetStore()
    # we can provide custom printers for PIDs below
    self._handlers = {
        'SUPPORTED_PARAMETERS': self.SupportedParameters,
        'PROXIED_DEVICES': self.ProxiedDevices,
    }

  def PrintResponse(self, uid, pid_value, response_data):
    pid = self.pid_store.GetPid(pid_value, uid.manufacturer_id)
    if pid is None:
      print('PID: 0x%04hx' % pid_value)
      self.Default(uid, response_data)
    else:
      print('PID: %s' % pid)
      if pid.name in self._handlers:
        self._handlers[pid.name](uid, response_data)
      else:
        self.Default(uid, response_data)

  def SupportedParameters(self, uid, response_data):
    params = sorted([p['param_id'] for p in response_data['params']])
    for pid_value in params:
      pid = self.pid_store.GetPid(pid_value, uid.manufacturer_id)
      if pid:
        print(pid)
      else:
        print('0x%hx' % pid_value)

  def ProxiedDevices(self, uid, response_data):
    uids = []
    for uid_data in response_data['uids']:
      uids.append(uid_data['uid'])
    for uid in sorted(uids):
      print(uid)

  def Default(self, uid, response_data):
    if isinstance(response_data, dict):
      for key, value in response_data.items():
        print('%s: %r' % (key, value))
    else:
      print(response_data)


class InteractiveModeController(cmd.Cmd):
  """Interactive mode!"""
  def __init__(self, universe, uid, sub_device, pid_location):
    """Create a new InteractiveModeController.

    Args:
      universe:
      uid:
      sub_device:
      pid_location:
    """
    cmd.Cmd.__init__(self)
    self._universe = universe
    self._uid = uid
    self._sub_device = sub_device
    self.pid_store = PidStore.GetStore(pid_location)
    self.wrapper = ClientWrapper()
    self.client = self.wrapper.Client()
    self.rdm_api = RDMAPI(self.client, self.pid_store)
    self._uids = []
    self._response_printer = ResponsePrinter()

    # tuple of (sub_device, command_class, pid)
    self._outstanding_request = None

    self.prompt = '> '

  def emptyline(self):
    pass

  def do_exit(self, s):
    """Exit the interpreter."""
    return True

  def do_EOF(self, s):
    print('')
    return self.do_exit('')

  def do_uid(self, line):
    """Sets the active UID."""
    args = line.split()
    if len(args) != 1:
      print('*** Requires a single UID argument')
      return

    uid = UID.FromString(args[0])
    if uid is None:
      print('*** Invalid UID')
      return

    if uid not in self._uids:
      print('*** UID not found')
      return

    self._uid = uid
    print('Fetching queued messages...')
    self._FetchQueuedMessages()
    self.wrapper.Run()

  def complete_uid(self, text, line, start_index, end_index):
    tokens = line.split()
    if len(tokens) > 1 and text == '':
      return []

    uids = [str(uid) for uid in self._uids if str(uid).startswith(text)]
    return uids

  def do_subdevice(self, line):
    """Sets the sub device."""
    args = line.split()
    if len(args) != 1:
      print('*** Requires a single int argument')
      return

    try:
      sub_device = int(args[0])
    except ValueError:
      print('*** Requires a single int argument')
      return

    if sub_device < 0 or sub_device > PidStore.ALL_SUB_DEVICES:
      print('*** Argument must be between 0 and 0x%hx' %
            PidStore.ALL_SUB_DEVICES)
      return
    self._sub_device = sub_device

  def do_print(self, line):
    """Prints the current universe, UID and sub device."""
    print(textwrap.dedent("""\
      Universe: %d
      UID: %s
      Sub Device: %d""" % (
        self._universe,
        self._uid,
        self._sub_device)))

  def do_uids(self, line):
    """List the UIDs for this universe."""
    self.client.FetchUIDList(self._universe, self._DisplayUids)
    self.wrapper.Run()

  def _DisplayUids(self, state, uids):
    self._uids = []
    if state.Succeeded():
      self._UpdateUids(uids)
      for uid in uids:
        print(str(uid))
    self.wrapper.Stop()

  def do_full_discovery(self, line):
    """Run full RDM discovery for this universe."""
    self.client.RunRDMDiscovery(self._universe, True, self._DiscoveryDone)
    self.wrapper.Run()

  def do_incremental_discovery(self, line):
    """Run incremental RDM discovery for this universe."""
    self.client.RunRDMDiscovery(self._universe, False, self._DiscoveryDone)
    self.wrapper.Run()

  def _DiscoveryDone(self, state, uids):
    if state.Succeeded():
      self._UpdateUids(uids)
    self.wrapper.Stop()

  def _UpdateUids(self, uids):
    self._uids = []
    for uid in uids:
      self._uids.append(uid)

  def do_list(self, line):
    """List the pids available."""
    names = []
    for pid in self.pid_store.Pids():
      names.append('%s (0x%04hx)' % (pid.name.lower(), pid.value))
    if self._uid:
      for pid in self.pid_store.ManufacturerPids(self._uid.manufacturer_id):
        names.append('%s (0x%04hx)' % (pid.name.lower(), pid.value))
    names.sort()
    print('\n'.join(names))

  def do_queued(self, line):
    """Fetch all the queued messages."""
    self._FetchQueuedMessages()
    self.wrapper.Run()

  def do_get(self, line):
    """Send a GET command."""
    self.GetOrSet(PidStore.RDM_GET, line)

  def complete_get(self, text, line, start_index, end_index):
    return self.CompleteGetOrSet(PidStore.RDM_GET, text, line)

  def do_set(self, line):
    """Send a SET command."""
    self.GetOrSet(PidStore.RDM_SET, line)

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
    pid_names = sorted([pid.name.lower() for pid in pids
                        if pid.RequestSupported(request_type)])

    return pid_names

  def GetOrSet(self, request_type, line):
    if self._uid is None:
      print('*** No UID selected, use the uid command')
      return

    args = line.split()
    command = 'get'
    if request_type == PidStore.RDM_SET:
      command = 'set'
    if len(args) < 1:
      print('%s <pid> [args]' % command)
      return

    pid = None
    try:
      pid = self.pid_store.GetPid(int(args[0], 0), self._uid.manufacturer_id)
    except ValueError:
      pid = self.pid_store.GetName(args[0].upper(), self._uid.manufacturer_id)
    if pid is None:
      print('*** Unknown pid %s' % args[0])
      return

    if not pid.RequestSupported(request_type):
      print('*** PID does not support command')
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
        self._outstanding_request = (self._sub_device, request_type, pid)
        self.wrapper.Run()
    except PidStore.ArgsValidationError as e:
      args, help_string = pid.GetRequestDescription(request_type)
      print('Usage: %s %s %s' % (command, pid.name.lower(), args))
      print(help_string)
      print('')
      print('*** %s' % e)
      return

  def _FetchQueuedMessages(self):
    """Fetch messages until the queue is empty."""
    pid = self.pid_store.GetName('QUEUED_MESSAGE', self._uid.manufacturer_id)
    self.rdm_api.Get(self._universe,
                     self._uid,
                     PidStore.ROOT_DEVICE,
                     pid,
                     self._QueuedMessageComplete,
                     ['error'])

  def _RDMRequestComplete(self, response, unpacked_data, unpack_exception):
    if not self._CheckForAckOrNack(response):
      return

    # at this stage the response is either a ack or nack
    self._outstanding_request = None
    self.wrapper.Stop()

    if response.response_type == OlaClient.RDM_NACK_REASON:
      print('Got nack with reason: %s' % response.nack_reason)
    elif unpack_exception:
      print(unpack_exception)
    else:
      self._response_printer.PrintResponse(self._uid, response.pid,
                                           unpacked_data)

    if response.queued_messages:
      print('%d queued messages remain' % response.queued_messages)

  def _QueuedMessageComplete(self, response, unpacked_data, unpack_exception):
    if not self._CheckForAckOrNack(response):
      return

    if response.response_type == OlaClient.RDM_NACK_REASON:
      if (self._outstanding_request and
          response.sub_device == self._outstanding_request[0] and
          response.command_class == self._outstanding_request[1] and
          response.pid == self._outstanding_request[2].value):
        # we found what we were looking for
        self._outstanding_request = None
        self.wrapper.StopIfNoEvents()

      elif (response.nack_reason == RDMNack.NR_UNKNOWN_PID and
            response.pid == self.pid_store.GetName('QUEUED_MESSAGE').value):
        print('Device doesn\'t support queued messages')
        self.wrapper.StopIfNoEvents()
      else:
        print('Got nack for 0x%04hx with reason: %s' % (
            response.pid, response.nack_reason))

    elif unpack_exception:
      print('Invalid Param data: %s' % unpack_exception)
    else:
      status_messages_pid = self.pid_store.GetName('STATUS_MESSAGES')
      if (response.pid == status_messages_pid.value and
          unpacked_data.get('messages', []) == []):
        self.wrapper.StopIfNoEvents()
        return

      self._response_printer.PrintResponse(self._uid,
                                           response.pid,
                                           unpacked_data)

    if response.queued_messages:
      self._FetchQueuedMessages()
    else:
      self.wrapper.StopIfNoEvents()

  def _CheckForAckOrNack(self, response):
    """Check for all the different error conditions.

    Returns:
      True if this response was an ACK or NACK, False for all other cases.
    """
    if not response.status.Succeeded():
      print(response.status.message)
      self.wrapper.Stop()
      return False

    if response.response_code != OlaClient.RDM_COMPLETED_OK:
      print(response.ResponseCodeAsString())
      self.wrapper.Stop()
      return False

    if response.response_type == OlaClient.RDM_ACK_TIMER:
      # schedule the fetch
      self.wrapper.AddEvent(response.ack_timer, self._FetchQueuedMessages)
      return False

    return True


def main():
  readline.set_completer_delims(' \t')
  try:
    opts, args = getopt.getopt(sys.argv[1:], 'd:hilp:u:',
                               ['sub-device=', 'help', 'interactive',
                                'list-pids', 'pid-location=', 'uid=',
                                'universe='])
  except getopt.GetoptError as err:
    print(str(err))
    Usage()
    sys.exit(2)

  universe = None
  uid = None
  sub_device = 0
  list_pids = False
  pid_location = None
  interactive_mode = False
  for o, a in opts:
    if o in ('-d', '--sub-device'):
      sub_device = int(a)
    elif o in ('-h', '--help'):
      Usage()
      sys.exit()
    elif o in ('-i', '--interactive'):
      interactive_mode = True
    elif o in ('-l', '--list-pids'):
      list_pids = True
    elif o in ('--uid',):
      uid = UID.FromString(a)
    elif o in ('-p', '--pid-location',):
      pid_location = a
    elif o in ('-u', '--universe'):
      universe = int(a)

  if universe is None and not list_pids:
    Usage()
    sys.exit()

  if not uid and not list_pids and not interactive_mode:
    Usage()
    sys.exit()

  # try to load the PID store so we fail early if we're missing PIDs
  try:
    PidStore.GetStore(pid_location)
  except PidStore.MissingPLASAPIDs as e:
    print(e)
    sys.exit()

  controller = InteractiveModeController(universe,
                                         uid,
                                         sub_device,
                                         pid_location)
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
