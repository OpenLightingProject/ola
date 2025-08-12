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
# ModelCollector.py
# Copyright (C) 2011 Simon Newton

from __future__ import print_function

import logging

import ola.RDMConstants
from ola.OlaClient import OlaClient, RDMNack
from ola.RDMAPI import RDMAPI

from ola import PidStore

'''Quick script to collect information about responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


DEFAULT_LANGUAGE = "en"


class Error(Exception):
  """Base exception class."""


class DiscoveryException(Error):
  """Raised when discovery fails."""


class ModelCollector(object):
  """A controller that fetches data for responders."""

  (EMPTYING_QUEUE,
   DEVICE_INFO,
   MODEL_DESCRIPTION,
   SUPPORTED_PARAMS,
   SOFTWARE_VERSION_LABEL,
   PERSONALITIES,
   SENSORS,
   MANUFACTURER_PIDS,
   LANGUAGE,
   LANGUAGES,
   SLOT_INFO,
   SLOT_DESCRIPTION,
   SLOT_DEFAULT_VALUE) = range(13)

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

    # strip various info that is redundant
    for model_list in self.data.values():
      for model in model_list:
        for key in ['language', 'current_personality', 'personality_count',
                    'sensor_count']:
          if key in model:
            del model[key]
    return self.data

  def _ResetData(self):
    """Reset the internal data structures."""
    self.uids = []
    self.uid = None  # the current uid we're fetching from
    self.outstanding_pid = None
    self.work_state = None
    self.manufacturer_pids = []
    self.slots = []
    self.personalities = []
    self.sensors = []
    # keyed by manufacturer id
    self.data = {}

  def _GetDevice(self):
    return self.data[self.uid.manufacturer_id][-1]

  def _GetVersion(self):
    this_device = self._GetDevice()
    software_versions = this_device['software_versions']
    return software_versions[list(software_versions.keys())[0]]

  def _GetCurrentPersonality(self):
    this_device = self._GetDevice()
    this_version = self._GetVersion()
    personalities = this_version['personalities']
    return personalities[(this_device['current_personality'] - 1)]

  def _GetSlotData(self, slot):
    this_personality = self._GetCurrentPersonality()
    return this_personality.setdefault('slots', {}).setdefault(slot, {})

  def _GetLanguage(self):
    this_device = self._GetDevice()
    return this_device.get('language', DEFAULT_LANGUAGE)

  def _CheckPidSupported(self, pid):
    this_version = self._GetVersion()
    if pid.value in this_version['supported_parameters']:
      return True
    else:
      return False

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
    if self.work_state == self.DEVICE_INFO:
      self._HandleDeviceInfo(unpacked_data)
    elif self.work_state == self.MODEL_DESCRIPTION:
      self._HandleDeviceModelDescription(unpacked_data)
    elif self.work_state == self.SUPPORTED_PARAMS:
      self._HandleSupportedParams(unpacked_data)
    elif self.work_state == self.SOFTWARE_VERSION_LABEL:
      self._HandleSoftwareVersionLabel(unpacked_data)
    elif self.work_state == self.PERSONALITIES:
      self._HandlePersonalityData(unpacked_data)
    elif self.work_state == self.SENSORS:
      self._HandleSensorData(unpacked_data)
    elif self.work_state == self.MANUFACTURER_PIDS:
      self._HandleManufacturerPids(unpacked_data)
    elif self.work_state == self.LANGUAGE:
      self._HandleLanguage(unpacked_data)
    elif self.work_state == self.LANGUAGES:
      self._HandleLanguages(unpacked_data)
    elif self.work_state == self.SLOT_INFO:
      self._HandleSlotInfo(unpacked_data)
    elif self.work_state == self.SLOT_DESCRIPTION:
      self._HandleSlotDescription(unpacked_data)
    elif self.work_state == self.SLOT_DEFAULT_VALUE:
      self._HandleSlotDefaultValue(unpacked_data)

  def _HandleDeviceInfo(self, data):
    """Called when we get a DEVICE_INFO response."""
    this_device = self._GetDevice()
    fields = ['device_model',
              'product_category',
              'current_personality',
              'personality_count',
              'sensor_count',
              'sub_device_count']
    for field in fields:
      this_device[field] = data[field]

    this_device['software_versions'][data['software_version']] = {
        'languages': [],
        'personalities': [],
        'manufacturer_pids': [],
        'supported_parameters': [],
        'sensors': [],
    }

    self.personalities = list(range(1, data['personality_count'] + 1))
    self.sensors = list(range(0, data['sensor_count']))
    self._NextState()

  def _HandleDeviceModelDescription(self, data):
    """Called when we get a DEVICE_MODEL_DESCRIPTION response."""
    this_device = self._GetDevice()
    this_device['model_description'] = data['description']
    self._NextState()

  def _HandleSupportedParams(self, data):
    """Called when we get a SUPPORTED_PARAMETERS response."""
    this_version = self._GetVersion()
    for param_info in data['params']:
      this_version['supported_parameters'].append(param_info['param_id'])
      if (param_info['param_id'] >=
          ola.RDMConstants.RDM_MANUFACTURER_PID_MIN and
          param_info['param_id'] <=
          ola.RDMConstants.RDM_MANUFACTURER_PID_MAX):
        self.manufacturer_pids.append(param_info['param_id'])
    self._NextState()

  def _HandleSoftwareVersionLabel(self, data):
    """Called when we get a SOFTWARE_VERSION_LABEL response."""
    this_version = self._GetVersion()
    this_version['label'] = data['label']
    self._NextState()

  def _HandlePersonalityData(self, data):
    """Called when we get a DMX_PERSONALITY_DESCRIPTION response."""
    this_version = self._GetVersion()
    this_version['personalities'].append({
      'description': data['name'],
      'index': data['personality'],
      'slot_count': data['slots_required'],
    })
    self._FetchNextPersonality()

  def _HandleSensorData(self, data):
    """Called when we get a SENSOR_DEFINITION response."""
    this_version = self._GetVersion()
    this_version['sensors'].append({
      'index': data['sensor_number'],
      'description': data['name'],
      'type': data['type'],
      'supports_recording': data['supports_recording'],
    })
    self._FetchNextSensor()

  def _HandleManufacturerPids(self, data):
    """Called when we get a PARAMETER_DESCRIPTION response."""
    this_version = self._GetVersion()
    this_version['manufacturer_pids'].append({
      'pid': data['pid'],
      'pdl_size': data['pdl_size'],
      'description': data['description'],
      'command_class': data['command_class'],
      'data_type': data['data_type'],
      'unit': data['unit'],
      'prefix': data['prefix'],
      'min_value': data['min_value'],
      'default_value': data['default_value'],
      'max_value': data['max_value'],
    })
    self._FetchNextManufacturerPid()

  def _HandleLanguage(self, data):
    """Called when we get a LANGUAGE response."""
    this_device = self._GetDevice()
    this_device['language'] = data['language']
    self._NextState()

  def _HandleLanguages(self, data):
    """Called when we get a LANGUAGE_CAPABILITIES response."""
    this_version = self._GetVersion()
    for language in data['languages']:
      this_version['languages'].append(language['language'])
    self._NextState()

  def _HandleSlotInfo(self, data):
    """Called when we get a SLOT_INFO response."""
    for slot in data['slots']:
      this_slot_data = self._GetSlotData(slot['slot_offset'])
      this_slot_data['label_id'] = slot['slot_label_id']
      this_slot_data['type'] = slot['slot_type']
      self.slots.append(slot['slot_offset'])
    self._NextState()

  def _HandleSlotDescription(self, data):
    """Called when we get a SLOT_DESCRIPTION response."""
    if data is not None:
      # Got valid data, not a nack
      this_slot_data = self._GetSlotData(data['slot_number'])
      this_slot_data.setdefault('name', {})[self._GetLanguage()] = data['name']
    self._FetchNextSlotDescription()

  def _HandleSlotDefaultValue(self, data):
    """Called when we get a DEFAULT_SLOT_VALUE response."""
    for slot in data['slot_values']:
      this_slot_data = self._GetSlotData(slot['slot_offset'])
      this_slot_data['default_value'] = slot['default_slot_value']
    self._NextState()

  def _NextState(self):
    """Move to the next state of information fetching."""
    if self.work_state == self.EMPTYING_QUEUE:
      # fetch device info
      pid = self.pid_store.GetName('DEVICE_INFO')
      self._GetPid(pid)
      self.work_state = self.DEVICE_INFO
    elif self.work_state == self.DEVICE_INFO:
      # fetch device model description
      pid = self.pid_store.GetName('DEVICE_MODEL_DESCRIPTION')
      self._GetPid(pid)
      self.work_state = self.MODEL_DESCRIPTION
    elif self.work_state == self.MODEL_DESCRIPTION:
      # fetch supported params
      pid = self.pid_store.GetName('SUPPORTED_PARAMETERS')
      self._GetPid(pid)
      self.work_state = self.SUPPORTED_PARAMS
    elif self.work_state == self.SUPPORTED_PARAMS:
      # fetch software version label
      pid = self.pid_store.GetName('SOFTWARE_VERSION_LABEL')
      self._GetPid(pid)
      self.work_state = self.SOFTWARE_VERSION_LABEL
    elif self.work_state == self.SOFTWARE_VERSION_LABEL:
      self.work_state = self.PERSONALITIES
      self._FetchNextPersonality()
    elif self.work_state == self.PERSONALITIES:
      self.work_state = self.SENSORS
      self._FetchNextSensor()
    elif self.work_state == self.SENSORS:
      self.work_state = self.MANUFACTURER_PIDS
      self._FetchNextManufacturerPid()
    elif self.work_state == self.MANUFACTURER_PIDS:
      self.work_state = self.LANGUAGE
      pid = self.pid_store.GetName('LANGUAGE')
      if self._CheckPidSupported(pid):
        self._GetPid(pid)
      else:
        logging.debug("Skipping pid %s as it's not supported on this device" %
                      pid)
        self._NextState()
    elif self.work_state == self.LANGUAGE:
      # fetch language capabilities
      self.work_state = self.LANGUAGES
      pid = self.pid_store.GetName('LANGUAGE_CAPABILITIES')
      if self._CheckPidSupported(pid):
        self._GetPid(pid)
      else:
        logging.debug("Skipping pid %s as it's not supported on this device" %
                      pid)
        self._NextState()
    elif self.work_state == self.LANGUAGES:
      # fetch slot info
      self.work_state = self.SLOT_INFO
      pid = self.pid_store.GetName('SLOT_INFO')
      if self._CheckPidSupported(pid):
        self._GetPid(pid)
      else:
        logging.debug("Skipping pid %s as it's not supported on this device" %
                      pid)
        self._NextState()
    elif self.work_state == self.SLOT_INFO:
      self.work_state = self.SLOT_DESCRIPTION
      self._FetchNextSlotDescription()
    elif self.work_state == self.SLOT_DESCRIPTION:
      # fetch slot default value
      self.work_state = self.SLOT_DEFAULT_VALUE
      pid = self.pid_store.GetName('DEFAULT_SLOT_VALUE')
      if self._CheckPidSupported(pid):
        self._GetPid(pid)
      else:
        logging.debug("Skipping pid %s as it's not supported on this device" %
                      pid)
        self._NextState()
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
    devices = self.data.setdefault(self.uid.manufacturer_id, [])
    devices.append({
      'software_versions': {},
    })

    self.work_state = self.EMPTYING_QUEUE
    if self.skip_queued_messages:
      # proceed to the fetch now
      self._NextState()
    else:
      self.queued_message_failures = 0
      self._FetchQueuedMessages()

  def _FetchNextPersonality(self):
    """Fetch the info for the next personality, or proceed to the next state if
       there are none left.
    """
    pid = self.pid_store.GetName('DMX_PERSONALITY_DESCRIPTION')
    if self._CheckPidSupported(pid):
      if self.personalities:
        personality_index = self.personalities.pop(0)
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
    else:
      if self.personalities:
        # If we have personalities but no description, we still need the basic
        # data structure to add the other info to
        this_version = self._GetVersion()
        for personality_index in self.personalities:
          this_version['personalities'].append({
            'index': personality_index,
          })
      logging.debug("Skipping pid %s as it's not supported on this device" %
                    pid)
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

  def _FetchNextManufacturerPid(self):
    """Fetch the info for the next manufacturer PID, or proceed to the next
       state if there are none left.
    """
    if self.manufacturer_pids:
      manufacturer_pid = self.manufacturer_pids.pop(0)
      pid = self.pid_store.GetName('PARAMETER_DESCRIPTION')
      self.rdm_api.Get(self.universe,
                       self.uid,
                       PidStore.ROOT_DEVICE,
                       pid,
                       self._RDMRequestComplete,
                       [manufacturer_pid])
      logging.debug('Sent PARAMETER_DESCRIPTION request')
      self.outstanding_pid = pid
    else:
      self._NextState()

  def _FetchNextSlotDescription(self):
    """Fetch the description for the next slot, or proceed to the next state if
       there are none left.
    """
    pid = self.pid_store.GetName('SLOT_DESCRIPTION')
    if self._CheckPidSupported(pid):
      if self.slots:
        slot = self.slots.pop(0)
        self.rdm_api.Get(self.universe,
                         self.uid,
                         PidStore.ROOT_DEVICE,
                         pid,
                         self._RDMRequestComplete,
                         [slot])
        logging.debug('Sent SLOT_DESCRIPTION request for slot %d' % slot)
        self.outstanding_pid = pid
      else:
        self._NextState()
    else:
      logging.debug("Skipping pid %s as it's not supported on this device" %
                    pid)
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

  def _RDMRequestComplete(self, response, unpacked_data, unpack_exception):
    if not self._CheckForAckOrNack(response):
      return

    # at this stage the response is either a ack or nack
    # We have to allow nacks from SLOT_DESCRIPTION, as it may not have a
    # description for every slot
    if (response.response_type == OlaClient.RDM_NACK_REASON and
        response.pid != self.pid_store.GetName('SLOT_DESCRIPTION').value):
      print('Got nack with reason for pid %s: %s' %
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
        logging.debug('Received matching queued message, response was NACK')
        self.outstanding_pid = None
        self._NextState()

      elif (response.nack_reason == RDMNack.NR_UNKNOWN_PID and
            response.pid == self.pid_store.GetName('QUEUED_MESSAGE').value):
        logging.debug('Device doesn\'t support queued messages')
        self._NextState()
      else:
        print('Got nack for 0x%04hx with reason: %s' %
              (response.pid, response.nack_reason))

    elif unpack_exception:
      print('Invalid Param data: %s' % unpack_exception)
      self.queued_message_failures += 1
      if self.queued_message_failures >= 10:
        # declare this bad and move on
        self._NextState()
      else:
        self._FetchQueuedMessages()
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
