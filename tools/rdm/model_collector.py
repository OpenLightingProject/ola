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
from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OlaClient, RDMNack
from ola.RDMAPI import RDMAPI
from ola.UID import UID

"""
Still to collect:
   - supported params
   - personality info
"""

def Usage():
  print textwrap.dedent("""\
  Usage: model_collector.py --universe <universe>

  Collect information about responders attached to a universe and output in a
  format that can be imported into the RDM manufacturer index
  (http://rdm.openlighting.org/)

    -d, --debug               Print extra debug info.
    -h, --help                Display this help message and exit.
    --pid_file                The PID data store to use.
    -u, --universe <universe> Universe number.""")


class ModelCollectorController(object):
  """A controller that fetches data for responders."""

  (EMPTYING_QUEUE, DEVICE_INFO, MODEL_DESCRIPTION, SUPPORTED_PARAMS,
   SOFTWARE_VERSION_LABEL) = xrange(5)

  def __init__(self, universe, pid_file):
    self.universe = universe
    self.pid_store = PidStore.GetStore(pid_file)
    self.wrapper = ClientWrapper()
    self.client = self.wrapper.Client()
    self.rdm_api = RDMAPI(self.client, self.pid_store)
    self.uids = []
    self.uid = None  # the current uid we're fetching from
    self.outstanding_pid = None
    self.work_state = None

    # keyed by manufacturer id
    self.data = {}

  def Run(self):
    """Run the collector."""
    self.client.FetchUIDList(self.universe, self._HandleUIDList)
    self.wrapper.Run()
    pprint.pprint(self.data)

  def _HandleUIDList(self, state, uids):
    """Called when the UID list arrives."""
    if not state.Succeeded():
      self.wrapper.Stop()
      return

    found_uids = set()
    for uid in uids:
      found_uids.add(uid)
      logging.debug(uid)

    self.uids = list(found_uids)
    self._FetchNextUID()

  def _HandleResponse(self, unpacked_data):
    logging.debug(unpacked_data)
    if self.work_state == self.DEVICE_INFO:
      self._HandleDeviceInfo(unpacked_data)
    elif self.work_state == self.MODEL_DESCRIPTION:
      self._HandleDeviceModelDescription(unpacked_data)
    elif self.work_state == self.SUPPORTED_PARAMS:
      self._HandleSupportedParams(unpacked_data)
    elif self.work_state == self.SOFTWARE_VERSION_LABEL:
      self._HandleSoftwareVersionLabel(unpacked_data)

  def _HandleDeviceInfo(self, data):
    """Called when we get a DEVICE_INFO response."""
    this_device = self.data[self.uid.manufacturer_id][-1]
    fields = ['device_model',
              'product_category',
              'personality_count']
    for field in fields:
      this_device[field] = data[field]

    this_device['software_versions'][data['software_version']] = {
        'personalities': [],
        'supported_parameters': [],
        'sensors': [],
    }
    self._NextState()

  def _HandleDeviceModelDescription(self, data):
    """Called when we get a DEVICE_MODEL_DESCRIPTION response."""
    this_device = self.data[self.uid.manufacturer_id][-1]
    this_device['model_description'] = data['description']
    self._NextState()

  def _HandleSupportedParams(self, data):
    """Called when we get a SUPPORTED_PARAMETERS response."""
    this_device = self.data[self.uid.manufacturer_id][-1]
    software_versions = this_device['software_versions']
    this_version = software_versions[software_versions.keys()[0]]
    for param_info in data['params']:
      this_version['supported_parameters'].append(param_info['param_id'])
    self._NextState()

  def _HandleSoftwareVersionLabel(self, data):
    """Called when we get a SOFTWARE_VERSION_LABEL response."""
    this_device = self.data[self.uid.manufacturer_id][-1]
    software_versions = this_device['software_versions']
    this_version = software_versions[software_versions.keys()[0]]
    this_version['label'] = data['label']
    self._NextState()

  def _NextState(self):
    """Move to the next state of information fetching."""
    if self.work_state == self.EMPTYING_QUEUE:
      # fetch device info
      device_info_pid = self.pid_store.GetName('DEVICE_INFO')
      self.rdm_api.Get(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       device_info_pid,
                       self._RDMRequestComplete)
      logging.debug('Sent DEVICE_INFO request')
      self.outstanding_pid = device_info_pid
      self.work_state = self.DEVICE_INFO
    elif self.work_state == self.DEVICE_INFO:
      # fetch device model description
      model_description_pid = self.pid_store.GetName(
          'DEVICE_MODEL_DESCRIPTION')
      self.rdm_api.Get(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       model_description_pid,
                       self._RDMRequestComplete)
      logging.debug('Sent DEVICE_MODEL_DESCRIPTION request')
      self.outstanding_pid = model_description_pid
      self.work_state = self.MODEL_DESCRIPTION
    elif self.work_state == self.MODEL_DESCRIPTION:
      # fetch supported params
      supported_params_pid = self.pid_store.GetName(
          'SUPPORTED_PARAMETERS')
      self.rdm_api.Get(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       supported_params_pid,
                       self._RDMRequestComplete)
      logging.debug('Sent SUPPORTED_PARAMETERS request')
      self.outstanding_pid = supported_params_pid
      self.work_state = self.SUPPORTED_PARAMS
    elif self.work_state == self.SUPPORTED_PARAMS:
      # fetch supported params
      pid = self.pid_store.GetName('SOFTWARE_VERSION_LABEL')
      self.rdm_api.Get(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       pid,
                       self._RDMRequestComplete)
      logging.debug('Sent SOFTWARE_VERSION_LABEL request')
      self.outstanding_pid = pid
      self.work_state = self.SOFTWARE_VERSION_LABEL
    else:
      # this one is done, onto the next UID
      self._FetchNextUID()

  def _FetchNextUID(self):
    """Start fetching the info for the next UID."""
    if not self.uids:
      self.wrapper.Stop()
      return

    self.uid = self.uids.pop()
    logging.debug('Fetching data for %s' % self.uid)
    self.work_state = self.EMPTYING_QUEUE
    devices = self.data.setdefault(self.uid.manufacturer_id, [])
    devices.append({
      'software_versions': {},
    })
    self._FetchQueuedMessages()

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
      print 'Got nack with reason: %s' % response.nack_reason
      self._NextState()
    elif unpack_exception:
      print unpack_exception
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
        print "found, but nacked"
        self.outstanding_pid = None
        self._NextState()

      elif (response.nack_reason == RDMNack.NR_UNKNOWN_PID and
            response.pid == self.pid_store.GetName('QUEUED_MESSAGE').value):
        logging.debug('Device doesn\'t support queued messages')
        self._NextState()
      else:
        print 'Got nack for 0x%04hx with reason: %s' % (
            response.pid, response.nack_reason)

    elif unpack_exception:
      print 'Invalid Param data: %s' % unpack_exception
    else:
      status_message_pid = self.pid_store.GetName('STATUS_MESSAGE')
      queued_message_pid = self.pid_store.GetName('QUEUED_MESSAGE')
      if (response.pid == status_message_pid.value and
          unpacked_data.get('messages', []) == []):
        logging.debug('Got back empty list of STATUS_MESSAGE')
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
      print response.status.message
      self.wrapper.Stop()
      return False

    if response.response_code != OlaClient.RDM_COMPLETED_OK:
      print response.ResponseCodeAsString()
      self.wrapper.Stop()
      return False

    if response.response_type == OlaClient.RDM_ACK_TIMER:
      # schedule the fetch
      logging.debug('Got ack timer for %d ms' % response.ack_timer)
      self.wrapper.AddEvent(response.ack_timer, self._FetchQueuedMessages)
      return False
    return True


def main():
  try:
    opts, args = getopt.getopt(sys.argv[1:],
                               'dhp:u:',
                               ['debug', 'help', 'pid_file=', 'universe='])
  except getopt.GetoptError, err:
    print str(err)
    Usage()
    sys.exit(2)

  universe = None
  pid_file = None
  level = logging.INFO
  for o, a in opts:
    if o in ('-d', '--debug'):
      level = logging.DEBUG
    elif o in ('-h', '--help'):
      Usage()
      sys.exit()
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

  controller = ModelCollectorController(universe, pid_file)
  controller.Run()

if __name__ == '__main__':
  main()
