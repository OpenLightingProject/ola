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
# TestMixins.py
# Copyright (C) 2010 Simon Newton

import struct

from ola.DMXConstants import DMX_UNIVERSE_SIZE
from ola.DUBDecoder import DecodeResponse
from ola.OlaClient import OlaClient, RDMNack
from ola.PidStore import ROOT_DEVICE
from ola.RDMConstants import RDM_MAX_STRING_LENGTH
from ola.StringUtils import StringEscape
from ola.testing.rdm.ExpectedResults import (AckDiscoveryResult, AckGetResult,
                                             BroadcastResult, DUBResult,
                                             TimeoutResult, UnsupportedResult)
from ola.testing.rdm.ResponderTest import ResponderTestFixture
from ola.testing.rdm.TestCategory import TestCategory
from ola.testing.rdm.TestHelpers import ContainsUnprintable
from ola.UID import UID

from ola import PidStore

'''Mixins used by the test definitions.

This module contains classes which can be inherited from to simplify writing
test definitions.
'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


MAX_DMX_ADDRESS = DMX_UNIVERSE_SIZE


# Generic GET Mixins
# These don't care about the format of the message.
# -----------------------------------------------------------------------------
class UnsupportedGetMixin(ResponderTestFixture):
  """Check that Get fails with NR_UNSUPPORTED_COMMAND_CLASS."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    self.AddIfGetSupported(
        self.NackGetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid)


class UnsupportedGetWithDataMixin(ResponderTestFixture):
  """Check that GET with random param data fails with
    NR_UNSUPPORTED_COMMAND_CLASS.
  """
  CATEGORY = TestCategory.ERROR_CONDITIONS
  DATA = b'foo'

  def Test(self):
    self.AddIfGetSupported(
        self.NackGetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class AllSubDevicesUnsupportedGetMixin(ResponderTestFixture):
  """Check that a GET to ALL_SUB_DEVICES fails with
    NR_UNSUPPORTED_COMMAND_CLASS.
  """
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    # E1.20, section 9.2.2
    results = [self.NackGetResult(RDMNack.NR_SUB_DEVICE_OUT_OF_RANGE),
               # Some devices check the command class before the sub device.
               self.NackGetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)]
    if not self.PidSupported():
      # Some devices check the PID first.
      results.append(self.NackGetResult(RDMNack.NR_UNKNOWN_PID))
    self.AddExpectedResults(results)
    self.SendRawGet(PidStore.ALL_SUB_DEVICES, self.pid)


class GetMixin(ResponderTestFixture):
  """GET Mixin for an optional PID. Verify EXPECTED_FIELDS is in the response.

    This mixin also sets one or more properties if PROVIDES is defined.  The
    target class needs to defined EXPECTED_FIELDS and optionally PROVIDES.

    If ALLOWED_NACKS is non-empty, this adds a custom NackGetResult to the list
    of allowed results for each entry.
  """
  ALLOWED_NACKS = []
  EXPECTED_FIELDS = None

  def Test(self):
    if self.EXPECTED_FIELDS is None:
      self.SetBroken('No EXPECTED_FIELDS given for %s' %
                     (self.__class__.__name__))
      return

    results = [self.AckGetResult(field_names=self.EXPECTED_FIELDS)]
    for nack in self.ALLOWED_NACKS:
      results.append(self.NackGetResult(nack))
    self.AddIfGetSupported(results)
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if response.WasAcked() and self.PROVIDES:
      for i in range(0, min(len(self.PROVIDES), len(self.EXPECTED_FIELDS))):
        self.SetProperty(self.PROVIDES[i], fields[self.EXPECTED_FIELDS[i]])


class GetStringMixin(GetMixin):
  """GET Mixin for an optional string PID. Verify EXPECTED_FIELDS are in the
    response.

    This mixin also sets a property if PROVIDES is defined.  The target class
    needs to defined EXPECTED_FIELDS and optionally PROVIDES.
  """
  MIN_LENGTH = 0
  MAX_LENGTH = RDM_MAX_STRING_LENGTH

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    string_field = fields[self.EXPECTED_FIELDS[0]]

    if self.PROVIDES:
      self.SetProperty(self.PROVIDES[0], string_field)

    if ContainsUnprintable(string_field):
      self.AddAdvisory(
          '%s field in %s contains unprintable characters, was %s' %
          (self.EXPECTED_FIELDS[0].capitalize(), self.pid.name,
           StringEscape(string_field)))

    if self.MIN_LENGTH and len(string_field) < self.MIN_LENGTH:
      self.SetFailed(
          '%s field in %s was shorter than expected, was %d, expected %d' %
          (self.EXPECTED_FIELDS[0].capitalize(), self.pid.name,
           len(string_field), self.MIN_LENGTH))

    if self.MAX_LENGTH and len(string_field) > self.MAX_LENGTH:
      self.SetFailed(
          '%s field in %s was longer than expected, was %d, expected %d' %
          (self.EXPECTED_FIELDS[0].capitalize(), self.pid.name,
           len(string_field), self.MAX_LENGTH))


class GetRequiredMixin(ResponderTestFixture):
  """GET Mixin for a required PID. Verify EXPECTED_FIELDS is in the response.

    This mixin also sets a property if PROVIDES is defined.  The target class
    needs to defined EXPECTED_FIELDS and optionally PROVIDES.
  """
  EXPECTED_FIELDS = None

  def Test(self):
    if self.EXPECTED_FIELDS is None:
      self.SetBroken('No EXPECTED_FIELDS given for %s' %
                     (self.__class__.__name__))
      return

    self.AddExpectedResults(
        self.AckGetResult(field_names=self.EXPECTED_FIELDS))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if response.WasAcked() and self.PROVIDES:
      for i in range(0, min(len(self.PROVIDES), len(self.EXPECTED_FIELDS))):
        self.SetProperty(self.PROVIDES[i], fields[self.EXPECTED_FIELDS[i]])


class GetRequiredStringMixin(GetRequiredMixin):
  """GET Mixin for a required string PID. Verify EXPECTED_FIELDS is in the
    response.

    This mixin also sets a property if PROVIDES is defined.  The target class
    needs to defined EXPECTED_FIELDS and optionally PROVIDES.
  """
  MIN_LENGTH = 0
  MAX_LENGTH = RDM_MAX_STRING_LENGTH

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    string_field = fields[self.EXPECTED_FIELDS[0]]

    if self.PROVIDES:
      for i in range(0, min(len(self.PROVIDES), len(self.EXPECTED_FIELDS))):
        self.SetProperty(self.PROVIDES[i], fields[self.EXPECTED_FIELDS[i]])

    if ContainsUnprintable(string_field):
      self.AddAdvisory(
          '%s field in %s contains unprintable characters, was %s' %
          (self.EXPECTED_FIELDS[0].capitalize(), self.pid.name,
           StringEscape(string_field)))

    if self.MIN_LENGTH and len(string_field) < self.MIN_LENGTH:
      self.SetFailed(
          '%s field in %s was shorter than expected, was %d, expected %d' %
          (self.EXPECTED_FIELDS[0].capitalize(), self.pid.name,
           len(string_field), self.MIN_LENGTH))

    if self.MAX_LENGTH and len(string_field) > self.MAX_LENGTH:
      self.SetFailed(
          '%s field in %s was longer than expected, was %d, expected %d' %
          (self.EXPECTED_FIELDS[0].capitalize(), self.pid.name,
           len(string_field), self.MAX_LENGTH))


class GetWithDataMixin(ResponderTestFixture):
  """GET a PID with junk param data.

    If ALLOWED_NACKS is non-empty, this adds a custom NackGetResult to the list
    of allowed results for each entry.
  """
  CATEGORY = TestCategory.ERROR_CONDITIONS
  DATA = b'foo'
  ALLOWED_NACKS = []

  def Test(self):
    results = [
      self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
      self.AckGetResult(
        warning='Get %s with data returned an ack' % self.pid.name)
    ]
    for nack in self.ALLOWED_NACKS:
      results.append(self.NackGetResult(nack))
    self.AddIfGetSupported(results)
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class GetMandatoryPIDWithDataMixin(ResponderTestFixture):
  """GET a mandatory PID with junk param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  DATA = b'foo'

  def Test(self):
    # PID must return something as this PID is required (can't return
    # unsupported)
    self.AddExpectedResults([
      self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
      self.AckGetResult(
        warning='Get %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class GetWithNoDataMixin(ResponderTestFixture):
  """GET with no data, expect NR_FORMAT_ERROR."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_FORMAT_ERROR))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid)


class AllSubDevicesGetMixin(ResponderTestFixture):
  """Send a GET to ALL_SUB_DEVICES."""
  CATEGORY = TestCategory.SUB_DEVICES
  DATA = []

  def Test(self):
    # E1.20, section 9.2.2
    results = [self.NackGetResult(RDMNack.NR_SUB_DEVICE_OUT_OF_RANGE)]
    # Some devices check the PID before the sub device.
    if not self.PidSupported():
      results.append(self.NackGetResult(RDMNack.NR_UNKNOWN_PID))
    self.AddExpectedResults(results)
    self.SendGet(PidStore.ALL_SUB_DEVICES, self.pid, self.DATA)


# Generic SET Mixins
# These don't care about the format of the message.
# -----------------------------------------------------------------------------
class UnsupportedSetMixin(ResponderTestFixture):
  """Check that SET fails with NR_UNSUPPORTED_COMMAND_CLASS."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    self.AddIfSetSupported(
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid)


class UnsupportedSetWithDataMixin(ResponderTestFixture):
  """Check that SET with random param data fails with
    NR_UNSUPPORTED_COMMAND_CLASS.
  """
  CATEGORY = TestCategory.ERROR_CONDITIONS
  DATA = b'foo'

  def Test(self):
    self.AddIfSetSupported(
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class SetWithDataMixin(ResponderTestFixture):
  """SET a PID with junk param data.

    If ALLOWED_NACKS is non-empty, this adds a custom NackSetResult to the list
    of allowed results for each entry.
  """
  CATEGORY = TestCategory.ERROR_CONDITIONS
  DATA = b'foo'
  ALLOWED_NACKS = []

  def Test(self):
    results = [
      self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
      # TODO(Peter): Fix this, ideally we change behaviour based on past
      # support of the PID
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.AckSetResult(
        warning='Set %s with data returned an ack' % self.pid.name)
    ]
    for nack in self.ALLOWED_NACKS:
      results.append(self.NackSetResult(nack))
    self.AddIfSetSupported(results)
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.DATA)

  # TODO(simon): add a method to check this didn't change the value


class SetWithNoDataMixin(ResponderTestFixture):
  """Attempt a set with no data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    results = [
      # TODO(Peter): Fix this, ideally we change behaviour based on past
      # support of the PID
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.NackSetResult(RDMNack.NR_FORMAT_ERROR)
    ]
    self.AddIfSetSupported(results)
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, b'')

  # TODO(simon): add a method to check this didn't change the value


# Generic Label Mixins
# -----------------------------------------------------------------------------
class SetLabelMixin(ResponderTestFixture):
  """Set a PID and make sure the label is updated.

  If PROVIDES is non empty, the first property will be used to indicate if the
  set action is supported. If an ack is returned it'll be set to true,
  otherwise false.
  """
  TEST_LABEL = 'test label'
  PROVIDES = []

  SET, VERIFY, RESET = range(3)

  def OldValue(self):
    self.SetBroken('Base OldValue method of SetLabelMixin called')

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
                       (self.pid.name, len(new_label)))
    else:
      self.SetFailed('Labels didn\'t match, expected "%s", got "%s"' %
                     (StringEscape(self.TEST_LABEL),
                      StringEscape(new_label)))

  def ResetState(self):
    old_value = self.OldValue()
    if old_value is None:
      return
    self._test_state = self.RESET
    self.AddExpectedResults(self.AckSetResult())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [old_value])
    self._wrapper.Run()


class SetNonUnicastLabelMixin(SetLabelMixin):
  """Send a SET device label to a broadcast or vendorcast UID."""
  def Uid(self):
    self.SetBroken('Base method of SetNonUnicastLabelMixin called')

  def Test(self):
    target_uid = self.Uid()
    if target_uid is None:
      return

    if not self.Property('set_device_label_supported'):
      self.SetNotRun('Previous set label was nacked')
      self.Stop()
      return

    self._test_state = self.SET
    self.AddExpectedResults(BroadcastResult(action=self.VerifySet))
    self.SendDirectedSet(target_uid, PidStore.ROOT_DEVICE, self.pid,
                         [self.TEST_LABEL])


class SetOversizedLabelMixin(ResponderTestFixture):
  """Send an over-sized SET label command."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  LONG_STRING = b'this is a string which is more than 32 characters'

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
      if fields['label'] != self.LONG_STRING[0:RDM_MAX_STRING_LENGTH]:
        self.AddWarning(
            'Setting an oversized %s set the first %d characters' %
            (self.pid.name, len(fields['label'])))


# Generic Set Mixins
# -----------------------------------------------------------------------------
class SetMixin(ResponderTestFixture):
  """The base class for set mixins."""
  EXPECTED_FIELDS = None

  def OldValue(self):
    self.SetBroken('Base OldValue method of SetMixin called')

  def NewValue(self):
    self.SetBroken('Base NewValue method of SetMixin called')

  def Test(self):
    if self.EXPECTED_FIELDS is None:
      self.SetBroken('No EXPECTED_FIELDS given for %s' %
                     (self.__class__.__name__))
      return

    new_value = self.NewValue()
    if new_value is None:
      return

    self.AddIfSetSupported([
      self.AckSetResult(action=self.VerifySet),
      self.NackSetResult(
        RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
        advisory='SET for %s returned unsupported command class' %
                 self.pid.name),
    ])
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [new_value])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(
          field_names=self.EXPECTED_FIELDS,
          field_values={self.EXPECTED_FIELDS[0]: self.NewValue()}))
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
# -----------------------------------------------------------------------------
class SetDMXStartAddressMixin(ResponderTestFixture):
  """Set the dmx start address."""
  SET, VERIFY, RESET = range(3)
  start_address = 1

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


class SetNonUnicastDMXStartAddressMixin(SetDMXStartAddressMixin):
  """Send a set dmx start address to a non unicast UID."""

  def Uid(self):
    self.SetBroken('Base method of SetNonUnicastDMXStartAddressMixin called')

  def Test(self):
    target_uid = self.Uid()
    if target_uid is None:
      return

    footprint = self.Property('dmx_footprint')
    current_address = self.Property('dmx_address')
    if footprint == 0 or current_address == 0xffff:
      self.SetNotRun("Device doesn't use a DMX address")
      self.Stop()
      return

    if not self.Property('set_dmx_address_supported'):
      self.SetNotRun('Previous set start address was NAcked')
      self.Stop()
      return

    self._test_state = self.SET
    self.start_address = self.CalculateNewAddress(current_address, footprint)
    self.AddExpectedResults(BroadcastResult(action=self.VerifySet))
    self.SendDirectedSet(target_uid, PidStore.ROOT_DEVICE, self.pid,
                         [self.start_address])


# Identify Device Mixin
# -----------------------------------------------------------------------------
class SetNonUnicastIdentifyMixin(ResponderTestFixture):
  """Sets the identify device state.

  To avoid sending a broadcast identify on (which may strike all lamps in a
  large rig), we instead turn identify on and then send a broadcast off.
  """
  REQUIRES = ['identify_state']

  def Uid(self):
    self.SetBroken('Base method of SetNonUnicastIdentifyMixin called')

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
    target_uid = self.Uid()
    if target_uid is None:
      self.Stop()
      return

    self.AddExpectedResults(BroadcastResult(action=self.NextState()))
    self.SendDirectedSet(target_uid, PidStore.ROOT_DEVICE, self.pid, [False])

  def VerifyOff(self):
    self.AddExpectedResults(
        self.AckGetResult(field_values={'identify_state': False}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def ResetState(self):
    # reset back to the old value, this doesn't reset any other devices
    # affected by the broadcast identify off
    self.SendSet(PidStore.ROOT_DEVICE, self.pid,
                 [self.Property('identify_state')])
    self._wrapper.Run()


# Sensor mixins
# -----------------------------------------------------------------------------
class SetUndefinedSensorValues(ResponderTestFixture):
  """Attempt to set sensor values for all sensors that weren't defined."""
  def Test(self):
    sensors = self.Property('sensor_definitions')
    self._missing_sensors = []
    for i in range(0, 0xff):
      if i not in sensors:
        self._missing_sensors.append(i)

    if self._missing_sensors:
      # loop and get all values
      self._DoAction()
    else:
      self.SetNotRun('All sensors declared')
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
# -----------------------------------------------------------------------------
class SetOutOfRangePresetStatusMixin(ResponderTestFixture):
  """Set an out of range scene for PRESET_STATUS"""
  PID = 'PRESET_STATUS'
  REQUIRES = ['preset_info']

  def BuildPresetStatus(self, scene):
    preset_info = self.Property('preset_info')
    fade_time = 0
    wait_time = 0
    if preset_info:
      fade_time = preset_info['min_preset_fade_time']
      wait_time = preset_info['min_preset_wait_time']

    return struct.pack('!HHHHB', scene, int(fade_time), int(fade_time),
                       int(wait_time), 0)

  def PresetStatusSceneNumber(self):
    self.SetBroken('Base method of SetOutOfRangePresetStatusMixin called')
    return

  def Test(self):
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    scene_number = self.PresetStatusSceneNumber()
    if scene_number is None:
      return

    data = self.BuildPresetStatus(scene_number)
    self.SendRawSet(ROOT_DEVICE, self.pid, data)


# Discovery Mixins
# -----------------------------------------------------------------------------
class DiscoveryMixin(ResponderTestFixture):
  """UnMute the device, send a DUB, confirm the UID, then mute again.

    This mixin requires:
      LowerBound() the lower UID to use in the DUB
      UpperBound() the upper UID to use in the DUB

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
  # Global Mute here ensures we run after all devices have been muted
  REQUIRES = ['mute_supported', 'unmute_supported', 'global_mute']

  def LowerBound(self):
    self.SetBroken('Base LowerBound method of DiscoveryMixin called')

  def UpperBound(self):
    self.SetBroken('Base UpperBound method of DiscoveryMixin called')

  def DUBResponseCode(self, response_code):
    pass

  def ExpectResponse(self):
    return True

  def Target(self):
    return UID.AllDevices()

  def UnMuteDevice(self, next_method):
    unmute_pid = self.LookupPid('DISC_UN_MUTE')
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
    lower_bound = self.LowerBound()
    upper_bound = self.UpperBound()
    if lower_bound is None or upper_bound is None:
      return

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
                               [lower_bound, upper_bound])

  def VerifyResult(self, response, fields):
    if self._muting:
      return

    self.DUBResponseCode(response.response_code)
    if (response.response_code ==
        OlaClient.RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED):
      return

    if not self.ExpectResponse():
      return

    if len(response.frames) != 1:
      self.SetFailed('Multiple DUB responses returned')
      return

    uid = DecodeResponse(bytearray(response.frames[0].data))
    if uid is None or uid != self._uid:
      self.SetFailed('Missing UID in DUB response')

    self.LogDebug(' Located UID: %s' % uid)

  def ResetState(self):
    # mute the device again
    mute_pid = self.LookupPid('DISC_MUTE')
    self.SendDiscovery(PidStore.ROOT_DEVICE, mute_pid)
    self._wrapper.Run()


# E1.37-1 Mixins
# -----------------------------------------------------------------------------
class SetDMXFailModeMixin(ResponderTestFixture):
  PID = 'DMX_FAIL_MODE'
  REQUIRES = ['dmx_fail_settings', 'preset_info', 'set_dmx_fail_mode_supported']
  CATEGORY = TestCategory.DMX_SETUP

  INFINITE_TIME = 6553.5  # 0xffff * 10^-1 (multiplier)

  def ResetState(self):
    if not self.PidSupported():
      return

    settings = self.Property('dmx_fail_settings')
    if settings is None:
      self.SetBroken('Failed to restore DMX_FAIL_MODE settings')
      return

    for key in ('scene_number', 'hold_time', 'loss_of_signal_delay', 'level'):
      if key not in settings:
        self.SetBroken(
            'Failed to restore DMX_FAIL_MODE settings, missing %s' % key)
        return

    self.SendSet(
        ROOT_DEVICE, self.pid,
        [settings['scene_number'], settings['loss_of_signal_delay'],
         settings['hold_time'], settings['level']])
    self._wrapper.Run()


class SetDMXStartupModeMixin(ResponderTestFixture):
  PID = 'DMX_STARTUP_MODE'
  REQUIRES = ['dmx_startup_settings', 'preset_info',
              'set_dmx_startup_mode_supported']
  CATEGORY = TestCategory.DMX_SETUP

  INFINITE_TIME = 6553.5  # 0xffff * 10^-1 (multiplier)

  def ResetState(self):
    if not self.PidSupported():
      return

    settings = self.Property('dmx_startup_settings')
    if settings is None:
      self.SetBroken('Failed to restore DMX_STARTUP_MODE settings')
      return

    for key in ('scene_number', 'hold_time', 'startup_delay', 'level'):
      if key not in settings:
        self.SetBroken(
            'Failed to restore DMX_STARTUP_MODE settings, missing %s' % key)
        return

    self.SendSet(
        ROOT_DEVICE, self.pid,
        [settings['scene_number'], settings['startup_delay'],
         settings['hold_time'], settings['level']])
    self._wrapper.Run()


class SetMaximumLevelMixin(ResponderTestFixture):
  PID = 'MAXIMUM_LEVEL'
  REQUIRES = ['maximum_level', 'set_maximum_level_supported']
  CATEGORY = TestCategory.DIMMER_SETTINGS

  def ResetState(self):
    if (not self.PidSupported() or
        not self.Property('set_maximum_level_supported')):
      return

    level = self.Property('maximum_level')
    if level is not None:
      self.SendSet(ROOT_DEVICE, self.pid, [level])
      self._wrapper.Run()


class SetMinimumLevelMixin(ResponderTestFixture):
  PID = 'MINIMUM_LEVEL'
  REQUIRES = ['minimum_level_settings', 'set_minimum_level_supported',
              'split_levels_supported']
  CATEGORY = TestCategory.DIMMER_SETTINGS

  def MinLevelIncreasing(self):
    return self.settings['minimum_level_increasing']

  def MinLevelDecreasing(self):
    return self.settings['minimum_level_decreasing']

  def OnBelowMin(self):
    return self.settings['on_below_minimum']

  def ExpectedResults(self):
    return self.AckSetResult(action=self.GetMinLevel)

  def ShouldSkip(self):
    return False

  def Test(self):
    self.settings = self.Property('minimum_level_settings')
    if self.settings is None:
      self.SetNotRun('Unable to determine current minimum level settings')
      return

    set_supported = self.Property('set_minimum_level_supported')
    if set_supported is None or not set_supported:
      self.SetNotRun('SET MINIMUM_LEVEL not supported')
      return

    if self.ShouldSkip():
      return

    self.AddIfSetSupported(self.ExpectedResults())
    self.SendSet(
        ROOT_DEVICE, self.pid,
        [self.MinLevelIncreasing(),
         self.MinLevelDecreasing(),
         self.OnBelowMin()])

  def GetMinLevel(self):
    min_level_decreasing = self.MinLevelDecreasing()
    if not self.Property('split_levels_supported'):
      min_level_decreasing = self.MinLevelIncreasing()
    self.AddIfGetSupported(self.AckGetResult(
        field_values={
          'minimum_level_increasing': self.MinLevelIncreasing(),
          'minimum_level_decreasing': min_level_decreasing,
          'on_below_minimum': self.OnBelowMin()
        }
    ))
    self.SendGet(ROOT_DEVICE, self.pid)

  def ResetState(self):
    if not self.PidSupported():
      return

    self.SendSet(
        ROOT_DEVICE, self.pid,
        [self.settings['minimum_level_increasing'],
         self.settings['minimum_level_decreasing'],
         self.settings['on_below_minimum']])
    self._wrapper.Run()


class GetZeroMixin(ResponderTestFixture):
  """Send a get to index 0, expect NR_DATA_OUT_OF_RANGE"""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  DATA = None

  def Test(self):
    if self.DATA is None:
      self.SetBroken('No DATA given for %s' % self.__class__.__name__)
      return

    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendRawGet(ROOT_DEVICE, self.pid, self.DATA)


class GetZeroUInt8Mixin(GetZeroMixin):
  """Get a UInt8 parameter with value 0, expect NR_DATA_OUT_OF_RANGE"""
  DATA = struct.pack('!B', 0)


class GetZeroUInt16Mixin(GetZeroMixin):
  """Get a UInt16 parameter with value 0, expect NR_DATA_OUT_OF_RANGE"""
  DATA = struct.pack('!H', 0)


class GetZeroUInt32Mixin(GetZeroMixin):
  """Get a UInt32 parameter with value 0, expect NR_DATA_OUT_OF_RANGE"""
  DATA = struct.pack('!I', 0)


class SetZeroMixin(ResponderTestFixture):
  """Send a set to index 0, expect NR_DATA_OUT_OF_RANGE"""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  DATA = None

  def Test(self):
    if self.DATA is None:
      self.SetBroken('No DATA given for %s' % self.__class__.__name__)
      return

    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendRawSet(ROOT_DEVICE, self.pid, self.DATA)


class SetZeroUInt8Mixin(SetZeroMixin):
  """Set a UInt8 parameter with value 0, expect NR_DATA_OUT_OF_RANGE"""
  DATA = struct.pack('!B', 0)


class SetZeroUInt16Mixin(SetZeroMixin):
  """Set a UInt16 parameter with value 0, expect NR_DATA_OUT_OF_RANGE"""
  DATA = struct.pack('!H', 0)


class SetZeroUInt32Mixin(SetZeroMixin):
  """Set a UInt32 parameter with value 0, expect NR_DATA_OUT_OF_RANGE"""
  DATA = struct.pack('!I', 0)


class GetOutOfRangeUInt8Mixin(ResponderTestFixture):
  """The subclass provides the NumberOfSettings() method."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  LABEL = None

  def NumberOfSettings(self):
    # By default we use the first property from REQUIRES
    return self.Property(self.REQUIRES[0])

  def Test(self):
    if self.LABEL is None:
      self.SetBroken('No LABEL given for %s' % self.__class__.__name__)
      return

    settings_supported = self.NumberOfSettings()
    if settings_supported is None:
      self.SetNotRun('Unable to determine number of %s' % self.LABEL)
      return

    if settings_supported == 255:
      self.SetNotRun('All %s are supported' % self.LABEL)
      return

    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendGet(ROOT_DEVICE, self.pid, [settings_supported + 1])


class SetOutOfRangeUInt8Mixin(ResponderTestFixture):
  """The subclass provides the NumberOfSettings() method."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  LABEL = None

  def NumberOfSettings(self):
    # By default we use the first property from REQUIRES
    return self.Property(self.REQUIRES[0])

  def Test(self):
    if self.LABEL is None:
      self.SetBroken('No LABEL given for %s' % self.__class__.__name__)
      return

    settings_supported = self.NumberOfSettings()
    if settings_supported is None:
      self.SetNotRun('Unable to determine number of %s' % self.LABEL)
      return

    if settings_supported == 255:
      self.SetNotRun('All %s are supported' % self.LABEL)
      return

    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(ROOT_DEVICE, self.pid, [settings_supported + 1])


class GetSettingDescriptionsMixin(ResponderTestFixture):
  """Perform a GET for each setting in a list.

    The list is returned by ListOfSettings which subclasses must implement. See
    GetSettingDescriptionsMixinRange and GetSettingDescriptionsMixinList for
    some implementations.

    If there are no entries in the list, it will fetch FIRST_INDEX_OFFSET and
    expect a NACK.

    Subclasses must define EXPECTED_FIELDS, the first of which is the field
    used to validate the index against and DESCRIPTION_FIELD, which is the
    field to check for unprintable characters.

    If ALLOWED_NACKS is non-empty, this adds a custom NackGetResult to the list
    of allowed results for each entry.
  """
  ALLOWED_NACKS = []
  FIRST_INDEX_OFFSET = 1
  EXPECTED_FIELDS = None
  DESCRIPTION_FIELD = None

  def ListOfSettings(self):
    self.SetBroken('Base method of GetSettingDescriptionsMixin called')

  def Test(self):
    if self.EXPECTED_FIELDS is None:
      self.SetBroken('No EXPECTED_FIELDS given for %s' %
                     (self.__class__.__name__))
      return

    if self.DESCRIPTION_FIELD is None:
      self.SetBroken('No DESCRIPTION_FIELD given for %s' %
                     (self.__class__.__name__))
      return

    self.items = self.ListOfSettings()
    if not self.items:
      # Try to GET first item, this should NACK
      self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
      self.SendGet(ROOT_DEVICE, self.pid, [self.FIRST_INDEX_OFFSET])
      return

    # Otherwise fetch the description for each known setting.
    self._GetNextDescription()

  def _GetNextDescription(self):
    if not self.items:
      self.Stop()
      return

    results = [self.AckGetResult(field_names=self.EXPECTED_FIELDS,
                                 action=self._GetNextDescription)]
    for nack in self.ALLOWED_NACKS:
      results.append(self.NackGetResult(nack, action=self._GetNextDescription))
    self.AddIfGetSupported(results)
    self.current_item = self.items.pop()
    self.SendGet(ROOT_DEVICE, self.pid, [self.current_item])

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    if fields[self.EXPECTED_FIELDS[0]] != self.current_item:
      self.AddWarning(
          '%s mismatch, sent %d, received %d' %
          (self.pid.name, self.current_item, fields[self.EXPECTED_FIELDS[0]]))

    if ContainsUnprintable(fields[self.DESCRIPTION_FIELD]):
      self.AddAdvisory(
          '%s field in %s for %s %d contains unprintable characters, was %s' %
          (self.DESCRIPTION_FIELD.capitalize(),
           self.pid.name,
           self.DESCRIPTION_FIELD,
           self.current_item,
           StringEscape(fields[self.DESCRIPTION_FIELD])))


class GetSettingDescriptionsRangeMixin(GetSettingDescriptionsMixin):
  """Perform a GET for each setting in a range.

    The range is a count, it will check FIRST_INDEX_OFFSET to
    FIRST_INDEX_OFFSET + NumberOfSettings().
  """

  def NumberOfSettings(self):
    # By default we use the first property from REQUIRES
    return self.Property(self.REQUIRES[0])

  def ListOfSettings(self):
    # We generate a range from FIRST_INDEX_OFFSET to NumberOfSettings()
    if self.NumberOfSettings() is None:
        return []
    else:
      return list(range(self.FIRST_INDEX_OFFSET,
                        self.NumberOfSettings() + self.FIRST_INDEX_OFFSET))


class GetSettingDescriptionsListMixin(GetSettingDescriptionsMixin):
  """Perform a GET for each setting in a list.

    The list is an array of settings, which don't need to be
    sequential
  """

  def ListOfSettings(self):
    # By default we use the first property from REQUIRES
    return self.Property(self.REQUIRES[0])
