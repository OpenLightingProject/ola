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

import datetime
import struct
from ResponderTest import ExpectedResult, ResponderTest, TestCategory
from ola import PidStore
from ola.OlaClient import RDMNack
import TestMixins

MAX_DMX_ADDRESS = 512
MAX_LABEL_SIZE = 32
MAX_PERSONALITY_NUMBER = 255

# First up we try to fetch device info which other tests depend on.
#------------------------------------------------------------------------------
class DeviceInfoTest(object):
  """The base device info test class."""
  PID = 'DEVICE_INFO'

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
    self.AddExpectedResults(self.AckResponse(self.FIELDS, self.FIELD_VALUES))
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
        self.AddAdvisory('DMX Footprint non 0, but no personalities listed')
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
      self.NackResponse(RDMNack.NR_FORMAT_ERROR),
      self.AckResponse(
        self.FIELDS,
        self.FIELD_VALUES,
        warning='Get %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class SetDeviceInfo(ResponderTest, DeviceInfoTest):
  """SET device info."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def Test(self):
    self.AddExpectedResults(TestMixins.GetUnsupportedNacks(self.pid))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid)


class AllSubDevicesDeviceInfo(ResponderTest, DeviceInfoTest):
  """Send a Get Device Info to ALL_SUB_DEVICES."""
  CATEGORY = TestCategory.SUB_DEVICES
  def Test(self):
    self.AddExpectedResults(
        self.NackResponse(RDMNack.NR_SUB_DEVICE_OUT_OF_RANGE))
    self.SendGet(PidStore.ALL_SUB_DEVICES, self.pid)


# Supported Parameters Tests & Mixin
#------------------------------------------------------------------------------
class GetSupportedParameters(ResponderTest):
  """GET supported parameters."""
  CATEGORY = TestCategory.CORE
  PID = 'SUPPORTED_PARAMETERS'

  # declaring support for any of these is a warning:
  MANDATORY_PIDS = ['SUPPORTED_PARAMETERS',
                    'PARAMETER_DESCRIPTION',
                    'DEVICE_INFO',
                    'SOFTWARE_VERSION_LABEL',
                    'DMX_START_ADDRESS',
                    'IDENTIFY_DEVICE']

  # Banned PIDs, these are pid values that can not appear in the list of
  # supported parameters (these are used for discovery)
  BANNED_PID_VALUES = [1, 2, 3]

  # If responders support any of the pids in these groups, the should really
  # support all of them.
  PID_GROUPS = [
      ('LANGUAGE_CAPABILITIES', 'LANGUAGE'),
      ('DMX_PERSONALITY', 'DMX_PERSONALITY_DESCRIPTION'),
      ('SENSOR_DEFINITION', 'SENSOR_VALUE'),
      ('SELF_TEST_DESCRIPTION', 'PERFORM_SELF_TEST'),
  ]

  # If the first pid in each group is supported, the remainer of the group
  # must be.
  PID_DEPENDENCIES = [
      ('RECORD_SENSORS', 'SENSOR_VALUE'),
      ('DEFAULT_SLOT_VALUE', 'SLOT_DESCRIPTION'),
  ]

  def SupportsPid(self, pid):
    """Returns true if this device supports this pid.

    Args:
      pid: A PidStore.Pid object.
    """
    return pid.value in self.supported_parameters

  def Test(self):
    self._pid_supported = False
    self.supported_parameters = []
    self.manufacturer_parameters = []

    self.AddExpectedResults([
      # TODO(simon): We should cross check this against support for anything
      # more than the required set of parameters at the end of all tests.
      self.NackResponse(RDMNack.NR_UNKNOWN_PID),
      self.AckResponse(),
    ])
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if not status.WasSuccessfull():
      return

    mandatory_pids = {}
    for p in self.MANDATORY_PIDS:
      pid = self.LookupPid(p)
      mandatory_pids[pid.value] = pid

    self._pid_supported = True
    for item in fields['params']:
      param_id = item['param_id']
      if param_id in self.BANNED_PID_VALUES:
        self.AddWarning('%d listed in supported parameters' % param_id)
        continue

      if param_id in mandatory_pids:
        self.AddAdvisory('%s listed in supported parameters' %
                         mandatory_pids[param_id].name)
        continue

      self.supported_parameters.append(param_id)
      if param_id >= 0x8000 and param_id < 0xffe0:
        self.manufacturer_parameters.append(param_id)

    pid_store = PidStore.GetStore()

    for pid_names in self.PID_GROUPS:
      supported_pids = []
      unsupported_pids = []
      for pid_name in pid_names:
        pid = self.LookupPid(pid_name)
        if self.SupportsPid(pid):
          supported_pids.append(pid_name)
        else:
          unsupported_pids.append(pid_name)

      if supported_pids and unsupported_pids:
        self.AddAdvisory(
            '%s supported but %s is not' %
            (','.join(supported_pids), ','.join(unsupported_pids)))

    for pid_names in self.PID_DEPENDENCIES:
      if not self.SupportsPid(self.LookupPid(pid_names[0])):
        continue

      unsupported_pids = []
      for pid_name in pid_names[1:]:
        pid = self.LookupPid(pid_name)
        if not self.SupportsPid(pid):
          unsupported_pids.append(pid_name)
      if unsupported_pids:
        self.AddAdvisory('%s supported but %s is not' %
                         (pid_names[0], ','.join(unsupported_pids)))


class GetSupportedParametersWithData(ResponderTest):
  """GET supported parameters with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'SUPPORTED_PARAMETERS'

  def Test(self):
    self.AddExpectedResults([
      self.NackResponse(RDMNack.NR_FORMAT_ERROR),
      self.AckResponse(
        warning='Get %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class SetSupportedParameters(ResponderTest):
  """Attempt to SET supported parameters."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'SUPPORTED_PARAMETERS'

  def Test(self):
    self.AddExpectedResults(TestMixins.GetUnsupportedNacks(self.pid))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid)


class IsSupportedMixin(object):
  """A Mixin that changes the expected result if the pid isn't supported."""
  DEPS = [GetSupportedParameters]

  def PidSupported(self):
    return self.Deps(GetSupportedParameters).SupportsPid(self.pid)

  def AddIfSupported(self, result):
    if not self.PidSupported():
      result = self.NackResponse(RDMNack.NR_UNKNOWN_PID)
    self.AddExpectedResults(result)


# Sub Devices Test
#------------------------------------------------------------------------------
class FindSubDevices(ResponderTest):
  """Locate the sub devices by sending DEVICE_INFO messages."""
  CATEGORY = TestCategory.SUB_DEVICES
  PID = 'DEVICE_INFO'
  DEPS = [GetDeviceInfo]

  def Test(self):
    self._device_count = self.Deps(GetDeviceInfo).GetField('sub_device_count')
    self._sub_devices = []  # stores the sub device indices
    self._current_index = 0  # the current sub device we're trying to query
    self._CheckForSubDevice()

  def _CheckForSubDevice(self):
    # For each supported param message we should either see a sub device out of
    # range or an ack
    if len(self._sub_devices) == self._device_count:
      if self._device_count == 0:
        self.SetNotRun(' No sub devices declared')
      self.Stop()
      return

    if self._current_index >= PidStore.MAX_VALID_SUB_DEVICE:
      self.SetFailed('Only found %d of %d sub devices' %
                     (len(self._sub_devices), self._device_count))
      self.Stop()
      return

    self.AddExpectedResults([
      self.NackResponse(RDMNack,NR_SUB_DEVICE_OUT_OF_RANGE,
                        action=self._CheckForSubDevice),
      self.AckResponse(action=self._CheckForSubDevice)
    ])
    self._current_index += 1
    self.SendGet(self._current_index, self.pid)

  def VerifyResult(self, status, fields):
    if status.WasSuccessfull():
      self._sub_devices.append(self._current_index)


# Parameter Description
#------------------------------------------------------------------------------
class GetParamDescription(ResponderTest):
  """Check that GET parameter description works for any manufacturer params."""
  CATEGORY = TestCategory.RDM_INFORMATION
  PID = 'PARAMETER_DESCRIPTION'
  DEPS = [GetSupportedParameters]

  def Test(self):
    self.params = self.Deps(GetSupportedParameters).manufacturer_parameters[:]
    if len(self.params) == 0:
      self.SetNotRun(' No manufacturer params found')
      self.Stop()
      return
    self._GetParam()

  def _GetParam(self):
    if len(self.params) == 0:
      self.Stop()
      return

    self.AddExpectedResults(
      self.AckResponse(action=self._GetParam))
    self.current_param = self.params.pop()
    self.SendGet(PidStore.ROOT_DEVICE, self.pid, [self.current_param])

  def VerifyResult(self, status, fields):
    #TODO(simon): Hook into this to add new PIDs to the store
    if not status.WasSuccessfull():
      return

    if self.current_param != fields['pid']:
      self.SetFailed('Request for pid 0x%hx returned pid 0x%hx' %
                     (self.current_param, fields['pid']))

    if fields['type'] != 0:
      self.AddWarning('type field in parameter description is not 0, was %d' %
                      fields['type'])

    if fields['command_class'] > 3:
      self.AddWarning(
          'command class field in parameter description should be 1, 2 or 3, '
          'was %d' % fields['command_class'])
    self._logger.debug(fields)


class GetParamDescriptionForNonManufacturerPid(ResponderTest):
  """GET parameter description for a non-manufacturer pid."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PARAMETER_DESCRIPTION'
  DEPS = [GetSupportedParameters]

  def Test(self):
    device_info_pid = self.LookupPid('DEVICE_INFO')
    results = [
        self.NackResponse(RDMNack.NR_UNKNOWN_PID),
        self.NackResponse(
            RDMNack.NR_DATA_OUT_OF_RANGE,
            advisory='Parameter Description appears to be supported but no'
                     'manufacturer pids were declared'),
    ]
    if self.Deps(GetSupportedParameters).manufacturer_parameters:
      results = self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE)

    self.AddExpectedResults(results)
    self.SendGet(PidStore.ROOT_DEVICE, self.pid, [device_info_pid.value])


class GetParamDescriptionWithData(ResponderTest):
  """GET parameter description with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PARAMETER_DESCRIPTION'
  DEPS = [GetSupportedParameters]

  def Test(self):
    results = [
        self.NackResponse(RDMNack.NR_UNKNOWN_PID),
        self.NackResponse(RDMNack.NR_FORMAT_ERROR,
                          advisory='Parameter Description appears to be '
                                   'supported but no manufacturer pids were '
                                   'declared'),
    ]
    if self.Deps(GetSupportedParameters).manufacturer_parameters:
      results = self.NackResponse(RDMNack.NR_FORMAT_ERROR)
    self.AddExpectedResults(results)
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


# Comms Stqtus
#------------------------------------------------------------------------------
class GetCommsStatus(IsSupportedMixin, ResponderTest):
  """Get the comms status."""
  CATEGORY = TestCategory.STATUS_COLLECTION
  PID = 'COMMS_STATUS'

  def Test(self):
    self.AddIfSupported(self.AckResponse())
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetCommsStatusWithData(IsSupportedMixin,
                             TestMixins.GetWithData,
                             ResponderTest):
  """Get the comms status with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'COMMS_STATUS'


class ClearCommsStatus(IsSupportedMixin, ResponderTest):
  """Clear the comms status."""
  CATEGORY = TestCategory.STATUS_COLLECTION
  PID = 'COMMS_STATUS'

  def Test(self):
    self.AddIfSupported(self.AckResponse(action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid)

  def VerifySet(self):
    self.AddIfSupported(
        self.AckResponse(field_dict={
            'short_message': 0,
            'length_mismatch': 0,
            'checksum_fail': 0
        }))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class ClearCommsStatusWithData(IsSupportedMixin,
                               TestMixins.SetWithData,
                               ResponderTest):
  """Clear the comms status with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'COMMS_STATUS'


# Product Detail Id List
#------------------------------------------------------------------------------
class GetProductDetailIdList(IsSupportedMixin, ResponderTest):
  """GET the list of product detail ids."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'PRODUCT_DETAIL_ID_LIST'

  def Test(self):
    self.AddIfSupported(self.AckResponse(['detail_ids']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetProductDetailIdListWithData(IsSupportedMixin, TestMixins.GetWithData,
                                     ResponderTest):
  """GET product detail id list with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PRODUCT_DETAIL_ID_LIST'


class SetProductDetailIdList(IsSupportedMixin,
                             TestMixins.UnsupportedSetMixin,
                             ResponderTest):
  """SET product detail id list."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PRODUCT_DETAIL_ID_LIST'


# Device Model Description
#------------------------------------------------------------------------------
class GetDeviceModelDescription(IsSupportedMixin,
                                TestMixins.GetLabelMixin,
                                ResponderTest):
  """GET the device model description."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_MODEL_DESCRIPTION'


class GetDeviceModelDescriptionWithData(IsSupportedMixin,
                                        TestMixins.GetWithData,
                                        ResponderTest):
  """Get device model description with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_MODEL_DESCRIPTION'


class SetDeviceModelDescription(IsSupportedMixin,
                                TestMixins.UnsupportedSetMixin,
                                ResponderTest):
  """Attempt to SET the device model description with no data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_MODEL_DESCRIPTION'


class SetDeviceModelDescriptionWithData(IsSupportedMixin,
                                        TestMixins.UnsupportedSetMixin,
                                        ResponderTest):
  """SET the device model description with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_MODEL_DESCRIPTION'
  DATA = 'FOO BAR'


# Manufacturer Label
#------------------------------------------------------------------------------
class GetManufacturerLabel(IsSupportedMixin,
                           TestMixins.GetLabelMixin,
                           ResponderTest):
  """GET the manufacturer label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'MANUFACTURER_LABEL'


class GetManufacturerLabelWithData(IsSupportedMixin,
                                   TestMixins.GetWithData,
                                   ResponderTest):
  """Get manufacturer label with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'MANUFACTURER_LABEL'


class SetManufacturerLabel(IsSupportedMixin,
                           TestMixins.UnsupportedSetMixin,
                           ResponderTest):
  """Attempt to SET the manufacturer label with no data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'MANUFACTURER_LABEL'


class SetManufacturerLabelWithData(IsSupportedMixin,
                                   TestMixins.UnsupportedSetMixin,
                                   ResponderTest):
  """SET the manufacturer label with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'MANUFACTURER_LABEL'
  DATA = 'FOO BAR'


# Device Label
#------------------------------------------------------------------------------
class GetDeviceLabel(IsSupportedMixin,
                     TestMixins.GetLabelMixin,
                     ResponderTest):
  """GET the device label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'


class GetDeviceLabelWithData(IsSupportedMixin,
                             TestMixins.GetWithData,
                             ResponderTest):
  """GET the device label with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_LABEL'


class SetDeviceLabel(IsSupportedMixin,
                     TestMixins.SetLabelMixin,
                     ResponderTest):
  """SET the device label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceLabel]

  def OldValue(self):
    return self.Deps(GetDeviceLabel).GetField('label')


class SetFullSizeDeviceLabel(IsSupportedMixin,
                             TestMixins.SetLabelMixin,
                             ResponderTest):
  """SET the device label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceLabel]
  TEST_LABEL = 'this is a string with 32 charact'

  def OldValue(self):
    return self.Deps(GetDeviceLabel).GetField('label')


class SetNonAsciiDeviceLabel(IsSupportedMixin,
                             TestMixins.SetLabelMixin,
                             ResponderTest):
  """SET the device label to something that contains non-ascii data."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceLabel]
  TEST_LABEL = 'string with\x0d non ascii\xc0'

  def ExpectedResults(self):
    return [
      self.NackResponse(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.NackResponse(RDMNack.NR_FORMAT_ERROR),
      self.AckResponse(action=self.VerifySet)
    ]

  def OldValue(self):
    return self.Deps(GetDeviceLabel).GetField('label')


class SetEmptyDeviceLabel(IsSupportedMixin,
                          TestMixins.SetLabelMixin,
                          ResponderTest):
  """SET the device label with no data."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceLabel]
  TEST_LABEL = ''

  def OldValue(self):
    return self.Deps(GetDeviceLabel).GetField('label')


class SetOversizedDeviceLabel(IsSupportedMixin,
                              TestMixins.SetOversizedLabelMixin,
                              ResponderTest):
  """SET the device label with more than 32 bytes of data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_LABEL'

  def OldValue(self):
    return self.Deps(GetDeviceLabel).GetField('label')


# Factory Defaults
#------------------------------------------------------------------------------
class GetFactoryDefaults(IsSupportedMixin, ResponderTest):
  """GET the factory defaults pid."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'FACTORY_DEFAULTS'

  def Test(self):
    self.AddIfSupported(
      self.AckResponse(['using_defaults']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetFactoryDefaultsWithData(IsSupportedMixin,
                                 TestMixins.GetWithData,
                                 ResponderTest):
  """GET the factory defaults pid with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'FACTORY_DEFAULTS'


class ResetFactoryDefaults(IsSupportedMixin, ResponderTest):
  """Reset to factory defaults."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'FACTORY_DEFAULTS'

  def Test(self):
    self.AddIfSupported(
      self.AckResponse(action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid)

  def VerifySet(self):
    self.AddIfSupported(
      self.AckResponse(field_values={'using_defaults': True}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class ResetFactoryDefaultsWithData(IsSupportedMixin,
                                   TestMixins.SetWithData,
                                   ResponderTest):
  """Reset to factory defaults with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'FACTORY_DEFAULTS'


# Language Capabilities
#------------------------------------------------------------------------------
class GetLanguageCapabilities(IsSupportedMixin, ResponderTest):
  """GET the language capabilities pid."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE_CAPABILITIES'

  def Test(self):
    self.languages = []
    self.AddIfSupported(self.AckResponse(['languages']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if not status.WasSuccessfull():
      return

    self.languages = [f['language'] for f in fields['languages']]

    if len(self.languages) == 0:
      self.AddWarning('No languages returned for LANGUAGE_CAPABILITIES')

    language_set = set()
    for language in self.languages:
      if language in language_set:
        self.AddAdvisory('%s listed twice in language capabilities' % language)
      language_set.add(language)


class GetLanguageCapabilitiesWithData(IsSupportedMixin,
                                      TestMixins.GetWithData,
                                      ResponderTest):
  """GET the language capabilities pid with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LANGUAGE_CAPABILITIES'


# Language
#------------------------------------------------------------------------------
class GetLanguage(IsSupportedMixin, ResponderTest):
  """GET the language."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'

  def Test(self):
    self.language = None
    self.AddIfSupported(self.AckResponse(['language']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    """Store the current language."""
    if status.WasSuccessfull():
      self.language = fields['language']


class SetLanguage(IsSupportedMixin, ResponderTest):
  """SET the language."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'
  DEPS = IsSupportedMixin.DEPS + [GetLanguageCapabilities]

  def Test(self):
    ack = self.AckResponse(action=self.VerifySet)
    nack = self.NackResponse(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)

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
      self.AckResponse(field_values={'language': self.new_language}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetNonAsciiLanguage(IsSupportedMixin, ResponderTest):
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'
  DEPS = IsSupportedMixin.DEPS + [GetLanguageCapabilities]

  def Test(self):
    self.AddIfSupported(self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE))

    self.SendSet(PidStore.ROOT_DEVICE, self.pid, ['\x0d\xc0'])


class SetUnsupportedLanguage(IsSupportedMixin, ResponderTest):
  """Try to set a language that doesn't exist in Language Capabilities."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LANGUAGE'
  DEPS = IsSupportedMixin.DEPS + [GetLanguageCapabilities]

  def Test(self):
    available_langugages = self.Deps(GetLanguageCapabilities).languages
    if 'zz' in available_langugages:
      self.SetBroken('zz exists in the list of available languages')
      self.Stop()
      return

    self.AddIfSupported([
      self.NackResponse(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE),
    ])
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, ['zz'])


# Software Version Label
#------------------------------------------------------------------------------
class GetSoftwareVersionLabel(ResponderTest):
  """GET the software version label."""
  # We don't use the GetLabelMixin here because this PID is mandatory
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'SOFTWARE_VERSION_LABEL'

  def Test(self):
    self.AddExpectedResults(self.AckResponse(['label']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetSoftwareVersionLabelWithData(TestMixins.GetWithData, ResponderTest):
  """GET the software_version_label with param data."""
  # We don't use the GetLabelMixin here because this PID is mandatory
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'SOFTWARE_VERSION_LABEL'

  def AddIfSupported(self, results):
    self.AddExpectedResults(results)


class SetSoftwareVersionLabel(TestMixins.UnsupportedSetMixin, ResponderTest):
  """Attempt to SET the software version label."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'SOFTWARE_VERSION_LABEL'


# TODO(simon): Add a test for every sub device


# Boot Software Version
#------------------------------------------------------------------------------
class GetBootSoftwareVersion(IsSupportedMixin,
                             ResponderTest):
  """GET the boot software version."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'BOOT_SOFTWARE_VERSION'

  def Test(self):
    self.AddIfSupported(self.AckResponse(['version']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetBootSoftwareVersionWithData(IsSupportedMixin,
                                     TestMixins.GetWithData,
                                     ResponderTest):
  """GET the boot software version with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'BOOT_SOFTWARE_VERSION'


class SetBootSoftwareVersion(TestMixins.UnsupportedSetMixin, ResponderTest):
  """Attempt to SET the boot software version."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'BOOT_SOFTWARE_VERSION'


# Boot Software Version Label
#------------------------------------------------------------------------------
class GetBootSoftwareLabel(IsSupportedMixin,
                           TestMixins.GetLabelMixin,
                           ResponderTest):
  """GET the boot software label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'BOOT_SOFTWARE_LABEL'


class GetBootSoftwareLabelWithData(IsSupportedMixin,
                                   TestMixins.GetWithData,
                                   ResponderTest):
  """GET the boot software label with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'BOOT_SOFTWARE_LABEL'

class SetBootSoftwareLabel(IsSupportedMixin,
                           TestMixins.UnsupportedSetMixin,
                           ResponderTest):
  """SET the boot software label."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'BOOT_SOFTWARE_LABEL'


# DMX Personality
#------------------------------------------------------------------------------
class GetZeroPersonalityDescription(IsSupportedMixin, ResponderTest):
  """GET the personality description for the 0th personality."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_PERSONALITY_DESCRIPTION'

  def Test(self):
    self.AddIfSupported(self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid, [0])


class GetOutOfRangePersonalityDescription(IsSupportedMixin, ResponderTest):
  """GET the personality description for the N + 1 personality."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_PERSONALITY_DESCRIPTION'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceInfo]

  def Test(self):
    personality_count = self.Deps(GetDeviceInfo).GetField('personality_count')
    self.AddIfSupported(self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid, [personality_count + 1])


class GetPersonalityDescription(IsSupportedMixin, ResponderTest):
  """GET the personality description for the current personality."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_PERSONALITY_DESCRIPTION'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceInfo]

  def Test(self):
    current_personality = self.Deps(GetDeviceInfo).GetField(
        'current_personality')
    footprint = self.Deps(GetDeviceInfo).GetField('dmx_footprint')
    # cross check against what we got from device info
    self.AddIfSupported(self.AckResponse(field_values={
        'personality': current_personality,
        'slots_required': footprint
      }))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid, [current_personality])


class GetPersonality(IsSupportedMixin, ResponderTest):
  """Get the current personality settings."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_PERSONALITY'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceInfo]

  def Test(self):
    self.AddIfSupported(self.AckResponse(
      ['current_personality', 'personality_count']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if not status.WasSuccessfull():
      return

    current_personality = self.Deps(GetDeviceInfo).GetField(
      'current_personality')
    personality_count = self.Deps(GetDeviceInfo).GetField('personality_count')
    warning_str = ("Personality information in device info doesn't match"
      ' that in dmx_personality')

    if current_personality != fields['current_personality']:
      self.SetFailed('%s: current_personality %d != %d' % (
        warning_str, current_personality, fields['current_personality']))

    if personality_count != fields['personality_count']:
      self.SetFailed('%s: personality_count %d != %d' % (
        warning_str, personality_count, fields['personality_count']))


class GetPersonalities(IsSupportedMixin, ResponderTest):
  """Get information about all the personalities."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_PERSONALITY_DESCRIPTION'

  DEPS = IsSupportedMixin.DEPS + [GetDeviceInfo]

  def Test(self):
    self.personalities = []
    self._personality_count = self.Deps(GetDeviceInfo).GetField(
        'personality_count')
    self._current_index = 0
    self._GetPersonality()

  def _GetPersonality(self):
    self._current_index += 1
    if self._current_index > self._personality_count:
      if self._personality_count == 0:
        self.SetNotRun(' No personalities declared')
      self.Stop()
      return

    if self._current_index >= MAX_PERSONALITY_NUMBER:
      # This should never happen because personality_count is a uint8
      self.SetFailed('Could not find all personalities')
      self.Stop()
      return

    self.AddIfSupported(self.AckResponse(
        ['slots_required', 'name'],
        {'personality': self._current_index},
        action=self._GetPersonality))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid, [self._current_index])

  def VerifyResult(self, status, fields):
    """Save the personality for other tests to use."""
    if status.WasSuccessfull():
      self.personalities.append(fields)


class SetPersonality(IsSupportedMixin, ResponderTest):
  """Set the personality."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_PERSONALITY'
  # We depend on GetPersonality here so we don't set it before GetPersonality
  DEPS = IsSupportedMixin.DEPS + [
      GetPersonalities, GetPersonality, GetPersonalityDescription]

  def Test(self):
    count = self.Deps(GetPersonality).GetField('personality_count')
    if count is None or count == 0:
      self.AddExpectedResults(self.NackResponse(RDMNack.NR_UNKNOWN_PID))
      self.new_personality = {'personality': 1}  # can use anything here really
    else:
      personalities = self.Deps(GetPersonalities).personalities
      current = self.Deps(GetPersonality).GetField('current_personality')

      if len(personalities) == 0:
        self.SetFailed(
          'personality_count was non 0 but failed to fetch all personalities')
        self.Stop()
        return

      self.new_personality = personalities[0]
      for personality in personalities:
        if personality['personality'] != current:
          self.new_personality = personality
          break

      self.AddIfSupported(self.AckResponse(action=self.VerifySet))

    self.SendSet(PidStore.ROOT_DEVICE,
                 self.pid,
                 [self.new_personality['personality']])

  def VerifySet(self):
    self.AddIfSupported(
      self.AckResponse(
        field_values={
          'current_personality': self.new_personality['personality'],
        },
        action=self.VerifyDeviceInfo))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyDeviceInfo(self):
    device_info_pid = self.LookupPid('DEVICE_INFO')
    self.AddIfSupported(
      ExpectedResult.AckResponse(
        device_info_pid.value,
        field_values={
          'current_personality': self.new_personality['personality'],
          'dmx_footprint': self.new_personality['slots_required'],
        }))
    self.SendGet(PidStore.ROOT_DEVICE, device_info_pid)


class SetZeroPersonality(IsSupportedMixin, ResponderTest):
  """Try to set the personality to 0."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_PERSONALITY'
  DEPS = IsSupportedMixin.DEPS

  def Test(self):
    self.AddIfSupported(self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [0])


class SetOutOfRangePersonality(IsSupportedMixin, ResponderTest):
  """Try to set the personality to 0."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_PERSONALITY'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceInfo]

  def Test(self):
    personality_count = self.Deps(GetDeviceInfo).GetField('personality_count')
    self.AddIfSupported(self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [personality_count + 1])


class SetOversizedPersonality(IsSupportedMixin, ResponderTest):
  """Send an over-sized SET personality command."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_PERSONALITY'

  def Test(self):
    self.AddIfSupported(self.NackResponse(RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, 'foo')


# DMX Start Address tests
#------------------------------------------------------------------------------
class GetStartAddress(ResponderTest):
  """GET the DMX start address."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_START_ADDRESS'
  DEPS = [GetDeviceInfo]

  def Test(self):
    if self.Deps(GetDeviceInfo).GetField('dmx_footprint') > 0:
      results = self.AckResponse(['dmx_address'])
    else:
      results = [
          self.AckResponse(field_values={'dmx_address': 0xffff}),
          self.NackResponse(RDMNack.NR_UNKNOWN_PID)
      ]
    self.AddExpectedResults(results)
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetStartAddress(ResponderTest):
  """Set the DMX start address."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_START_ADDRESS'
  DEPS = [GetStartAddress, GetDeviceInfo]

  def Test(self):
    footprint = self.Deps(GetDeviceInfo).GetField('dmx_footprint')
    self.start_address = 1

    current_address = self.Deps(GetStartAddress).GetField('dmx_address')
    if footprint == 0 or current_address == 0xffff:
      result = self.NackResponse(RDMNack.NR_UNKNOWN_PID)
    else:
      if footprint != MAX_DMX_ADDRESS:
        self.start_address = current_address + 1
        if self.start_address + footprint > MAX_DMX_ADDRESS + 1:
          self.start_address = 1
      result = self.AckResponse(action=self.VerifySet)
    self.AddExpectedResults(result)
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.start_address])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckResponse(field_values={'dmx_address': self.start_address},
                       action=self.VerifyDeviceInfo))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyDeviceInfo(self):
    device_info_pid = self.LookupPid('DEVICE_INFO')
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        device_info_pid.value,
        field_values = {'start_address': self.start_address}))
    self.SendGet(PidStore.ROOT_DEVICE, device_info_pid)


class SetOutOfRangeStartAddress(ResponderTest):
  """Check that the DMX address can't be set to > 512."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_START_ADDRESS'
  DEPS = [SetStartAddress, GetDeviceInfo]

  def Test(self):
    if self.Deps(GetDeviceInfo).GetField('dmx_footprint') > 0:
      self.AddExpectedResults(self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE))
    else:
      self.AddExpectedResults(self.NackResponse(RDMNack.NR_UNKNOWN_PID))
    data = struct.pack('!H', MAX_DMX_ADDRESS + 1)
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, data)


class SetZeroStartAddress(ResponderTest):
  """Check the DMX address can't be set to 0."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_START_ADDRESS'
  DEPS = [GetDeviceInfo, SetStartAddress]

  def Test(self):
    if self.Deps(GetDeviceInfo).GetField('dmx_footprint') > 0:
      self.AddExpectedResults(self.NackResponse(RDMNack.NR_DATA_OUT_OF_RANGE))
    else:
      self.AddExpectedResults(self.NackResponse(RDMNack.NR_UNKNOWN_PID))
    data = struct.pack('!H', 0)
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, data)


class SetOversizedStartAddress(ResponderTest):
  """Send an over-sized SET dmx start address."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_START_ADDRESS'
  DEPS = [SetStartAddress, GetDeviceInfo]

  def Test(self):
    if self.Deps(GetDeviceInfo).GetField('dmx_footprint') > 0:
      self.AddExpectedResults(self.NackResponse(RDMNack.NR_FORMAT_ERROR))
    else:
      self.AddExpectedResults(self.NackResponse(RDMNack.NR_UNKNOWN_PID))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, 'foo')


# Device Hours
#------------------------------------------------------------------------------
class GetDeviceHours(IsSupportedMixin,
                     TestMixins.GetUInt32Mixin,
                     ResponderTest):
  """GET the device hours."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_HOURS'
  EXPECTED_FIELD = 'hours'


class GetDeviceHoursWithData(IsSupportedMixin,
                             TestMixins.GetWithData,
                             ResponderTest):
  """GET the device hours with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_HOURS'


class SetDeviceHours(IsSupportedMixin,
                     TestMixins.SetUInt32Mixin,
                     ResponderTest):
  """Attempt to SET the device hours."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_HOURS'
  EXPECTED_FIELD = 'hours'
  DEPS = IsSupportedMixin.DEPS + [GetDeviceHours]

  def OldValue(self):
    return self.Deps(GetDeviceHours).GetField(self.EXPECTED_FIELD)


class SetDeviceHoursWithNoData(IsSupportedMixin,
                               TestMixins.SetUInt32NoDataMixin,
                               ResponderTest):
  """Set the device hours with no param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_HOURS'


# Lamp Hours
#------------------------------------------------------------------------------
class GetLampHours(IsSupportedMixin,
                   TestMixins.GetUInt32Mixin,
                   ResponderTest):
  """GET the device hours."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_HOURS'
  EXPECTED_FIELD = 'hours'


class GetLampHoursWithData(IsSupportedMixin,
                           TestMixins.GetWithData,
                           ResponderTest):
  """GET the device hours with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LAMP_HOURS'


class SetLampHours(IsSupportedMixin,
                   TestMixins.SetUInt32Mixin,
                   ResponderTest):
  """Attempt to SET the device hours."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_HOURS'
  EXPECTED_FIELD = 'hours'
  DEPS = IsSupportedMixin.DEPS + [GetLampHours]

  def OldValue(self):
    return self.Deps(GetLampHours).GetField(self.EXPECTED_FIELD)


class SetLampHoursWithNoData(IsSupportedMixin,
                             TestMixins.SetUInt32NoDataMixin,
                             ResponderTest):
  """Set the device hours with no param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LAMP_HOURS'


# Lamp Strikes
#------------------------------------------------------------------------------
class GetLampStrikes(IsSupportedMixin,
                     TestMixins.GetUInt32Mixin,
                     ResponderTest):
  """GET the device strikes."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_STRIKES'
  EXPECTED_FIELD = 'strikes'


class GetLampStrikesWithData(IsSupportedMixin,
                             TestMixins.GetWithData,
                             ResponderTest):
  """GET the device strikes with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LAMP_STRIKES'


class SetLampStrikes(IsSupportedMixin,
                     TestMixins.SetUInt32Mixin,
                     ResponderTest):
  """Attempt to SET the device strikes."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_STRIKES'
  EXPECTED_FIELD = 'strikes'
  DEPS = IsSupportedMixin.DEPS + [GetLampStrikes]

  def OldValue(self):
    return self.Deps(GetLampStrikes).GetField(self.EXPECTED_FIELD)


class SetLampStrikesWithNoData(IsSupportedMixin,
                               TestMixins.SetUInt32NoDataMixin,
                               ResponderTest):
  """Set the device strikes with no param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LAMP_STRIKES'


# Device Hours
#------------------------------------------------------------------------------
class GetDevicePowerCycles(IsSupportedMixin,
                           TestMixins.GetUInt32Mixin,
                           ResponderTest):
  """GET the device power_cycles."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_POWER_CYCLES'
  EXPECTED_FIELD = 'power_cycles'


class GetDevicePowerCyclesWithData(IsSupportedMixin,
                                   TestMixins.GetWithData,
                                   ResponderTest):
  """GET the device power_cycles with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_POWER_CYCLES'


class SetDevicePowerCycles(IsSupportedMixin,
                           TestMixins.SetUInt32Mixin,
                           ResponderTest):
  """Attempt to SET the device power_cycles."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_POWER_CYCLES'
  EXPECTED_FIELD = 'power_cycles'
  DEPS = IsSupportedMixin.DEPS + [GetDevicePowerCycles]

  def OldValue(self):
    return self.Deps(GetDevicePowerCycles).GetField(self.EXPECTED_FIELD)


class SetDevicePowerCyclesWithNoData(IsSupportedMixin,
                                     TestMixins.SetUInt32NoDataMixin,
                                     ResponderTest):
  """Set the device power_cycles with no param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DEVICE_POWER_CYCLES'


# Display Level
#------------------------------------------------------------------------------
class GetDisplayLevel(IsSupportedMixin,
                      TestMixins.GetBoolMixin,
                      ResponderTest):
  """GET the display level setting."""
  CATEGORY = TestCategory.DISPLAY_SETTINGS
  PID = 'DISPLAY_LEVEL'
  EXPECTED_FIELD = 'level'


class GetDisplayLevelWithData(IsSupportedMixin,
                              TestMixins.GetWithData,
                              ResponderTest):
  """GET the pan invert setting with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DISPLAY_LEVEL'


class SetDisplayLevel(IsSupportedMixin,
                      TestMixins.SetUInt8Mixin,
                      ResponderTest):
  """Attempt to SET the display level setting."""
  CATEGORY = TestCategory.DISPLAY_SETTINGS
  PID = 'DISPLAY_LEVEL'
  EXPECTED_FIELD = 'level'
  DEPS = IsSupportedMixin.DEPS + [GetDisplayLevel]

  def OldValue(self):
    return self.Deps(GetDisplayLevel).GetField(self.EXPECTED_FIELD)


class SetDisplayLevelWithNoData(IsSupportedMixin,
                                TestMixins.SetUInt8NoDataMixin,
                                ResponderTest):
  """Set the display level with no param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DISPLAY_LEVEL'


# Pan Invert
#------------------------------------------------------------------------------
class GetPanInvert(IsSupportedMixin, TestMixins.GetBoolMixin, ResponderTest):
  """GET the pan invert setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'PAN_INVERT'
  EXPECTED_FIELD = 'invert'


class GetPanInvertWithData(IsSupportedMixin,
                           TestMixins.GetWithData,
                           ResponderTest):
  """GET the pan invert setting with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PAN_INVERT'


class SetPanInvert(IsSupportedMixin, TestMixins.SetBoolMixin, ResponderTest):
  """Attempt to SET the pan invert setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'PAN_INVERT'
  EXPECTED_FIELD = 'invert'
  DEPS = IsSupportedMixin.DEPS + [GetPanInvert]

  def OldValue(self):
    return self.Deps(GetPanInvert).GetField(self.EXPECTED_FIELD)


class SetPanInvertWithNoData(IsSupportedMixin,
                             TestMixins.SetBoolNoDataMixin,
                             ResponderTest):
  """Set the pan invert with no param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PAN_INVERT'


# Tilt Invert
#------------------------------------------------------------------------------
class GetTiltInvert(IsSupportedMixin, TestMixins.GetBoolMixin, ResponderTest):
  """GET the tilt invert setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'TILT_INVERT'
  EXPECTED_FIELD = 'invert'


class GetTiltInvertWithData(IsSupportedMixin,
                            TestMixins.GetWithData,
                            ResponderTest):
  """GET the tilt invert setting with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'TILT_INVERT'


class SetTiltInvert(IsSupportedMixin, TestMixins.SetBoolMixin, ResponderTest):
  """Attempt to SET the tilt invert setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'TILT_INVERT'
  EXPECTED_FIELD = 'invert'
  DEPS = IsSupportedMixin.DEPS + [GetTiltInvert]

  def OldValue(self):
    return self.Deps(GetTiltInvert).GetField(self.EXPECTED_FIELD)


class SetTiltInvertWithNoData(IsSupportedMixin,
                              TestMixins.SetBoolNoDataMixin,
                              ResponderTest):
  """Set the tilt invert with no param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'TILT_INVERT'


# Pan Tilt Swap Invert
#------------------------------------------------------------------------------
class GetPanTiltSwap(IsSupportedMixin, TestMixins.GetBoolMixin, ResponderTest):
  """GET the pan tilt swap setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'PAN_TILT_SWAP'
  EXPECTED_FIELD = 'swap'


class GetPanTiltSwapWithData(IsSupportedMixin,
                             TestMixins.GetWithData,
                             ResponderTest):
  """GET the pan tilt swap setting with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PAN_TILT_SWAP'


class SetPanTiltSwap(IsSupportedMixin, TestMixins.SetBoolMixin, ResponderTest):
  """Attempt to SET the pan tilt swap setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'PAN_TILT_SWAP'
  EXPECTED_FIELD = 'swap'
  DEPS = IsSupportedMixin.DEPS + [GetPanTiltSwap]

  def OldValue(self):
    return self.Deps(GetPanTiltSwap).GetField(self.EXPECTED_FIELD)


class SetPanTiltSwapWithNoData(IsSupportedMixin,
                               TestMixins.SetBoolNoDataMixin,
                               ResponderTest):
  """Set the pan tilt swap with no param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PAN_TILT_SWAP'


# Real time clock
#------------------------------------------------------------------------------
class GetRealTimeClock(IsSupportedMixin, ResponderTest):
  """GET the real time clock setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'REAL_TIME_CLOCK'

  ALLOWED_RANGES = {
      'year': (2003, 65535),
      'month': (1, 12),
      'day': (1, 31),
      'hour': (0, 23),
      'minute': (0, 59),
  }

  def Test(self):
    self.AddIfSupported(
      self.AckResponse(self.ALLOWED_RANGES.keys() + ['second']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if not status.WasSuccessfull():
      return

    for field, range in ALLOWED_RANGES:
      value = fields[field]
      if value < range[0] or value > range[1]:
        self.AddWarning('%s in GET %s is out of range, was %d, expeced %s' %
                        (field, self.PID, value, range))


class GetRealTimeClockWithData(IsSupportedMixin,
                               TestMixins.GetWithData,
                               ResponderTest):
  """GET the teal time clock with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'REAL_TIME_CLOCK'


class SetRealTimeClock(IsSupportedMixin, ResponderTest):
  """Set the real time clock."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'REAL_TIME_CLOCK'

  def Test(self):
    n = datetime.datetime.now()
    self.AddIfSupported(self.AckResponse())
    args = [n.year, n.month, n.day, n.hour, n.minute, n.second]
    self.SendSet(PidStore.ROOT_DEVICE, self.pid,
                 args)


class SetRealTimeClockWithNoData(IsSupportedMixin, ResponderTest):
  """Set the real time clock without any data."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'REAL_TIME_CLOCK'

  def Test(self):
    self.AddIfSupported(self.NackResponse(RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, '')


# Identify Device
#------------------------------------------------------------------------------
class GetIdentifyDevice(ResponderTest):
  """Get the identify mode."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_DEVICE'

  def Test(self):
    # don't inherit from GetBoolMixin because this is required
    self.AddExpectedResults(self.AckResponse())
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetIdentifyDeviceWithData(ResponderTest):
  """Get the identify mode with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'IDENTIFY_DEVICE'

  def Test(self):
    # don't inherit from GetWithData because this is required
    self.AddExpectedResults(self.NackResponse(RDMNack.NR_FORMAT_ERROR))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class SetIdentifyDevice(ResponderTest):
  """Set the identify mode."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_DEVICE'
  DEPS = [GetIdentifyDevice]

  def Test(self):
    self.identify_mode = self.Deps(GetIdentifyDevice).GetField('identify_state')
    self.new_mode = not self.identify_mode

    self.AddExpectedResults(self.AckResponse(action=self.VerifyIdentifyMode))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.new_mode])

  def VerifyIdentifyMode(self):
    self.AddExpectedResults(self.AckResponse(
        field_values = {'identify_state': self.new_mode},
        action=self.ResetMode))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def ResetMode(self):
    self.AddExpectedResults(self.AckResponse())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.identify_mode])


class SetIdentifyDeviceWithNoData(ResponderTest):
  """Set the identify mode with no data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'IDENTIFY_DEVICE'

  def Test(self):
    self.AddExpectedResults(self.NackResponse(RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, '')


# Power State
#------------------------------------------------------------------------------
class GetPowerState(IsSupportedMixin, ResponderTest):
  """Get the power state mode."""
  CATEGORY = TestCategory.CONTROL
  PID = 'POWER_STATE'

  ALLOWED_STATES = [0, 1, 2, 0xff]

  def Test(self):
    self.AddIfSupported(self.AckResponse())
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if not status.WasSuccessfull():
      return

    if fields['state'] not in self.ALLOWED_STATES:
      self.AddWarning('Power state of 0x%hx is not defined' % fields['state'])


class GetPowerStateWithData(IsSupportedMixin,
                            TestMixins.GetWithData,
                            ResponderTest):
  """Get the power state mode with data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'POWER_STATE'


class SetPowerState(IsSupportedMixin, TestMixins.SetUInt8Mixin, ResponderTest):
  """Set the power state."""
  CATEGORY = TestCategory.CONTROL
  PID = 'POWER_STATE'
  DEPS = IsSupportedMixin.DEPS + [GetPowerState]
  EXPECTED_FIELD = 'state'

  def NewValue(self):
    self.old_value = self.Deps(GetPowerState).GetField(self.EXPECTED_FIELD)
    try:
      index = GetPowerState.ALLOWED_STATES.index(self.old_value)
    except ValueError:
      return GetPowerState.ALLOWED_STATES[0]

    length = len(GetPowerState.ALLOWED_STATES)
    return GetPowerState.ALLOWED_STATES[(self.old_value + 1) % length]

  def ResetState(self):
    if not self.old_value:
      return

    # reset back to the old value
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.old_value])
    self._wrapper.Run()

class SetPowerStateWithNoData(IsSupportedMixin, ResponderTest):
  """Set the power state with no data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'POWER_STATE'

  def Test(self):
    self.AddIfSupported(self.NackResponse(RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, '')
