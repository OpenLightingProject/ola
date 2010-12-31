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
  """GET device info & verify."""
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
  """GET device info with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
      ExpectedResult.AckResponse(self.pid.value, self.FIELDS,
                                 self.FIELD_VALUES)
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class SetDeviceInfo(ResponderTest, DeviceInfoTest):
  """Attempt to SET device info."""
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
#  CATEGORY = TestCategory.SUB_DEVICES
#  def Test(self):
#    self.AddExpectedResults(
#      ExpectedResult.NackResponse(self.pid.value,
#                                  RDMNack.NR_SUB_DEVICE_OUT_OF_RANGE))
#    self.SendGet(PidStore.ALL_SUB_DEVICES, self.pid)


# Supported Parameters Tests & Mixin
#------------------------------------------------------------------------------
class GetSupportedParameters(ResponderTest):
  """GET supported parameters."""
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
    if not status.WasSuccessfull():
      return

    self._pid_supported = True
    for item in fields:
      self.supported_parameters.append(item['param'])

    pid_store = PidStore.GetStore()
    langugage_capability_pid = self.LookupPid('language_capabilities')
    language_pid = self.LookupPid('language')
    if (self.SupportsPid(langugage_capability_pid) and not
        self.SupportsPid(language_pid)):
      self.AddWarning('language_capabilities supported but language is not')

  @property
  def supported(self):
    return self._pid_supported

  def SupportsPid(self, pid):
    return pid.value in self.supported_parameters


class SetSupportedParameters(ResponderTest):
  """Attempt to SET supported parameters."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'supported_parameters'

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid)


class IsSupportedMixin(object):
  """A Mixin that changes the result if the pid isn't in the supported list."""
  DEPS = [GetSupportedParameters]

  def PidSupported(self):
    return self.Deps(GetSupportedParameters).SupportsPid(self.pid)

  def AddIfSupported(self, result):
    if not self.PidSupported():
      result = ExpectedResult.NackResponse(self.pid.value,
                                           RDMNack.NR_UNKNOWN_PID)
    self.AddExpectedResults(result)


# Sub Devices Test
#------------------------------------------------------------------------------
class FindSubDevices(ResponderTest):
  """Locate the sub devices by sending DeviceInfo messages."""
  CATEGORY = TestCategory.SUB_DEVICES
  PID = 'device_info'

  DEPS = [GetDeviceInfo]

  def PreCondition(self):
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
  """GET the DMX start address."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'dmx_start_address'
  DEPS = [GetDeviceInfo]

  def Test(self):
    result = ExpectedResult.NackResponse(self.pid.value,
                                         RDMNack.NR_UNKNOWN_PID)
    if self.Deps(GetDeviceInfo).GetField('dmx_footprint') > 0:
      result = ExpectedResult.AckResponse(self.pid.value, ['dmx_address'])
    self.AddExpectedResults(result)
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetStartAddress(ResponderTest):
  """Set the DMX start address."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'dmx_start_address'
  DEPS = [GetStartAddress, GetDeviceInfo]

  def Test(self):
    footprint = self.Deps(GetDeviceInfo).GetField('dmx_footprint')
    self.start_address = None

    if footprint == 0:
      result = ExpectedResult.NackResponse(self.pid.value,
                                           RDMNack.NR_UNKNOWN_PID)
    else:
      current_address = self.Deps(GetStartAddress).GetField('dmx_address')
      if footprint == MAX_DMX_ADDRESS:
        self.start_address = 1
      else:
        self.start_address = current_address + 1
        if self.start_address + footprint > MAX_DMX_ADDRESS + 1:
          self.start_address = 1
        result = ExpectedResult.AckResponse(self.pid.value,
                                            action=self.VerifySet)
    self.AddExpectedResults(result)
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.start_address])

  def VerifySet(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={'dmx_address': self.start_address}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class DeviceInfoCheckStartAddress(ResponderTest):
  """Confirm device info is updated after the dmx address is set."""
  CATEGORY = TestCategory.DMX_SETUP
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
  """Check that the DMX address can't be set to > 512."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'dmx_start_address'
  DEPS = [SetStartAddress]

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [MAX_DMX_ADDRESS + 1])


class SetZeroStartAddress(ResponderTest):
  """Check the DMX address can't be set to 0."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'dmx_start_address'
  DEPS = [SetStartAddress]

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [0])


class SetOversizedStartAddress(ResponderTest):
  """Send an over-sized SET dmx start address."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
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
  """Check that GET parameter description works for any manufacturer params."""
  CATEGORY = TestCategory.RDM_INFORMATION
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
    self._logger.debug(fields)
    pass


# Product Detail Id List
#------------------------------------------------------------------------------
class GetProductDetailIdList(IsSupportedMixin, ResponderTest):
  """GET the list of product detail ids."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'product_detail_id_list'

  def Test(self):
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, ['detail_id']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


# Device Model Description
#------------------------------------------------------------------------------
class GetDeviceModelLabel(IsSupportedMixin, TestMixins.GetLabelMixin,
                          ResponderTest):
  """GET the device model label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'device_model_description'


class GetDeviceModelLabelWithData(IsSupportedMixin,
                                  TestMixins.GetLabelWithDataMixin,
                                  ResponderTest):
  """Get device_model label with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'device_model_description'


class SetDeviceModelLabel(IsSupportedMixin,
                          TestMixins.UnsupportedSetMixin, ResponderTest):
  """Attempt to SET the device_model label with no data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'device_model_description'


class SetDeviceModelLabelWithData(IsSupportedMixin,
                                  TestMixins.UnsupportedSetMixin,
                                  ResponderTest):
  """SET the device_model label with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'device_model_description'
  DATA = 'foo bar'


# Manufacturer Label
#------------------------------------------------------------------------------
class GetManufacturerLabel(IsSupportedMixin, TestMixins.GetLabelMixin,
                           ResponderTest):
  """GET the manufacturer label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'manufacturer_label'


class GetManufacturerLabelWithData(IsSupportedMixin,
                                   TestMixins.GetLabelWithDataMixin,
                                   ResponderTest):
  """Get manufacturer label with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'manufacturer_label'


class SetManufacturerLabel(IsSupportedMixin, TestMixins.UnsupportedSetMixin,
                           ResponderTest):
  """Attempt to SET the manufacturer label with no data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'manufacturer_label'


class SetManufacturerLabelWithData(IsSupportedMixin,
                                   TestMixins.UnsupportedSetMixin,
                                   ResponderTest):
  """SET the manufacturer label with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'manufacturer_label'
  DATA = 'foo bar'


# Device Label
#------------------------------------------------------------------------------
class GetDeviceLabel(IsSupportedMixin, TestMixins.GetLabelMixin,
                     ResponderTest):
  """GET the device label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'device_label'


class GetDeviceLabelWithData(IsSupportedMixin,
                             TestMixins.GetLabelWithDataMixin,
                             ResponderTest):
  """GET the device label with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'device_label'


class SetDeviceLabel(IsSupportedMixin, TestMixins.SetLabelMixin,
                     ResponderTest):
  """SET the device label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'device_label'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceLabel]


class SetEmptyDeviceLabel(IsSupportedMixin, TestMixins.SetEmptyLabelMixin,
                          ResponderTest):
  """SET the device label with no data."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'device_label'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceLabel]


class SetOversizedDeviceLabel(IsSupportedMixin,
                              TestMixins.SetOversizedLabelMixin,
                              ResponderTest):
  """SET the device label with more than 32 bytes of data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'device_label'


# Factory Defaults
#------------------------------------------------------------------------------
class GetFactoryDefaults(IsSupportedMixin, ResponderTest):
  """GET the factory defaults pid."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'factory_defaults'

  def Test(self):
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, ['using_defaults']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetFactoryDefaultsWithData(IsSupportedMixin, ResponderTest):
  """GET the factory defaults pid with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'factory_defaults'

  def Test(self):
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, ['using_defaults']))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foobar')


class ResetFactoryDefaults(IsSupportedMixin, ResponderTest):
  """Reset to factory defaults."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'factory_defaults'

  def Test(self):
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid)

  def VerifySet(self):
    self.AddIfSupported(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={'using_defaults': True}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


# Language Capabilities
#------------------------------------------------------------------------------
class GetLanguageCapabilities(IsSupportedMixin, ResponderTest):
  """GET the language capabilities pid."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'language_capabilities'

  def Test(self):
    self.languages = []
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, ['language']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if not status.WasSuccessfull():
      return

    self.languages = [f['language'] for f in fields]

    if len(self.language) == 0:
      self.SetFailed('No languages returned')

    language_set = set()
    for language in self.languages:
      if language in language_set:
        self.AddWarning('%s listed twice in language capabilities' % language)
      language_set.add(language)


class GetLanguageCapabilitiesWithData(IsSupportedMixin, ResponderTest):
  """GET the language capabilities pid."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'language_capabilities'

  def Test(self):
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, ['language']))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foobar')


# Language
#------------------------------------------------------------------------------
class GetLanguage(IsSupportedMixin, ResponderTest):
  """GET the language."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'language'

  def Test(self):
    self.language = None
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, ['language']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    """Store the current language."""
    if status.WasSuccessfull():
      self.language = fields['language']


class SetLanguage(IsSupportedMixin, ResponderTest):
  """SET the language."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'language'
  DEPS = IsSupportedMixin.DEPS + [GetLanguageCapabilities]

  def Test(self):
    ack = ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet)
    nack = ExpectedResult.NackResponse(self.pid.value,
                                       RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)

    # This is either empty, if GetLanguageCapabilities was NACK'ed or > 0
    available_langugages = self.Deps(GetLanguageCapabilities).languages
    if len(available_langugages) > 0:
      self.new_language = available_langugages[0]
      # if the responder only supports 1 lang, we may not be able to set it
      if len(available_langugages) > 1:
        self.AddIfSupported(ack)
      else:
        self.AddIfSupported([ack, nack])
    else:
      # Get languages returned no languages so we expect a nack
      self.AddIfSupported(nack)
      self.new_language = 'en'

    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.new_language])

  def VerifySet(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={'dmx_address': self.start_address}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetUnsupportedLanguage(IsSupportedMixin, ResponderTest):
  """Try to set a language that doesn't exist in Language Capabilities."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'language'
  DEPS = IsSupportedMixin.DEPS + [GetLanguageCapabilities]

  def Test(self):
    available_langugages = self.Deps(GetLanguageCapabilities).languages
    if 'zz' in available_langugages:
      self.SetBroken('zz exists in the list of available languages')
      self.Stop()
      return

    self.AddIfSupported([
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_DATA_OUT_OF_RANGE)])
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, ['zz'])


# Software Version Label
#------------------------------------------------------------------------------
class GetSoftwareVersionLabel(ResponderTest):
  """GET the software version label."""
  # We don't use the GetLabelMixin here because this PID is mandatory
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'software_version_label'

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(self.pid.value, ['label'])
    )
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetSoftwareVersionLabelWithData(ResponderTest):
  """GET the software_version_label with param data."""
  # We don't use the GetLabelMixin here because this PID is mandatory
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'software_version_label'

  def Test(self):
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
      ExpectedResult.AckResponse(self.pid.value, ['label'])
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foobarbaz')


class SetSoftwareVersionLabel(TestMixins.UnsupportedSetMixin, ResponderTest):
  """Attempt to SET the software version label."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'software_version_label'


# TODO(simon): Add a test for every sub device



#------------------------------------------------------------------------------
