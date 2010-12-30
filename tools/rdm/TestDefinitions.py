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
# TestDefinitions.py
# Copyright (C) 2010 Simon Newton

'''This defines all the tests for RDM responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'

from ResponderTest import ExpectedResult, ResponderTest, TestCategory
from ola import PidStore
from ola.OlaClient import RDMNack
import TestMixins

MAX_DMX_ADDRESS = 512
MAX_LABEL_SIZE = 32

# First up we try to fetch device info which other tests depend on.
#------------------------------------------------------------------------------
class DeviceInfoTest(object):
  """The base device info test class."""
  PID = 'device_info'

  FIELDS = ['device_model', 'product_category', 'software_version',
            'dmx_footprint', 'current_personality', 'personality_count',
            'start_address', 'sub_device_count', 'sensor_count']
  FIELD_VALUES = {
      'protocol_major': 1,
      'protocol_minor': 0,
  }


class GetDeviceInfo(ResponderTest, DeviceInfoTest):
  """Check that GET device info works."""
  CATEGORY = TestCategory.CORE

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(self.pid.value,
                                 self.FIELDS,
                                 self.FIELD_VALUES))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, unused_status, fields):
    """Check the footprint, personalities & sub devices."""
    footprint = fields['dmx_footprint']
    if footprint > MAX_DMX_ADDRESS:
      self.AddWarning('DMX Footprint of %d, was more than 512' % footprint)
    if footprint > 0:
      personality_count = fields['personality_count']
      current_personality = fields['current_personality']
      if personality_count == 0:
        self.AddWarning('DMX Footprint non 0, but no personalities listed')
      if current_personality == 0:
        self.AddWarning('Current personality should be >= 1, was %d' %
            current_personality)
      elif current_personality > personality_count:
        self.AddWarning('Current personality (%d) should be less than the '
                        'personality count (%d)' %
                        (current_personality, personality_count))

    sub_devices = fields['sub_device_count']
    if sub_devices > 512:
      self.AddWarning('Sub device count > 512, was %d' % sub_devices)


# Device Info tests
#------------------------------------------------------------------------------
class GetDeviceInfoWithData(ResponderTest, DeviceInfoTest):
  """Check a GET device info request with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
      ExpectedResult.AckResponse(self.pid.value, self.FIELDS,
                                 self.FIELD_VALUES)
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class SetDeviceInfo(ResponderTest, DeviceInfoTest):
  """Check that SET device info fails with a NACK."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid)

# TODO(simon): This fails with the RDM-TRI because the widget notices the
# response has a different sub device.
#class AllSubDevicesDeviceInfo(ResponderTest, DeviceInfoTest):
#  """Devices should NACK a GET request sent to ALL_SUB_DEVICES."""
#  def Test(self):
#    self.AddExpectedResults(
#      ExpectedResult.NackResponse(self.pid.value,
#                                  RDMNack.NR_SUB_DEVICE_OUT_OF_RANGE))
#    self.SendGet(PidStore.ALL_SUB_DEVICES, self.pid)


# Supported Parameters Tests
#------------------------------------------------------------------------------
class GetSupportedParameters(ResponderTest):
  """Check that we can get the supported parameters message."""
  CATEGORY = TestCategory.CORE
  PID = 'supported_parameters'

  def Test(self):
    self._pid_supported = False
    self.supported_parameters = []

    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_UNKNOWN_PID),
      ExpectedResult.AckResponse(self.pid.value)
    ])
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if status.WasSuccessfull():
      self._pid_supported = True
      for item in fields:
        self.supported_parameters.append(item['param'])

  @property
  def supported(self):
    return self._pid_supported


class SetSupportedParameters(ResponderTest):
  """Check that SET device info fails with a NACK."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'supported_parameters'

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid)


# Sub Devices Test
#------------------------------------------------------------------------------
class FindSubDevices(ResponderTest):
  """Locate the sub devices by sending DeviceInfo messages."""
  CATEGORY = TestCategory.SUB_DEVICES
  PID = 'device_info'

  DEPS = [GetSupportedParameters, GetDeviceInfo]

  def PreCondition(self):
    if not self.Deps(GetSupportedParameters).supported:
      return False

    self._device_count = self.Deps(GetDeviceInfo).GetField('sub_device_count')
    self._sub_devices = []
    self._current_index = 0
    return True

  def Test(self):
    # TODO(simon): There seems to be an issue with the RDM_TRI and sub devices
    # > 255. Investigate this.
    self._CheckForSubDevice()

  def _CheckForSubDevice(self):
    # For each supported param message we should either see a sub device out of
    # range or an ack
    if len(self._sub_devices) == self._device_count:
      if self._device_count == 0:
        self.SetNotRun()
      self.Stop()
      return

    if self._current_index >= PidStore.MAX_VALID_SUB_DEVICE:
      self.SetFailed('Could not find all sub devices')
      self.Stop()
      return

    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_SUB_DEVICE_OUT_OF_RANGE,
                                  action=self._CheckForSubDevice),
      ExpectedResult.AckResponse(self.pid.value,
                                 action=self._CheckForSubDevice)
    ])
    self._current_index += 1
    self.SendGet(self._current_index, self.pid)

  def VerifyResult(self, status, fields):
    if status.WasSuccessfull():
      self._sub_devices.append(self._current_index)


# DMX Start Address tests
#------------------------------------------------------------------------------
class GetStartAddress(ResponderTest):
  """Check that we can fetch the DMX start address."""
  PID = 'dmx_start_address'
  DEPS = [GetDeviceInfo]

  def PreCondition(self):
    footprint = self.Deps(GetDeviceInfo).GetField('dmx_footprint')
    return footprint > 0

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(self.pid.value, ['dmx_address']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetStartAddress(ResponderTest):
  """Check that we can set the DMX start address."""
  PID = 'dmx_start_address'
  DEPS = [GetStartAddress, GetDeviceInfo]

  def Test(self):
    current_address = self.Deps(GetStartAddress).GetField('dmx_address')
    footprint = self.Deps(GetDeviceInfo).GetField('dmx_footprint')

    if footprint == MAX_DMX_ADDRESS:
      self.start_address = 1
    else:
      self.start_address = current_address + 1
      if self.start_address + footprint > MAX_DMX_ADDRESS + 1:
        self.start_address = 1
    self.AddExpectedResults(
      ExpectedResult.AckResponse(self.pid.value,
                                 action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.start_address])

  def VerifySet(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={'dmx_address': self.start_address}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class DeviceInfoCheckStartAddress(ResponderTest):
  """Check that device info is updated after the dmx address is set."""
  PID = 'device_info'
  DEPS = [SetStartAddress]

  def Test(self):
    start_address =  self.Deps(SetStartAddress).start_address
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values = {'start_address': start_address}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetOutOfRangeStartAddress(ResponderTest):
  """Check that we can't set the DMX address > 512."""
  PID = 'dmx_start_address'
  DEPS = [SetStartAddress]

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [MAX_DMX_ADDRESS + 1])


class SetZeroStartAddress(ResponderTest):
  """Check that we can't set the start address to 0."""
  PID = 'dmx_start_address'
  DEPS = [SetStartAddress]

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [0])


class SetOversizedStartAddress(ResponderTest):
  """Send an over-sized SET dmx start address."""
  PID = 'dmx_start_address'
  DEPS = [SetStartAddress]

  def Test(self):
    self.verify_result = False
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
    )
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, 'foo')


# Parameter Description
#------------------------------------------------------------------------------
class GetParamDescription(ResponderTest):
  """Check that we can get the descriptions of any manufacturer params."""
  CATEGORY = TestCategory.MANUFACTURER_PIDS
  PID = 'parameter_description'
  DEPS = [GetSupportedParameters]

  def PreCondition(self):
    params = self.Deps(GetSupportedParameters).supported_parameters
    self.params = [p for p in params if p >= 0x8000 and p < 0xffe0]
    return len(self.params) > 0

  def Test(self):
    self._GetParam()

  def _GetParam(self):
    if len(self.params) == 0:
      self.Stop()
      return

    self.AddExpectedResults(
      ExpectedResult.AckResponse(self.pid.value,
                                 action=self._GetParam)
    )
    param = self.params.pop()
    self.SendGet(PidStore.ROOT_DEVICE, self.pid, [param])

  def VerifyResult(self, status, fields):
    #TODO(simon): Hook into this to add new PIDs to the store
    pass


# Manufacturer Label
#------------------------------------------------------------------------------
class GetManufacturerLabel(TestMixins.GetLabelMixin,
                           ResponderTest):
  """Check that we can get the manufacturer label."""
  PID = 'manufacturer_label'


class GetManufacturerLabelWithData(TestMixins.GetLabelWithDataMixin,
                                   ResponderTest):
  """Send a get manufacturer label with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'manufacturer_label'


class SetManufacturerLabel(TestMixins.UnsupportedSetMixin,
                           ResponderTest):
  """Check that SET manufacturer label fails with a NACK."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'manufacturer_label'


class SetManufacturerLabelWithData(TestMixins.UnsupportedSetMixin,
                                   ResponderTest):
  """Check that SET manufacturer label with data fails with a NACK."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'manufacturer_label'
  DATA = 'foo bar'


# Device Label
#------------------------------------------------------------------------------
class GetDeviceLabel(TestMixins.GetLabelMixin, ResponderTest):
  """Check that we can get the device label."""
  PID = 'device_label'


class GetDeviceLabelWithData(TestMixins.GetLabelWithDataMixin, ResponderTest):
  """Send a get device label with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'device_label'


class SetDeviceLabel(TestMixins.SetLabelMixin, ResponderTest):
  """Check that we can set the device label."""
  PID = 'device_label'
  DEPS = [GetDeviceLabel]


class SetEmptyDeviceLabel(TestMixins.SetEmptyLabelMixin, ResponderTest):
  """Send an empty SET device label."""
  PID = 'device_label'
  DEPS = [GetDeviceLabel]


class SetOversizedDeviceLabel(TestMixins.SetOversizedLabelMixin,
                              ResponderTest):
  """Send an over-sized SET device label."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'device_label'
  DEPS = [GetDeviceLabel]


# Software Version Label
#------------------------------------------------------------------------------
class GetSoftwareVersionLabel(ResponderTest):
  """Check that we can get the software version label."""
  # We don't use the GetLabelMixin here because this PID is mandatory
  PID = 'software_version_label'

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(self.pid.value, ['label'])
    )
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

class GetSoftwareVersionLabelWithData(ResponderTest):
  """Send a GET software_version_label with data."""
  # We don't use the GetLabelMixin here because this PID is mandatory
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'software_version_label'

  def Test(self):
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
      ExpectedResult.AckResponse(self.pid.value, ['label'])
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foobarbaz')


class SetSoftwareVersionLabel(TestMixins.UnsupportedSetMixin,
                              ResponderTest):
  """Check that SET software version label fails with a NACK."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'software_version_label'


# TODO(simon): Add a test for every sub device



#------------------------------------------------------------------------------
