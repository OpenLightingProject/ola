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
# rdm_snapshot.py
# Copyright (C) 2012 Simon Newton

import getopt
import logging
import pickle
import pprint
import sys
import textwrap

from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OlaClient, RDMNack
from ola.RDMAPI import RDMAPI
from ola.UID import UID

from ola import PidStore

'''Quick script to collect the settings from a rig.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


class Error(Exception):
  """Base exception class."""


class DiscoveryException(Error):
  """Raised when discovery fails."""


class SaveException(Error):
  """Raised when we can't write to the output file."""


class LoadException(Error):
  """Raised when we can't write to the output file."""


class ConfigReader(object):
  """A controller that fetches data for responders."""

  (EMPTYING_QUEUE, DMX_START_ADDRESS, DEVICE_LABEL, PERSONALITY) = range(4)

  def __init__(self, wrapper, pid_store):
    self.wrapper = wrapper
    self.pid_store = pid_store
    self.client = self.wrapper.Client()
    self.rdm_api = RDMAPI(self.client, self.pid_store)

  def Run(self, universe, skip_queued_messages):
    """Run the collector.

    Args:
      universe: The universe to collect
      skip_queued_messages:
    """
    self.universe = universe
    self.skip_queued_messages = skip_queued_messages
    self._ResetData()

    self.client.RunRDMDiscovery(self.universe, True, self._HandleUIDList)
    self.wrapper.Run()
    return self.data

  def _ResetData(self):
    """Reset the internal data structures."""
    self.uids = []
    self.uid = None  # the current uid we're fetching from
    self.outstanding_pid = None
    self.work_state = None
    # uid: {'start_address': '', 'personality': '', 'label': ''}
    self.data = {}

  def _GetPid(self, pid):
      self.rdm_api.Get(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       pid,
                       self._RDMRequestComplete)
      logging.debug('Sent %s request' % pid)
      self.outstanding_pid = pid

  def _HandleUIDList(self, state, uids):
    """Called when the UID list arrives."""
    if not state.Succeeded():
      raise DiscoveryException(state.message)

    found_uids = set()
    for uid in uids:
      found_uids.add(uid)
      logging.debug(uid)

    self.uids = list(found_uids)
    self._FetchNextUID()

  def _HandleResponse(self, unpacked_data):
    logging.debug(unpacked_data)
    if self.work_state == self.DMX_START_ADDRESS:
      self._HandleStartAddress(unpacked_data)
    elif self.work_state == self.DEVICE_LABEL:
      self._HandleDeviceLabel(unpacked_data)
    elif self.work_state == self.PERSONALITY:
      self._HandlePersonality(unpacked_data)

  def _HandleStartAddress(self, data):
    """Called when we get a DMX_START_ADDRESS response."""
    this_device = self.data.setdefault(str(self.uid), {})
    this_device['dmx_start_address'] = data['dmx_address']
    self._NextState()

  def _HandleDeviceLabel(self, data):
    """Called when we get a DEVICE_LABEL response."""
    this_device = self.data.setdefault(str(self.uid), {})
    this_device['label'] = data['label']
    self._NextState()

  def _HandlePersonality(self, data):
    """Called when we get a DMX_PERSONALITY response."""
    this_device = self.data.setdefault(str(self.uid), {})
    this_device['personality'] = data['current_personality']
    self._NextState()

  def _NextState(self):
    """Move to the next state of information fetching."""
    if self.work_state == self.EMPTYING_QUEUE:
      # fetch start address
      pid = self.pid_store.GetName('DMX_START_ADDRESS')
      self._GetPid(pid)
      self.work_state = self.DMX_START_ADDRESS
    elif self.work_state == self.DMX_START_ADDRESS:
      # fetch device label
      pid = self.pid_store.GetName('DEVICE_LABEL')
      self._GetPid(pid)
      self.work_state = self.DEVICE_LABEL
    elif self.work_state == self.DEVICE_LABEL:
      # fetch personality
      pid = self.pid_store.GetName('DMX_PERSONALITY')
      self._GetPid(pid)
      self.work_state = self.PERSONALITY
    else:
      # this one is done, onto the next UID
      self._FetchNextUID()

  def _FetchNextUID(self):
    """Start fetching the info for the next UID."""
    if not self.uids:
      self.wrapper.Stop()
      return

    self.uid = self.uids.pop()
    self.personalities = []
    self.sensors = []
    logging.debug('Fetching data for %s' % self.uid)

    self.work_state = self.EMPTYING_QUEUE
    if self.skip_queued_messages:
      # proceed to the fetch now
      self._NextState()
    else:
      self._FetchQueuedMessages()

  def _FetchNextPersonality(self):
    """Fetch the info for the next personality, or proceed to the next state if
       there are none left.
    """
    if self.personalities:
      personality_index = self.personalities.pop(0)
      pid = self.pid_store.GetName('DMX_PERSONALITY_DESCRIPTION')
      self.rdm_api.Get(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       pid,
                       self._RDMRequestComplete,
                       [personality_index])
      logging.debug('Sent DMX_PERSONALITY_DESCRIPTION request')
      self.outstanding_pid = pid
    else:
      self._NextState()

  def _FetchNextSensor(self):
    """Fetch the info for the next sensor, or proceed to the next state if
       there are none left.
    """
    if self.sensors:
      sensor_index = self.sensors.pop(0)
      pid = self.pid_store.GetName('SENSOR_DEFINITION')
      self.rdm_api.Get(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       pid,
                       self._RDMRequestComplete,
                       [sensor_index])
      logging.debug('Sent SENSOR_DEFINITION request')
      self.outstanding_pid = pid
    else:
      self._NextState()

  def _FetchQueuedMessages(self):
    """Fetch messages until the queue is empty."""
    pid = self.pid_store.GetName('QUEUED_MESSAGE')
    self.rdm_api.Get(self.universe,
                     self.uid,
                     PidStore.ROOT_DEVICE,
                     pid,
                     self._QueuedMessageComplete,
                     ['advisory'])
    logging.debug('Sent GET QUEUED_MESSAGE')

  def _QueuedMessageFound(self):
    if self.work_state == self.EMPTYING_QUEUE:
      self._FetchQueuedMessages()

  def _RDMRequestComplete(self, response, unpacked_data, unpack_exception):
    if not self._CheckForAckOrNack(response):
      return

    # at this stage the response is either a ack or nack
    if response.response_type == OlaClient.RDM_NACK_REASON:
      print('Got nack with reason for PID %d: %s' %
            (response.pid, response.nack_reason))
      self._NextState()
    elif unpack_exception:
      print(unpack_exception)
      self.wrapper.Stop()
    else:
      self._HandleResponse(unpacked_data)

  def _QueuedMessageComplete(self, response, unpacked_data, unpack_exception):
    """Called when a queued message is returned."""
    if not self._CheckForAckOrNack(response):
      return

    if response.response_type == OlaClient.RDM_NACK_REASON:
      if (self.outstanding_pid and
          response.command_class == OlaClient.RDM_GET_RESPONSE and
          response.pid == self.outstanding_pid.value):
        # we found what we were looking for
        print("found, but nacked")
        self.outstanding_pid = None
        self._NextState()

      elif (response.nack_reason == RDMNack.NR_UNKNOWN_PID and
            response.pid == self.pid_store.GetName('QUEUED_MESSAGE').value):
        logging.debug('Device doesn\'t support queued messages')
        self._NextState()
      else:
        print('Got nack for 0x%04hx with reason: %s' % (
            response.pid, response.nack_reason))

    elif unpack_exception:
      print('Invalid Param data: %s' % unpack_exception)
    else:
      status_messages_pid = self.pid_store.GetName('STATUS_MESSAGES')
      queued_message_pid = self.pid_store.GetName('QUEUED_MESSAGE')
      if (response.pid == status_messages_pid.value and
          unpacked_data.get('messages', []) == []):
        logging.debug('Got back empty list of STATUS_MESSAGES')
        self._NextState()
        return
      elif response.pid == queued_message_pid.value:
        logging.debug('Got back QUEUED_MESSAGE, this is a bad responder')
        self._NextState()
        return

      logging.debug('Got pid 0x%hx' % response.pid)
      if self.outstanding_pid and response.pid == self.outstanding_pid.value:
        self._HandleResponse(unpacked_data)
      else:
        self._FetchQueuedMessages()

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
      logging.debug('Got ack timer for %d ms' % response.ack_timer)
      self.wrapper.AddEvent(response.ack_timer, self._FetchQueuedMessages)
      return False
    return True


class ConfigWriter(object):
  """A controller that applies configuration to a universe."""
  (DMX_START_ADDRESS, DEVICE_LABEL, PERSONALITY, COMPLETE) = range(4)

  def __init__(self, wrapper, pid_store):
    self.wrapper = wrapper
    self.pid_store = pid_store
    self.client = self.wrapper.Client()
    self.rdm_api = RDMAPI(self.client, self.pid_store)

  def Run(self, universe, configuration):
    """Run the collector.

    Args:
      universe: The universe to collect
      configuration: The config to apply
    """
    self.universe = universe
    self.configuration = configuration
    self.uids = list(configuration.keys())

    self.client.RunRDMDiscovery(self.universe, True, self._HandleUIDList)
    self.wrapper.Run()

  def _HandleUIDList(self, state, uids):
    """Called when the UID list arrives."""
    if not state.Succeeded():
      raise DiscoveryException(state.message)

    found_uids = set()
    for uid in uids:
      found_uids.add(uid)
      logging.debug(uid)

    for uid in self.configuration.keys():
      if uid not in found_uids:
        print('Device %s has been removed' % uid)
    self._SetNextUID()

  def _SetNextUID(self):
    """Start setting the info for the next UID."""
    if not self.uids:
      self.wrapper.Stop()
      return

    self.uid = self.uids.pop()
    print('Doing %s' % self.uid)
    self.work_state = self.DMX_START_ADDRESS
    self._NextState()

  def _NextState(self):
    """Move to the next state of information fetching."""
    if self.work_state == self.DMX_START_ADDRESS:
      address = self.configuration[self.uid].get('dmx_start_address')
      self.work_state = self.DEVICE_LABEL
      if address is not None:
        pid = self.pid_store.GetName('DMX_START_ADDRESS')
        self._SetPid(pid, [address])
        return

    if self.work_state == self.DEVICE_LABEL:
      label = self.configuration[self.uid].get('label')
      self.work_state = self.PERSONALITY
      if label is not None:
        pid = self.pid_store.GetName('DEVICE_LABEL')
        self._SetPid(pid, [label])
        return

    if self.work_state == self.PERSONALITY:
      personality = self.configuration[self.uid].get('personality')
      self.work_state = self.COMPLETE
      if personality is not None:
        pid = self.pid_store.GetName('DMX_PERSONALITY')
        self._SetPid(pid, [personality])
        return

    # this one is done, onto the next UID
    self._SetNextUID()

  def _SetPid(self, pid, values):
      self.rdm_api.Set(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       pid,
                       self._RDMRequestComplete,
                       values)
      logging.debug('Sent %s request' % pid)
      self.outstanding_pid = pid

  def _RDMRequestComplete(self, response, unpacked_data, unpack_exception):
    if not response.status.Succeeded():
      print(response.status.message)
      self.wrapper.Stop()
      return

    if response.response_code != OlaClient.RDM_COMPLETED_OK:
      print(response.ResponseCodeAsString())
      self.wrapper.Stop()
      return

    if response.response_type == OlaClient.RDM_ACK_TIMER:
      # schedule the fetch
      logging.debug('Got ack timer for %d ms' % response.ack_timer)
      self.wrapper.AddEvent(response.ack_timer, self._FetchQueuedMessages)
      return

    # at this stage the response is either a ack or nack
    if response.response_type == OlaClient.RDM_NACK_REASON:
      print('Got nack with reason: %s' % response.nack_reason)
    self._NextState()


def Usage():
  print("Usage: rdm_snapshot.py --universe <universe> [--input <file>] "
        "[--output <file>]\n")
  print(textwrap.dedent("""\
  Save and restore RDM settings for a universe. This includes the start address,
  personality and device label.

  Fetch and save a configuration:
    rdm_snapshot.py -u 1 --output /tmp/save

  Restore configuration:
    rdm_snapshot.py -u 1 --input /tmp/save

    -d, --debug               Print extra debug info.
    -h, --help                Display this help message and exit.
    -i, --input               File to read configuration from.
    -p, --pid-location        The directory to read PID definitions from.
    -o, --output              File to save configuration to.
    --skip-queued-messages    Don't attempt to fetch queued messages for the
                              device.
    -u, --universe <universe> Universe number."""))


def WriteToFile(filename, output):
  try:
    log_file = open(filename, 'w')
  except IOError as e:
    raise SaveException(
        'Failed to write to %s: %s' % (filename, e.message))

  pickle.dump(output, log_file)
  logging.info('Wrote log file %s' % (log_file.name))
  log_file.close()


def ReadFile(filename):
  try:
    f = open(filename, 'rb')
  except IOError as e:
    raise LoadException(e)

  raw_data = pickle.load(f)
  f.close()
  data = {}
  for uid, settings in raw_data.items():
    data[UID.FromString(uid)] = settings
  return data


def main():
  try:
    opts, args = getopt.getopt(
        sys.argv[1:],
        'dhi:o:p:u:',
        ['debug', 'help', 'input=', 'skip-queued-messages', 'output=',
         'pid-location=', 'universe='])
  except getopt.GetoptError as err:
    print(str(err))
    Usage()
    sys.exit(2)

  universe = None
  output_file = None
  input_file = None
  pid_location = None
  level = logging.INFO
  skip_queued_messages = False
  for o, a in opts:
    if o in ('-d', '--debug'):
      level = logging.DEBUG
    elif o in ('-h', '--help'):
      Usage()
      sys.exit()
    elif o in ('-i', '--input'):
      input_file = a
    elif o in ('--skip-queued-messages'):
      skip_queued_messages = True
    elif o in ('-o', '--output'):
      output_file = a
    elif o in ('-p', '--pid-location',):
      pid_location = a
    elif o in ('-u', '--universe'):
      universe = int(a)

  if universe is None:
    Usage()
    sys.exit()

  if input_file and output_file:
    print('Only one of --input and --output can be provided.')
    sys.exit()

  logging.basicConfig(
      level=level,
      format='%(message)s')

  client_wrapper = ClientWrapper()
  pid_store = PidStore.GetStore(pid_location)

  if input_file:
    configuration = ReadFile(input_file)
    if configuration:
      writer = ConfigWriter(client_wrapper, pid_store)
      writer.Run(universe, configuration)

  else:
    controller = ConfigReader(client_wrapper, pid_store)
    data = controller.Run(universe, skip_queued_messages)

    if output_file:
      WriteToFile(output_file, data)
    else:
      pprint.pprint(data)


if __name__ == '__main__':
  main()
