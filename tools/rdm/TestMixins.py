#  This program is free software; you can redistribute it and/or modify
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
# TestMixins.py
# Copyright (C) 2010 Simon Newton

'''Mixins used by the test definitions.

This module contains classes which can be inherited from to simplify writing
test definitions.
'''

__author__ = 'nomis52@gmail.com (Simon Newton)'

import struct
from collections import deque
from ola import PidStore
from ola.DUBDecoder import DecodeResponse
from ola.OlaClient import RDMNack
from ola.UID import UID
from ExpectedResults import *
from ResponderTest import ResponderTestFixture

MAX_LABEL_SIZE = 32
MAX_DMX_ADDRESS = 512

def UnsupportedSetNacks(pid):
  """Repsonders use either NR_UNSUPPORTED_COMMAND_CLASS or NR_UNKNOWN_PID."""
  return [
    NackSetResult(pid.value, RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
    NackSetResult(pid.value, RDMNack.NR_UNKNOWN_PID),
  ]


# Generic Get / Set Mixins
# These don't care about the format of the message.
#------------------------------------------------------------------------------
class UnsupportedGetMixin(object):
  """Check that Get fails with NR_UNSUPPORTED_COMMAND_CLASS."""
  def Test(self):
    self.AddIfGetSupported(
        self.NackGetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid)


class GetMixin(object):
  """GET Mixin that also sets a property if PROVIDES is set.

  The target class needs to set EXPECTED_FIELD and optionally PROVIDES.
  """
  def Test(self):
    self.AddIfGetSupported(self.AckGetResult(
      field_names=[self.EXPECTED_FIELD]))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if self.PROVIDES:
      value = None
      if response.WasAcked():
        value = fields[self.EXPECTED_FIELD]
      self.SetProperty(self.PROVIDES[0], value)


class GetRequiredMixin(object):
  """GET Mixin that also sets a property if PROVIDES is set.

  The target class needs to set EXPECTED_FIELD and optionally PROVIDES.
  """
  def Test(self):
    self.AddExpectedResults(self.AckGetResult(
      field_names=[self.EXPECTED_FIELD]))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if response.WasAcked() and self.PROVIDES:
      self.SetProperty(self.PROVIDES[0], fields[self.EXPECTED_FIELD])


class GetWithDataMixin(object):
  """GET a PID with random param data."""
  def Test(self):
    self.AddIfGetSupported([
      self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
      self.AckGetResult(
        warning='Get %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class GetWithNoDataMixin(object):
  """Attempt a get with no data."""
  def Test(self):
    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_FORMAT_ERROR))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid)


class AllSubDevicesGetMixin(object):
  """Attempt a get to ALL_SUB_DEVICES."""
  DATA = []

  def Test(self):
    # 9.2.2
    self.AddExpectedResults(
        self.NackGetResult(RDMNack.NR_SUB_DEVICE_OUT_OF_RANGE))
    self.SendGet(PidStore.ALL_SUB_DEVICES, self.pid, self.DATA)

class UnsupportedSetMixin(object):
  """Check that SET fails with NR_UNSUPPORTED_COMMAND_CLASS."""
  DATA = ''

  def Test(self):
    self.AddExpectedResults(UnsupportedSetNacks(self.pid))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class SetWithDataMixin(ResponderTestFixture):
  """SET a PID with random param data."""
  def Test(self):
    self.AddIfSetSupported([
      self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
      self.AckSetResult(
        warning='Set %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class SetWithNoDataMixin(object):
  """Attempt a set with no data."""
  def Test(self):
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, '')

  # TODO(simon): add a method to check this didn't change the value


# Generic Label Mixins
# These all work in conjunction with the IsSupportedMixin
#------------------------------------------------------------------------------
class SetLabelMixin(object):
  """Set a PID and make sure the value is saved.

  If PROVIDES is non empty, the first property will be used to indicate if the
  set action is supported. If an ack is returned it'll be set to true,
  otherwise false.
  """
  TEST_LABEL = 'test label'
  PROVIDES = []

  SET, VERIFY, RESET = xrange(3)

  def ExpectedResults(self):
    return [
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.AckSetResult(action=self.VerifySet)
    ]

  def Test(self):
    self._test_state = self.SET
    self.AddIfSetSupported(self.ExpectedResults())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.TEST_LABEL])

  def VerifySet(self):
    self._test_state = self.VERIFY
    self.AddExpectedResults(self.AckGetResult(field_names=['label']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if self._test_state == self.SET:
      if self.PROVIDES:
        self.SetProperty(self.PROVIDES[0], response.WasAcked())
      return
    elif self._test_state == self.RESET:
      return

    new_label = fields['label']
    if self.TEST_LABEL == new_label:
      return

    if (len(new_label) < len(self.TEST_LABEL) and
        self.TEST_LABEL.startswith(new_label)):
      self.AddAdvisory('Label for %s was truncated to %d characters' %
                       (self.pid, len(new_label)))
    else:
      self.SetFailed('Labels didn\'t match, expected "%s", got "%s"' %
                     (self.TEST_LABEL.encode('string-escape'),
                      new_label.encode('string-escape')))

  def ResetState(self):
    if not self.OldValue():
      return
    self._test_state = self.RESET
    self.AddExpectedResults(self.AckSetResult())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.OldValue()])
    self._wrapper.Run()


class NonUnicastSetLabelMixin(SetLabelMixin):
  """Send a SET device label to a broadcast or vendorcast uid."""
  def Test(self):
    if not self.Property('set_device_label_supported'):
      self.SetNotRun(' Previous set label was nacked')
      self.Stop()
      return

    self._test_state = self.SET
    self.AddExpectedResults(BroadcastResult(action=self.VerifySet))
    self.SendDirectedSet(self.Uid(), PidStore.ROOT_DEVICE, self.pid,
                         [self.TEST_LABEL])


class SetOversizedLabelMixin(object):
  """Send an over-sized SET label command."""
  LONG_STRING = 'this is a string which is more than 32 characters'

  def Test(self):
    self.verify_result = False
    self.AddIfSetSupported([
      self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
      self.NackSetResult(RDMNack.NR_PACKET_SIZE_UNSUPPORTED),
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.AckSetResult(action=self.VerifySet),
    ])
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.LONG_STRING)

  def VerifySet(self):
    """If we got an ACK back, we send a GET to check what the result was."""
    self.verify_result = True
    self.AddExpectedResults(self.AckGetResult(field_names=['label']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not self.verify_result:
      return

    if 'label' not in fields:
      self.SetFailed('Missing label field in response')
    else:
      if fields['label'] != self.LONG_STRING[0:MAX_LABEL_SIZE]:
        self.AddWarning(
            'Setting an oversized %s set the first %d characters' % (
            self.PID, len(fields['label'])))


# Generic Set Mixins
# These all work in conjunction with the IsSupportedMixin
#------------------------------------------------------------------------------
class SetMixin(object):
  """The base class for set mixins."""

  def OldValue(self):
    self.SetBroken('base method of SetMixin called')

  def NewValue(self):
    self.SetBroken('base method of SetMixin called')

  def Test(self):
    self.AddIfSetSupported([
      self.AckSetResult(action=self.VerifySet),
      self.NackSetResult(
        RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
        advisory='SET for %s returned unsupported command class' % self.PID),
    ])
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.NewValue()])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(field_values={self.EXPECTED_FIELD: self.NewValue()}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def ResetState(self):
    old_value = self.OldValue()
    if old_value is None:
      return
    self.AddExpectedResults(self.AckSetResult())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [old_value])
    self._wrapper.Run()


class SetBoolMixin(SetMixin):
  """Attempt to SET a bool field."""
  VALUE = True

  def NewValue(self):
    """Decide the new value to set based on the old one.
      This ensures we change it.
    """
    value = self.OldValue()
    if value is not None:
      return not value
    return self.VALUE


class SetUInt8Mixin(SetMixin):
  """Attempt to SET a uint8 field."""
  VALUE = True

  def NewValue(self):
    """Decide the new value to set based on the old one.
      This ensures we change it.
    """
    value = self.OldValue()
    if value is not None:
      return (value + 1) % 0xff
    return self.VALUE

class SetUInt16Mixin(SetMixin):
  """Attempt to SET a uint16 field."""
  VALUE = True

  def NewValue(self):
    """Decide the new value to set based on the old one.
      This ensures we change it.
    """
    value = self.OldValue()
    if value is not None:
      return (value + 1) % 0xffff
    return self.VALUE


class SetUInt32Mixin(SetMixin):
  """Attempt to SET a uint32 field."""
  VALUE = 100

  def NewValue(self):
    """Decide the new value to set based on the old one.
      This ensures we change it.
    """
    value = self.OldValue()
    if value is not None:
      return (value + 1) % 0xffffffff
    return self.VALUE


# Start address mixins
#------------------------------------------------------------------------------
class SetStartAddressMixin(object):
  """Set the dmx start address."""
  SET, VERIFY, RESET = xrange(3)

  def CalculateNewAddress(self, current_address, footprint):
    if footprint == MAX_DMX_ADDRESS:
      start_address = 1
    else:
      start_address = current_address + 1
      if start_address + footprint > MAX_DMX_ADDRESS + 1:
        start_address = 1
    return start_address

  def VerifySet(self):
    self._test_state = self.VERIFY
    self.AddExpectedResults(
      self.AckGetResult(field_values={'dmx_address': self.start_address},
                        action=self.VerifyDeviceInfo))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyDeviceInfo(self):
    device_info_pid = self.LookupPid('DEVICE_INFO')
    self.AddExpectedResults(
      AckGetResult(
        device_info_pid.value,
        field_values={'dmx_start_address': self.start_address}))
    self.SendGet(PidStore.ROOT_DEVICE, device_info_pid)

  def ResetState(self):
    old_address = self.Property('dmx_address')
    if not old_address or old_address == 0xffff:
      return
    self._test_state = self.RESET
    self.AddExpectedResults(self.AckSetResult())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid,
                 [self.Property('dmx_address')])
    self._wrapper.Run()


class SetNonUnicastStartAddressMixin(SetStartAddressMixin):
  """Send a set dmx start address to a non unicast uid."""

  def Test(self):
    footprint = self.Property('dmx_footprint')
    current_address = self.Property('dmx_address')
    if footprint == 0 or current_address == 0xffff:
      self.SetNotRun(" Device doesn't use a DMX address")
      self.Stop()
      return

    if not self.Property('set_dmx_address_supported'):
      self.SetNotRun(' Previous set start address was nacked')
      self.Stop()
      return

    self._test_state = self.SET
    self.start_address = self.CalculateNewAddress(current_address, footprint)
    self.AddExpectedResults(BroadcastResult(action=self.VerifySet))
    self.SendDirectedSet(self.Uid(), PidStore.ROOT_DEVICE, self.pid,
                         [self.start_address])

# Identify Device Mixin
#------------------------------------------------------------------------------
class SetNonUnicastIdentifyMixin(object):
  """Sets the identify device state.

  To avoid sending a broadcast identify on (which may strike all lamps in a
  large rig), we instead turn identify on and then send a broadcast off.
  """
  REQUIRES = ['identify_state']

  def States(self):
    return [
      self.TurnOn,
      self.VerifyOn,
      self.TurnOff,
      self.VerifyOff,
    ]

  def NextState(self):
    self._current_state += 1
    try:
      return self.States()[self._current_state]
    except IndexError:
      return None

  def Test(self):
    self._current_state = -1
    self.NextState()()

  def TurnOn(self):
    self.AddExpectedResults(
        self.AckSetResult(action=self.NextState()))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [True])

  def VerifyOn(self):
    self.AddExpectedResults(
        self.AckGetResult(field_values={'identify_state': True},
                          action=self.NextState()))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def TurnOff(self):
    self.AddExpectedResults(BroadcastResult(action=self.NextState()))
    self.SendDirectedSet(self.Uid(), PidStore.ROOT_DEVICE, self.pid, [False])

  def VerifyOff(self):
    self.AddExpectedResults(
        self.AckGetResult(field_values={'identify_state': False}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def ResetState(self):
    # reset back to the old value
    self.SendSet(PidStore.ROOT_DEVICE, self.pid,
                 [self.Property('identify_state')])
    self._wrapper.Run()


# Sensor mixins
#------------------------------------------------------------------------------
class SetUndefinedSensorValues(object):
  """Attempt to set sensor values for all sensors that weren't defined."""
  def Test(self):
    sensors = self.Property('sensor_definitions')
    self._missing_sensors = []
    for i in xrange(0, 0xff):
      if i not in sensors:
        self._missing_sensors.append(i)

    if self._missing_sensors:
      # loop and get all values
      self._DoAction()
    else:
      self.SetNotRun(' All sensors declared')
      self.Stop()
      return

  def _DoAction(self):
    if not self._missing_sensors:
      self.Stop()
      return

    self.AddIfSetSupported([
        self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE,
                           action=self._DoAction),
        # SET SENSOR_VALUE may not be supported
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
                           action=self._DoAction),
                           ])
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self._missing_sensors.pop(0)])

# Preset Status mixins
#------------------------------------------------------------------------------
class SetPresetStatusMixin(object):
  REQUIRES = ['preset_info']

  def BuildPresetStatus(self, scene):
    preset_info = self.Property('preset_info')
    fade_time = 0
    wait_time = 0
    if preset_info:
      fade_time = preset_info['min_preset_fade_time']
      wait_time = preset_info['min_preset_wait_time']

    return struct.pack('!HHHHB', scene, fade_time, fade_time, wait_time, 0)


# Discovery Mixins
#------------------------------------------------------------------------------
class DiscoveryMixin(ResponderTestFixture):
  """UnMute the device, send a DUB, confirm the UID, then mute again.

    This mixin requires:
      LowerBound() the lower UID to use in the DUB
      UpperBound() the upprt UID to use in the DUB

    And Optionally:
      DUBResponseCode(response_code): called when the discovery request
        completes.

      ExpectResponse(): returns true if we expect the device to answer the DUB
        request, false otherwise.

      Target:
        returns the UID to address the UID command to, defaults to
        ffff:ffffffff
  """
  PID = 'DISC_UNIQUE_BRANCH'
  REQUIRES = ['mute_supported', 'unmute_supported']

  def DUBResponseCode(self, response_code):
    pass

  def ExpectResponse(self):
    return True

  def Target(self):
    return UID.AllDevices()

  def UnMuteDevice(self, next_method):
    unmute_pid = self.LookupPid('DISC_UNMUTE')
    self.AddExpectedResults([
        AckDiscoveryResult(unmute_pid.value, action=next_method),
    ])
    self.SendDiscovery(PidStore.ROOT_DEVICE, unmute_pid)

  def Test(self):
    self._muting = True
    if not (self.Property('unmute_supported') and
            self.Property('mute_supported')):
      self.SetNotRun('Controller does not support mute / unmute commands')
      self.Stop()
      return

    self.UnMuteDevice(self.SendDUB)

  def SendDUB(self):
    self._muting = False
    results = [UnsupportedResult()]
    if self.ExpectResponse():
      results.append(DUBResult())
    else:
      results.append(TimeoutResult())
    self.AddExpectedResults(results)
    self.SendDirectedDiscovery(self.Target(),
                               PidStore.ROOT_DEVICE,
                               self.pid,
                               [self.LowerBound(), self.UpperBound()])

  def VerifyResult(self, response, fields):
    if self._muting:
      return

    self.DUBResponseCode(response.response_code)
    if (response.response_code ==
        OlaClient.RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED):
      return

    if not self.ExpectResponse():
      return

    if len(response.raw_response) != 1:
      self.SetFailed('Multiple DUB responses returned')
      return

    uid = DecodeResponse(bytearray(response.raw_response[0]))
    if uid is None or uid != self._uid:
      self.SetFailed('Missing UID in DUB response')

    self.LogDebug(' Located UID: %s' % uid)

  def ResetState(self):
    # mute the device again
    mute_pid = self.LookupPid('DISC_MUTE')
    self.SendDiscovery(PidStore.ROOT_DEVICE, mute_pid)
    self._wrapper.Run()
