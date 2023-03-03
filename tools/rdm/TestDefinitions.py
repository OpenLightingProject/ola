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
# TestDefinitions.py
# Copyright (C) 2010 Simon Newton

import datetime
import operator
import struct

from ola.OlaClient import OlaClient, RDMNack
from ola.PidStore import ROOT_DEVICE
from ola.RDMConstants import (INTERFACE_HARDWARE_TYPE_ETHERNET,
                              RDM_INTERFACE_INDEX_MAX, RDM_INTERFACE_INDEX_MIN,
                              RDM_MANUFACTURER_PID_MAX,
                              RDM_MANUFACTURER_PID_MIN,
                              RDM_MANUFACTURER_SD_MAX, RDM_MANUFACTURER_SD_MIN,
                              RDM_MAX_DOMAIN_NAME_LENGTH,
                              RDM_MAX_HOSTNAME_LENGTH, RDM_MIN_HOSTNAME_LENGTH,
                              RDM_ZERO_FOOTPRINT_DMX_ADDRESS)
from ola.StringUtils import StringEscape
from ola.testing.rdm import TestMixins
from ola.testing.rdm.ExpectedResults import (RDM_GET, RDM_SET, AckGetResult,
                                             BroadcastResult, InvalidResponse,
                                             NackGetResult, TimeoutResult,
                                             UnsupportedResult)
from ola.testing.rdm.ResponderTest import (OptionalParameterTestFixture,
                                           ParamDescriptionTestFixture,
                                           ResponderTestFixture, TestFixture)
from ola.testing.rdm.TestCategory import TestCategory
from ola.testing.rdm.TestHelpers import ContainsUnprintable
from ola.UID import UID

from ola import PidStore, RDMConstants

'''This defines all the tests for RDM responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


MAX_PERSONALITY_NUMBER = 255


# Mute Tests
# -----------------------------------------------------------------------------
class MuteDevice(ResponderTestFixture):
  """Mute device and verify response."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  PID = 'DISC_MUTE'
  PROVIDES = ['mute_supported', 'mute_control_fields']

  def Test(self):
    self.AddExpectedResults([
      self.AckDiscoveryResult(),
      UnsupportedResult(
        warning='RDM Controller does not support DISCOVERY commands')
    ])
    self.SendDiscovery(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    supported = (response.response_code !=
                 OlaClient.RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED)
    self.SetProperty('mute_supported', supported)

    if supported:
      self.SetProperty('mute_control_fields', fields['control_field'])
      binding_uids = fields.get('binding_uid', [])
      if binding_uids:
        if (binding_uids[0]['binding_uid'].manufacturer_id !=
            self.uid.manufacturer_id):
          self.AddWarning(
            'Binding UID manufacturer ID 0x%04hx does not equal device '
            'manufacturer ID of 0x%04hx' % (
              binding_uids[0]['binding_uid'].manufacturer_id,
              self.uid.manufacturer_id))


class MuteDeviceWithData(ResponderTestFixture):
  """Mute device with param data."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  PID = 'DISC_MUTE'

  def Test(self):
    # Section 6.3.4 of E1.20
    self.AddExpectedResults([
      TimeoutResult(),
      UnsupportedResult()
    ])
    self.SendRawDiscovery(ROOT_DEVICE, self.pid, b'x')


class UnMuteDevice(ResponderTestFixture):
  """UnMute device and verify response."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  PID = 'DISC_UN_MUTE'
  PROVIDES = ['unmute_supported']
  REQUIRES = ['mute_control_fields']

  def Test(self):
    self.AddExpectedResults([
      self.AckDiscoveryResult(),
      UnsupportedResult()
    ])
    self.SendDiscovery(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    supported = (response.response_code !=
                 OlaClient.RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED)
    self.SetProperty('unmute_supported', supported)
    if supported:
      if fields['control_field'] != self.Property('mute_control_fields'):
        self.AddWarning(
            "Mute / Unmute control fields don't match. 0x%hx != 0x%hx" %
            (self.Property('mute_control_fields'), fields['control_field']))


class UnMuteDeviceWithData(ResponderTestFixture):
  """UnMute device info with param data."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  PID = 'DISC_UN_MUTE'

  def Test(self):
    # Section 6.3.4 of E1.20
    self.AddExpectedResults([
      TimeoutResult(),
      UnsupportedResult()
    ])
    self.SendRawDiscovery(ROOT_DEVICE, self.pid, b'x')


class RequestsWhileUnmuted(ResponderTestFixture):
  """Unmute the device, send a GET DEVICE_INFO request, mute device again."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  PID = 'DISC_UN_MUTE'
  # This requires sub_device_count so that we know DEVICE_INFO is supported
  REQUIRES = ['mute_supported', 'unmute_supported', 'sub_device_count']

  def Test(self):
    if not (self.Property('unmute_supported') and
            self.Property('mute_supported')):
      self.SetNotRun('Controller does not support mute / unmute commands')
      return

    self.AddExpectedResults(self.AckDiscoveryResult(action=self.GetDeviceInfo))
    self.SendDiscovery(ROOT_DEVICE, self.pid)

  def GetDeviceInfo(self):
    device_info_pid = self.LookupPid('DEVICE_INFO')
    self.AddExpectedResults(AckGetResult(device_info_pid.value))
    self.SendGet(ROOT_DEVICE, device_info_pid)

  def ResetState(self):
    # Mute the device again
    mute_pid = self.LookupPid('DISC_MUTE')
    self.SendDiscovery(ROOT_DEVICE, mute_pid)
    self._wrapper.Run()


# Invalid DISCOVERY_PIDs
# -----------------------------------------------------------------------------
class InvalidDiscoveryPID(ResponderTestFixture):
  """Send an invalid Discovery CC PID, see E1.20 6.3.4"""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def PidRequired(self):
    return False

  # We need to mock out a PID here
  class MockPid(object):
    def __init__(self):
      self.value = 0x000f

    def ValidateAddressing(request_params, request_type):
      return True

    def __str__(self):
      return '0x%04hx' % self.value

  def Test(self):
    mock_pid = self.MockPid()
    self.AddExpectedResults([
      TimeoutResult(),
      UnsupportedResult()
    ])
    self.SendRawDiscovery(ROOT_DEVICE, mock_pid)


# DUB Tests
# -----------------------------------------------------------------------------
class MuteAllDevices(ResponderTestFixture):
  """Mute all devices, so we can perform DUB tests"""
  PID = 'DISC_MUTE'
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['mute_supported']
  # This is a fake property used to ensure this tests runs before the DUB tests.
  PROVIDES = ['global_mute']

  def Test(self):
    # Set the fake property
    self.SetProperty(self.PROVIDES[0], True)
    if not (self.Property('mute_supported')):
      self.SetNotRun('RDM Controller does not support DISCOVERY commands')
      return

    self.AddExpectedResults([
      BroadcastResult(),
      UnsupportedResult()
    ])

    self.SendDirectedDiscovery(
        UID.AllDevices(),
        PidStore.ROOT_DEVICE,
        self.pid)


class DUBFullTree(TestMixins.DiscoveryMixin,
                  ResponderTestFixture):
  """Confirm the device responds within the entire DUB range."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  PROVIDES = ['dub_supported']

  def LowerBound(self):
    return UID(0, 0)

  def UpperBound(self):
    return UID.AllDevices()

  def DUBResponseCode(self, response_code):
    self.SetProperty(
        'dub_supported',
        response_code != OlaClient.RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED)


class DUBManufacturerTree(TestMixins.DiscoveryMixin,
                          ResponderTestFixture):
  """Confirm the device responds within it's manufacturer DUB range."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID(self.uid.manufacturer_id, 0)

  def UpperBound(self):
    return UID.VendorcastAddress(self.uid.manufacturer_id)


class DUBSingleUID(TestMixins.DiscoveryMixin,
                   ResponderTestFixture):
  """Confirm the device responds to just it's own range."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return self.uid

  def UpperBound(self):
    return self.uid


class DUBSingleLowerUID(TestMixins.DiscoveryMixin,
                        ResponderTestFixture):
  """DUB from <UID> - 1 to <UID> - 1."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID.PreviousUID(self.uid)

  def UpperBound(self):
    return UID.PreviousUID(self.uid)

  def ExpectResponse(self):
    return False


class DUBSingleUpperUID(TestMixins.DiscoveryMixin,
                        ResponderTestFixture):
  """DUB from <UID> + 1 to <UID> + 1."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID.NextUID(self.uid)

  def UpperBound(self):
    return UID.NextUID(self.uid)

  def ExpectResponse(self):
    return False


class DUBAffirmativeLowerBound(TestMixins.DiscoveryMixin,
                               ResponderTestFixture):
  """DUB from <UID> to ffff:ffffffff."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return self.uid

  def UpperBound(self):
    return UID.AllDevices()


class DUBNegativeLowerBound(TestMixins.DiscoveryMixin,
                            ResponderTestFixture):
  """DUB from <UID> + 1 to ffff:ffffffff."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID.NextUID(self.uid)

  def UpperBound(self):
    return UID.AllDevices()

  def ExpectResponse(self):
    return False


class DUBAffirmativeUpperBound(TestMixins.DiscoveryMixin,
                               ResponderTestFixture):
  """DUB from 0000:00000000 to <UID>."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID(0, 0)

  def UpperBound(self):
    return self.uid


class DUBNegativeUpperBound(TestMixins.DiscoveryMixin,
                            ResponderTestFixture):
  """DUB from 0000:00000000 to <UID> - 1."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID(0, 0)

  def UpperBound(self):
    return UID.PreviousUID(self.uid)

  def ExpectResponse(self):
    return False


class DUBDifferentManufacturer(TestMixins.DiscoveryMixin,
                               ResponderTestFixture):
  """DUB with a different manufacturer's range."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID(self.uid.manufacturer_id - 1, 0)

  def UpperBound(self):
    return UID(self.uid.manufacturer_id - 1, 0xffffffff)

  def ExpectResponse(self):
    return False


class DUBSignedComparisons(TestMixins.DiscoveryMixin,
                           ResponderTestFixture):
  """DUB to check UIDs aren't using signed values."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    # Section 5.1 of E1.20 limits the manufacturer ID range to 0 - 0x7fff so
    # this should be safe for all cases.
    return UID(0x8000, 0)

  def UpperBound(self):
    return UID.AllDevices()

  def ExpectResponse(self):
    return False


class DUBNegativeVendorcast(TestMixins.DiscoveryMixin,
                            ResponderTestFixture):
  """DUB to another manufacturer's vendorcast address."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID(0, 0)

  def UpperBound(self):
    return UID.AllDevices()

  def ExpectResponse(self):
    return False

  def Target(self):
    return UID(self.uid.manufacturer_id - 1, 0xffffffff)


class DUBPositiveVendorcast(TestMixins.DiscoveryMixin,
                            ResponderTestFixture):
  """DUB to this manufacturer's vendorcast address."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID(0, 0)

  def UpperBound(self):
    return UID.AllDevices()

  def Target(self):
    return UID(self.uid.manufacturer_id, 0xffffffff)


class DUBPositiveUnicast(TestMixins.DiscoveryMixin,
                         ResponderTestFixture):
  """DUB to the device's address."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID(0, 0)

  def UpperBound(self):
    return UID.AllDevices()

  def Target(self):
    return self.uid


class DUBInvertedFullTree(TestMixins.DiscoveryMixin,
                          ResponderTestFixture):
  """DUB from ffff:ffffffff to 0000:00000000."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID.AllDevices()

  def UpperBound(self):
    return UID(0, 0)

  def ExpectResponse(self):
    return False


class DUBInvertedRange(TestMixins.DiscoveryMixin,
                       ResponderTestFixture):
  """DUB from <UID> + 1 to <UID> - 1."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID.NextUID(self.uid)

  def UpperBound(self):
    return UID.PreviousUID(self.uid)

  def ExpectResponse(self):
    return False


class DUBInvertedLowerUID(TestMixins.DiscoveryMixin,
                          ResponderTestFixture):
  """DUB from <UID> to <UID> - 1."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return self.uid

  def UpperBound(self):
    return UID.PreviousUID(self.uid)

  def ExpectResponse(self):
    return False


class DUBInvertedUpperUID(TestMixins.DiscoveryMixin,
                          ResponderTestFixture):
  """DUB from <UID> + 1 to <UID>."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  REQUIRES = ['dub_supported'] + TestMixins.DiscoveryMixin.REQUIRES

  def LowerBound(self):
    return UID.NextUID(self.uid)

  def UpperBound(self):
    return self.uid

  def ExpectResponse(self):
    return False


# Device Info tests
# -----------------------------------------------------------------------------
class DeviceInfoTest(object):
  """The base device info test class."""
  PID = 'DEVICE_INFO'

  FIELDS = ['device_model', 'product_category', 'software_version',
            'dmx_footprint', 'current_personality', 'personality_count',
            'dmx_start_address', 'sub_device_count', 'sensor_count']
  FIELD_VALUES = {
      'protocol_major': 1,
      'protocol_minor': 0,
  }


class GetDeviceInfo(DeviceInfoTest, ResponderTestFixture):
  """GET device info & verify."""
  CATEGORY = TestCategory.CORE

  PROVIDES = [
      'current_personality',
      'dmx_footprint',
      'dmx_start_address',
      'personality_count',
      'sensor_count',
      'software_version',
      'sub_device_count',
  ]

  def Test(self):
    self.AddExpectedResults(self.AckGetResult(
      field_names=self.FIELDS,
      field_values=self.FIELD_VALUES))
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, unused_response, fields):
    """Check the footprint, personalities & sub devices."""
    for property_name in self.PROVIDES:
      self.SetPropertyFromDict(fields, property_name)

    footprint = fields['dmx_footprint']
    if footprint > TestMixins.MAX_DMX_ADDRESS:
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

    start_address = fields['dmx_start_address']
    if (start_address == 0 or
        (start_address > TestMixins.MAX_DMX_ADDRESS and
         start_address != RDM_ZERO_FOOTPRINT_DMX_ADDRESS)):
      self.AddWarning('Invalid DMX address %d in DEVICE_INFO' % start_address)

    sub_devices = fields['sub_device_count']
    if sub_devices > 512:
      self.AddWarning('Sub device count > 512, was %d' % sub_devices)


class GetDeviceInfoWithData(DeviceInfoTest, ResponderTestFixture):
  """GET device info with param data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PROVIDES = ['supports_over_sized_pdl']

  def Test(self):
    self.AddExpectedResults([
      self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
      self.AckGetResult(
        field_names=self.FIELDS,
        field_values=self.FIELD_VALUES,
        warning='Get %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawGet(ROOT_DEVICE, self.pid, b'x')

  def VerifyResult(self, response, fields):
    self.SetProperty('supports_over_sized_pdl', True)


class GetMaxPacketSize(DeviceInfoTest, ResponderTestFixture):
  """Check if the responder can handle a packet of the maximum size."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  MAX_PDL = 231
  PROVIDES = ['supports_max_sized_pdl']

  def Test(self):
    self.AddExpectedResults([
      self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
      self.NackGetResult(RDMNack.NR_PACKET_SIZE_UNSUPPORTED),
      self.AckGetResult(),  # Some crazy devices continue to ack
      InvalidResponse(
          advisory='Responder returned an invalid response to a command with '
                   'PDL of %d' % self.MAX_PDL
      ),
      TimeoutResult(
          advisory='Responder timed out to a command with PDL of %d' %
                   self.MAX_PDL),
    ])
    # Incrementing list, so we can find out which bit we have where in memory
    # if it overflows
    data = b''
    for i in range(0, self.MAX_PDL):
      data += chr(i)
    self.SendRawGet(ROOT_DEVICE, self.pid, data)

  def VerifyResult(self, response, fields):
    ok = response.response_code not in [OlaClient.RDM_INVALID_RESPONSE,
                                        OlaClient.RDM_TIMEOUT]

    self.SetProperty('supports_max_sized_pdl', ok)


class DetermineMaxPacketSize(DeviceInfoTest, ResponderTestFixture):
  """Binary search the pdl length space to determine the max packet size."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  REQUIRES = ['supports_over_sized_pdl', 'supports_max_sized_pdl']

  def Test(self):
    if self.Property('supports_max_sized_pdl'):
      self.SetNotRun('Device supports full sized packet')
      return

    self._lower = 1
    self._upper = GetMaxPacketSize.MAX_PDL
    self.SendPacket()

  def SendPacket(self):
    if self._lower + 1 == self._upper:
      self.AddWarning('Max PDL supported is < %d, was %d' %
                      (GetMaxPacketSize.MAX_PDL, self._lower))
      self.Stop()
      return

    self._current = (self._lower + self._upper) / 2
    self.AddExpectedResults([
      self.NackGetResult(RDMNack.NR_FORMAT_ERROR, action=self.GetPassed),
      self.AckGetResult(action=self.GetPassed),
      InvalidResponse(action=self.GetFailed),
      TimeoutResult(action=self.GetFailed),
    ])
    self.SendRawGet(ROOT_DEVICE, self.pid, b'x' * self._current)

  def GetPassed(self):
    self._lower = self._current
    self.SendPacket()

  def GetFailed(self):
    self._upper = self._current
    self.SendPacket()


class SetDeviceInfo(TestMixins.UnsupportedSetMixin,
                    DeviceInfoTest,
                    ResponderTestFixture):
  """Attempt to SET device info with no data."""
  def Test(self):
    self.AddExpectedResults([
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.NackSetResult(RDMNack.NR_UNKNOWN_PID,
                         advisory='NR_UNSUPPORTED_COMMAND_CLASS would be more '
                                  'appropriate for a mandatory PID')
    ])
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid)


class SetDeviceInfoWithData(TestMixins.UnsupportedSetWithDataMixin,
                            DeviceInfoTest,
                            ResponderTestFixture):
  """SET device info with data."""
  def Test(self):
    self.AddExpectedResults([
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.NackSetResult(RDMNack.NR_UNKNOWN_PID,
                         advisory='NR_UNSUPPORTED_COMMAND_CLASS would be more '
                                  'appropriate for a mandatory PID')
    ])
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class AllSubDevicesGetDeviceInfo(TestMixins.AllSubDevicesGetMixin,
                                 DeviceInfoTest,
                                 ResponderTestFixture):
  """Send a Get Device Info to ALL_SUB_DEVICES."""


# Supported Parameters Tests & Mixin
# -----------------------------------------------------------------------------
class GetSupportedParameters(ResponderTestFixture):
  """GET supported parameters."""
  CATEGORY = TestCategory.CORE
  PID = 'SUPPORTED_PARAMETERS'
  PROVIDES = ['manufacturer_parameters', 'supported_parameters',
              'acks_supported_parameters']

  # Declaring support for any of these is a warning:
  MANDATORY_PIDS = ['SUPPORTED_PARAMETERS',
                    'PARAMETER_DESCRIPTION',
                    'DEVICE_INFO',
                    'SOFTWARE_VERSION_LABEL',
                    'DMX_START_ADDRESS',
                    'IDENTIFY_DEVICE']

  # Banned PIDs, these are PID values that can not appear in the list of
  # supported parameters (these are used for discovery)
  BANNED_PIDS = ['DISC_UNIQUE_BRANCH',
                 'DISC_MUTE',
                 'DISC_UN_MUTE']

  # If responders support any of the PIDs in these groups, they should really
  # support all of them.
  PID_GROUPS = [
      ('PROXIED_DEVICE_COUNT', 'PROXIED_DEVICES'),
      ('LANGUAGE_CAPABILITIES', 'LANGUAGE'),
      ('DMX_PERSONALITY', 'DMX_PERSONALITY_DESCRIPTION'),
      ('SENSOR_DEFINITION', 'SENSOR_VALUE'),
      ('SELF_TEST_DESCRIPTION', 'PERFORM_SELFTEST'),
  ]

  # If the first PID is supported, the PIDs in the group must be.
  PID_DEPENDENCIES = [
      ('RECORD_SENSORS', ['SENSOR_VALUE']),
      ('DEFAULT_SLOT_VALUE', ['SLOT_DESCRIPTION']),
      ('CURVE', ['CURVE_DESCRIPTION']),
      ('OUTPUT_RESPONSE_TIME', ['OUTPUT_RESPONSE_TIME_DESCRIPTION']),
      ('MODULATION_FREQUENCY', ['MODULATION_FREQUENCY_DESCRIPTION']),
      ('LOCK_STATE', ['LOCK_STATE_DESCRIPTION']),
  ]

  # If any of the PIDs in the group are supported, the first one must be too.
  PID_REVERSE_DEPENDENCIES = [
      ('LIST_INTERFACES',
       ['INTERFACE_LABEL',
        'INTERFACE_HARDWARE_ADDRESS_TYPE1', 'IPV4_DHCP_MODE',
        'IPV4_ZEROCONF_MODE', 'IPV4_CURRENT_ADDRESS', 'IPV4_STATIC_ADDRESS',
        'INTERFACE_RENEW_DHCP', 'INTERFACE_RELEASE_DHCP',
        'INTERFACE_APPLY_CONFIGURATION', 'IPV4_DEFAULT_ROUTE',
        'DNS_IPV4_NAME_SERVER', 'DNS_HOSTNAME', 'DNS_DOMAIN_NAME']),
  ]

  def Test(self):
    self.AddExpectedResults([
      # TODO(simon): We should cross check this against support for anything
      # more than the required set of parameters at the end of all tests.
      self.NackGetResult(RDMNack.NR_UNKNOWN_PID),
      self.AckGetResult(),
    ])
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      self.SetProperty('manufacturer_parameters', [])
      self.SetProperty('supported_parameters', [])
      self.SetProperty('acks_supported_parameters', False)
      return

    self.SetProperty('acks_supported_parameters', True)
    mandatory_pids = {}
    for p in self.MANDATORY_PIDS:
      pid = self.LookupPid(p)
      mandatory_pids[pid.value] = pid

    banned_pids = {}
    for p in self.BANNED_PIDS:
      pid = self.LookupPid(p)
      banned_pids[pid.value] = pid

    supported_parameters = []
    manufacturer_parameters = []
    count_by_pid = {}

    for item in fields['params']:
      param_id = item['param_id']
      count_by_pid[param_id] = count_by_pid.get(param_id, 0) + 1
      if param_id in banned_pids:
        self.AddWarning('%s listed in supported parameters' %
                        banned_pids[param_id].name)
        continue

      if param_id in mandatory_pids:
        self.AddAdvisory('%s listed in supported parameters' %
                         mandatory_pids[param_id].name)
        continue

      supported_parameters.append(param_id)
      if (param_id >= RDM_MANUFACTURER_PID_MIN and
          param_id <= RDM_MANUFACTURER_PID_MAX):
        manufacturer_parameters.append(param_id)

    # Check for duplicate PIDs
    for pid, count in count_by_pid.items():
      if count > 1:
        pid_obj = self.LookupPidValue(pid)
        if pid_obj:
          self.AddAdvisory('%s listed %d times in supported parameters' %
                           (pid_obj, count))
        else:
          self.AddAdvisory('PID 0x%hx listed %d times in supported parameters' %
                           (pid, count))

    self.SetProperty('manufacturer_parameters', manufacturer_parameters)
    self.SetProperty('supported_parameters', supported_parameters)

    for pid_names in self.PID_GROUPS:
      supported_pids = []
      unsupported_pids = []
      for pid_name in pid_names:
        pid = self.LookupPid(pid_name)
        if pid.value in supported_parameters:
          supported_pids.append(pid.name)
        else:
          unsupported_pids.append(pid.name)

      if supported_pids and unsupported_pids:
        self.AddAdvisory(
            '%s supported but %s is not' %
            (','.join(supported_pids), ','.join(unsupported_pids)))

    for p, dependent_pids in self.PID_DEPENDENCIES:
      pid = self.LookupPid(p)
      if pid is None:
        self.SetBroken('Failed to lookup info for PID %s' % p)
        return

      if pid.value not in supported_parameters:
        continue

      unsupported_pids = []
      for pid_name in dependent_pids:
        pid = self.LookupPid(pid_name)
        if pid is None:
          self.SetBroken('Failed to lookup info for PID %s' % pid_name)
          return

        if pid.value not in supported_parameters:
          unsupported_pids.append(pid.name)
      if unsupported_pids:
        self.AddAdvisory('%s supported but %s is not' %
                         (p, ','.join(unsupported_pids)))

    for p, rev_dependent_pids in self.PID_REVERSE_DEPENDENCIES:
      if self.LookupPid(p).value in supported_parameters:
        continue

      dependent_pids = []
      for pid_name in rev_dependent_pids:
        pid = self.LookupPid(pid_name)
        if pid is None:
          self.SetBroken('Failed to lookup info for PID %s' % pid_name)
          return

        if pid.value in supported_parameters:
          dependent_pids.append(pid.name)
      if (dependent_pids and
         (self.LookupPid(p).value in supported_parameters)):
        self.AddAdvisory('%s supported but %s is not' %
                         (','.join(unsupported_pids), p))


class GetSupportedParametersWithData(TestMixins.GetWithDataMixin,
                                     ResponderTestFixture):
  """GET supported parameters with param data."""
  PID = 'SUPPORTED_PARAMETERS'
  REQUIRES = ['acks_supported_parameters']

  def Test(self):
    if self.Property('acks_supported_parameters'):
      self.AddExpectedResults([
        self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
        self.AckGetResult(
          warning='Get %s with data returned an ack' % self.pid.name)
      ])
    else:
      self.AddExpectedResults(self.NackGetResult(RDMNack.NR_UNKNOWN_PID))
    self.SendRawGet(ROOT_DEVICE, self.pid, self.DATA)


class SetSupportedParameters(TestMixins.UnsupportedSetMixin,
                             ResponderTestFixture):
  """Attempt to SET supported parameters."""
  PID = 'SUPPORTED_PARAMETERS'


class SetSupportedParametersWithData(TestMixins.UnsupportedSetWithDataMixin,
                                     ResponderTestFixture):
  """SET supported parameters with data."""
  PID = 'SUPPORTED_PARAMETERS'


class AllSubDevicesGetSupportedParameters(TestMixins.AllSubDevicesGetMixin,
                                          ResponderTestFixture):
  """Send a Get SUPPORTED_PARAMETERS to ALL_SUB_DEVICES."""
  PID = 'SUPPORTED_PARAMETERS'


class GetSubDeviceSupportedParameters(ResponderTestFixture):
  """Check that SUPPORTED_PARAMETERS is consistent across sub devices."""
  CATEGORY = TestCategory.SUB_DEVICES
  PID = 'SUPPORTED_PARAMETERS'
  REQUIRES = ['sub_device_addresses']
  PROVIDES = ['sub_device_supported_parameters']

  # E1.37, 2.1 Sub devices are required to support these.
  MANDATORY_PIDS = ['SUPPORTED_PARAMETERS',
                    'DEVICE_INFO',
                    'SOFTWARE_VERSION_LABEL',
                    'IDENTIFY_DEVICE']

  def Test(self):
    self._sub_devices = list(self.Property('sub_device_addresses').keys())
    self._sub_devices.reverse()
    self._params = {}
    self._GetSupportedParams()

  def _GetSupportedParams(self):
    if not self._sub_devices:
      self._CheckForConsistency()
      self.Stop()
      return

    self.AddExpectedResults(self.AckGetResult(action=self._GetSupportedParams))
    self.SendGet(self._sub_devices[-1], self.pid)

  def VerifyResult(self, response, fields):
    sub_device = self._sub_devices.pop()
    supported_params = set()
    for p in fields['params']:
      supported_params.add(p['param_id'])
    self._params[sub_device] = supported_params

  def _CheckForConsistency(self):
    if not self._params:
      return

    supported_pids = set()
    for pids in self._params.values():
      if not supported_pids:
        supported_pids = pids
      elif supported_pids != pids:
        self.SetFailed('SUPPORTED_PARAMETERS for sub-devices do not match')
        return

    mandatory_pids = set(self.LookupPid(p).value for p in self.MANDATORY_PIDS)
    missing_pids = mandatory_pids - supported_pids
    if missing_pids:
      self.SetFailed("Missing PIDs %s from sub device's supported pid list " %
                     ', '.join('0x%04hx' % p for p in missing_pids))
      return

    self.SetProperty('sub_device_supported_parameters', supported_pids)


# Sub Devices Test
# -----------------------------------------------------------------------------
class FindSubDevices(ResponderTestFixture):
  """Locate the sub devices by sending DEVICE_INFO messages."""
  CATEGORY = TestCategory.SUB_DEVICES
  PID = 'DEVICE_INFO'
  PROVIDES = ['sub_device_addresses', 'sub_device_footprints']
  REQUIRES = ['sub_device_count']

  def Test(self):
    self._device_count = self.Property('sub_device_count')
    self._sub_device_addresses = {}  # Index to start address mapping
    self._sub_device_footprints = {}  # Index to footprint mapping
    self._current_index = 0  # the current sub device we're trying to query
    self._CheckForSubDevice()

  def _CheckForSubDevice(self):
    # For each supported param message we should either see a sub device out of
    # range or an ack
    if len(self._sub_device_addresses) == self._device_count:
      if self._device_count == 0:
        self.SetNotRun('No sub devices declared')
      self.SetProperty('sub_device_addresses', self._sub_device_addresses)
      self.SetProperty('sub_device_footprints', self._sub_device_footprints)
      self.Stop()
      return

    if self._current_index >= PidStore.MAX_VALID_SUB_DEVICE:
      self.SetFailed('Only found %d of %d sub devices' %
                     (len(self._sub_device_addresses), self._device_count))
      return

    self.AddExpectedResults([
      self.NackGetResult(RDMNack.NR_SUB_DEVICE_OUT_OF_RANGE,
                         action=self._CheckForSubDevice),
      self.AckGetResult(action=self._CheckForSubDevice)
    ])
    self._current_index += 1
    self.SendGet(self._current_index, self.pid)

  def VerifyResult(self, response, fields):
    if response.WasAcked():
      if fields['sub_device_count'] != self._device_count:
        self.SetFailed(
            'For sub-device %d, DEVICE_INFO reported %d sub devices '
            ' but the root device reported %s. See section 10.5.1' %
            (self._current_index, fields['sub_device_count'],
             self._device_count))
      self._sub_device_addresses[self._current_index] = (
          fields['dmx_start_address'])
      self._sub_device_footprints[self._current_index] = fields['dmx_footprint']


# Status Messages
# -----------------------------------------------------------------------------
# TODO(Peter): These might all upset queued messages?
class AllSubDevicesGetStatusMessages(TestMixins.AllSubDevicesGetMixin,
                                     OptionalParameterTestFixture):
  """Send a get STATUS_MESSAGES to ALL_SUB_DEVICES."""
  PID = 'STATUS_MESSAGES'
  DATA = [0x00]


# class GetStatusMessages(TestMixins.,
#                         OptionalParameterTestFixture):
#   CATEGORY = TestCategory.
#   PID = 'STATUS_MESSAGES'
# TODO(Peter): Test get, use STATUS_NONE (0x00)


class GetStatusMessagesWithNoData(TestMixins.GetWithNoDataMixin,
                                  OptionalParameterTestFixture):
  """GET STATUS_MESSAGES with no argument given."""
  PID = 'STATUS_MESSAGES'


class GetStatusMessagesWithExtraData(TestMixins.GetWithDataMixin,
                                     OptionalParameterTestFixture):
  """GET STATUS_MESSAGES with more than 1 byte of data."""
  PID = 'STATUS_MESSAGES'


class SetStatusMessages(TestMixins.UnsupportedSetMixin,
                        OptionalParameterTestFixture):
  """Attempt to SET STATUS_MESSAGES."""
  PID = 'STATUS_MESSAGES'


class SetStatusMessagesWithData(TestMixins.UnsupportedSetWithDataMixin,
                                OptionalParameterTestFixture):
  """Attempt to SET STATUS_MESSAGES with data."""
  PID = 'STATUS_MESSAGES'


# Status ID Description
# -----------------------------------------------------------------------------
class AllSubDevicesGetStatusIdDescription(TestMixins.AllSubDevicesGetMixin,
                                          OptionalParameterTestFixture):
  """Send a get STATUS_ID_DESCRIPTION to ALL_SUB_DEVICES."""
  PID = 'STATUS_ID_DESCRIPTION'
  DATA = [0x0001]


# class GetStatusIdDescription(TestMixins.,
#                              OptionalParameterTestFixture):
#   CATEGORY = TestCategory.STATUS_COLLECTION
#   PID = 'STATUS_ID_DESCRIPTION'
# TODO(Peter): Test get


class GetStatusIdDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                       OptionalParameterTestFixture):
  """GET STATUS_ID_DESCRIPTION with no argument given."""
  PID = 'STATUS_ID_DESCRIPTION'


class GetStatusIdDescriptionWithExtraData(TestMixins.GetWithDataMixin,
                                          OptionalParameterTestFixture):
  """GET STATUS_ID_DESCRIPTION with more than 2 bytes of data."""
  PID = 'STATUS_ID_DESCRIPTION'


class SetStatusIdDescription(TestMixins.UnsupportedSetMixin,
                             OptionalParameterTestFixture):
  """Attempt to SET STATUS_ID_DESCRIPTION."""
  PID = 'STATUS_ID_DESCRIPTION'


class SetStatusIdDescriptionWithData(TestMixins.UnsupportedSetWithDataMixin,
                                     OptionalParameterTestFixture):
  """Attempt to SET STATUS_ID_DESCRIPTION with data."""
  PID = 'STATUS_ID_DESCRIPTION'


# Clear Status ID
# -----------------------------------------------------------------------------
class GetClearStatusMessages(TestMixins.UnsupportedGetMixin,
                             OptionalParameterTestFixture):
  """GET clear status id."""
  PID = 'CLEAR_STATUS_ID'


class ClearStatusMessagesWithData(TestMixins.SetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Clear the status message queue with extra data."""
  PID = 'CLEAR_STATUS_ID'


class ClearStatusMessages(OptionalParameterTestFixture):
  """Clear the status message queue."""
  CATEGORY = TestCategory.STATUS_COLLECTION
  PID = 'CLEAR_STATUS_ID'

  def Test(self):
    # I don't believe there is a reliable way to check that the queue is
    # cleared. Note that this PID should only clear status messages, not
    # responses to ACK_TIMERS so we can't check if the message count is 0.
    self.AddIfSetSupported(self.AckSetResult())
    self.SendSet(ROOT_DEVICE, self.pid, [])


# Sub device status report threshold
# -----------------------------------------------------------------------------
class AllSubDevicesGetSubDeviceStatusReportThreshold(
        TestMixins.AllSubDevicesGetMixin,
        OptionalParameterTestFixture):
  """Send a get SUB_DEVICE_STATUS_REPORT_THRESHOLD to ALL_SUB_DEVICES."""
  PID = 'SUB_DEVICE_STATUS_REPORT_THRESHOLD'


# class GetSubDeviceStatusReportThreshold(TestMixins.,
#                                         OptionalParameterTestFixture):
#   CATEGORY = TestCategory.
#   PID = 'SUB_DEVICE_STATUS_REPORT_THRESHOLD'
# TODO(Peter): Test get


class GetSubDeviceStatusReportThresholdWithData(TestMixins.GetWithDataMixin,
                                                OptionalParameterTestFixture):
  """GET SUB_DEVICE_STATUS_REPORT_THRESHOLD with data."""
  PID = 'SUB_DEVICE_STATUS_REPORT_THRESHOLD'


# class SetSubDeviceStatusReportThreshold(TestMixins.,
#                                         OptionalParameterTestFixture):
#   CATEGORY = TestCategory.
#   PID = 'SUB_DEVICE_STATUS_REPORT_THRESHOLD'
# TODO(Peter): Test set


class SetSubDeviceStatusReportThresholdWithNoData(TestMixins.SetWithNoDataMixin,
                                                  OptionalParameterTestFixture):
  """Set SUB_DEVICE_STATUS_REPORT_THRESHOLD command with no data."""
  PID = 'SUB_DEVICE_STATUS_REPORT_THRESHOLD'


class SetSubDeviceStatusReportThresholdWithExtraData(
        TestMixins.SetWithDataMixin,
        OptionalParameterTestFixture):
  """Send a SET SUB_DEVICE_STATUS_REPORT_THRESHOLD command with extra data."""
  PID = 'SUB_DEVICE_STATUS_REPORT_THRESHOLD'


# Parameter Description
# -----------------------------------------------------------------------------
class GetParameterDescription(ParamDescriptionTestFixture):
  """Check that GET parameter description works for any manufacturer params."""
  CATEGORY = TestCategory.RDM_INFORMATION
  PID = 'PARAMETER_DESCRIPTION'
  REQUIRES = ['manufacturer_parameters']

  def Test(self):
    self.params = self.Property('manufacturer_parameters')[:]
    if len(self.params) == 0:
      self.SetNotRun('No manufacturer params found')
      # This case is tested in GetParameterDescriptionForNonManufacturerPid
      return
    self._GetParam()

  def _GetParam(self):
    if len(self.params) == 0:
      self.Stop()
      return

    self.AddExpectedResults(
      self.AckGetResult(action=self._GetParam))
    self.current_param = self.params.pop()
    self.SendGet(ROOT_DEVICE, self.pid, [self.current_param])

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
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

    if ContainsUnprintable(fields['description']):
      self.AddAdvisory(
          'Description field in %s contains unprintable characters, was %s' %
          (self.pid.name, StringEscape(fields['description'])))


class GetParameterDescriptionForNonManufacturerPid(ParamDescriptionTestFixture):
  """GET parameter description for a non-manufacturer pid."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PARAMETER_DESCRIPTION'
  REQUIRES = ['manufacturer_parameters']

  def Test(self):
    device_info_pid = self.LookupPid('DEVICE_INFO')
    results = [
      self.NackGetResult(RDMNack.NR_UNKNOWN_PID),
      self.NackGetResult(
          RDMNack.NR_DATA_OUT_OF_RANGE,
          advisory='Parameter Description appears to be supported but no '
                   'manufacturer PIDs were declared'),
    ]
    if self.Property('manufacturer_parameters'):
      results = self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE)

    self.AddExpectedResults(results)
    self.SendGet(ROOT_DEVICE, self.pid, [device_info_pid.value])


class GetParameterDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                        ParamDescriptionTestFixture):
  """GET PARAMETER_DESCRIPTION with no argument given."""
  PID = 'PARAMETER_DESCRIPTION'


class GetParameterDescriptionWithExtraData(TestMixins.GetWithDataMixin,
                                           ParamDescriptionTestFixture):
  """GET parameter description with extra param data."""
  PID = 'PARAMETER_DESCRIPTION'
  REQUIRES = ['manufacturer_parameters']

  def Test(self):
    results = [
      self.NackGetResult(RDMNack.NR_UNKNOWN_PID),
      self.NackGetResult(RDMNack.NR_FORMAT_ERROR,
                         advisory='Parameter Description appears to be '
                                  'supported but no manufacturer PIDs were '
                                  'declared'),
    ]
    if self.Property('manufacturer_parameters'):
      results = self.NackGetResult(RDMNack.NR_FORMAT_ERROR)
    self.AddExpectedResults(results)
    self.SendRawGet(ROOT_DEVICE, self.pid, self.DATA)


class SetParameterDescription(TestMixins.UnsupportedSetMixin,
                              ParamDescriptionTestFixture):
  """SET the parameter description."""
  PID = 'PARAMETER_DESCRIPTION'


class SetParameterDescriptionWithData(TestMixins.UnsupportedSetWithDataMixin,
                                      ParamDescriptionTestFixture):
  """SET the parameter description with data."""
  PID = 'PARAMETER_DESCRIPTION'


class AllSubDevicesGetParameterDescription(TestMixins.AllSubDevicesGetMixin,
                                           ParamDescriptionTestFixture):
  """Send a Get PARAMETER_DESCRIPTION to ALL_SUB_DEVICES."""
  PID = 'PARAMETER_DESCRIPTION'
  DATA = [0x8000]


# Proxied Device Count
# -----------------------------------------------------------------------------
class GetProxiedDeviceCount(OptionalParameterTestFixture):
  """GET the proxied device count."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  PID = 'PROXIED_DEVICE_COUNT'
  REQUIRES = ['proxied_devices']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, unpacked_data):
    if not response.WasAcked():
      return

    proxied_devices = self.Property('proxied_devices')
    if proxied_devices is None:
      self.AddWarning(
         'PROXIED_DEVICE_COUNT ack\'ed but PROXIED_DEVICES didn\'t')
      return

    if not unpacked_data['list_changed']:
      # We expect the count to match the length of the list previously returned
      if unpacked_data['device_count'] != len(proxied_devices):
        self.SetFailed(
           'Proxied device count doesn\'t match number of devices returned')


class GetProxiedDeviceCountWithData(TestMixins.GetWithDataMixin,
                                    OptionalParameterTestFixture):
  """GET the proxied device count with extra data."""
  PID = 'PROXIED_DEVICE_COUNT'


class SetProxiedDeviceCount(TestMixins.UnsupportedSetMixin,
                            OptionalParameterTestFixture):
  """SET the count of proxied devices."""
  PID = 'PROXIED_DEVICE_COUNT'


class SetProxiedDeviceCountWithData(TestMixins.UnsupportedSetWithDataMixin,
                                    OptionalParameterTestFixture):
  """SET the count of proxied devices with data."""
  PID = 'PROXIED_DEVICE_COUNT'


class AllSubDevicesGetProxiedDeviceCount(TestMixins.AllSubDevicesGetMixin,
                                         OptionalParameterTestFixture):
  """Send a Get PROXIED_DEVICE_COUNT to ALL_SUB_DEVICES."""
  PID = 'PROXIED_DEVICE_COUNT'


# Proxied Devices
# -----------------------------------------------------------------------------
class GetProxiedDevices(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the list of proxied devices."""
  CATEGORY = TestCategory.NETWORK_MANAGEMENT
  PID = 'PROXIED_DEVICES'
  EXPECTED_FIELDS = ['uids']
  PROVIDES = ['proxied_devices']


class GetProxiedDevicesWithData(TestMixins.GetWithDataMixin,
                                OptionalParameterTestFixture):
  """GET the list of proxied devices with extra data."""
  PID = 'PROXIED_DEVICES'


class SetProxiedDevices(TestMixins.UnsupportedSetMixin,
                        OptionalParameterTestFixture):
  """SET the list of proxied devices."""
  PID = 'PROXIED_DEVICES'


class SetProxiedDevicesWithData(TestMixins.UnsupportedSetWithDataMixin,
                                OptionalParameterTestFixture):
  """SET the list of proxied devices with data."""
  PID = 'PROXIED_DEVICES'


class AllSubDevicesGetProxiedDevices(TestMixins.AllSubDevicesGetMixin,
                                     OptionalParameterTestFixture):
  """Send a Get PROXIED_DEVICES to ALL_SUB_DEVICES."""
  PID = 'PROXIED_DEVICES'


# Comms Status
# -----------------------------------------------------------------------------
class GetCommsStatus(OptionalParameterTestFixture):
  """GET the comms status."""
  CATEGORY = TestCategory.STATUS_COLLECTION
  PID = 'COMMS_STATUS'

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)


class GetCommsStatusWithData(TestMixins.GetWithDataMixin,
                             OptionalParameterTestFixture):
  """GET the comms status with extra data."""
  PID = 'COMMS_STATUS'


class ClearCommsStatus(OptionalParameterTestFixture):
  """Clear the comms status."""
  CATEGORY = TestCategory.STATUS_COLLECTION
  PID = 'COMMS_STATUS'

  def Test(self):
    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(ROOT_DEVICE, self.pid)

  def VerifySet(self):
    self.AddIfGetSupported(
        self.AckGetResult(field_values={
            'short_message': 0,
            'length_mismatch': 0,
            'checksum_fail': 0
        }))
    self.SendGet(ROOT_DEVICE, self.pid)


class ClearCommsStatusWithData(TestMixins.SetWithDataMixin,
                               OptionalParameterTestFixture):
  """Clear the comms status with data."""
  PID = 'COMMS_STATUS'


class AllSubDevicesGetCommsStatus(TestMixins.AllSubDevicesGetMixin,
                                  OptionalParameterTestFixture):
  """Send a Get COMMS_STATUS to ALL_SUB_DEVICES."""
  PID = 'COMMS_STATUS'


# Product Detail Id List
# -----------------------------------------------------------------------------
class GetProductDetailIdList(OptionalParameterTestFixture):
  """GET the list of product detail ids."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'PRODUCT_DETAIL_ID_LIST'

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult(field_names=['detail_ids']))
    self.SendGet(ROOT_DEVICE, self.pid)


class GetProductDetailIdListWithData(TestMixins.GetWithDataMixin,
                                     OptionalParameterTestFixture):
  """GET product detail id list with param data."""
  PID = 'PRODUCT_DETAIL_ID_LIST'


class SetProductDetailIdList(TestMixins.UnsupportedSetMixin,
                             OptionalParameterTestFixture):
  """SET product detail id list."""
  PID = 'PRODUCT_DETAIL_ID_LIST'


class SetProductDetailIdListWithData(TestMixins.UnsupportedSetWithDataMixin,
                                     OptionalParameterTestFixture):
  """SET product detail id list with data."""
  PID = 'PRODUCT_DETAIL_ID_LIST'


class AllSubDevicesGetProductDetailIdList(TestMixins.AllSubDevicesGetMixin,
                                          OptionalParameterTestFixture):
  """Send a Get PRODUCT_DETAIL_ID_LIST to ALL_SUB_DEVICES."""
  PID = 'PRODUCT_DETAIL_ID_LIST'


# Device Model Description
# -----------------------------------------------------------------------------
class GetDeviceModelDescription(TestMixins.GetStringMixin,
                                OptionalParameterTestFixture):
  """GET the device model description."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_MODEL_DESCRIPTION'
  EXPECTED_FIELDS = ['description']
  PROVIDES = ['model_description']


class GetDeviceModelDescriptionWithData(TestMixins.GetWithDataMixin,
                                        OptionalParameterTestFixture):
  """Get device model description with param data."""
  PID = 'DEVICE_MODEL_DESCRIPTION'


class SetDeviceModelDescription(TestMixins.UnsupportedSetMixin,
                                OptionalParameterTestFixture):
  """Attempt to SET the device model description with no data."""
  PID = 'DEVICE_MODEL_DESCRIPTION'


class SetDeviceModelDescriptionWithData(TestMixins.UnsupportedSetWithDataMixin,
                                        OptionalParameterTestFixture):
  """SET the device model description with data."""
  PID = 'DEVICE_MODEL_DESCRIPTION'


class AllSubDevicesGetDeviceModelDescription(TestMixins.AllSubDevicesGetMixin,
                                             OptionalParameterTestFixture):
  """Send a Get DEVICE_MODEL_DESCRIPTION to ALL_SUB_DEVICES."""
  PID = 'DEVICE_MODEL_DESCRIPTION'


# Manufacturer Label
# -----------------------------------------------------------------------------
class GetManufacturerLabel(TestMixins.GetStringMixin,
                           OptionalParameterTestFixture):
  """GET the manufacturer label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'MANUFACTURER_LABEL'
  EXPECTED_FIELDS = ['label']
  PROVIDES = ['manufacturer_label']


class GetManufacturerLabelWithData(TestMixins.GetWithDataMixin,
                                   OptionalParameterTestFixture):
  """Get manufacturer label with param data."""
  PID = 'MANUFACTURER_LABEL'


class SetManufacturerLabel(TestMixins.UnsupportedSetMixin,
                           OptionalParameterTestFixture):
  """Attempt to SET the manufacturer label with no data."""
  PID = 'MANUFACTURER_LABEL'


class SetManufacturerLabelWithData(TestMixins.UnsupportedSetWithDataMixin,
                                   OptionalParameterTestFixture):
  """SET the manufacturer label with data."""
  PID = 'MANUFACTURER_LABEL'


class AllSubDevicesGetManufacturerLabel(TestMixins.AllSubDevicesGetMixin,
                                        OptionalParameterTestFixture):
  """Send a Get MANUFACTURER_LABEL to ALL_SUB_DEVICES."""
  PID = 'MANUFACTURER_LABEL'


# Device Label
# -----------------------------------------------------------------------------
class GetDeviceLabel(TestMixins.GetStringMixin,
                     OptionalParameterTestFixture):
  """GET the device label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  PROVIDES = ['device_label']
  EXPECTED_FIELDS = ['label']


class GetDeviceLabelWithData(TestMixins.GetWithDataMixin,
                             OptionalParameterTestFixture):
  """GET the device label with param data."""
  PID = 'DEVICE_LABEL'


class SetDeviceLabel(TestMixins.SetLabelMixin,
                     OptionalParameterTestFixture):
  """SET the device label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  REQUIRES = ['device_label']
  PROVIDES = ['set_device_label_supported']

  def OldValue(self):
    return self.Property('device_label')


class AllSubDevicesGetDeviceLabel(TestMixins.AllSubDevicesGetMixin,
                                  OptionalParameterTestFixture):
  """Send a Get DEVICE_LABEL to ALL_SUB_DEVICES."""
  PID = 'DEVICE_LABEL'


class SetVendorcastDeviceLabel(TestMixins.SetNonUnicastLabelMixin,
                               OptionalParameterTestFixture):
  """SET the device label using the vendorcast address."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  REQUIRES = ['device_label', 'set_device_label_supported']
  TEST_LABEL = 'vendorcast label'

  def Uid(self):
    return UID.VendorcastAddress(self._uid.manufacturer_id)

  def OldValue(self):
    return self.Property('device_label')


class SetBroadcastDeviceLabel(TestMixins.SetNonUnicastLabelMixin,
                              OptionalParameterTestFixture):
  """SET the device label using the broadcast address."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  REQUIRES = ['device_label', 'set_device_label_supported']
  TEST_LABEL = 'broadcast label'

  def Uid(self):
    return UID.AllDevices()

  def OldValue(self):
    return self.Property('device_label')


class SetFullSizeDeviceLabel(TestMixins.SetLabelMixin,
                             OptionalParameterTestFixture):
  """SET the device label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  REQUIRES = ['device_label']
  TEST_LABEL = 'this is a string with 32 charact'

  def OldValue(self):
    return self.Property('device_label')


class SetNonPrintableAsciiDeviceLabel(TestMixins.SetLabelMixin,
                                      OptionalParameterTestFixture):
  """SET the device label to something that contains non-printable ASCII
     characters.
  """
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  REQUIRES = ['device_label']
  TEST_LABEL = 'str w\x0d non\x1bprint ASCII\x7f'

  def ExpectedResults(self):
    return [
      self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
      self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.AckSetResult(action=self.VerifySet)
    ]

  def OldValue(self):
    return self.Property('device_label')


# TODO(Peter): Get this test to work where we just compare the returned string
# as bytes so we don't try and fail to decode it as ASCII/UTF-8
# class SetNonAsciiDeviceLabel(TestMixins.SetLabelMixin,
#                              OptionalParameterTestFixture):
#   """SET the device label to something that contains non-ASCII data."""
#   CATEGORY = TestCategory.PRODUCT_INFORMATION
#   PID = 'DEVICE_LABEL'
#   REQUIRES = ['device_label']
#   # Store directly as bytes so we don't try and decode as UTF-8
#   TEST_LABEL = b'string with\x0d non ASCII\xc0'
#
#   def ExpectedResults(self):
#     return [
#       self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
#       self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
#       self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
#       self.AckSetResult(action=self.VerifySet)
#     ]
#
#   def OldValue(self):
#     return self.Property('device_label')
#
#   def Test(self):
#     # We have to override test here as this has to be raw as we can't encode
#     # it
#     # on Python 3 as it turns it to UTF-8 or escapes it
#     # It's also technically out of spec for E1.20 unless it's sent as UTF-8
#     self._test_state = self.SET
#     self.AddIfSetSupported(self.ExpectedResults())
#     self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.TEST_LABEL)


class SetEmptyDeviceLabel(TestMixins.SetLabelMixin,
                          OptionalParameterTestFixture):
  """SET the device label with no data."""
  # AKA SetDeviceLabelWithNoData
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'DEVICE_LABEL'
  REQUIRES = ['device_label']
  TEST_LABEL = ''

  def OldValue(self):
    return self.Property('device_label')


class SetOversizedDeviceLabel(TestMixins.SetOversizedLabelMixin,
                              OptionalParameterTestFixture):
  """SET the device label with more than 32 bytes of data."""
  # AKA SetDeviceLabelWithExtraData
  REQUIRES = ['device_label']
  PID = 'DEVICE_LABEL'

  def OldValue(self):
    return self.Property('device_label')


# Language Capabilities
# -----------------------------------------------------------------------------
class GetLanguageCapabilities(OptionalParameterTestFixture):
  """GET the language capabilities pid."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE_CAPABILITIES'
  PROVIDES = ['languages_capabilities']

  def Test(self):
    self.languages = []
    self.AddIfGetSupported(self.AckGetResult(field_names=['languages']))
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      self.SetProperty('languages_capabilities', [])
      return

    self.languages = [f['language'] for f in fields['languages']]

    if len(self.languages) == 0:
      self.AddWarning('No languages returned for LANGUAGE_CAPABILITIES')

    language_set = set()
    for language in self.languages:
      if language in language_set:
        self.AddAdvisory('%s listed twice in language capabilities' % language)
      language_set.add(language)
      if ContainsUnprintable(language):
        self.AddAdvisory(
            'Language name in language capabilities contains unprintable '
            'characters, was %s' % StringEscape(language))

    self.SetProperty('languages_capabilities', language_set)


class GetLanguageCapabilitiesWithData(TestMixins.GetWithDataMixin,
                                      OptionalParameterTestFixture):
  """GET the language capabilities with extra data."""
  PID = 'LANGUAGE_CAPABILITIES'


class SetLanguageCapabilities(TestMixins.UnsupportedSetMixin,
                              OptionalParameterTestFixture):
  """Attempt to SET the language capabilities with no data."""
  PID = 'LANGUAGE_CAPABILITIES'


class SetLanguageCapabilitiesWithData(TestMixins.UnsupportedSetWithDataMixin,
                                      OptionalParameterTestFixture):
  """SET the language capabilities with data."""
  PID = 'LANGUAGE_CAPABILITIES'


class AllSubDevicesGetLanguageCapabilities(TestMixins.AllSubDevicesGetMixin,
                                           OptionalParameterTestFixture):
  """Send a Get LANGUAGE_CAPABILITIES to ALL_SUB_DEVICES."""
  PID = 'LANGUAGE_CAPABILITIES'


# Language
# -----------------------------------------------------------------------------
class GetLanguage(TestMixins.GetStringMixin, OptionalParameterTestFixture):
  """GET the language."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'
  PROVIDES = ['language']
  EXPECTED_FIELDS = ['language']
  MIN_LENGTH = 2
  MAX_LENGTH = 2
  # TODO(Peter): We should cross check this against the declared list of
  # supported languages, and also that the language is alpha only


class GetLanguageWithData(TestMixins.GetWithDataMixin,
                          OptionalParameterTestFixture):
  """GET the language with extra data."""
  PID = 'LANGUAGE'


class SetLanguage(OptionalParameterTestFixture):
  """SET the language."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'
  REQUIRES = ['language', 'languages_capabilities']

  def Test(self):
    ack = self.AckSetResult(action=self.VerifySet)
    nack = self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)

    available_languages = list(self.Property('languages_capabilities'))
    if available_languages:
      if len(available_languages) > 1:
        # If the responder only supports 1 lang, we may not be able to set it
        self.AddIfSetSupported(ack)
        self.new_language = available_languages[0]
        if self.new_language == self.Property('language'):
          self.new_language = available_languages[1]
      else:
        self.new_language = available_languages[0]
        self.AddIfSetSupported([ack, nack])
    else:
      # Get languages returned no languages so we expect a nack
      self.AddIfSetSupported(nack)
      self.new_language = 'en'

    self.SendSet(ROOT_DEVICE, self.pid, [self.new_language])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(field_values={'language': self.new_language}))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetNumericLanguage(OptionalParameterTestFixture):
  """Try to set the language to ASCII numeric characters."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'

  def Test(self):
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(ROOT_DEVICE, self.pid, ['01'])


class SetNullLanguage(OptionalParameterTestFixture):
  """Try to set the language to two null ASCII characters."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'

  def Test(self):
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(ROOT_DEVICE, self.pid, ['\x00\x00'])


class SetNonPrintableAsciiLanguage(OptionalParameterTestFixture):
  """Try to set the language to non-printable ASCII characters."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'

  def Test(self):
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(ROOT_DEVICE, self.pid, ['\x1b\x7f'])


class SetNonAsciiLanguage(OptionalParameterTestFixture):
  """Try to set the language to non-ASCII characters."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'LANGUAGE'

  def Test(self):
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    # This has to be raw as we can't encode it on Python 3 as it turns it to
    # UTF-8 and too many characters
    # It's also technically out of spec for E1.20 unless it's sent as UTF-8 at
    # which point we're back at square one
    self.SendRawSet(ROOT_DEVICE, self.pid, b'\x0d\xc0')


class SetUnsupportedLanguage(OptionalParameterTestFixture):
  """Try to set a language that doesn't exist in Language Capabilities."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LANGUAGE'
  REQUIRES = ['languages_capabilities']

  def Test(self):
    if 'zz' in self.Property('languages_capabilities'):
      self.SetBroken('zz exists in the list of available languages')
      return

    self.AddIfSetSupported([
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
    ])
    self.SendSet(ROOT_DEVICE, self.pid, ['zz'])


class SetLanguageWithNoData(TestMixins.SetWithNoDataMixin,
                            OptionalParameterTestFixture):
  """Set LANGUAGE command with no data."""
  PID = 'LANGUAGE'


class SetLanguageWithExtraData(TestMixins.SetWithDataMixin,
                               OptionalParameterTestFixture):
  """Send a SET LANGUAGE command with extra data."""
  PID = 'LANGUAGE'


class AllSubDevicesGetLanguage(TestMixins.AllSubDevicesGetMixin,
                               OptionalParameterTestFixture):
  """Send a Get LANGUAGE to ALL_SUB_DEVICES."""
  PID = 'LANGUAGE'


# Software Version Label
# -----------------------------------------------------------------------------
class GetSoftwareVersionLabel(TestMixins.GetRequiredStringMixin,
                              ResponderTestFixture):
  """GET the software version label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'SOFTWARE_VERSION_LABEL'
  EXPECTED_FIELDS = ['label']
  # This makes it a fail, it should probably be an advisory, but we ought to
  # flag mandatory PIDs with no label returned
  MIN_LENGTH = 1


class GetSoftwareVersionLabelWithData(TestMixins.GetMandatoryPIDWithDataMixin,
                                      ResponderTestFixture):
  """GET the software_version_label with param data."""
  PID = 'SOFTWARE_VERSION_LABEL'


class SetSoftwareVersionLabel(TestMixins.UnsupportedSetMixin,
                              ResponderTestFixture):
  """Attempt to SET the software version label."""
  PID = 'SOFTWARE_VERSION_LABEL'


class SetSoftwareVersionLabelWithData(TestMixins.UnsupportedSetWithDataMixin,
                                      ResponderTestFixture):
  """Attempt to SET the software version label with data."""
  PID = 'SOFTWARE_VERSION_LABEL'


class AllSubDevicesGetSoftwareVersionLabel(TestMixins.AllSubDevicesGetMixin,
                                           ResponderTestFixture):
  """Send a Get SOFTWARE_VERSION_LABEL to ALL_SUB_DEVICES."""
  PID = 'SOFTWARE_VERSION_LABEL'


class GetSubDeviceSoftwareVersionLabel(ResponderTestFixture):
  """Check that SOFTWARE_VERSION_LABEL is supported on all sub devices."""
  CATEGORY = TestCategory.SUB_DEVICES
  PID = 'SOFTWARE_VERSION_LABEL'
  REQUIRES = ['sub_device_addresses']

  def Test(self):
    self._sub_devices = list(self.Property('sub_device_addresses').keys())
    self._sub_devices.reverse()
    self._GetSoftwareVersion()

  def _GetSoftwareVersion(self):
    if not self._sub_devices:
      self.Stop()
      return

    self.AddExpectedResults(self.AckGetResult(action=self._GetSoftwareVersion))
    sub_device = self._sub_devices.pop()
    self.SendGet(sub_device, self.pid)


# Boot Software Version
# -----------------------------------------------------------------------------
class GetBootSoftwareVersionId(OptionalParameterTestFixture):
  """GET the boot software version."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'BOOT_SOFTWARE_VERSION_ID'

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult(field_names=['version']))
    self.SendGet(ROOT_DEVICE, self.pid)


class GetBootSoftwareVersionIdWithData(TestMixins.GetWithDataMixin,
                                       OptionalParameterTestFixture):
  """GET the boot software version with extra data."""
  PID = 'BOOT_SOFTWARE_VERSION_ID'


class SetBootSoftwareVersionId(TestMixins.UnsupportedSetMixin,
                               OptionalParameterTestFixture):
  """Attempt to SET the boot software version."""
  PID = 'BOOT_SOFTWARE_VERSION_ID'


class SetBootSoftwareVersionIdWithData(TestMixins.UnsupportedSetWithDataMixin,
                                       OptionalParameterTestFixture):
  """Attempt to SET the boot software version with data."""
  PID = 'BOOT_SOFTWARE_VERSION_ID'


class AllSubDevicesGetBootSoftwareVersionId(TestMixins.AllSubDevicesGetMixin,
                                            OptionalParameterTestFixture):
  """Send a Get BOOT_SOFTWARE_VERSION_ID to ALL_SUB_DEVICES."""
  PID = 'BOOT_SOFTWARE_VERSION_ID'


# Boot Software Version Label
# -----------------------------------------------------------------------------
class GetBootSoftwareVersionLabel(TestMixins.GetStringMixin,
                                  OptionalParameterTestFixture):
  """GET the boot software label."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'BOOT_SOFTWARE_VERSION_LABEL'
  EXPECTED_FIELDS = ['label']


class GetBootSoftwareVersionLabelWithData(TestMixins.GetWithDataMixin,
                                          OptionalParameterTestFixture):
  """GET the boot software label with param data."""
  PID = 'BOOT_SOFTWARE_VERSION_LABEL'


class SetBootSoftwareVersionLabel(TestMixins.UnsupportedSetMixin,
                                  OptionalParameterTestFixture):
  """SET the boot software label."""
  PID = 'BOOT_SOFTWARE_VERSION_LABEL'


class SetBootSoftwareVersionLabelWithData(
        TestMixins.UnsupportedSetWithDataMixin,
        OptionalParameterTestFixture):
  """SET the boot software label with data."""
  PID = 'BOOT_SOFTWARE_VERSION_LABEL'


class AllSubDevicesGetBootSoftwareVersionLabel(TestMixins.AllSubDevicesGetMixin,
                                               OptionalParameterTestFixture):
  """Send a Get BOOT_SOFTWARE_VERSION_LABEL to ALL_SUB_DEVICES."""
  PID = 'BOOT_SOFTWARE_VERSION_LABEL'


# DMX Personality
# -----------------------------------------------------------------------------
class GetDMXPersonality(OptionalParameterTestFixture):
  """Get the current personality settings."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_PERSONALITY'
  REQUIRES = ['current_personality', 'personality_count']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult(
      field_names=['current_personality', 'personality_count']))
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    current_personality = self.Property('current_personality')
    personality_count = self.Property('personality_count')
    warning_str = ("Personality information in device info doesn't match that "
                   "in dmx_personality")

    if current_personality != fields['current_personality']:
      self.SetFailed('%s: current_personality %d != %d' % (
        warning_str, current_personality, fields['current_personality']))

    if personality_count != fields['personality_count']:
      self.SetFailed('%s: personality_count %d != %d' % (
        warning_str, personality_count, fields['personality_count']))


class GetDMXPersonalityWithData(TestMixins.GetWithDataMixin,
                                OptionalParameterTestFixture):
  """Get DMX_PERSONALITY with invalid data."""
  PID = 'DMX_PERSONALITY'


class SetDMXPersonality(OptionalParameterTestFixture):
  """Set the personality."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_PERSONALITY'
  REQUIRES = ['current_personality', 'personalities']

  def Test(self):
    self._personalities = list(self.Property('personalities'))
    self._consumes_slots = False
    for personality in self._personalities:
      if personality['slots_required'] > 0:
        self._consumes_slots = True
        break

    if len(self._personalities) > 0:
      self._CheckPersonality()
      return

    # Check we get a NR_UNKNOWN_PID
    self.AddExpectedResults(self.NackSetResult(RDMNack.NR_UNKNOWN_PID))
    self.new_personality = {'personality': 1}  # can use anything here really
    self.SendSet(ROOT_DEVICE, self.pid, [1])

  def _CheckPersonality(self):
    if not self._personalities:
      # End of the list, we're done
      self.Stop()
      return

    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(ROOT_DEVICE,
                 self.pid,
                 [self._personalities[0]['personality']])

  def VerifySet(self):
    self.AddIfGetSupported(
      self.AckGetResult(
        field_values={
          'current_personality': self._personalities[0]['personality'],
        },
        action=self.VerifyDeviceInfo))
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyDeviceInfo(self):
    device_info_pid = self.LookupPid('DEVICE_INFO')

    next_action = self.NextPersonality
    if self._personalities[0]['slots_required'] == 0:
      # If this personality has a footprint of 0, verify the start address is
      # 0xffff
      next_action = self.VerifyFootprint0DMXStartAddress

    self.AddExpectedResults(
      AckGetResult(
        device_info_pid.value,
        field_values={
          'current_personality': self._personalities[0]['personality'],
          'dmx_footprint': self._personalities[0]['slots_required'],
        },
        action=next_action))
    self.SendGet(ROOT_DEVICE, device_info_pid)

  def VerifyFootprint0DMXStartAddress(self):
    address_pid = self.LookupPid('DMX_START_ADDRESS')
    expected_results = [
      AckGetResult(
        address_pid.value,
        field_values={'dmx_address': RDM_ZERO_FOOTPRINT_DMX_ADDRESS},
        action=self.NextPersonality),
    ]
    if not self._consumes_slots:
      expected_results.append(
        NackGetResult(address_pid.value,
                      RDMNack.NR_UNKNOWN_PID,
                      action=self.NextPersonality)
      )
    self.AddExpectedResults(expected_results)
    self.SendGet(ROOT_DEVICE, address_pid)

  def NextPersonality(self):
    self._personalities = self._personalities[1:]
    self._CheckPersonality()

  def ResetState(self):
    # Reset back to the old value
    personality = self.Property('current_personality')
    if personality == 0 or personality > 255:
      return

    self.SendSet(ROOT_DEVICE,
                 self.pid,
                 [self.Property('current_personality')])
    self._wrapper.Run()


class SetZeroDMXPersonality(TestMixins.SetZeroUInt8Mixin,
                            OptionalParameterTestFixture):
  """Set DMX_PERSONALITY for personality 0."""
  PID = 'DMX_PERSONALITY'


class SetOutOfRangeDMXPersonality(TestMixins.SetOutOfRangeUInt8Mixin,
                                  OptionalParameterTestFixture):
  """Set DMX_PERSONALITY to an out-of-range value."""
  PID = 'DMX_PERSONALITY'
  REQUIRES = ['personality_count']
  LABEL = 'personalities'


class SetDMXPersonalityWithExtraData(TestMixins.SetWithDataMixin,
                                     OptionalParameterTestFixture):
  """Send a SET DMX_PERSONALITY command with extra data."""
  PID = 'DMX_PERSONALITY'


class SetDMXPersonalityWithNoData(TestMixins.SetWithNoDataMixin,
                                  OptionalParameterTestFixture):
  """Set DMX_PERSONALITY with no data."""
  PID = 'DMX_PERSONALITY'


class AllSubDevicesGetDMXPersonality(TestMixins.AllSubDevicesGetMixin,
                                     OptionalParameterTestFixture):
  """Send a Get DMX_PERSONALITY to ALL_SUB_DEVICES."""
  PID = 'DMX_PERSONALITY'


# DMX Personality Description
# -----------------------------------------------------------------------------
class GetZeroDMXPersonalityDescription(TestMixins.GetZeroUInt8Mixin,
                                       OptionalParameterTestFixture):
  """GET DMX_PERSONALITY_DESCRIPTION for personality 0."""
  PID = 'DMX_PERSONALITY_DESCRIPTION'


class GetOutOfRangeDMXPersonalityDescription(TestMixins.GetOutOfRangeUInt8Mixin,
                                             OptionalParameterTestFixture):
  """GET the personality description for the N + 1 personality."""
  PID = 'DMX_PERSONALITY_DESCRIPTION'
  REQUIRES = ['personality_count']
  LABEL = 'personality descriptions'


class AllSubDevicesGetDMXPersonalityDescription(
        TestMixins.AllSubDevicesGetMixin,
        OptionalParameterTestFixture):
  """Send a Get DMX_PERSONALITY_DESCRIPTION to ALL_SUB_DEVICES."""
  PID = 'DMX_PERSONALITY_DESCRIPTION'
  DATA = [1]


class GetDMXPersonalityDescription(OptionalParameterTestFixture):
  """GET the personality description for the current personality."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_PERSONALITY_DESCRIPTION'
  REQUIRES = ['current_personality', 'dmx_footprint', 'personality_count']

  def Test(self):
    personality_count = self.Property('personality_count')
    current_personality = self.Property('current_personality')
    if current_personality == 0 and personality_count > 0:
      # It's probably off by one, so fix it
      current_personality = 1

    if personality_count > 0:
      # Cross check against what we got from device info
      self.AddIfGetSupported(self.AckGetResult(field_values={
          'personality': current_personality,
          'slots_required': self.Property('dmx_footprint'),
        }))
      self.SendGet(ROOT_DEVICE, self.pid, [current_personality])
    else:
      self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
      self.SendGet(ROOT_DEVICE, self.pid, [1])

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    if ContainsUnprintable(fields['name']):
      self.AddAdvisory(
          'Name field in %s contains unprintable characters, was %s' %
          (self.pid.name, StringEscape(fields['name'])))

    # TODO(Peter): Advisory if name is 0 length


class GetDMXPersonalityDescriptions(OptionalParameterTestFixture):
  """Get information about all the personalities."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_PERSONALITY_DESCRIPTION'
  REQUIRES = ['personality_count']
  PROVIDES = ['personalities']

  def Test(self):
    self._personalities = []
    self._personality_count = self.Property('personality_count')
    self._current_index = 0
    self._GetPersonality()

  def _GetPersonality(self):
    self._current_index += 1
    if self._current_index > self._personality_count:
      if self._personality_count == 0:
        self.SetNotRun('No personalities declared')
      self.SetProperty('personalities', self._personalities)
      self.Stop()
      return

    if self._current_index > MAX_PERSONALITY_NUMBER:
      # This should never happen because personality_count is a uint8
      self.SetFailed('Could not find all personalities')
      return

    self.AddIfGetSupported(self.AckGetResult(
        field_names=['slots_required', 'name'],
        field_values={'personality': self._current_index},
        action=self._GetPersonality))
    self.SendGet(ROOT_DEVICE, self.pid, [self._current_index])

  def VerifyResult(self, response, fields):
    """Save the personality for other tests to use."""
    if response.WasAcked():
      self._personalities.append(fields)

      if ContainsUnprintable(fields['name']):
        self.AddAdvisory(
            'Name field in %s contains unprintable characters, was %s' %
            (self.pid.name, StringEscape(fields['name'])))

    # TODO(Peter): Advisory if name is 0 length


class GetDMXPersonalityDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                             OptionalParameterTestFixture):
  """GET DMX_PERSONALITY_DESCRIPTION with no argument given."""
  PID = 'DMX_PERSONALITY_DESCRIPTION'


class GetDMXPersonalityDescriptionWithExtraData(TestMixins.GetWithDataMixin,
                                                OptionalParameterTestFixture):
  """GET DMX_PERSONALITY_DESCRIPTION with more than 1 byte of data."""
  PID = 'DMX_PERSONALITY_DESCRIPTION'


class SetDMXPersonalityDescription(TestMixins.UnsupportedSetMixin,
                                   OptionalParameterTestFixture):
  """Attempt to SET DMX_PERSONALITY_DESCRIPTION."""
  PID = 'DMX_PERSONALITY_DESCRIPTION'


class SetDMXPersonalityDescriptionWithData(
        TestMixins.UnsupportedSetWithDataMixin,
        OptionalParameterTestFixture):
  """Attempt to SET DMX_PERSONALITY_DESCRIPTION with data."""
  PID = 'DMX_PERSONALITY_DESCRIPTION'


# DMX Start Address tests
# -----------------------------------------------------------------------------
class GetDMXStartAddress(ResponderTestFixture):
  """GET the DMX start address."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_START_ADDRESS'
  REQUIRES = ['dmx_footprint', 'dmx_start_address']
  PROVIDES = ['dmx_address']

  def Test(self):
    if self.Property('dmx_footprint') > 0:
      results = self.AckGetResult(field_names=['dmx_address'])
    else:
      results = [
        self.AckGetResult(field_values={
            'dmx_address': RDM_ZERO_FOOTPRINT_DMX_ADDRESS}),
        self.NackGetResult(RDMNack.NR_UNKNOWN_PID),
        self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
      ]
    self.AddExpectedResults(results)
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      self.SetProperty('dmx_address', None)
      return

    if self.Property('dmx_start_address') != fields['dmx_address']:
      self.SetFailed(
          'DMX_START_ADDRESS (%d) doesn\'t match what was in DEVICE_INFO (%d)'
          % (fields['dmx_address'], self.Property('dmx_start_address')))
    self.SetPropertyFromDict(fields, 'dmx_address')


class GetDMXStartAddressWithData(TestMixins.GetWithDataMixin,
                                 ResponderTestFixture):
  """GET the DMX start address with data."""
  PID = 'DMX_START_ADDRESS'
  REQUIRES = ['dmx_footprint']

  def Test(self):
    if self.Property('dmx_footprint') > 0:
      # If we have a footprint, PID must return something as this PID is
      # required (can't return unsupported)
      results = [
        self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
        self.AckGetResult(
          warning='Get %s with data returned an ack' % self.pid.name)
      ]
    else:
      # If we don't have a footprint, PID may return something, or may return
      # unsupported, as this PID becomes optional
      results = [
        self.NackGetResult(RDMNack.NR_UNKNOWN_PID),
        self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
        self.AckGetResult(
          warning='Get %s with data returned an ack' % self.pid.name),
      ]
    self.AddExpectedResults(results)
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class SetDMXStartAddress(TestMixins.SetDMXStartAddressMixin,
                         ResponderTestFixture):
  """Set the DMX start address."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_START_ADDRESS'
  REQUIRES = ['dmx_footprint', 'dmx_address']
  PROVIDES = ['set_dmx_address_supported']

  def Test(self):
    footprint = self.Property('dmx_footprint')
    current_address = self.Property('dmx_address')

    if footprint == 0 or current_address == RDM_ZERO_FOOTPRINT_DMX_ADDRESS:
      results = [
        self.NackSetResult(RDMNack.NR_UNKNOWN_PID),
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
        self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE)
      ]
    else:
      self.start_address = self.CalculateNewAddress(current_address, footprint)
      results = self.AckSetResult(action=self.VerifySet)

    self._test_state = self.SET
    self.AddExpectedResults(results)
    self.SendSet(ROOT_DEVICE, self.pid, [self.start_address])

  def VerifyResult(self, response, fields):
    if self._test_state == self.SET:
      self.SetProperty(self.PROVIDES[0], response.WasAcked())


class SetVendorcastDMXStartAddress(TestMixins.SetNonUnicastDMXStartAddressMixin,
                                   ResponderTestFixture):
  """SET the DMX start address using the vendorcast address."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_START_ADDRESS'
  REQUIRES = ['dmx_footprint', 'dmx_address', 'set_dmx_address_supported']

  def Uid(self):
    return UID.VendorcastAddress(self._uid.manufacturer_id)


class SetBroadcastDMXStartAddress(TestMixins.SetNonUnicastDMXStartAddressMixin,
                                  ResponderTestFixture):
  """SET the dmx start address using the broadcast address."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_START_ADDRESS'
  REQUIRES = ['dmx_footprint', 'dmx_address', 'set_dmx_address_supported']

  def Uid(self):
    return UID.AllDevices()


class SetOutOfRangeDMXStartAddress(ResponderTestFixture):
  """Check that the DMX address can't be set to > 512."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_START_ADDRESS'
  # We depend on GetDMXStartAddress to make sure this runs after it, while
  # still allowing this test to run if the other test fails.
  DEPS = [GetDMXStartAddress]
  REQUIRES = ['dmx_footprint']

  def Test(self):
    if self.Property('dmx_footprint') > 0:
      self.AddExpectedResults(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    else:
      self.AddExpectedResults([
          self.NackSetResult(RDMNack.NR_UNKNOWN_PID),
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
          self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE)
      ])
    data = struct.pack('!H', TestMixins.MAX_DMX_ADDRESS + 1)
    self.SendRawSet(ROOT_DEVICE, self.pid, data)


class SetZeroDMXStartAddress(ResponderTestFixture):
  """Check the DMX address can't be set to 0."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_START_ADDRESS'
  # We depend on GetDMXStartAddress to make sure this runs after it, while
  # still allowing this test to run if the other test fails.
  DEPS = [GetDMXStartAddress]
  REQUIRES = ['dmx_footprint']

  def Test(self):
    if self.Property('dmx_footprint') > 0:
      self.AddExpectedResults(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    else:
      self.AddExpectedResults([
          self.NackSetResult(RDMNack.NR_UNKNOWN_PID),
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
          self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE)
      ])
    data = struct.pack('!H', 0)
    self.SendRawSet(ROOT_DEVICE, self.pid, data)


class SetDMXStartAddressWithNoData(TestMixins.SetWithNoDataMixin,
                                   ResponderTestFixture):
  """Send a SET dmx start address with no data."""
  PID = 'DMX_START_ADDRESS'
  # We depend on GetDMXStartAddress to make sure this runs after it, while
  # still allowing this test to run if the other test fails.
  DEPS = [GetDMXStartAddress]
  REQUIRES = ['dmx_footprint']

  def Test(self):
    if self.Property('dmx_footprint') > 0:
      self.AddExpectedResults(self.NackSetResult(RDMNack.NR_FORMAT_ERROR))
    else:
      self.AddExpectedResults([
          self.NackSetResult(RDMNack.NR_UNKNOWN_PID),
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
          self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
      ])
    self.SendRawSet(ROOT_DEVICE, self.pid, '')


class SetDMXStartAddressWithExtraData(TestMixins.SetWithDataMixin,
                                      ResponderTestFixture):
  """Send a SET dmx start address with extra data."""
  PID = 'DMX_START_ADDRESS'
  # We depend on GetDMXStartAddress to make sure this runs after it, while
  # still allowing this test to run if the other test fails.
  DEPS = [GetDMXStartAddress]
  REQUIRES = ['dmx_footprint']

  def Test(self):
    if self.Property('dmx_footprint') > 0:
      self.AddExpectedResults(self.NackSetResult(RDMNack.NR_FORMAT_ERROR))
    else:
      self.AddExpectedResults([
          self.NackSetResult(RDMNack.NR_UNKNOWN_PID),
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
          self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
      ])
    self.SendRawSet(ROOT_DEVICE, self.pid, self.DATA)


class AllSubDevicesGetDMXStartAddress(TestMixins.AllSubDevicesGetMixin,
                                      OptionalParameterTestFixture):
  """Send a Get DMX_START_ADDRESS to ALL_SUB_DEVICES."""
  PID = 'DMX_START_ADDRESS'


# Slot Info
# -----------------------------------------------------------------------------
class GetSlotInfo(OptionalParameterTestFixture):
  """Get SLOT_INFO."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'SLOT_INFO'
  PROVIDES = ['defined_slots', 'undefined_definition_slots',
              'undefined_type_sec_slots']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      self.SetProperty('defined_slots', set())
      self.SetProperty('undefined_definition_slots', [])
      self.SetProperty('undefined_type_sec_slots', [])
      return

    slots = [d['slot_offset'] for d in fields['slots']]
    self.SetProperty('defined_slots', set(slots))
    undefined_definition_slots = []
    undefined_type_sec_slots = []

    for slot in fields['slots']:
      if slot['slot_type'] not in RDMConstants.SLOT_TYPE_TO_NAME:
        self.AddWarning('Unknown slot type %d for slot %d' %
                        (slot['slot_type'], slot['slot_offset']))

      if slot['slot_type'] == RDMConstants.SLOT_TYPES['ST_PRIMARY']:
        # slot_label_id must be valid
        if ((slot['slot_label_id'] not in
             RDMConstants.SLOT_DEFINITION_TO_NAME) and
            (slot['slot_label_id'] < RDM_MANUFACTURER_SD_MIN or
             slot['slot_label_id'] > RDM_MANUFACTURER_SD_MAX)):
          self.AddWarning('Unknown slot id %d for slot %d' %
                          (slot['slot_label_id'], slot['slot_offset']))
        if (slot['slot_label_id'] ==
            RDMConstants.SLOT_DEFINITIONS['SD_UNDEFINED']):
          undefined_definition_slots.append(slot['slot_offset'])
      else:
        # slot_label_id must reference a defined slot
        if slot['slot_label_id'] not in slots:
          self.AddWarning(
              'Slot %d is of type secondary and references an unknown slot %d'
              % (slot['slot_offset'], slot['slot_label_id']))
        if slot['slot_type'] == RDMConstants.SLOT_TYPES['ST_SEC_UNDEFINED']:
          undefined_type_sec_slots.append(slot['slot_offset'])

    self.SetProperty('undefined_definition_slots', undefined_definition_slots)
    self.SetProperty('undefined_type_sec_slots', undefined_type_sec_slots)


class GetSlotInfoWithData(TestMixins.GetWithDataMixin,
                          OptionalParameterTestFixture):
  """Get SLOT_INFO with invalid data."""
  PID = 'SLOT_INFO'


class SetSlotInfo(TestMixins.UnsupportedSetMixin,
                  OptionalParameterTestFixture):
  """Set SLOT_INFO."""
  PID = 'SLOT_INFO'


class SetSlotInfoWithData(TestMixins.UnsupportedSetWithDataMixin,
                          OptionalParameterTestFixture):
  """Attempt to SET SLOT_INFO with data."""
  PID = 'SLOT_INFO'


class AllSubDevicesGetSlotInfo(TestMixins.AllSubDevicesGetMixin,
                               OptionalParameterTestFixture):
  """Send a Get SLOT_INFO to ALL_SUB_DEVICES."""
  PID = 'SLOT_INFO'


# Slot Description
# -----------------------------------------------------------------------------
class GetSlotDescriptions(TestMixins.GetSettingDescriptionsRangeMixin,
                          OptionalParameterTestFixture):
  """Get the slot descriptions for all defined slots."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'SLOT_DESCRIPTION'
  REQUIRES = ['dmx_footprint']
  FIRST_INDEX_OFFSET = 0
  EXPECTED_FIELDS = ['slot_number']
  DESCRIPTION_FIELD = 'name'
  ALLOWED_NACKS = [RDMNack.NR_DATA_OUT_OF_RANGE]


class GetSlotDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                   OptionalParameterTestFixture):
  """Get the slot description with no slot number specified."""
  PID = 'SLOT_DESCRIPTION'


class GetSlotDescriptionWithExtraData(TestMixins.GetWithDataMixin,
                                      OptionalParameterTestFixture):
  """Get the slot description with more than 2 bytes of data."""
  PID = 'SLOT_DESCRIPTION'


class GetUndefinedSlotDefinitionDescriptions(OptionalParameterTestFixture):
  """Get the slot description for all slots with undefined definition."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'SLOT_DESCRIPTION'
  REQUIRES = ['undefined_definition_slots']

  def Test(self):
    self.undef_slots = self.Property('undefined_definition_slots')[:]
    if len(self.undef_slots) == 0:
      self.SetNotRun('No undefined definition slots found')
      return
    self._GetSlotDescription()

  def _GetSlotDescription(self):
    if len(self.undef_slots) == 0:
      self.Stop()
      return

    self.AddExpectedResults([
      self.AckGetResult(action=self._GetSlotDescription),
      self.NackGetResult(RDMNack.NR_UNKNOWN_PID,
                         action=self._GetSlotDescription),
      self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE,
                         action=self._GetSlotDescription)
    ])
    self.current_slot = self.undef_slots.pop()
    self.SendGet(ROOT_DEVICE, self.pid, [self.current_slot])

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      if response.nack_reason == RDMNack.NR_UNKNOWN_PID:
        self.AddWarning(
            '%s not supported for slot %d with undefined '
            'definition' %
            (self.pid.name, self.current_slot))
      if response.nack_reason == RDMNack.NR_DATA_OUT_OF_RANGE:
        self.AddWarning(
            'Slot description for slot %d with undefined definition was missing'
            % (self.current_slot))
      return

    if not fields['name']:
      self.AddWarning(
          'Slot description for slot %d with undefined definition was blank' %
          (self.current_slot))
      return


class GetUndefinedSecondarySlotTypeDescriptions(OptionalParameterTestFixture):
  """Get the slot description for all secondary slots with an undefined type."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'SLOT_DESCRIPTION'
  REQUIRES = ['undefined_type_sec_slots']

  def Test(self):
    self.undef_sec_slots = self.Property('undefined_type_sec_slots')[:]
    if len(self.undef_sec_slots) == 0:
      self.SetNotRun('No undefined type secondary slots found')
      return
    self._GetSlotDescription()

  def _GetSlotDescription(self):
    if len(self.undef_sec_slots) == 0:
      self.Stop()
      return

    self.AddExpectedResults([
      self.AckGetResult(action=self._GetSlotDescription),
      self.NackGetResult(RDMNack.NR_UNKNOWN_PID,
                         action=self._GetSlotDescription),
      self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE,
                         action=self._GetSlotDescription)
    ])
    self.current_slot = self.undef_sec_slots.pop()
    self.SendGet(ROOT_DEVICE, self.pid, [self.current_slot])

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      if response.nack_reason == RDMNack.NR_UNKNOWN_PID:
        self.AddAdvisory(
            '%s not supported for secondary slot %d with undefined type' %
            (self.pid.name, self.current_slot))
      if response.nack_reason == RDMNack.NR_DATA_OUT_OF_RANGE:
        self.AddAdvisory(
            'Slot description for secondary slot %d with undefined type was '
            'missing'
            % (self.current_slot))
      return

    if not fields['name']:
      self.AddAdvisory(
          'Slot description for secondary slot %d with undefined type was '
          'blank' %
          (self.current_slot))
      return


class SetSlotDescription(TestMixins.UnsupportedSetMixin,
                         OptionalParameterTestFixture):
  """Set SLOT_DESCRIPTION."""
  PID = 'SLOT_DESCRIPTION'


class SetSlotDescriptionWithData(TestMixins.UnsupportedSetWithDataMixin,
                                 OptionalParameterTestFixture):
  """Attempt to SET SLOT_DESCRIPTION with data."""
  PID = 'SLOT_DESCRIPTION'


class AllSubDevicesGetSlotDescription(TestMixins.AllSubDevicesGetMixin,
                                      OptionalParameterTestFixture):
  """Send a Get SLOT_DESCRIPTION to ALL_SUB_DEVICES."""
  PID = 'SLOT_DESCRIPTION'
  DATA = [1]


# Default Slot Value
# -----------------------------------------------------------------------------
class GetDefaultSlotValues(OptionalParameterTestFixture):
  """Get DEFAULT_SLOT_VALUE."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DEFAULT_SLOT_VALUE'
  REQUIRES = ['defined_slots']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    defined_slots = self.Property('defined_slots')
    default_slots = set()

    for slot in fields['slot_values']:
      if slot['slot_offset'] not in defined_slots:
        self.AddWarning(
          "DEFAULT_SLOT_VALUE contained slot %d, which wasn't in SLOT_INFO" %
          slot['slot_offset'])
      default_slots.add(slot['slot_offset'])

    for slot_offset in defined_slots:
      if slot_offset not in default_slots:
        self.AddAdvisory(
          "SLOT_INFO contained slot %d, which wasn't in DEFAULT_SLOT_VALUE" %
          slot_offset)


class GetDefaultSlotValueWithData(TestMixins.GetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Get DEFAULT_SLOT_VALUE with invalid data."""
  PID = 'DEFAULT_SLOT_VALUE'


class SetDefaultSlotValue(TestMixins.UnsupportedSetMixin,
                          OptionalParameterTestFixture):
  """Set DEFAULT_SLOT_VALUE."""
  PID = 'DEFAULT_SLOT_VALUE'


class SetDefaultSlotValueWithData(TestMixins.UnsupportedSetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Attempt to SET DEFAULT_SLOT_VALUE with data."""
  PID = 'DEFAULT_SLOT_VALUE'


class AllSubDevicesGetDefaultSlotValue(TestMixins.AllSubDevicesGetMixin,
                                       OptionalParameterTestFixture):
  """Send a Get DEFAULT_SLOT_VALUE to ALL_SUB_DEVICES."""
  PID = 'DEFAULT_SLOT_VALUE'
  DATA = [1]


# Sensor Consistency Checks
# -----------------------------------------------------------------------------
class CheckSensorConsistency(ResponderTestFixture):
  """Check that sensor support is consistent."""
  CATEGORY = TestCategory.SENSORS
  REQUIRES = ['sensor_count', 'sensor_recording_supported',
              'supported_parameters']

  def PidRequired(self):
    return False

  def IsSupported(self, pid):
    return pid.value in self.Property('supported_parameters')

  def CheckConsistency(self, pid_name, check_for_support=True):
    pid = self.LookupPid(pid_name)
    if (check_for_support and
        (not self.IsSupported(pid)) and
        self.Property('sensor_count')) > 0:
      self.AddAdvisory('%s not supported but sensor count was > 0' % pid)
    if self.IsSupported(pid) and self.Property('sensor_count') == 0:
      self.AddAdvisory('%s supported but sensor count was 0' % pid)

  def Test(self):
    self.CheckConsistency('SENSOR_DEFINITION')
    self.CheckConsistency('SENSOR_VALUE')
    self.CheckConsistency('RECORD_SENSORS',
                          self.Property('sensor_recording_supported'))
    self.SetPassed()


# Sensor Definition
# -----------------------------------------------------------------------------
class GetSensorDefinition(OptionalParameterTestFixture):
  """Fetch all the sensor definitions."""
  CATEGORY = TestCategory.SENSORS
  PID = 'SENSOR_DEFINITION'
  REQUIRES = ['sensor_count']
  PROVIDES = ['sensor_definitions', 'sensor_recording_supported']
  MAX_SENSOR_INDEX = 0xfe
  RECORDED_VALUE_MASK = 0x01

  PREDICATE_DICT = {
      '==': operator.eq,
      '<': operator.lt,
      '>': operator.gt,
  }

  def Test(self):
    # Default to false
    self._sensors = {}  # Stores the discovered sensors
    self._current_index = -1  # The current sensor we're trying to query
    self._sensor_holes = []  # Indices of sensors that are missing

    self._CheckForSensor()

  def _MissingSensorWarning(self):
    # An advisory for zero sensors is covered in CheckSensorConsistency
    max_sensor = max(self._sensors.keys()) if self._sensors else 0
    missing_sensors = [i for i in self._sensor_holes if i < max_sensor]
    if missing_sensors:
      self.AddWarning('Sensors missing in positions %s' % missing_sensors)

  def _CheckForSensor(self):
    if self.PidSupported():
      # If this PID is supported we attempt to locate all sensors
      if self._current_index == self.MAX_SENSOR_INDEX:
        if len(self._sensors) < self.Property('sensor_count'):
          self.AddWarning('Only found %d/%d sensors' %
                          (len(self._sensors), self.Property('sensor_count')))
        elif len(self._sensors) > self.Property('sensor_count'):
          self.AddWarning('Found too many %d/%d sensors' %
                          (len(self._sensors), self.Property('sensor_count')))

        self.SetProperty('sensor_definitions', self._sensors)

        supports_recording = False
        for sensor_def in self._sensors.values():
          supports_recording |= (
              sensor_def['supports_recording'] & self.RECORDED_VALUE_MASK)
        self.SetProperty('sensor_recording_supported', supports_recording)

        self._MissingSensorWarning()
        self.Stop()
        return

      # For each message we should either see a NR_DATA_OUT_OF_RANGE or an ack
      self.AddExpectedResults([
        self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE,
                           action=self._AddToHoles),
        self.AckGetResult(action=self._CheckForSensor)
      ])
    else:
      # Not supported, just check we get a NR_UNKNOWN_PID
      self.AddExpectedResults(self.NackGetResult(RDMNack.NR_UNKNOWN_PID))
      self.SetProperty('sensor_definitions', {})

    self._current_index += 1
    self.SendGet(ROOT_DEVICE, self.pid, [self._current_index])

  def _AddToHoles(self):
    self._sensor_holes.append(self._current_index)
    self._CheckForSensor()

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    sensor_number = fields['sensor_number']
    if self._current_index != sensor_number:
      self.AddWarning(
          'Requested sensor %d, message returned sensor %d' %
          (self._current_index, fields['sensor_number']))
      return

    self._sensors[self._current_index] = fields

    # Perform sanity checks on the sensor information
    if fields['type'] not in RDMConstants.SENSOR_TYPE_TO_NAME:
      if fields['type'] >= 0x80:
        self.AddAdvisory('Using a manufacturer specific type %d for sensor %d,'
                         ' is there no suitable defined type?' %
                         (fields['type'], sensor_number))
      else:
        self.AddWarning('Unknown type %d for sensor %d' %
                        (fields['type'], sensor_number))

    if fields['unit'] not in RDMConstants.UNIT_TO_NAME:
      if fields['unit'] >= 0x80:
        self.AddAdvisory('Using a manufacturer specific unit %d for sensor %d,'
                         ' is there no suitable defined unit?' %
                         (fields['unit'], sensor_number))
      else:
        self.AddWarning('Unknown unit %d for sensor %d' %
                        (fields['unit'], sensor_number))

    if fields['prefix'] not in RDMConstants.PREFIX_TO_NAME:
      self.AddWarning('Unknown prefix %d for sensor %d' %
                      (fields['prefix'], sensor_number))

    self.CheckCondition(sensor_number, fields, 'range_min', '>', 'range_max')
    self.CheckCondition(sensor_number, fields, 'range_min', '==', 'range_max')

    self.CheckCondition(sensor_number, fields, 'normal_min', '>', 'normal_max')
    self.CheckCondition(sensor_number, fields, 'normal_min', '==',
                        'normal_max')

    self.CheckCondition(sensor_number, fields, 'normal_min', '<', 'range_min')
    self.CheckCondition(sensor_number, fields, 'normal_max', '>', 'range_max')

    if fields['supports_recording'] & 0xfc:
      self.AddWarning('bits 7-2 in the recorded message support fields are set'
                      ' for sensor %d' % sensor_number)

    if ContainsUnprintable(fields['name']):
      self.AddAdvisory(
          'Name field in sensor definition for sensor %d  contains unprintable'
          ' characters, was %s' % (self._current_index,
                                   StringEscape(fields['name'])))

  def CheckCondition(self, sensor_number, fields, lhs, predicate_str, rhs):
    """Check for a condition and add a warning if it isn't true."""
    predicate = self.PREDICATE_DICT[predicate_str]
    if predicate(fields[lhs], fields[rhs]):
      self.AddAdvisory(
          'Sensor %d, %s (%d) %s %s (%d)' %
          (sensor_number, lhs, fields[lhs], predicate_str, rhs, fields[rhs]))


class GetSensorDefinitionWithNoData(TestMixins.GetWithNoDataMixin,
                                    OptionalParameterTestFixture):
  """Get the sensor definition with no data."""
  PID = 'SENSOR_DEFINITION'


class GetSensorDefinitionWithExtraData(TestMixins.GetWithDataMixin,
                                       OptionalParameterTestFixture):
  """Get the sensor definition with more than 1 byte of data."""
  PID = 'SENSOR_DEFINITION'


class GetInvalidSensorDefinition(OptionalParameterTestFixture):
  """Get the sensor definition with the all sensor value (0xff)."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'SENSOR_DEFINITION'

  def Test(self):
    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    data = struct.pack('!B', 0xff)
    self.SendRawGet(ROOT_DEVICE, self.pid, data)


class SetSensorDefinition(TestMixins.UnsupportedSetMixin,
                          OptionalParameterTestFixture):
  """SET the sensor definition."""
  PID = 'SENSOR_DEFINITION'


class SetSensorDefinitionWithData(TestMixins.UnsupportedSetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Attempt to SET SENSOR_DEFINITION with data."""
  PID = 'SENSOR_DEFINITION'


class AllSubDevicesGetSensorDefinition(TestMixins.AllSubDevicesGetMixin,
                                       OptionalParameterTestFixture):
  """Send a Get SENSOR_DEFINITION to ALL_SUB_DEVICES."""
  PID = 'SENSOR_DEFINITION'
  DATA = [1]


# Sensor Value
# -----------------------------------------------------------------------------
class GetSensorValues(OptionalParameterTestFixture):
  """Get values for all defined sensors."""
  CATEGORY = TestCategory.SENSORS
  PID = 'SENSOR_VALUE'
  REQUIRES = ['sensor_definitions']
  PROVIDES = ['sensor_values']

  HIGHEST_LOWEST_MASK = 0x02
  RECORDED_VALUE_MASK = 0x01

  def Test(self):
    # The head of the list is the current sensor we're querying
    self._sensors = list(self.Property('sensor_definitions').values())
    self._sensor_values = []

    if self._sensors:
      # Loop and get all values
      self._GetSensorValue()
    else:
      # No sensors found, make sure we get a NR_DATA_OUT_OF_RANGE
      self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
      self.SendGet(ROOT_DEVICE, self.pid, [0])

  def _GetSensorValue(self):
    if not self._sensors:
      # Finished
      self.SetProperty('sensor_values', self._sensor_values)
      self.Stop()
      return

    sensor_index = self._sensors[0]['sensor_number']
    self.AddExpectedResults([
      self.AckGetResult(action=self._GetNextSensor),
      self.NackGetResult(
        RDMNack.NR_HARDWARE_FAULT,
        advisory="Sensor %d NACK'ed GET SENSOR_VALUE with NR_HARDWARE_FAULT" %
                 sensor_index,
        action=self._GetNextSensor)
    ])
    self.SendGet(ROOT_DEVICE, self.pid, [sensor_index])

  def _GetNextSensor(self):
    self._sensors.pop(0)
    self._GetSensorValue()

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    sensor_def = self._sensors[0]
    sensor_number = fields['sensor_number']
    if sensor_def['sensor_number'] != sensor_number:
      self.AddWarning(
          'Requested sensor value for %d, message returned sensor %d' %
          (sensor_def['sensor_number'], fields['sensor_number']))
      return

    self._sensor_values.append(fields)
    range_min = sensor_def['range_min']
    range_max = sensor_def['range_max']

    # Perform sanity checks on the sensor information
    self._CheckValueWithinRange(sensor_number, fields, 'present_value',
                                range_min, range_max)

    if sensor_def['supports_recording'] & self.HIGHEST_LOWEST_MASK:
      self._CheckValueWithinRange(sensor_number, fields, 'lowest',
                                  range_min, range_max)
      self._CheckValueWithinRange(sensor_number, fields, 'highest',
                                  range_min, range_max)
    else:
      self._CheckForZeroField(sensor_number, fields, 'lowest')
      self._CheckForZeroField(sensor_number, fields, 'highest')

    if sensor_def['supports_recording'] & self.RECORDED_VALUE_MASK:
      self._CheckValueWithinRange(sensor_number, fields, 'recorded',
                                  range_min, range_max)
    else:
      self._CheckForZeroField(sensor_number, fields, 'recorded')

  def _CheckValueWithinRange(self, sensor_number, fields, name, min, max):
    if fields[name] < min or fields[name] > max:
      self.AddWarning(
        '%s for sensor %d not within range %d - %d, was %d' %
        (name, sensor_number, min, max, fields[name]))

  def _CheckForZeroField(self, sensor_number, fields, name):
    if fields[name]:
      self.AddWarning(
        '%s value for sensor %d non-0, but support not declared, was %d' %
        (name, sensor_number, fields[name]))


class GetUndefinedSensorValues(OptionalParameterTestFixture):
  """Attempt to get sensor values for all sensors that weren't defined."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'SENSOR_VALUE'
  REQUIRES = ['sensor_definitions']

  def Test(self):
    sensors = self.Property('sensor_definitions')
    self._missing_sensors = []
    for i in range(0, 0xff):
      if i not in sensors:
        self._missing_sensors.append(i)

    if self._missing_sensors:
      # Loop and get all values
      self._GetSensorValue()
    else:
      self.SetNotRun('All sensors declared')
      return

  def _GetSensorValue(self):
    if not self._missing_sensors:
      self.Stop()
      return

    self.AddIfGetSupported(
        self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE,
                           action=self._GetSensorValue))
    self.SendGet(ROOT_DEVICE, self.pid, [self._missing_sensors.pop(0)])


class GetInvalidSensorValue(OptionalParameterTestFixture):
  """Get the sensor value with the all sensor value (0xff)."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'SENSOR_VALUE'

  def Test(self):
    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    data = struct.pack('!B', 0xff)
    self.SendRawGet(ROOT_DEVICE, self.pid, data)


class GetSensorValueWithNoData(TestMixins.GetWithNoDataMixin,
                               OptionalParameterTestFixture):
  """GET sensor value without any sensor number."""
  PID = 'SENSOR_VALUE'


class GetSensorValueWithExtraData(TestMixins.GetWithDataMixin,
                                  OptionalParameterTestFixture):
  """GET SENSOR_VALUE with more than 1 byte of data."""
  PID = 'SENSOR_VALUE'


class ResetSensorValue(OptionalParameterTestFixture):
  """Reset sensor values for all defined sensors."""
  CATEGORY = TestCategory.SENSORS
  PID = 'SENSOR_VALUE'
  REQUIRES = ['sensor_definitions']

  def Test(self):
    # The head of the list is the current sensor we're querying
    self._sensors = list(self.Property('sensor_definitions').values())
    self._sensor_values = []

    if self._sensors:
      # Loop and get all values
      self._ResetSensor()
    else:
      # No sensors found, make sure we get a NR_DATA_OUT_OF_RANGE
      self.AddIfSetSupported([
          self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      ])
      self.SendSet(ROOT_DEVICE, self.pid, [0])

  def _ResetSensor(self):
    if not self._sensors:
      # Finished
      self.Stop()
      return

    sensor_index = self._sensors[0]['sensor_number']
    self.AddExpectedResults([
        self.AckSetResult(action=self._ResetNextSensor),
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
                           action=self._ResetNextSensor),
        self.NackSetResult(
          RDMNack.NR_HARDWARE_FAULT,
          advisory="Sensor %d NACK'ed Set SENSOR_VALUE with NR_HARDWARE_FAULT" %
                   sensor_index,
          action=self._ResetNextSensor)
    ])
    self.SendSet(ROOT_DEVICE, self.pid, [sensor_index])

  def _ResetNextSensor(self):
    self._sensors.pop(0)
    self._ResetSensor()

  def VerifyResult(self, response, fields):
    # It's not clear at all what to expect in this case.
    # See http://www.rdmprotocol.org/showthread.php?p=2160
    # TODO(simonn, E1.20 task group): figure this out
    pass


class ResetAllSensorValues(OptionalParameterTestFixture):
  """Set SENSOR_VALUE with sensor number set to 0xff."""
  CATEGORY = TestCategory.SENSORS
  PID = 'SENSOR_VALUE'
  REQUIRES = ['sensor_definitions']

  RECORDED_VALUE_MASK = 0x01
  ALL_SENSORS = 0xff

  def Test(self):
    supports_recording = False
    for sensor_def in self.Property('sensor_definitions').values():
      supports_recording |= (
          sensor_def['supports_recording'] & self.RECORDED_VALUE_MASK)

    # Some devices don't have set
    results = [self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)]
    if supports_recording:
      results = [
        self.AckSetResult(),
        self.NackSetResult(
            RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
            warning="One or more recorded sensors found but Set SENSOR_VALUE "
                    "wasn't supported")
      ]
    else:
      results = [
        self.AckSetResult(),
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)
      ]
    self.AddIfSetSupported(results)
    self.SendSet(ROOT_DEVICE, self.pid, [self.ALL_SENSORS])


class ResetUndefinedSensorValues(TestMixins.SetUndefinedSensorValues,
                                 OptionalParameterTestFixture):
  """Attempt to reset sensor values for all sensors that weren't defined."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'SENSOR_VALUE'
  REQUIRES = ['sensor_definitions']


class SetSensorValueWithNoData(TestMixins.SetWithNoDataMixin,
                               OptionalParameterTestFixture):
  """SET sensor value without any sensor number."""
  PID = 'SENSOR_VALUE'
  ALLOWED_NACKS = [RDMNack.NR_UNSUPPORTED_COMMAND_CLASS]


class SetSensorValueWithExtraData(TestMixins.SetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Send a SET SENSOR_VALUE command with extra data."""
  PID = 'SENSOR_VALUE'


class AllSubDevicesGetSensorValue(TestMixins.AllSubDevicesGetMixin,
                                  OptionalParameterTestFixture):
  """Send a Get SENSOR_VALUE to ALL_SUB_DEVICES."""
  PID = 'SENSOR_VALUE'
  DATA = [1]


# Record Sensors
# -----------------------------------------------------------------------------
class GetRecordSensors(TestMixins.UnsupportedGetMixin,
                       OptionalParameterTestFixture):
  """GET record sensors."""
  PID = 'RECORD_SENSORS'


class GetRecordSensorsWithData(TestMixins.UnsupportedGetWithDataMixin,
                               OptionalParameterTestFixture):
  """GET RECORD_SENSORS with data."""
  PID = 'RECORD_SENSORS'


class RecordSensorValues(OptionalParameterTestFixture):
  """Record values for all defined sensors."""
  CATEGORY = TestCategory.SENSORS
  PID = 'RECORD_SENSORS'
  REQUIRES = ['sensor_definitions']

  RECORDED_VALUE_MASK = 0x01

  def Test(self):
    # The head of the list is the current sensor we're querying
    self._sensors = list(self.Property('sensor_definitions').values())
    self._sensor_values = []

    if self._sensors:
      # Loop and get all values
      self._RecordSensor()
    else:
      # No sensors found, make sure we get a NR_DATA_OUT_OF_RANGE
      self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
      self.SendSet(ROOT_DEVICE, self.pid, [0])

  def _RecordSensor(self):
    if not self._sensors:
      # Finished
      self.Stop()
      return

    sensor_def = self._sensors[0]
    if sensor_def['supports_recording'] & self.RECORDED_VALUE_MASK:
      self.AddExpectedResults(self.AckSetResult(action=self._RecordNextSensor))
    else:
      message = ("Sensor %d ack'ed RECORD_SENSOR but recorded support was not "
                 "declared" % sensor_def['sensor_number'])
      self.AddIfSetSupported([
          self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE,
                             action=self._RecordNextSensor),
          self.AckSetResult(action=self._RecordNextSensor,
                            advisory=message),

      ])
    self.SendSet(ROOT_DEVICE, self.pid, [self._sensors[0]['sensor_number']])

  def _RecordNextSensor(self):
    self._sensors.pop(0)
    self._RecordSensor()


class RecordAllSensorValues(OptionalParameterTestFixture):
  """Set RECORD_SENSORS with sensor number set to 0xff."""
  CATEGORY = TestCategory.SENSORS
  PID = 'RECORD_SENSORS'
  REQUIRES = ['sensor_recording_supported']

  ALL_SENSORS = 0xff

  def Test(self):
    if self.Property('sensor_recording_supported'):
      self.AddIfSetSupported(self.AckSetResult())
    else:
      self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(ROOT_DEVICE, self.pid, [self.ALL_SENSORS])


class RecordUndefinedSensorValues(TestMixins.SetUndefinedSensorValues,
                                  OptionalParameterTestFixture):
  """Attempt to reset sensor values for all sensors that weren't defined."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'RECORD_SENSORS'
  REQUIRES = ['sensor_definitions']


class SetRecordSensorsWithNoData(TestMixins.SetWithNoDataMixin,
                                 OptionalParameterTestFixture):
  """SET record sensors without any sensor number."""
  PID = 'RECORD_SENSORS'


class SetRecordSensorsWithExtraData(TestMixins.SetWithDataMixin,
                                     OptionalParameterTestFixture):
  """Send a SET RECORD_SENSORS command with extra data."""
  PID = 'RECORD_SENSORS'


class AllSubDevicesGetRecordSensors(TestMixins.AllSubDevicesUnsupportedGetMixin,
                                    OptionalParameterTestFixture):
  """Attempt to send a get RECORD_SENSORS to ALL_SUB_DEVICES."""
  PID = 'RECORD_SENSORS'


# Device Hours
# -----------------------------------------------------------------------------
class GetDeviceHours(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the device hours."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_HOURS'
  EXPECTED_FIELDS = ['hours']
  PROVIDES = ['device_hours']


class GetDeviceHoursWithData(TestMixins.GetWithDataMixin,
                             OptionalParameterTestFixture):
  """GET the device hours with extra data."""
  PID = 'DEVICE_HOURS'


class SetDeviceHours(TestMixins.SetUInt32Mixin,
                     OptionalParameterTestFixture):
  """Attempt to SET the device hours."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_HOURS'
  EXPECTED_FIELDS = ['hours']
  PROVIDES = ['set_device_hours_supported']
  REQUIRES = ['device_hours']

  def OldValue(self):
    return self.Property('device_hours')

  def VerifyResult(self, response, fields):
    if response.command_class == PidStore.RDM_SET:
      set_supported = (
          response.WasAcked() or
          response.nack_reason != RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)
      self.SetProperty('set_device_hours_supported', set_supported)


class SetDeviceHoursWithNoData(TestMixins.SetWithNoDataMixin,
                               OptionalParameterTestFixture):
  """Set the device hours with no param data."""
  PID = 'DEVICE_HOURS'
  REQUIRES = ['set_device_hours_supported']

  def Test(self):
    if self.Property('set_device_hours_supported'):
      expected_result = RDMNack.NR_FORMAT_ERROR
    else:
      expected_result = RDMNack.NR_UNSUPPORTED_COMMAND_CLASS
    self.AddIfSetSupported(self.NackSetResult(expected_result))
    self.SendRawSet(ROOT_DEVICE, self.pid, b'')


class SetDeviceHoursWithExtraData(TestMixins.SetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Send a SET DEVICE_HOURS command with extra data."""
  PID = 'DEVICE_HOURS'
  DATA = 'foobar'


class AllSubDevicesGetDeviceHours(TestMixins.AllSubDevicesGetMixin,
                                  OptionalParameterTestFixture):
  """Send a Get DEVICE_HOURS to ALL_SUB_DEVICES."""
  PID = 'DEVICE_HOURS'


# Lamp Hours
# -----------------------------------------------------------------------------
class GetLampHours(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the device hours."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_HOURS'
  EXPECTED_FIELDS = ['hours']
  PROVIDES = ['lamp_hours']


class GetLampHoursWithData(TestMixins.GetWithDataMixin,
                           OptionalParameterTestFixture):
  """GET the device hours with extra data."""
  PID = 'LAMP_HOURS'


class SetLampHours(TestMixins.SetUInt32Mixin,
                   OptionalParameterTestFixture):
  """Attempt to SET the device hours."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_HOURS'
  EXPECTED_FIELDS = ['hours']
  PROVIDES = ['set_lamp_hours_supported']
  REQUIRES = ['lamp_hours']

  def OldValue(self):
    return self.Property('lamp_hours')

  def VerifyResult(self, response, fields):
    if response.command_class == PidStore.RDM_SET:
      set_supported = (
          response.WasAcked() or
          response.nack_reason != RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)
      self.SetProperty('set_lamp_hours_supported', set_supported)


class SetLampHoursWithNoData(TestMixins.SetWithNoDataMixin,
                             OptionalParameterTestFixture):
  """Set the device hours with no param data."""
  PID = 'LAMP_HOURS'
  REQUIRES = ['set_lamp_hours_supported']

  def Test(self):
    if self.Property('set_lamp_hours_supported'):
      expected_result = RDMNack.NR_FORMAT_ERROR
    else:
      expected_result = RDMNack.NR_UNSUPPORTED_COMMAND_CLASS
    self.AddIfSetSupported(self.NackSetResult(expected_result))
    self.SendRawSet(ROOT_DEVICE, self.pid, b'')


class SetLampHoursWithExtraData(TestMixins.SetWithDataMixin,
                                OptionalParameterTestFixture):
  """Send a SET LAMP_HOURS command with extra data."""
  PID = 'LAMP_HOURS'
  DATA = 'foobar'


class AllSubDevicesGetLampHours(TestMixins.AllSubDevicesGetMixin,
                                OptionalParameterTestFixture):
  """Send a Get LAMP_HOURS to ALL_SUB_DEVICES."""
  PID = 'LAMP_HOURS'


# Lamp Strikes
# -----------------------------------------------------------------------------
class GetLampStrikes(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the lamp strikes."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_STRIKES'
  EXPECTED_FIELDS = ['strikes']
  PROVIDES = ['lamp_strikes']


class GetLampStrikesWithData(TestMixins.GetWithDataMixin,
                             OptionalParameterTestFixture):
  """GET the lamp strikes with extra data."""
  PID = 'LAMP_STRIKES'


class SetLampStrikes(TestMixins.SetUInt32Mixin, OptionalParameterTestFixture):
  """Attempt to SET the lamp strikes."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_STRIKES'
  EXPECTED_FIELDS = ['strikes']
  PROVIDES = ['set_lamp_strikes_supported']
  REQUIRES = ['lamp_strikes']

  def OldValue(self):
    return self.Property('lamp_strikes')

  def VerifyResult(self, response, fields):
    if response.command_class == PidStore.RDM_SET:
      set_supported = (
          response.WasAcked() or
          response.nack_reason != RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)
      self.SetProperty('set_lamp_strikes_supported', set_supported)


class SetLampStrikesWithNoData(TestMixins.SetWithNoDataMixin,
                               OptionalParameterTestFixture):
  """Set the lamp strikes with no param data."""
  PID = 'LAMP_STRIKES'
  REQUIRES = ['set_lamp_strikes_supported']

  def Test(self):
    if self.Property('set_lamp_strikes_supported'):
      expected_result = RDMNack.NR_FORMAT_ERROR
    else:
      expected_result = RDMNack.NR_UNSUPPORTED_COMMAND_CLASS
    self.AddIfSetSupported(self.NackSetResult(expected_result))
    self.SendRawSet(ROOT_DEVICE, self.pid, b'')


class SetLampStrikesWithExtraData(TestMixins.SetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Send a SET LAMP_STRIKES command with extra data."""
  PID = 'LAMP_STRIKES'
  DATA = 'foobar'


class AllSubDevicesGetLampStrikes(TestMixins.AllSubDevicesGetMixin,
                                  OptionalParameterTestFixture):
  """Send a Get LAMP_STRIKES to ALL_SUB_DEVICES."""
  PID = 'LAMP_STRIKES'


# Lamp State
# -----------------------------------------------------------------------------
class GetLampState(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the lamp state."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_STATE'
  EXPECTED_FIELDS = ['state']
  PROVIDES = ['lamp_state']


class GetLampStateWithData(TestMixins.GetWithDataMixin,
                           OptionalParameterTestFixture):
  """GET the lamp state with extra data."""
  PID = 'LAMP_STATE'


class SetLampState(TestMixins.SetBoolMixin, OptionalParameterTestFixture):
  """Attempt to SET the lamp state."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_STATE'
  EXPECTED_FIELDS = ['state']
  REQUIRES = ['lamp_state']

  def OldValue(self):
    # We use a bool here so we toggle between off and on
    # Some responders may not support standby & strike
    return bool(self.Property('lamp_state'))


class SetLampStateWithNoData(TestMixins.SetWithNoDataMixin,
                             OptionalParameterTestFixture):
  """Set the device state with no param data."""
  PID = 'LAMP_STATE'


class SetLampStateWithExtraData(TestMixins.SetWithDataMixin,
                                OptionalParameterTestFixture):
  """Send a SET LAMP_STATE command with extra data."""
  PID = 'LAMP_STATE'


class AllSubDevicesGetLampState(TestMixins.AllSubDevicesGetMixin,
                                OptionalParameterTestFixture):
  """Send a Get LAMP_STATE to ALL_SUB_DEVICES."""
  PID = 'LAMP_STATE'


# Lamp On Mode
# -----------------------------------------------------------------------------
class GetLampOnMode(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the lamp on mode."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_ON_MODE'
  EXPECTED_FIELDS = ['mode']
  PROVIDES = ['lamp_on_mode']


class GetLampOnModeWithData(TestMixins.GetWithDataMixin,
                            OptionalParameterTestFixture):
  """GET the lamp on mode with extra data."""
  PID = 'LAMP_ON_MODE'


class SetLampOnMode(TestMixins.SetMixin, OptionalParameterTestFixture):
  """Attempt to SET the lamp on mode."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'LAMP_ON_MODE'
  EXPECTED_FIELDS = ['mode']
  REQUIRES = ['lamp_on_mode']
  ALLOWED_MODES = [0, 1, 2]
  ALL_MODES = ALLOWED_MODES + [3] + list(range(0x80, 0xe0))

  def OldValue(self):
    old = self.Property('lamp_on_mode')
    if old in self.ALL_MODES:
      return old
    return self.ALL_MODES[0]

  def NewValue(self):
    old_value = self.OldValue()
    try:
      self.ALLOWED_MODES.index(old_value)
    except ValueError:
      return self.ALLOWED_MODES[0]
    return self.ALLOWED_MODES[(old_value + 1) % len(self.ALLOWED_MODES)]


class SetLampOnModeWithNoData(TestMixins.SetWithNoDataMixin,
                              OptionalParameterTestFixture):
  """Set the device on mode with no param data."""
  PID = 'LAMP_ON_MODE'


class SetLampOnModeWithExtraData(TestMixins.SetWithDataMixin,
                                 OptionalParameterTestFixture):
  """Send a SET LAMP_ON_MODE command with extra data."""
  PID = 'LAMP_ON_MODE'


class AllSubDevicesGetLampOnMode(TestMixins.AllSubDevicesGetMixin,
                                 OptionalParameterTestFixture):
  """Send a Get LAMP_ON_MODE to ALL_SUB_DEVICES."""
  PID = 'LAMP_ON_MODE'


# Device Hours
# -----------------------------------------------------------------------------
class GetDevicePowerCycles(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the device power_cycles."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_POWER_CYCLES'
  EXPECTED_FIELDS = ['power_cycles']
  PROVIDES = ['power_cycles']


class GetDevicePowerCyclesWithData(TestMixins.GetWithDataMixin,
                                   OptionalParameterTestFixture):
  """GET the device power_cycles with extra data."""
  PID = 'DEVICE_POWER_CYCLES'


class ResetDevicePowerCycles(TestMixins.SetUInt32Mixin,
                             OptionalParameterTestFixture):
  """Attempt to SET the device power_cycles to zero."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_POWER_CYCLES'
  EXPECTED_FIELDS = ['power_cycles']
  REQUIRES = ['power_cycles']
  PROVIDES = ['set_device_power_cycles_supported']

  def OldValue(self):
    return self.Property('power_cycles')

  def NewValue(self):
    return 0

  def VerifyResult(self, response, fields):
    if response.command_class == PidStore.RDM_SET:
      set_supported = (
          response.WasAcked() or
          response.nack_reason != RDMNack.NR_UNSUPPORTED_COMMAND_CLASS)
      self.SetProperty('set_device_power_cycles_supported', set_supported)


class SetDevicePowerCycles(TestMixins.SetUInt32Mixin,
                           OptionalParameterTestFixture):
  """Attempt to SET the device power_cycles."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'DEVICE_POWER_CYCLES'
  EXPECTED_FIELDS = ['power_cycles']
  REQUIRES = ['power_cycles']

  def OldValue(self):
    return self.Property('power_cycles')

  def Test(self):
    self.AddIfSetSupported([
      self.AckSetResult(action=self.VerifySet),
      self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
      self.NackSetResult(
        RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
        advisory='SET for %s returned unsupported command class' %
                 self.pid.name),
    ])
    self.SendSet(ROOT_DEVICE, self.pid, [self.NewValue()])


class SetDevicePowerCyclesWithNoData(TestMixins.SetWithNoDataMixin,
                                     OptionalParameterTestFixture):
  """Set the device power_cycles with no param data."""
  PID = 'DEVICE_POWER_CYCLES'
  REQUIRES = ['set_device_power_cycles_supported']

  def Test(self):
    if self.Property('set_device_power_cycles_supported'):
      expected_result = RDMNack.NR_FORMAT_ERROR
    else:
      expected_result = RDMNack.NR_UNSUPPORTED_COMMAND_CLASS
    self.AddIfSetSupported(self.NackSetResult(expected_result))
    self.SendRawSet(ROOT_DEVICE, self.pid, b'')


class SetDevicePowerCyclesWithExtraData(TestMixins.SetWithDataMixin,
                                        OptionalParameterTestFixture):
  """Send a SET DEVICE_POWER_CYCLES command with extra data."""
  PID = 'DEVICE_POWER_CYCLES'
  DATA = 'foobar'


class AllSubDevicesGetDevicePowerCycles(TestMixins.AllSubDevicesGetMixin,
                                        OptionalParameterTestFixture):
  """Send a Get DEVICE_POWER_CYCLES to ALL_SUB_DEVICES."""
  PID = 'DEVICE_POWER_CYCLES'


# Display Invert
# -----------------------------------------------------------------------------
class GetDisplayInvert(TestMixins.GetMixin,
                       OptionalParameterTestFixture):
  """GET the display invert setting."""
  CATEGORY = TestCategory.DISPLAY_SETTINGS
  PID = 'DISPLAY_INVERT'
  EXPECTED_FIELDS = ['invert_status']
  PROVIDES = ['display_invert']


class GetDisplayInvertWithData(TestMixins.GetWithDataMixin,
                               OptionalParameterTestFixture):
  """GET the pan invert setting with extra data."""
  PID = 'DISPLAY_INVERT'


class SetDisplayInvert(TestMixins.SetMixin,
                       OptionalParameterTestFixture):
  """Attempt to SET the display invert setting."""
  CATEGORY = TestCategory.DISPLAY_SETTINGS
  PID = 'DISPLAY_INVERT'
  EXPECTED_FIELDS = ['invert_status']
  REQUIRES = ['display_invert']
  # Some devices can't do auto so we just use on and off here
  ALLOWED_MODES = [0, 1]
  ALL_MODES = ALLOWED_MODES + [2]

  def OldValue(self):
    old = self.Property('display_invert')
    if old in self.ALL_MODES:
      return old
    return self.ALL_MODES[0]

  def NewValue(self):
    old_value = self.OldValue()
    try:
      self.ALLOWED_MODES.index(old_value)
    except ValueError:
      return self.ALLOWED_MODES[0]
    return self.ALLOWED_MODES[(old_value + 1) % len(self.ALLOWED_MODES)]


class SetDisplayInvertWithNoData(TestMixins.SetWithNoDataMixin,
                                 OptionalParameterTestFixture):
  """Set the display invert with no param data."""
  PID = 'DISPLAY_INVERT'


class SetDisplayInvertWithExtraData(TestMixins.SetWithDataMixin,
                                    OptionalParameterTestFixture):
  """Send a SET DISPLAY_INVERT command with extra data."""
  PID = 'DISPLAY_INVERT'


class AllSubDevicesGetDisplayInvert(TestMixins.AllSubDevicesGetMixin,
                                    OptionalParameterTestFixture):
  """Send a Get DISPLAY_INVERT to ALL_SUB_DEVICES."""
  PID = 'DISPLAY_INVERT'


# Display Level
# -----------------------------------------------------------------------------
class GetDisplayLevel(TestMixins.GetMixin,
                      OptionalParameterTestFixture):
  """GET the display level setting."""
  CATEGORY = TestCategory.DISPLAY_SETTINGS
  PID = 'DISPLAY_LEVEL'
  EXPECTED_FIELDS = ['level']
  PROVIDES = ['display_level']


class GetDisplayLevelWithData(TestMixins.GetWithDataMixin,
                              OptionalParameterTestFixture):
  """GET the pan invert setting with extra data."""
  PID = 'DISPLAY_LEVEL'


class SetDisplayLevel(TestMixins.SetUInt8Mixin,
                      OptionalParameterTestFixture):
  """Attempt to SET the display level setting."""
  CATEGORY = TestCategory.DISPLAY_SETTINGS
  PID = 'DISPLAY_LEVEL'
  EXPECTED_FIELDS = ['level']
  REQUIRES = ['display_level']

  def OldValue(self):
    return self.Property('display_level')


class SetDisplayLevelWithNoData(TestMixins.SetWithNoDataMixin,
                                OptionalParameterTestFixture):
  """Set the display level with no param data."""
  PID = 'DISPLAY_LEVEL'


class SetDisplayLevelWithExtraData(TestMixins.SetWithDataMixin,
                                   OptionalParameterTestFixture):
  """Send a SET DISPLAY_LEVEL command with extra data."""
  PID = 'DISPLAY_LEVEL'


class AllSubDevicesGetDisplayLevel(TestMixins.AllSubDevicesGetMixin,
                                   OptionalParameterTestFixture):
  """Send a Get DISPLAY_LEVEL to ALL_SUB_DEVICES."""
  PID = 'DISPLAY_LEVEL'


# Pan Invert
# -----------------------------------------------------------------------------
class GetPanInvert(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the pan invert setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'PAN_INVERT'
  EXPECTED_FIELDS = ['invert']
  PROVIDES = ['pan_invert']


class GetPanInvertWithData(TestMixins.GetWithDataMixin,
                           OptionalParameterTestFixture):
  """GET the pan invert setting with extra data."""
  PID = 'PAN_INVERT'


class SetPanInvert(TestMixins.SetBoolMixin, OptionalParameterTestFixture):
  """Attempt to SET the pan invert setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'PAN_INVERT'
  EXPECTED_FIELDS = ['invert']
  REQUIRES = ['pan_invert']

  def OldValue(self):
    return self.Property('pan_invert')


class SetPanInvertWithNoData(TestMixins.SetWithNoDataMixin,
                             OptionalParameterTestFixture):
  """Set the pan invert with no param data."""
  PID = 'PAN_INVERT'


class SetPanInvertWithExtraData(TestMixins.SetWithDataMixin,
                                OptionalParameterTestFixture):
  """Send a SET PAN_INVERT command with extra data."""
  PID = 'PAN_INVERT'


class AllSubDevicesGetPanInvert(TestMixins.AllSubDevicesGetMixin,
                                OptionalParameterTestFixture):
  """Send a Get PAN_INVERT to ALL_SUB_DEVICES."""
  PID = 'PAN_INVERT'


# Tilt Invert
# -----------------------------------------------------------------------------
class GetTiltInvert(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the tilt invert setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'TILT_INVERT'
  EXPECTED_FIELDS = ['invert']
  PROVIDES = ['tilt_invert']


class GetTiltInvertWithData(TestMixins.GetWithDataMixin,
                            OptionalParameterTestFixture):
  """GET the tilt invert setting with extra data."""
  PID = 'TILT_INVERT'


class SetTiltInvert(TestMixins.SetBoolMixin, OptionalParameterTestFixture):
  """Attempt to SET the tilt invert setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'TILT_INVERT'
  EXPECTED_FIELDS = ['invert']
  REQUIRES = ['tilt_invert']

  def OldValue(self):
    return self.Property('tilt_invert')


class SetTiltInvertWithNoData(TestMixins.SetWithNoDataMixin,
                              OptionalParameterTestFixture):
  """Set the tilt invert with no param data."""
  PID = 'TILT_INVERT'


class SetTiltInvertWithExtraData(TestMixins.SetWithDataMixin,
                                 OptionalParameterTestFixture):
  """Send a SET TILT_INVERT command with extra data."""
  PID = 'TILT_INVERT'


class AllSubDevicesGetTiltInvert(TestMixins.AllSubDevicesGetMixin,
                                 OptionalParameterTestFixture):
  """Send a Get TILT_INVERT to ALL_SUB_DEVICES."""
  PID = 'TILT_INVERT'


# Pan Tilt Swap
# -----------------------------------------------------------------------------
class GetPanTiltSwap(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET the pan tilt swap setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'PAN_TILT_SWAP'
  EXPECTED_FIELDS = ['swap']
  PROVIDES = ['pan_tilt_swap']


class GetPanTiltSwapWithData(TestMixins.GetWithDataMixin,
                             OptionalParameterTestFixture):
  """GET the pan tilt swap setting with extra data."""
  PID = 'PAN_TILT_SWAP'


class SetPanTiltSwap(TestMixins.SetBoolMixin, OptionalParameterTestFixture):
  """Attempt to SET the pan tilt swap setting."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'PAN_TILT_SWAP'
  EXPECTED_FIELDS = ['swap']
  REQUIRES = ['pan_tilt_swap']

  def OldValue(self):
    return self.Property('pan_tilt_swap')


class SetPanTiltSwapWithNoData(TestMixins.SetWithNoDataMixin,
                               OptionalParameterTestFixture):
  """Set the pan tilt swap with no param data."""
  PID = 'PAN_TILT_SWAP'


class SetPanTiltSwapWithExtraData(TestMixins.SetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Send a SET PAN_TILT_SWAP command with extra data."""
  PID = 'PAN_TILT_SWAP'


class AllSubDevicesGetPanTiltSwap(TestMixins.AllSubDevicesGetMixin,
                                  OptionalParameterTestFixture):
  """Send a Get PAN_TILT_SWAP to ALL_SUB_DEVICES."""
  PID = 'PAN_TILT_SWAP'


# Real time clock
# -----------------------------------------------------------------------------
class GetRealTimeClock(OptionalParameterTestFixture):
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
    self.AddIfGetSupported(
      self.AckGetResult(field_names=list(self.ALLOWED_RANGES.keys()) +
                        ['second']))
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    for field, valid_range in self.ALLOWED_RANGES.items():
      value = fields[field]
      if value < valid_range[0] or value > valid_range[1]:
        self.AddWarning('%s in GET %s is out of range, was %d, expected %s' %
                        (field, self.pid.name, value, valid_range))


class GetRealTimeClockWithData(TestMixins.GetWithDataMixin,
                               OptionalParameterTestFixture):
  """GET the teal time clock with data."""
  PID = 'REAL_TIME_CLOCK'


class SetRealTimeClock(OptionalParameterTestFixture):
  """Set the real time clock."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'REAL_TIME_CLOCK'

  def Test(self):
    n = datetime.datetime.now()
    self.AddIfSetSupported([
        self.AckSetResult(),
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
    ])
    args = [n.year, n.month, n.day, n.hour, n.minute, n.second]
    self.SendSet(ROOT_DEVICE, self.pid,
                 args)


class SetRealTimeClockWithNoData(OptionalParameterTestFixture):
  """Set the real time clock without any data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'REAL_TIME_CLOCK'

  def Test(self):
    self.AddIfSetSupported([
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
        self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
    ])
    self.SendRawSet(ROOT_DEVICE, self.pid, b'')


class SetRealTimeClockWithExtraData(OptionalParameterTestFixture):
  """Send a SET REAL_TIME_CLOCK command with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'REAL_TIME_CLOCK'
  DATA = 'foobarbaz'

  def Test(self):
    self.AddIfSetSupported([
        self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
        self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
    ])
    self.SendRawSet(ROOT_DEVICE, self.pid, self.DATA)


class AllSubDevicesGetRealTimeClock(TestMixins.AllSubDevicesGetMixin,
                                    OptionalParameterTestFixture):
  """Send a Get REAL_TIME_CLOCK to ALL_SUB_DEVICES."""
  PID = 'REAL_TIME_CLOCK'


# Identify Device
# -----------------------------------------------------------------------------
class GetIdentifyDevice(TestMixins.GetRequiredMixin, ResponderTestFixture):
  """Get the identify state."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_DEVICE'
  PROVIDES = ['identify_state']
  EXPECTED_FIELDS = ['identify_state']


class GetIdentifyDeviceWithData(TestMixins.GetMandatoryPIDWithDataMixin,
                                ResponderTestFixture):
  """Get the identify state with data."""
  PID = 'IDENTIFY_DEVICE'


class SetIdentifyDevice(ResponderTestFixture):
  """Set the identify state."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_DEVICE'
  REQUIRES = ['identify_state']

  def Test(self):
    self.identify_mode = self.Property('identify_state')
    self.new_mode = not self.identify_mode

    self.AddExpectedResults(
        self.AckSetResult(action=self.VerifyIdentifyMode))
    self.SendSet(ROOT_DEVICE, self.pid, [self.new_mode])

  def VerifyIdentifyMode(self):
    self.AddExpectedResults(
        self.AckGetResult(field_values={'identify_state': self.new_mode}))
    self.SendGet(ROOT_DEVICE, self.pid)

  def ResetState(self):
    # Reset back to the old value
    self.SendSet(ROOT_DEVICE, self.pid, [self.identify_mode])
    self._wrapper.Run()


class SetOutOfRangeIdentifyDevice(ResponderTestFixture):
  """Set the identify state to a value which is out of range."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'IDENTIFY_DEVICE'
  REQUIRES = ['identify_state']

  def Test(self):
    self.AddExpectedResults(
        self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(ROOT_DEVICE, self.pid, [2])

  def ResetState(self):
    # Reset back to the old value
    self.SendSet(ROOT_DEVICE,
                 self.pid,
                 [self.Property('identify_state')])
    self._wrapper.Run()


class SetVendorcastIdentifyDevice(TestMixins.SetNonUnicastIdentifyMixin,
                                  ResponderTestFixture):
  """Set the identify state using the vendorcast uid."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_DEVICE'

  def Uid(self):
    return UID.VendorcastAddress(self._uid.manufacturer_id)


class SetBroadcastIdentifyDevice(TestMixins.SetNonUnicastIdentifyMixin,
                                 ResponderTestFixture):
  """Set the identify state using the broadcast uid."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_DEVICE'

  def Uid(self):
    return UID.AllDevices()


class SetOtherVendorcastIdentifyDevice(TestMixins.SetNonUnicastIdentifyMixin,
                                       ResponderTestFixture):
  """Send a vendorcast identify off to another manufacturer's ID."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_DEVICE'

  def States(self):
    return [
      self.TurnOn,
      self.VerifyOn,
      self.TurnOff,
      self.VerifyOn,
    ]

  def Uid(self):
    # Use a different vendor's vendorcast address
    vendorcast_id = self._uid.manufacturer_id
    if vendorcast_id == 0:
      vendorcast_id += 1
    else:
      vendorcast_id -= 1

    return UID(vendorcast_id, 0xffffffff)


class SetIdentifyDeviceWithNoData(ResponderTestFixture):
  """Set the identify state with no data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'IDENTIFY_DEVICE'
  REQUIRES = ['identify_state']

  def Test(self):
    self.AddExpectedResults(self.NackSetResult(RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(ROOT_DEVICE, self.pid, b'')

  def ResetState(self):
    self.SendSet(ROOT_DEVICE, self.pid, [self.Property('identify_state')])
    self._wrapper.Run()


class SetIdentifyDeviceWithExtraData(ResponderTestFixture):
  """Set the identify state with extra data."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'IDENTIFY_DEVICE'
  REQUIRES = ['identify_state']

  def Test(self):
    self.AddExpectedResults(self.NackSetResult(RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(ROOT_DEVICE, self.pid, 'foo')

  def ResetState(self):
    self.SendSet(ROOT_DEVICE, self.pid, [self.Property('identify_state')])
    self._wrapper.Run()


class AllSubDevicesGetIdentifyDevice(TestMixins.AllSubDevicesGetMixin,
                                     ResponderTestFixture):
  """Send a Get IDENTIFY_DEVICE to ALL_SUB_DEVICES."""
  PID = 'IDENTIFY_DEVICE'


class GetSubDeviceIdentifyDevice(ResponderTestFixture):
  """Check that IDENTIFY_DEVICE is supported on all sub devices."""
  CATEGORY = TestCategory.SUB_DEVICES
  PID = 'IDENTIFY_DEVICE'
  REQUIRES = ['sub_device_addresses']

  def Test(self):
    self._sub_devices = list(self.Property('sub_device_addresses').keys())
    self._sub_devices.reverse()
    self._GetIdentifyDevice()

  def _GetIdentifyDevice(self):
    if not self._sub_devices:
      self.Stop()
      return

    self.AddExpectedResults(self.AckGetResult(action=self._GetIdentifyDevice))
    sub_device = self._sub_devices.pop()
    self.SendGet(sub_device, self.pid)


# Power State
# -----------------------------------------------------------------------------
class GetPowerState(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get the power state mode."""
  CATEGORY = TestCategory.CONTROL
  PID = 'POWER_STATE'
  PROVIDES = ['power_state']
  EXPECTED_FIELDS = ['power_state']

  # The allowed power states
  ALLOWED_STATES = [0, 1, 2, 0xff]

  def VerifyResult(self, response, fields):
    super(GetPowerState, self).VerifyResult(response, fields)
    if response.WasAcked():
      if fields['power_state'] not in self.ALLOWED_STATES:
        self.AddWarning('Power state of 0x%hx is not defined' %
                        fields['power_state'])


class GetPowerStateWithData(TestMixins.GetWithDataMixin,
                            OptionalParameterTestFixture):
  """Get the power state mode with data."""
  PID = 'POWER_STATE'


class SetPowerState(TestMixins.SetMixin, OptionalParameterTestFixture):
  """Set the power state."""
  CATEGORY = TestCategory.CONTROL
  PID = 'POWER_STATE'
  REQUIRES = ['power_state']
  EXPECTED_FIELDS = ['power_state']

  def OldValue(self):
    old = self.Property('power_state')
    if old in GetPowerState.ALLOWED_STATES:
      return old
    return GetPowerState.ALLOWED_STATES[0]

  def NewValue(self):
    old_value = self.Property('power_state')
    try:
      GetPowerState.ALLOWED_STATES.index(old_value)
    except ValueError:
      return GetPowerState.ALLOWED_STATES[0]

    length = len(GetPowerState.ALLOWED_STATES)
    return GetPowerState.ALLOWED_STATES[(old_value + 1) % length]


class SetPowerStateWithNoData(TestMixins.SetWithNoDataMixin,
                              OptionalParameterTestFixture):
  """Set the power state with no data."""
  PID = 'POWER_STATE'


class SetPowerStateWithExtraData(TestMixins.SetWithDataMixin,
                                 OptionalParameterTestFixture):
  """Send a SET POWER_STATE command with extra data."""
  PID = 'POWER_STATE'


class AllSubDevicesGetPowerState(TestMixins.AllSubDevicesGetMixin,
                                 OptionalParameterTestFixture):
  """Send a Get POWER_STATE to ALL_SUB_DEVICES."""
  PID = 'POWER_STATE'


# Self Test
# -----------------------------------------------------------------------------
class GetPerformSelfTest(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get the current self test settings."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PERFORM_SELFTEST'
  EXPECTED_FIELDS = ['tests_active']


class GetPerformSelfTestWithData(TestMixins.GetWithDataMixin,
                                 OptionalParameterTestFixture):
  """Get the current self test settings with extra data."""
  PID = 'PERFORM_SELFTEST'


class SetPerformSelfTest(TestMixins.SetMixin,
                         OptionalParameterTestFixture):
  """Turn any running self tests off."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PERFORM_SELFTEST'
  EXPECTED_FIELDS = ['tests_active']

  def NewValue(self):
    return False

  def ResetState(self):
    # Override this so we don't reset
    pass


class SetPerformSelfTestWithNoData(TestMixins.SetWithNoDataMixin,
                                   OptionalParameterTestFixture):
  """Set the perform self test setting but don't provide any data."""
  PID = 'PERFORM_SELFTEST'


class AllSubDevicesGetPerformSelfTest(TestMixins.AllSubDevicesGetMixin,
                                      OptionalParameterTestFixture):
  """Send a Get PERFORM_SELFTEST to ALL_SUB_DEVICES."""
  PID = 'PERFORM_SELFTEST'


# Self Test Description
# -----------------------------------------------------------------------------
class GetSelfTestDescription(OptionalParameterTestFixture):
  """Get the self test description."""
  CATEGORY = TestCategory.CONTROL
  PID = 'SELF_TEST_DESCRIPTION'

  def Test(self):
    self.AddIfGetSupported([
      self.AckGetResult(),
      self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
    ])
    # Try to get a description for the first self test
    self.SendGet(ROOT_DEVICE, self.pid, [1])

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    if ContainsUnprintable(fields['description']):
      self.AddAdvisory(
          'Description field in self test description for test number %d '
          'contains unprintable characters, was %s' %
          (1, StringEscape(fields['description'])))


class GetSelfTestDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                       OptionalParameterTestFixture):
  """Get the self test description with no data."""
  PID = 'SELF_TEST_DESCRIPTION'


class GetSelfTestDescriptionWithExtraData(TestMixins.GetWithDataMixin,
                                          OptionalParameterTestFixture):
  """GET SELF_TEST_DESCRIPTION with more than 1 byte of data."""
  PID = 'SELF_TEST_DESCRIPTION'


class FindSelfTests(OptionalParameterTestFixture):
  """Locate the self tests by sending SELF_TEST_DESCRIPTION messages."""
  CATEGORY = TestCategory.CONTROL
  PID = 'SELF_TEST_DESCRIPTION'
  PROVIDES = ['self_test_descriptions']

  def Test(self):
    self._self_tests = {}  # Stores the discovered self tests
    self._current_index = 0  # The current self test we're trying to query
    self._CheckForSelfTest()

  def _CheckForSelfTest(self):
    # For each message we should either see a NR_DATA_OUT_OF_RANGE or an ack
    if self._current_index == 255:
      self.SetProperty('self_test_descriptions', self._self_tests)
      self.Stop()
      return

    if self.PidSupported():
      self.AddExpectedResults([
        self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE,
                           action=self._CheckForSelfTest),
        self.AckGetResult(action=self._CheckForSelfTest)
      ])
    else:
      self.AddExpectedResults(self.NackGetResult(RDMNack.NR_UNKNOWN_PID))

    self._current_index += 1
    self.SendGet(ROOT_DEVICE, self.pid, [self._current_index])

  def VerifyResult(self, response, fields):
    if response.WasAcked():
      if self._current_index != fields['test_number']:
        self.AddWarning(
            'Requested self test %d, message returned self test %d' %
            (self._current_index, fields['test_number']))
      else:
        self._self_tests[self._current_index] = fields['description']

      if ContainsUnprintable(fields['description']):
        self.AddAdvisory(
            'Description field in self test description for test number %d '
            'contains unprintable characters, was %s' %
            (fields['test_number'], StringEscape(fields['description'])))


class SetSelfTestDescription(TestMixins.UnsupportedSetMixin,
                             OptionalParameterTestFixture):
  """Attempt to SET SELF_TEST_DESCRIPTION."""
  PID = 'SELF_TEST_DESCRIPTION'


class SetSelfTestDescriptionWithData(TestMixins.UnsupportedSetWithDataMixin,
                                     OptionalParameterTestFixture):
  """Attempt to SET SELF_TEST_DESCRIPTION with data."""
  PID = 'SELF_TEST_DESCRIPTION'


class AllSubDevicesGetSelfTestDescription(TestMixins.AllSubDevicesGetMixin,
                                          OptionalParameterTestFixture):
  """Send a Get SELF_TEST_DESCRIPTION to ALL_SUB_DEVICES."""
  PID = 'SELF_TEST_DESCRIPTION'
  DATA = [1]


# Factory Defaults
# -----------------------------------------------------------------------------
class GetFactoryDefaults(OptionalParameterTestFixture):
  """GET the factory defaults pid."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'FACTORY_DEFAULTS'

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult(field_names=['using_defaults']))
    self.SendGet(ROOT_DEVICE, self.pid)


class GetFactoryDefaultsWithData(TestMixins.GetWithDataMixin,
                                 OptionalParameterTestFixture):
  """GET the factory defaults pid with extra data."""
  PID = 'FACTORY_DEFAULTS'


class SetFactoryDefaults(OptionalParameterTestFixture):
  """Reset to factory defaults."""
  CATEGORY = TestCategory.PRODUCT_INFORMATION
  PID = 'FACTORY_DEFAULTS'
  # Dependencies so that we don't reset the fields before checking them.
  DEPS = [GetDMXStartAddress, GetDMXPersonality]

  def Test(self):
    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(ROOT_DEVICE, self.pid)

  def VerifySet(self):
    self.AddIfGetSupported(
      self.AckGetResult(field_values={'using_defaults': True}))
    self.SendGet(ROOT_DEVICE, self.pid)


class ResetFactoryDefaultsWithData(TestMixins.SetWithDataMixin,
                                   OptionalParameterTestFixture):
  """Reset to factory defaults with extra data."""
  PID = 'FACTORY_DEFAULTS'


class AllSubDevicesGetFactoryDefaults(TestMixins.AllSubDevicesGetMixin,
                                      OptionalParameterTestFixture):
  """Send a Get FACTORY_DEFAULTS to ALL_SUB_DEVICES."""
  PID = 'FACTORY_DEFAULTS'


# CAPTURE_PRESET
# -----------------------------------------------------------------------------
class GetCapturePreset(TestMixins.UnsupportedGetMixin,
                       OptionalParameterTestFixture):
  """GET capture preset."""
  PID = 'CAPTURE_PRESET'


class GetCapturePresetWithData(TestMixins.UnsupportedGetWithDataMixin,
                               OptionalParameterTestFixture):
  """GET CAPTURE_PRESET with data."""
  PID = 'CAPTURE_PRESET'


class SetCapturePreset(OptionalParameterTestFixture):
  """Set Capture preset information."""
  CATEGORY = TestCategory.CONTROL
  PID = 'CAPTURE_PRESET'

  def Test(self):
    # This test doesn't check much because the standard is rather vague in this
    # area. There is also no way to read back preset data so it's impossible to
    # tell if this worked.
    self.AddIfSetSupported(self.AckSetResult())
    # Scene 1, no timing information
    self.SendSet(ROOT_DEVICE, self.pid, [1, 0, 0, 0])


class SetZeroCapturePreset(OptionalParameterTestFixture):
  """SET CAPTURE_PRESET to scene 0 and expect a data out of range."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'CAPTURE_PRESET'

  def Test(self):
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    # Scene 0, no timing information
    data = struct.pack('!HHHH', 0, 0, 0, 0)
    self.SendRawSet(ROOT_DEVICE, self.pid, data)


class SetCapturePresetWithNoData(TestMixins.SetWithNoDataMixin,
                                 OptionalParameterTestFixture):
  """Set capture preset with no data."""
  PID = 'CAPTURE_PRESET'


class SetCapturePresetWithExtraData(TestMixins.SetWithDataMixin,
                                    OptionalParameterTestFixture):
  """Set capture preset with extra data."""
  PID = 'CAPTURE_PRESET'
  DATA = 'foobarbaz'


class AllSubDevicesGetCapturePreset(TestMixins.AllSubDevicesUnsupportedGetMixin,
                                    OptionalParameterTestFixture):
  """Attempt to send a get CAPTURE_PRESET to ALL_SUB_DEVICES."""
  PID = 'CAPTURE_PRESET'


# PRESET_PLAYBACK
# -----------------------------------------------------------------------------
class GetPresetPlaybackWithData(TestMixins.GetWithDataMixin,
                                OptionalParameterTestFixture):
  """Get the preset playback with extra data."""
  PID = 'PRESET_PLAYBACK'


class GetPresetPlayback(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get the preset playback."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_PLAYBACK'
  EXPECTED_FIELDS = ['mode']


class SetPresetPlaybackWithNoData(TestMixins.SetWithNoDataMixin,
                                  OptionalParameterTestFixture):
  """Set preset playback with no data."""
  PID = 'PRESET_PLAYBACK'


class SetPresetPlaybackWithExtraData(TestMixins.SetWithDataMixin,
                                     OptionalParameterTestFixture):
  """Send a SET PRESET_PLAYBACK command with extra data."""
  PID = 'PRESET_PLAYBACK'
  DATA = 'foobar'


class SetPresetPlayback(OptionalParameterTestFixture):
  """Set the preset playback."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_PLAYBACK'
  OFF = 0
  FULL = 0xff

  def Test(self):
    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(ROOT_DEVICE, self.pid, [self.OFF, self.FULL])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(field_values={
        'mode': self.OFF,
        'level': self.FULL}))
    self.SendGet(ROOT_DEVICE, self.pid)


class AllSubDevicesGetPresetPlayback(TestMixins.AllSubDevicesGetMixin,
                                     OptionalParameterTestFixture):
  """Send a Get PRESET_PLAYBACK to ALL_SUB_DEVICES."""
  PID = 'PRESET_PLAYBACK'


# E1.37 PIDS
# =============================================================================

# IDENTIFY_MODE
# -----------------------------------------------------------------------------
class GetIdentifyMode(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get IDENTIFY_MODE."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_MODE'
  PROVIDES = ['identify_mode']
  EXPECTED_FIELDS = ['identify_mode']


class GetIdentifyModeWithData(TestMixins.GetWithDataMixin,
                              OptionalParameterTestFixture):
  """Get IDENTIFY_MODE with extra data."""
  PID = 'IDENTIFY_MODE'


class SetIdentifyMode(TestMixins.SetMixin, OptionalParameterTestFixture):
  """Set IDENTIFY_MODE with extra data."""
  CATEGORY = TestCategory.CONTROL
  PID = 'IDENTIFY_MODE'
  REQUIRES = ['identify_mode']
  LOUD = 0xff
  QUIET = 0x00
  EXPECTED_FIELDS = ['identify_mode']

  def OldValue(self):
    return self.Property('identify_mode')

  def NewValue(self):
    old_value = self.OldValue()
    if old_value is None:
      return self.QUIET

    if old_value:
      return self.QUIET
    else:
      return self.LOUD


class SetIdentifyModeWithNoData(TestMixins.SetWithNoDataMixin,
                                OptionalParameterTestFixture):
  """Set IDENTIFY_MODE with no data."""
  PID = 'IDENTIFY_MODE'


class SetIdentifyModeWithExtraData(TestMixins.SetWithDataMixin,
                                   OptionalParameterTestFixture):
  """Send a SET IDENTIFY_MODE command with extra data."""
  PID = 'IDENTIFY_MODE'


class AllSubDevicesGetIdentifyMode(TestMixins.AllSubDevicesGetMixin,
                                   OptionalParameterTestFixture):
  """Get IDENTIFY_MODE addressed to ALL_SUB_DEVICES."""
  PID = 'IDENTIFY_MODE'


# DMX_BLOCK_ADDRESS
# -----------------------------------------------------------------------------
class GetDMXBlockAddress(OptionalParameterTestFixture):
  """Get DMX_BLOCK_ADDRESS."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_BLOCK_ADDRESS'
  PROVIDES = ['total_sub_device_footprint', 'base_dmx_address']
  REQUIRES = ['sub_device_addresses', 'sub_device_footprints']
  NON_CONTIGUOUS = 0xffff

  def Test(self):
    self.expected_footprint = sum(
        self.Property('sub_device_footprints').values())
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    footprint = None
    base_address = None
    if response.WasAcked():
      footprint = fields['sub_device_footprint']
      base_address = fields['base_dmx_address']

      if footprint > TestMixins.MAX_DMX_ADDRESS:
        self.AddWarning('Sub device footprint > 512, was %d' % footprint)

      if (base_address == 0 or
          (base_address > TestMixins.MAX_DMX_ADDRESS and
           base_address != self.NON_CONTIGUOUS)):
        self.AddWarning('Base DMX address is outside range 1-512, was %d' %
                        base_address)

      if footprint != self.expected_footprint:
        self.SetFailed(
            "Sub device footprint (%d) didn't match sum of sub-device "
            "footprints (%d)" %
            (fields['sub_device_footprint'], self.expected_footprint))

      is_contiguous = self.CheckForContiguousSubDevices()
      if is_contiguous and base_address == self.NON_CONTIGUOUS:
        self.SetFailed(
            'Sub device addresses are contiguous, but block address returned '
            '0xffff')
      elif not (is_contiguous or base_address == self.NON_CONTIGUOUS):
        self.SetFailed(
            "Sub device addresses aren't contiguous, but block address "
            "didn't return 0xffff")

    self.SetProperty('total_sub_device_footprint', footprint)
    self.SetProperty('base_dmx_address', base_address)

  def CheckForContiguousSubDevices(self):
    addresses = self.Property('sub_device_addresses')
    footprints = self.Property('sub_device_footprints')
    next_address = None
    for index in sorted(addresses):
      if next_address is None:
        next_address = addresses[index] + footprints[index]
      elif addresses[index] != next_address:
        return False
      else:
        next_address += footprints[index]
    return True


class CheckDMXBlockAddressConsistency(OptionalParameterTestFixture):
  """Check that the device has subdevices if DMX_BLOCK_ADDRESS is supported."""
  CATEGORY = TestCategory.CONTROL
  REQUIRES = ['sub_device_count']
  PID = 'DMX_BLOCK_ADDRESS'

  def Test(self):
    if (self.PidSupported() and self.Property('sub_device_count') == 0):
      self.AddAdvisory('DMX_BLOCK_ADDRESS supported but sub device count was 0')
    self.SetPassed()


class GetDMXBlockAddressWithData(TestMixins.GetWithDataMixin,
                                 OptionalParameterTestFixture):
  """Get DMX_BLOCK_ADDRESS with extra data."""
  PID = 'DMX_BLOCK_ADDRESS'


class SetDMXBlockAddress(TestMixins.SetMixin, OptionalParameterTestFixture):
  """SET the DMX_BLOCK_ADDRESS."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_BLOCK_ADDRESS'
  REQUIRES = ['total_sub_device_footprint', 'base_dmx_address']
  EXPECTED_FIELDS = ['base_dmx_address']

  # TODO(Peter): Allow Nack write protect when 0 sub device footprint or no
  # addressable subs
  def NewValue(self):
    base_address = self.Property('base_dmx_address')
    footprint = self.Property('total_sub_device_footprint')

    if base_address is None or footprint is None:
      return 1

    if base_address == GetDMXBlockAddress.NON_CONTIGUOUS:
      return 1

    new_address = base_address + 1
    if new_address + footprint > TestMixins.MAX_DMX_ADDRESS:
      new_address = 1
    return new_address

  def ResetState(self):
    # We can't reset as the addresses may not have been contiguous
    pass


class SetZeroDMXBlockAddress(TestMixins.SetZeroUInt16Mixin,
                             OptionalParameterTestFixture):
  """Set DMX_BLOCK_ADDRESS to 0."""
  PID = 'DMX_BLOCK_ADDRESS'
  DEPS = [SetDMXBlockAddress]


class SetOutOfRangeDMXBlockAddress(OptionalParameterTestFixture):
  """Set DMX_BLOCK_ADDRESS to 513."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'DMX_BLOCK_ADDRESS'
  DEPS = [SetDMXBlockAddress]

  def Test(self):
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    data = struct.pack('!H', TestMixins.MAX_DMX_ADDRESS + 1)
    self.SendRawSet(ROOT_DEVICE, self.pid, data)


class SetDMXBlockAddressWithExtraData(TestMixins.SetWithDataMixin,
                                      OptionalParameterTestFixture):
  """Send a SET dmx block address with extra data."""
  PID = 'DMX_BLOCK_ADDRESS'
  DATA = 'foobar'


class SetDMXBlockAddressWithNoData(TestMixins.SetWithNoDataMixin,
                                   OptionalParameterTestFixture):
  """Set DMX_BLOCK_ADDRESS with no data."""
  PID = 'DMX_BLOCK_ADDRESS'


class AllSubDevicesGetDmxBlockAddress(TestMixins.AllSubDevicesGetMixin,
                                      OptionalParameterTestFixture):
  """Get DMX_BLOCK_ADDRESS addressed to ALL_SUB_DEVICES."""
  PID = 'DMX_BLOCK_ADDRESS'


# DMX_FAIL_MODE
# -----------------------------------------------------------------------------
class GetDMXFailMode(OptionalParameterTestFixture):
  """GET DMX_FAIL_MODE."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_FAIL_MODE'
  PROVIDES = ['dmx_fail_settings']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if fields is None:
      fields = {}
    self.SetProperty('dmx_fail_settings', fields)


class GetDMXFailModeWithData(TestMixins.GetWithDataMixin,
                             OptionalParameterTestFixture):
  """GET DMX_FAIL_MODE with extra data."""
  PID = 'DMX_FAIL_MODE'


class SetDMXFailMode(OptionalParameterTestFixture):
  """Set DMX_FAIL_MODE without changing the settings."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_FAIL_MODE'
  PROVIDES = ['set_dmx_fail_mode_supported']
  REQUIRES = ['dmx_fail_settings']

  def Test(self):
    settings = self.Property('dmx_fail_settings', {})

    self.AddIfSetSupported([
      self.AckSetResult(),
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
    ])
    self.SendSet(
        ROOT_DEVICE, self.pid,
        [settings.get('scene_number', 0),
         settings.get('hold_time', 0),
         settings.get('loss_of_signal_delay', 0),
         settings.get('level', 0)]
    )

  def VerifyResult(self, response, fields):
    self.SetProperty('set_dmx_fail_mode_supported', response.WasAcked())


class SetDMXFailModeMinimumTime(TestMixins.SetDMXFailModeMixin,
                                OptionalParameterTestFixture):
  """Verify the minimum times in PRESET_INFO are supported in DMX_FAIL_MODE."""
  def Test(self):
    self.in_get = False

    if self.Property('set_dmx_fail_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetFailMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    preset_info = self.Property('preset_info', {})
    self.known_limits = preset_info != {}
    self.delay_time = preset_info.get('min_fail_delay_time', 0)
    self.hold_time = preset_info.get('min_fail_hold_time', 0)

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, self.delay_time, self.hold_time, 255])

  def GetFailMode(self):
    self.in_get = True
    fields = {}
    if self.known_limits:
      fields['loss_of_signal_delay'] = self.delay_time
      fields['hold_time'] = self.hold_time

    self.AddIfGetSupported(self.AckGetResult(field_values=fields))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetDMXFailModeMaximumTime(TestMixins.SetDMXFailModeMixin,
                                OptionalParameterTestFixture):
  """Verify the maximum times in PRESET_INFO are supported in DMX_FAIL_MODE."""
  def Test(self):
    self.in_get = False

    if self.Property('set_dmx_fail_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetFailMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    preset_info = self.Property('preset_info', {})
    self.known_limits = preset_info != {}
    self.delay_time = preset_info.get('max_fail_delay_time', self.INFINITE_TIME)
    self.hold_time = preset_info.get('max_fail_hold_time', self.INFINITE_TIME)

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, self.delay_time, self.hold_time, 255])

  def GetFailMode(self):
    self.in_get = True
    fields = {}
    if self.known_limits:
      fields['loss_of_signal_delay'] = self.delay_time
      fields['hold_time'] = self.hold_time

    self.AddIfGetSupported(self.AckGetResult(field_values=fields))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetDMXFailModeInfiniteTimes(TestMixins.SetDMXFailModeMixin,
                                  OptionalParameterTestFixture):
  """Check if infinite times are supported for DMX_FAIL_MODE."""
  def Test(self):
    self.in_get = False

    if self.Property('set_dmx_fail_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetFailMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, 'infinite', 'infinite', 255])

  def GetFailMode(self):
    self.in_get = True
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked() or not self.in_get:
      return

    self.CheckField(
        'delay time',
        self.Property('preset_info', {}).get('fail_infinite_delay_supported'),
        fields['loss_of_signal_delay'])
    self.CheckField(
        'hold time',
        self.Property('preset_info', {}).get('fail_infinite_hold_supported'),
        fields['hold_time'])

  def CheckField(self, field_name, is_supported, new_value):
    if is_supported is None:
      # We can't tell is the new value is correct or not
      return

    if is_supported and new_value != self.INFINITE_TIME:
      self.SetFailed(
          'infinite %s was supported, but the value was truncated after a set.'
          ' Expected %d, got %d' %
          (field_name, self.INFINITE_TIME, new_value))
    elif not is_supported and new_value == self.INFINITE_TIME:
      self.SetFailed(
          'infinite %s was not supported, but the value was not truncated '
          'after a set.' % field_name)


class SetDMXFailModeOutOfRangeMaximumTime(TestMixins.SetDMXFailModeMixin,
                                          OptionalParameterTestFixture):
  """Check that the maximum times for DMX_FAIL_MODE are honored."""
  def Test(self):
    self.in_get = False
    preset_info = self.Property('preset_info', {})
    self.max_delay_time = preset_info.get('max_fail_delay_time')
    self.max_hold_time = preset_info.get('max_fail_hold_time')

    if self.max_delay_time is None or self.max_hold_time is None:
      self.SetNotRun("Max times unknown - PRESET_INFO wasn't acked")
      return

    delay_time = self.max_delay_time
    delay_time_field = self.pid.GetRequestField(RDM_SET,
                                                'loss_of_signal_delay')
    # 0xffff means 'fail mode not supported'
    if delay_time_field.RawValue(self.max_delay_time) < 0xffff - 1:
      delay_time = delay_time_field.DisplayValue(
          delay_time_field.RawValue(self.max_delay_time) + 1)  # Increment by 1

    hold_time = self.max_hold_time
    hold_time_field = self.pid.GetRequestField(RDM_SET, 'hold_time')
    # 0xffff means 'fail mode not supported'
    if hold_time_field.RawValue(self.max_hold_time) < 0xffff - 1:
      hold_time = hold_time_field.DisplayValue(
          hold_time_field.RawValue(self.max_hold_time) + 1)  # Increment by 1

    if self.Property('set_dmx_fail_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetFailMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, delay_time, hold_time, 255])

  def GetFailMode(self):
    self.in_get = True
    fields = {
      'loss_of_signal_delay': self.max_delay_time,
      'hold_time': self.max_hold_time,
    }
    self.AddIfGetSupported(self.AckGetResult(field_values=fields))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetDMXFailModeOutOfRangeMinimumTime(TestMixins.SetDMXFailModeMixin,
                                          OptionalParameterTestFixture):
  """Check that the minimum times for DMX_FAIL_MODE are honored."""
  def Test(self):
    self.in_get = False
    preset_info = self.Property('preset_info', {})
    self.min_delay_time = preset_info.get('min_fail_delay_time')
    self.min_hold_time = preset_info.get('min_fail_hold_time')

    if self.min_delay_time is None or self.min_hold_time is None:
      self.SetNotRun("Max times unknown - PRESET_INFO wasn't acked")
      return

    delay_time = self.min_delay_time
    delay_time_field = self.pid.GetRequestField(RDM_SET,
                                                'loss_of_signal_delay')
    # 0xffff means 'fail mode not supported'
    if (delay_time_field.RawValue(self.min_delay_time) > 1 and
        delay_time_field.RawValue(self.min_delay_time) < 0xffff):
      delay_time = delay_time_field.DisplayValue(
          delay_time_field.RawValue(self.min_delay_time) - 1)  # Decrement by 1

    hold_time = self.min_hold_time
    hold_time_field = self.pid.GetRequestField(RDM_SET, 'hold_time')
    # 0xffff means 'fail mode not supported'
    if (hold_time_field.RawValue(self.min_hold_time) > 1 and
        hold_time_field.RawValue(self.min_hold_time) < 0xffff):
      hold_time = hold_time_field.DisplayValue(
          hold_time_field.RawValue(self.min_hold_time) - 1)  # Decrement by 1

    if self.Property('set_dmx_fail_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetFailMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, delay_time, hold_time, 255])

  def GetFailMode(self):
    self.in_get = True
    fields = {
      'loss_of_signal_delay': self.min_delay_time,
      'hold_time': self.min_hold_time,
    }
    self.AddIfGetSupported(self.AckGetResult(field_values=fields))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetDMXFailModeWithNoData(TestMixins.SetWithNoDataMixin,
                               OptionalParameterTestFixture):
  """Set DMX_FAIL_MODE with no data."""
  PID = 'DMX_FAIL_MODE'


class SetDMXFailModeWithExtraData(TestMixins.SetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Send a SET DMX_FAIL_MODE command with extra data."""
  PID = 'DMX_FAIL_MODE'


class AllSubDevicesGetDMXFailMode(TestMixins.AllSubDevicesGetMixin,
                                  OptionalParameterTestFixture):
  """Get DMX_FAIL_MODE addressed to ALL_SUB_DEVICES."""
  PID = 'DMX_FAIL_MODE'


# DMX_STARTUP_MODE
# -----------------------------------------------------------------------------
class GetDMXStartupMode(OptionalParameterTestFixture):
  """Get DMX_STARTUP_MODE."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_STARTUP_MODE'
  PROVIDES = ['dmx_startup_settings']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if fields is None:
      fields = {}
    self.SetProperty('dmx_startup_settings', fields)


class GetDMXStartupModeWithData(TestMixins.GetWithDataMixin,
                                OptionalParameterTestFixture):
  """Get DMX_STARTUP_MODE with extra data."""
  PID = 'DMX_STARTUP_MODE'


class SetDMXStartupMode(OptionalParameterTestFixture):
  """Set DMX_STARTUP_MODE without changing the settings."""
  CATEGORY = TestCategory.DMX_SETUP
  PID = 'DMX_FAIL_MODE'
  PROVIDES = ['set_dmx_startup_mode_supported']
  REQUIRES = ['dmx_startup_settings']

  def Test(self):
    settings = self.Property('dmx_startup_settings', {})

    self.AddIfSetSupported([
      self.AckSetResult(),
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
    ])
    self.SendSet(
        ROOT_DEVICE, self.pid,
        [settings.get('scene_number', 0),
         settings.get('hold_time', 0),
         settings.get('startup_delay', 0),
         settings.get('level', 0)]
    )

  def VerifyResult(self, response, fields):
    self.SetProperty('set_dmx_startup_mode_supported', response.WasAcked())


class SetDMXStartupModeMinimumTime(TestMixins.SetDMXStartupModeMixin,
                                   OptionalParameterTestFixture):
  """Verify DMX_STARTUP_MODE supports the min. times from PRESET_INFO."""
  def Test(self):
    self.in_get = False

    if self.Property('set_dmx_startup_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetStartupMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    preset_info = self.Property('preset_info', {})
    self.known_limits = preset_info != {}
    self.delay_time = preset_info.get('min_startup_delay_time', 0)
    self.hold_time = preset_info.get('min_startup_hold_time', 0)

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, self.delay_time, self.hold_time, 255])

  def GetStartupMode(self):
    self.in_get = True
    fields = {}
    if self.known_limits:
      fields['startup_delay'] = self.delay_time
      fields['hold_time'] = self.hold_time

    self.AddIfGetSupported(self.AckGetResult(field_values=fields))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetDMXStartupModeMaximumTime(TestMixins.SetDMXStartupModeMixin,
                                   OptionalParameterTestFixture):
  """Verify DMX_STARTUP_MODE supports the max. times from PRESET_INFO."""
  def Test(self):
    self.in_get = False

    if self.Property('set_dmx_startup_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetStartupMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    preset_info = self.Property('preset_info', {})
    self.known_limits = preset_info != {}
    self.delay_time = preset_info.get('max_startup_delay_time',
                                      self.INFINITE_TIME)
    self.hold_time = preset_info.get('max_startup_hold_time',
                                     self.INFINITE_TIME)

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, self.delay_time, self.hold_time, 255])

  def GetStartupMode(self):
    self.in_get = True
    fields = {}
    if self.known_limits:
      fields['startup_delay'] = self.delay_time
      fields['hold_time'] = self.hold_time

    self.AddIfGetSupported(self.AckGetResult(field_values=fields))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetDMXStartupModeInfiniteTimes(TestMixins.SetDMXStartupModeMixin,
                                     OptionalParameterTestFixture):
  """Check if infinite times are supported for DMX_STARTUP_MODE."""
  def Test(self):
    self.in_get = False

    if self.Property('set_dmx_startup_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetStartupMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, 'infinite', 'infinite', 255])

  def GetStartupMode(self):
    self.in_get = True
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked() or not self.in_get:
      return

    self.CheckField(
        'delay time',
        self.Property('preset_info', {}).get(
           'startup_infinite_delay_supported'),
        fields['startup_delay'])
    self.CheckField(
        'hold time',
        self.Property('preset_info', {}).get('startup_infinite_hold_supported'),
        fields['hold_time'])

  def CheckField(self, field_name, is_supported, new_value):
    if is_supported is None:
      # We can't tell is the new value is correct or not
      return

    if is_supported and new_value != self.INFINITE_TIME:
      self.SetFailed(
          'infinite %s was supported, but the value was truncated after a set.'
          ' Expected %d, got %d' %
          (field_name, self.INFINITE_TIME, new_value))
    elif not is_supported and new_value == self.INFINITE_TIME:
      self.SetFailed(
          'infinite %s was not supported, but the value was not truncated '
          'after a set.' % field_name)


class SetDMXStartupModeOutOfRangeMaximumTime(TestMixins.SetDMXStartupModeMixin,
                                             OptionalParameterTestFixture):
  """Check that the maximum times for DMX_STARTUP_MODE are honored."""
  def Test(self):
    self.in_get = False
    preset_info = self.Property('preset_info', {})
    self.max_delay_time = preset_info.get('max_startup_delay_time')
    self.max_hold_time = preset_info.get('max_startup_hold_time')

    if self.max_delay_time is None or self.max_hold_time is None:
      self.SetNotRun("Max times unknown - PRESET_INFO wasn't acked")
      return

    delay_time = self.max_delay_time
    delay_time_field = self.pid.GetRequestField(RDM_SET, 'startup_delay')
    # 0xffff means 'startup mode not supported'
    if delay_time_field.RawValue(self.max_delay_time) < (0xffff - 1):
      delay_time = delay_time_field.DisplayValue(
          delay_time_field.RawValue(self.max_delay_time) + 1)  # Increment by 1

    hold_time = self.max_hold_time
    hold_time_field = self.pid.GetRequestField(RDM_SET, 'hold_time')
    # 0xffff means 'startup mode not supported'
    if hold_time_field.RawValue(self.max_hold_time) < (0xffff - 1):
      hold_time = hold_time_field.DisplayValue(
          hold_time_field.RawValue(self.max_hold_time) + 1)  # Increment by 1

    if self.Property('set_dmx_startup_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetStartupMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, delay_time, hold_time, 255])

  def GetStartupMode(self):
    self.in_get = True
    fields = {
      'startup_delay': self.max_delay_time,
      'hold_time': self.max_hold_time,
    }
    self.AddIfGetSupported(self.AckGetResult(field_values=fields))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetDMXStartupModeOutOfRangeMinimumTime(TestMixins.SetDMXStartupModeMixin,
                                             OptionalParameterTestFixture):
  """Check that the minimum times for DMX_STARTUP_MODE are honored."""
  def Test(self):
    self.in_get = False
    preset_info = self.Property('preset_info', {})
    self.min_delay_time = preset_info.get('min_startup_delay_time')
    self.min_hold_time = preset_info.get('min_startup_hold_time')

    if self.min_delay_time is None or self.min_hold_time is None:
      self.SetNotRun("Max times unknown - PRESET_INFO wasn't acked")
      return

    delay_time = self.min_delay_time
    delay_time_field = self.pid.GetRequestField(RDM_SET, 'startup_delay')
    # 0xffff means 'startup mode not supported'
    if (delay_time_field.RawValue(self.min_delay_time) > 1 and
        delay_time_field.RawValue(self.min_delay_time) < 0xffff):
      delay_time = delay_time_field.DisplayValue(
          delay_time_field.RawValue(self.min_delay_time) - 1)  # Decrement by 1

    hold_time = self.min_hold_time
    hold_time_field = self.pid.GetRequestField(RDM_SET, 'hold_time')
    # 0xffff means 'startup mode not supported'
    if (hold_time_field.RawValue(self.min_hold_time) > 1 and
        hold_time_field.RawValue(self.min_hold_time) < 0xffff):
      hold_time = hold_time_field.DisplayValue(
          hold_time_field.RawValue(self.min_hold_time) - 1)  # Decrement by 1

    if self.Property('set_dmx_startup_mode_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetStartupMode))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid,
                 [0, delay_time, hold_time, 255])

  def GetStartupMode(self):
    self.in_get = True
    fields = {
      'startup_delay': self.min_delay_time,
      'hold_time': self.min_hold_time,
    }
    self.AddIfGetSupported(self.AckGetResult(field_values=fields))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetDMXStartupModeWithNoData(TestMixins.SetWithNoDataMixin,
                                  OptionalParameterTestFixture):
  """Set DMX_STARTUP_MODE with no data."""
  PID = 'DMX_STARTUP_MODE'


class SetDMXStartupModeWithExtraData(TestMixins.SetWithDataMixin,
                                     OptionalParameterTestFixture):
  """Send a SET DMX_STARTUP_MODE command with extra data."""
  PID = 'DMX_STARTUP_MODE'


class AllSubDevicesGetDMXStartupMode(TestMixins.AllSubDevicesGetMixin,
                                     OptionalParameterTestFixture):
  """Get DMX_STARTUP_MODE addressed to ALL_SUB_DEVICES."""
  PID = 'DMX_STARTUP_MODE'


# POWER_ON_SELF_TEST
# -----------------------------------------------------------------------------
class GetPowerOnSelfTest(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get the POWER_ON_SELF_TEST."""
  CATEGORY = TestCategory.CONTROL
  PID = 'POWER_ON_SELF_TEST'
  EXPECTED_FIELDS = ['power_on_self_test']
  PROVIDES = ['power_on_self_test']


class GetPowerOnSelfTestWithData(TestMixins.GetWithDataMixin,
                                 OptionalParameterTestFixture):
  """Get the POWER_ON_SELF_TEST with extra data."""
  PID = 'POWER_ON_SELF_TEST'


class SetPowerOnSelfTest(TestMixins.SetBoolMixin,
                         OptionalParameterTestFixture):
  """Set POWER_ON_SELF_TEST."""
  CATEGORY = TestCategory.CONTROL
  PID = 'POWER_ON_SELF_TEST'
  EXPECTED_FIELDS = ['power_on_self_test']
  REQUIRES = ['power_on_self_test']

  def OldValue(self):
    return self.Property('power_on_self_test')


class SetPowerOnSelfTestWithNoData(TestMixins.SetWithNoDataMixin,
                                   OptionalParameterTestFixture):
  """Set the POWER_ON_SELF_TEST with no param data."""
  PID = 'POWER_ON_SELF_TEST'


class SetPowerOnSelfTestWithExtraData(TestMixins.SetWithDataMixin,
                                      OptionalParameterTestFixture):
  """Send a SET POWER_ON_SELF_TEST command with extra data."""
  PID = 'POWER_ON_SELF_TEST'


class AllSubDevicesGetPowerOnSelfTest(TestMixins.AllSubDevicesGetMixin,
                                      OptionalParameterTestFixture):
  """Get POWER_ON_SELF_TEST addressed to ALL_SUB_DEVICES."""
  PID = 'POWER_ON_SELF_TEST'


# LOCK_STATE
# -----------------------------------------------------------------------------
class GetLockState(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get LOCK_STATE."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = "LOCK_STATE"
  PROVIDES = ['current_lock_state', 'number_of_lock_states']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      for key in self.PROVIDES:
        self.SetProperty(key, None)
      return

    self.SetPropertyFromDict(fields, 'current_lock_state')
    self.SetPropertyFromDict(fields, 'number_of_lock_states')

    if fields['current_lock_state'] > fields['number_of_lock_states']:
      self.SetFailed('Lock State %d exceeded number of lock states %d' %
                     (fields['current_lock_state'],
                      fields['number_of_lock_states']))
      return


class GetLockStateWithData(TestMixins.GetWithDataMixin,
                           OptionalParameterTestFixture):
  """Get LOCK_STATE with extra data."""
  PID = 'LOCK_STATE'


class AllSubDevicesGetLockState(TestMixins.AllSubDevicesGetMixin,
                                OptionalParameterTestFixture):
  """Get LOCK_STATE addressed to ALL_SUB_DEVICES."""
  PID = 'LOCK_STATE'


class SetLockStateWithNoData(TestMixins.SetWithNoDataMixin,
                             OptionalParameterTestFixture):
  """Set LOCK_STATE without no data."""
  PID = 'LOCK_STATE'


class SetLockStateWithExtraData(TestMixins.SetWithDataMixin,
                                OptionalParameterTestFixture):
  """Send a SET LOCK_STATE command with extra data."""
  PID = 'LOCK_STATE'
  DATA = 'foobar'


class SetLockState(OptionalParameterTestFixture):
  """Set LOCK_STATE."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'LOCK_STATE'
  REQUIRES = ['current_lock_state', 'pin_code']

  def Test(self):
    self.lock_state = self.Property('current_lock_state')
    if self.lock_state is None:
      self.SetNotRun('Unable to determine pin code')
      return

    self.pin = self.Property('pin_code')
    if self.pin is None:
      # Try setting to a static value, we make old and new the same just on the
      # off chance this is actually the pin
      # http://www.datagenetics.com/blog/september32012/
      self.AddIfSetSupported([
        self.AckSetResult(),
        self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
      ])
      self.SendSet(PidStore.ROOT_DEVICE, self.pid, [439, self.lock_state])

    else:
      self.AddIfSetSupported([
        self.AckSetResult(action=self.VerifySet),
        self.NackSetResult(
          RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
          advisory='SET for %s returned unsupported command class' %
                   self.pid.name),
      ])
      self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.pin, self.lock_state])

  def VerifySet(self):
    self.AddExpectedResults(
        self.AckGetResult(field_values={'current_lock_state': self.lock_state}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


# LOCK_STATE_DESCRIPTION
# -----------------------------------------------------------------------------
class GetLockStateDescription(TestMixins.GetSettingDescriptionsRangeMixin,
                              OptionalParameterTestFixture):
  """Get LOCK_STATE_DESCRIPTION for all known states."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'LOCK_STATE_DESCRIPTION'
  REQUIRES = ['number_of_lock_states']
  EXPECTED_FIELDS = ['lock_state']
  DESCRIPTION_FIELD = 'lock_state_description'


class GetLockStateDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                        OptionalParameterTestFixture):
  """Get LOCK_STATE_DESCRIPTION with no lock state specified."""
  PID = 'LOCK_STATE_DESCRIPTION'


class GetLockStateDescriptionWithExtraData(TestMixins.GetWithDataMixin,
                                           OptionalParameterTestFixture):
  """Get LOCK_STATE_DESCRIPTION with extra data."""
  PID = 'LOCK_STATE_DESCRIPTION'


class GetZeroLockStateDescription(TestMixins.GetZeroUInt8Mixin,
                                  OptionalParameterTestFixture):
  """Get LOCK_STATE_DESCRIPTION for lock state 0."""
  PID = 'LOCK_STATE_DESCRIPTION'


class GetOutOfRangeLockStateDescription(TestMixins.GetOutOfRangeUInt8Mixin,
                                        OptionalParameterTestFixture):
  """Get LOCK_STATE_DESCRIPTION for an out-of-range lock state."""
  PID = 'LOCK_STATE_DESCRIPTION'
  REQUIRES = ['number_of_lock_states']
  LABEL = 'lock states'


class SetLockStateDescription(TestMixins.UnsupportedSetMixin,
                              OptionalParameterTestFixture):
  """Set the LOCK_STATE_DESCRIPTION."""
  PID = 'LOCK_STATE_DESCRIPTION'


class SetLockStateDescriptionWithData(TestMixins.UnsupportedSetWithDataMixin,
                                      OptionalParameterTestFixture):
  """Attempt to SET LOCK_STATE_DESCRIPTION with data."""
  PID = 'LOCK_STATE_DESCRIPTION'


class AllSubDevicesGetLockStateDescription(TestMixins.AllSubDevicesGetMixin,
                                           OptionalParameterTestFixture):
  """Get LOCK_STATE_DESCRIPTION addressed to ALL_SUB_DEVICES."""
  PID = 'LOCK_STATE_DESCRIPTION'
  DATA = [1]


# LOCK_PIN
# -----------------------------------------------------------------------------
class GetLockPin(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get LOCK_PIN."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'LOCK_PIN'
  EXPECTED_FIELDS = ['pin_code']
  PROVIDES = ['pin_code']
  # Some responders may not let you GET the pin.
  ALLOWED_NACKS = [RDMNack.NR_UNSUPPORTED_COMMAND_CLASS]


class GetLockPinWithData(TestMixins.GetWithDataMixin,
                         OptionalParameterTestFixture):
  """Get LOCK_PIN with data."""
  PID = 'LOCK_PIN'
  # Some responders may not let you GET the pin.
  ALLOWED_NACKS = [RDMNack.NR_UNSUPPORTED_COMMAND_CLASS]


class AllSubDevicesGetLockPin(TestMixins.AllSubDevicesGetMixin,
                              OptionalParameterTestFixture):
  """Get LOCK_PIN addressed to ALL_SUB_DEVICES."""
  PID = 'LOCK_PIN'


class SetLockPinWithNoData(TestMixins.SetWithNoDataMixin,
                           OptionalParameterTestFixture):
  """Set LOCK_PIN with no param data."""
  PID = 'LOCK_PIN'


class SetLockPinWithExtraData(TestMixins.SetWithDataMixin,
                              OptionalParameterTestFixture):
  """Send a SET LOCK_PIN command with extra data."""
  PID = 'LOCK_PIN'


class SetLockPin(OptionalParameterTestFixture):
  """Set LOCK_PIN."""
  CATEGORY = TestCategory.CONFIGURATION
  PID = 'LOCK_PIN'
  REQUIRES = ['pin_code']

  def Test(self):
    self.pin = self.Property('pin_code')
    if self.pin is None:
      # Try setting to a static value, we make old and new the same just on the
      # off chance this is actually the pin
      # http://www.datagenetics.com/blog/september32012/
      self.AddIfSetSupported([
        self.AckSetResult(),
        self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE),
      ])
      self.SendSet(PidStore.ROOT_DEVICE, self.pid, [439, 439])

    else:
      self.AddIfSetSupported([
        self.AckSetResult(action=self.VerifySet),
        self.NackSetResult(
          RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
          advisory='SET for %s returned unsupported command class' %
                   self.pid.name),
      ])
      self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.pin, self.pin])

  def VerifySet(self):
    self.AddExpectedResults([
      self.AckGetResult(field_values={'pin_code': self.pin}),
      self.NackGetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
    ])
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetInvalidLockPin(OptionalParameterTestFixture):
  """Set LOCK_PIN with the wrong pin code."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LOCK_PIN'

  REQUIRES = ['pin_code']

  def Test(self):
    self.pin = self.Property('pin_code')
    if self.pin is None:
      self.SetNotRun('Unable to determine pin code')
      return

    bad_pin = (self.pin + 1) % 10000
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendSet(ROOT_DEVICE, self.pid, [0, bad_pin])


class SetOutOfRangeLockPin(OptionalParameterTestFixture):
  """Set LOCK_PIN with an out-of-range pin code."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'LOCK_PIN'
  REQUIRES = ['pin_code']

  def Test(self):
    self.pin = self.Property('pin_code')
    if self.pin is None:
      self.SetNotRun('Unable to determine pin code')
      return

    # Section 3.9, out of range pins return NR_FORMAT_ERROR rather than
    # NR_DATA_OUT_OF_RANGE like one may expect. NR_DATA_OUT_OF_RANGE is
    # reserved for reporting when an incorrect PIN is used.
    self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_FORMAT_ERROR))
    data = struct.pack('!HH', 10001, self.pin)
    self.SendRawSet(ROOT_DEVICE, self.pid, data)


# BURN_IN
# -----------------------------------------------------------------------------
class GetBurnIn(TestMixins.GetMixin, OptionalParameterTestFixture):
  """GET BURN_IN."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'BURN_IN'
  EXPECTED_FIELDS = ['hours_remaining']
  PROVIDES = ['burn_in_hours']


class GetBurnInWithData(TestMixins.GetWithDataMixin,
                        OptionalParameterTestFixture):
  """Get BURN_IN with extra data."""
  PID = 'BURN_IN'


class SetBurnIn(TestMixins.SetUInt8Mixin, OptionalParameterTestFixture):
  """Set BURN_IN."""
  CATEGORY = TestCategory.POWER_LAMP_SETTINGS
  PID = 'BURN_IN'
  EXPECTED_FIELDS = ['hours_remaining']
  REQUIRES = ['burn_in_hours']

  def OldValue(self):
    return self.Property('burn_in_hours')

  def VerifySet(self):
    new_value = self.NewValue()
    results = [
      self.AckGetResult(
          field_names=self.EXPECTED_FIELDS,
          field_values={self.EXPECTED_FIELDS[0]: self.NewValue()}),
    ]
    # Since this is hours remaining, it may be decremented before we can read
    # it back
    if new_value:
      results.append(
        self.AckGetResult(
            field_names=self.EXPECTED_FIELDS,
            field_values={self.EXPECTED_FIELDS[0]: new_value - 1}))
    self.AddExpectedResults(results)
    self.SendGet(ROOT_DEVICE, self.pid)


class SetBurnInWithNoData(TestMixins.SetWithNoDataMixin,
                          OptionalParameterTestFixture):
  """Set BURN_IN with no param data."""
  PID = 'BURN_IN'


class SetBurnInWithExtraData(TestMixins.SetWithDataMixin,
                             OptionalParameterTestFixture):
  """Send a SET BURN_IN command with extra data."""
  PID = 'BURN_IN'


class AllSubDevicesGetBurnIn(TestMixins.AllSubDevicesGetMixin,
                             OptionalParameterTestFixture):
  """Get BURN_IN addressed to ALL_SUB_DEVICES."""
  PID = 'BURN_IN'


# DIMMER_INFO
# -----------------------------------------------------------------------------
class GetDimmerInfo(OptionalParameterTestFixture):
  """GET dimmer info."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'DIMMER_INFO'
  REQUIRES = ['supported_parameters']
  PROVIDES = ['minimum_level_lower', 'minimum_level_upper',
              'maximum_level_lower', 'maximum_level_upper',
              'number_curves_supported', 'levels_resolution',
              'split_levels_supported']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      for field in self.PROVIDES:
        self.SetProperty(field, None)
      return

    if fields['minimum_level_lower'] > fields['minimum_level_upper']:
      self.AddAdvisory('minimum_level_lower (%d) > minimum_level_upper (%d)'
                       % (fields['minimum_level_lower'],
                          fields['minimum_level_upper']))

    if fields['maximum_level_lower'] > fields['maximum_level_upper']:
      self.AddAdvisory('maximum_level_lower (%d) > maximum_level_upper (%d)'
                       % (fields['maximum_level_lower'],
                          fields['maximum_level_upper']))

    self.SetPropertyFromDict(fields, 'minimum_level_lower')
    self.SetPropertyFromDict(fields, 'minimum_level_upper')
    self.SetPropertyFromDict(fields, 'maximum_level_lower')
    self.SetPropertyFromDict(fields, 'maximum_level_upper')
    self.SetPropertyFromDict(fields, 'number_curves_supported')
    self.SetPropertyFromDict(fields, 'levels_resolution')

    self.SetProperty('split_levels_supported', fields['split_levels_supported'])

    self.CheckFieldsAreUnsupported(
        'MINIMUM_LEVEL', fields,
        {'minimum_level_lower': 0, 'minimum_level_upper': 0,
         'split_levels_supported': 0})
    self.CheckFieldsAreUnsupported(
        'MAXIMUM_LEVEL', fields,
        {'maximum_level_lower': 0xffff, 'maximum_level_upper': 0xffff})

  def CheckFieldsAreUnsupported(self, pid_name, fields, keys):
    if self.LookupPid(pid_name).value in self.Property('supported_parameters'):
      return

    for key, expected_value in keys.items():
      if fields[key] != expected_value:
        self.AddWarning(
            "%s isn't supported but %s in DIMMER_INFO was not %hx" %
            (pid_name, key, expected_value))


class GetDimmerInfoWithData(TestMixins.GetWithDataMixin,
                            OptionalParameterTestFixture):
  """Get DIMMER_INFO with extra data."""
  PID = 'DIMMER_INFO'


class SetDimmerInfo(TestMixins.UnsupportedSetMixin,
                    OptionalParameterTestFixture):
  """Set DIMMER_INFO."""
  PID = 'DIMMER_INFO'


class SetDimmerInfoWithData(TestMixins.UnsupportedSetWithDataMixin,
                            OptionalParameterTestFixture):
  """Attempt to SET DIMMER_INFO with data."""
  PID = 'DIMMER_INFO'


class AllSubDevicesGetDimmerInfo(TestMixins.AllSubDevicesGetMixin,
                                 OptionalParameterTestFixture):
  """Get DIMMER_INFO addressed to ALL_SUB_DEVICES."""
  PID = 'DIMMER_INFO'


# MINIMUM_LEVEL
# -----------------------------------------------------------------------------
class GetMinimumLevel(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get MINIMUM_LEVEL."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = "MINIMUM_LEVEL"
  REQUIRES = ['minimum_level_lower', 'minimum_level_upper',
              'split_levels_supported']
  PROVIDES = ['minimum_level_settings']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if fields is None:
      fields = {}
    self.SetProperty('minimum_level_settings', fields)

    if not response.WasAcked():
      return

    min_increasing = fields['minimum_level_increasing']
    min_decreasing = fields['minimum_level_decreasing']
    lower_limit = self.Property('minimum_level_lower')
    upper_limit = self.Property('minimum_level_upper')
    if lower_limit is not None and upper_limit is not None:
      if min_increasing < lower_limit or min_increasing > upper_limit:
        self.SetFailed(
            'minimum_level_increasing is outside the range [%d, %d] from '
            'DIMMER_INFO' % (lower_limit, upper_limit))
        return
      if min_decreasing < lower_limit or min_decreasing > upper_limit:
        self.SetFailed(
            'minimum_level_decreasing is outside the range [%d, %d] from '
            'DIMMER_INFO' % (lower_limit, upper_limit))
        return

    split_supported = self.Property('split_levels_supported')
    if split_supported is not None:
      if not split_supported and min_increasing != min_decreasing:
        self.SetFailed(
            'Split min levels not supported but min level increasing (%d)'
            ' != min level decreasing (%d)' % (min_increasing, min_decreasing))


class GetMinimumLevelWithData(TestMixins.GetWithDataMixin,
                              OptionalParameterTestFixture):
  """Get MINIMUM_LEVEL with extra data."""
  PID = 'MINIMUM_LEVEL'


class AllSubDevicesGetMinimumLevel(TestMixins.AllSubDevicesGetMixin,
                                   OptionalParameterTestFixture):
  """Get MINIMUM_LEVEL addressed to ALL_SUB_DEVICES."""
  PID = 'MINIMUM_LEVEL'


class SetMinimumLevel(OptionalParameterTestFixture):
  """Set MINIMUM_LEVEL without changing the settings."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'MINIMUM_LEVEL'
  REQUIRES = ['minimum_level_settings']
  PROVIDES = ['set_minimum_level_supported']

  def Test(self):
    settings = self.Property('minimum_level_settings')
    if not settings:
      self.SetNotRun('Unable to determine current MINIMUM_LEVEL settings')
      return

    self.AddIfSetSupported([
      self.AckSetResult(),
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
    ])
    self.SendSet(
        ROOT_DEVICE, self.pid,
        [settings['minimum_level_increasing'],
         settings['minimum_level_decreasing'],
         settings['on_below_minimum']])

  def VerifyResult(self, response, fields):
    self.SetProperty('set_minimum_level_supported', response.WasAcked())


class SetLowerIncreasingMinimumLevel(TestMixins.SetMinimumLevelMixin,
                                     OptionalParameterTestFixture):
  """Set MINIMUM_LEVEL to the smallest value from DIMMER_INFO."""
  REQUIRES = TestMixins.SetMinimumLevelMixin.REQUIRES + ['minimum_level_lower']

  def MinLevelIncreasing(self):
    return self.Property('minimum_level_lower')


class SetUpperIncreasingMinimumLevel(TestMixins.SetMinimumLevelMixin,
                                     OptionalParameterTestFixture):
  """Set MINIMUM_LEVEL to the largest value from DIMMER_INFO."""
  REQUIRES = TestMixins.SetMinimumLevelMixin.REQUIRES + ['minimum_level_upper']

  def MinLevelIncreasing(self):
    return self.Property('minimum_level_upper')


class SetOutOfRangeLowerIncreasingMinimumLevel(TestMixins.SetMinimumLevelMixin,
                                               OptionalParameterTestFixture):
  """Set MINIMUM_LEVEL to one less than the smallest value from DIMMER_INFO."""
  REQUIRES = TestMixins.SetMinimumLevelMixin.REQUIRES + ['minimum_level_lower']

  def ExpectedResults(self):
    return self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE)

  def ShouldSkip(self):
    self.lower = self.Property('minimum_level_lower')
    if self.lower == 0:
      self.SetNotRun('All values supported')
      return True
    return False

  def MinLevelIncreasing(self):
    return self.lower - 1


class SetOutOfRangeUpperIncreasingMinimumLevel(TestMixins.SetMinimumLevelMixin,
                                               OptionalParameterTestFixture):
  """Set MINIMUM_LEVEL to one more than the largest value from DIMMER_INFO."""
  REQUIRES = TestMixins.SetMinimumLevelMixin.REQUIRES + ['minimum_level_upper']

  def ExpectedResults(self):
    return self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE)

  def ShouldSkip(self):
    self.upper = self.Property('minimum_level_upper')
    if self.upper == 0xfffe:
      self.SetNotRun('All values supported')
      return True
    return False

  def MinLevelIncreasing(self):
    return self.upper + 1


class SetMinimumLevelWithNoData(TestMixins.SetWithNoDataMixin,
                                OptionalParameterTestFixture):
  """Set MINIMUM_LEVEL command with no data."""
  PID = 'MINIMUM_LEVEL'


class SetMinimumLevelWithExtraData(TestMixins.SetWithDataMixin,
                                   OptionalParameterTestFixture):
  """Send a SET MINIMUM_LEVEL command with extra data."""
  PID = 'MINIMUM_LEVEL'
  DATA = 'foobar'


# MAXIMUM_LEVEL
# -----------------------------------------------------------------------------
class GetMaximumLevel(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get MAXIMUM_LEVEL."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = "MAXIMUM_LEVEL"
  PROVIDES = ['maximum_level']
  EXPECTED_FIELDS = ['maximum_level']


class GetMaximumLevelWithData(TestMixins.GetWithDataMixin,
                              OptionalParameterTestFixture):
  """Get MAXIMUM_LEVEL with extra data."""
  PID = 'MAXIMUM_LEVEL'


class SetMaximumLevel(OptionalParameterTestFixture):
  """Set MAXIMUM_LEVEL without changing the settings."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'MAXIMUM_LEVEL'
  REQUIRES = ['maximum_level']
  PROVIDES = ['set_maximum_level_supported']

  def Test(self):
    current_value = self.Property('maximum_level')
    if current_value is None:
      current_value = 0xffff

    self.AddIfSetSupported([
      self.AckSetResult(),
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
    ])
    self.SendSet(ROOT_DEVICE, self.pid, [current_value])

  def VerifyResult(self, response, fields):
    self.SetProperty('set_maximum_level_supported', response.WasAcked())


class SetLowerMaximumLevel(TestMixins.SetMaximumLevelMixin,
                           OptionalParameterTestFixture):
  """Set MAXIMUM_LEVEL to the smallest value from DIMMER_INFO."""
  REQUIRES = TestMixins.SetMaximumLevelMixin.REQUIRES + ['maximum_level_lower']

  def Test(self):
    self.value = self.Property('maximum_level_lower')
    if self.value is None:
      self.SetNotRun('No lower maximum level from DIMMER_INFO')
      return

    if self.Property('set_maximum_level_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetMaxLevel))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid, [self.value])

  def GetMaxLevel(self):
    self.AddIfGetSupported(self.AckGetResult(
        field_values={'maximum_level': self.value}))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetUpperMaximumLevel(TestMixins.SetMaximumLevelMixin,
                           OptionalParameterTestFixture):
  """Set MAXIMUM_LEVEL to the largest value from DIMMER_INFO."""
  REQUIRES = TestMixins.SetMaximumLevelMixin.REQUIRES + ['maximum_level_upper']

  def Test(self):
    self.value = self.Property('maximum_level_upper')
    if self.value is None:
      self.SetNotRun('No upper maximum level from DIMMER_INFO')
      return

    if self.Property('set_maximum_level_supported'):
      self.AddIfSetSupported(self.AckSetResult(action=self.GetMaxLevel))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid, [self.value])

  def GetMaxLevel(self):
    self.AddIfGetSupported(self.AckGetResult(
        field_values={'maximum_level': self.value}))
    self.SendGet(ROOT_DEVICE, self.pid)


class SetLowerOutOfRangeMaximumLevel(OptionalParameterTestFixture):
  """Set MAXIMUM_LEVEL a value smaller than the minimum."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'MAXIMUM_LEVEL'
  REQUIRES = ['maximum_level_lower', 'set_maximum_level_supported']

  def Test(self):
    self.value = self.Property('maximum_level_lower')
    if self.value is None:
      self.SetNotRun('No lower maximum level from DIMMER_INFO')
      return

    if self.value == 0:
      self.SetNotRun('Range for maximum level begins at 0')
      return
    self.value -= 1

    if self.Property('set_maximum_level_supported'):
      self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid, [self.value])


class SetUpperOutOfRangeMaximumLevel(OptionalParameterTestFixture):
  """Set MAXIMUM_LEVEL a value larger than the maximum."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'MAXIMUM_LEVEL'
  REQUIRES = ['maximum_level_upper', 'set_maximum_level_supported']

  def Test(self):
    self.value = self.Property('maximum_level_upper')
    if self.value is None:
      self.SetNotRun('No upper maximum level from DIMMER_INFO')
      return

    if self.value == 0xffff:
      self.SetNotRun('Range for maximum level ends at 0xffff')
      return
    self.value += 1

    if self.Property('set_maximum_level_supported'):
      self.AddIfSetSupported(self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    else:
      self.AddIfSetSupported(
          self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))

    self.SendSet(ROOT_DEVICE, self.pid, [self.value])


class SetMaximumLevelWithNoData(TestMixins.SetWithNoDataMixin,
                                OptionalParameterTestFixture):
  """Set MAXIMUM_LEVEL command with no data."""
  PID = 'MAXIMUM_LEVEL'


class SetMaximumLevelWithExtraData(TestMixins.SetWithDataMixin,
                                   OptionalParameterTestFixture):
  """Send a SET MAXIMUM_LEVEL command with extra data."""
  PID = 'MAXIMUM_LEVEL'


class AllSubDevicesGetMaximumLevel(TestMixins.AllSubDevicesGetMixin,
                                   OptionalParameterTestFixture):
  """Get MAXIMUM_LEVEL addressed to ALL_SUB_DEVICES."""
  PID = 'MAXIMUM_LEVEL'


# CURVE
# -----------------------------------------------------------------------------
class GetCurve(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get CURVE."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = "CURVE"
  REQUIRES = ['number_curves_supported']
  PROVIDES = ['current_curve', 'number_curves']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      for key in self.PROVIDES:
        self.SetProperty(key, None)
      return

    self.SetPropertyFromDict(fields, 'current_curve')
    self.SetPropertyFromDict(fields, 'number_curves')

    if fields['current_curve'] == 0:
      self.SetFailed('Curves must be numbered from 1')
      return

    if fields['current_curve'] > fields['number_curves']:
      self.SetFailed('Curve %d exceeded number of curves %d' %
                     (fields['current_curve'], fields['number_curves']))
      return

    expected_curves = self.Property('number_curves_supported')
    if expected_curves is not None:
      if expected_curves != fields['number_curves']:
        self.AddWarning(
            'The number of curves reported in DIMMER_INFO (%d) does not '
            'match the number from CURVE (%d)' %
            (expected_curves, fields['number_curves']))


class GetCurveWithData(TestMixins.GetWithDataMixin,
                       OptionalParameterTestFixture):
  """Get CURVE with extra data."""
  PID = 'CURVE'


class SetCurve(OptionalParameterTestFixture):
  """Set CURVE."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = "CURVE"
  REQUIRES = ['current_curve', 'number_curves']

  def Test(self):
    curves = self.Property('number_curves')
    if curves:
      self.curves = [i + 1 for i in range(curves)]
      self._SetCurve()
    else:
      # Check we get a NR_UNKNOWN_PID
      self.AddExpectedResults(self.NackSetResult(RDMNack.NR_UNKNOWN_PID))
      self.curve = 1  # Can use anything here really
      self.SendSet(ROOT_DEVICE, self.pid, [1])

  def _SetCurve(self):
    if not self.curves:
      # End of the list, we're done
      self.Stop()
      return

    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(ROOT_DEVICE, self.pid, [self.curves[0]])

  def VerifySet(self):
    self.AddIfGetSupported(
      self.AckGetResult(
        field_values={'current_curve': self.curves[0]},
        action=self.NextCurve))
    self.SendGet(ROOT_DEVICE, self.pid)

  def NextCurve(self):
    self.curves = self.curves[1:]
    self._SetCurve()

  def ResetState(self):
    if not self.PidSupported() or not self.Property('current_curve'):
      return

    self.SendSet(ROOT_DEVICE, self.pid, [self.Property('current_curve')])
    self._wrapper.Run()


class SetZeroCurve(TestMixins.SetZeroUInt8Mixin,
                   OptionalParameterTestFixture):
  """Set CURVE to 0."""
  PID = 'CURVE'


class SetOutOfRangeCurve(TestMixins.SetOutOfRangeUInt8Mixin,
                         OptionalParameterTestFixture):
  """Set CURVE to an out-of-range value."""
  PID = 'CURVE'
  REQUIRES = ['number_curves']
  LABEL = 'curves'


class SetCurveWithNoData(TestMixins.SetWithNoDataMixin,
                         OptionalParameterTestFixture):
  """Set CURVE without any data."""
  PID = 'CURVE'


class SetCurveWithExtraData(TestMixins.SetWithDataMixin,
                            OptionalParameterTestFixture):
  """Send a SET CURVE command with extra data."""
  PID = 'CURVE'


class AllSubDevicesGetCurve(TestMixins.AllSubDevicesGetMixin,
                            OptionalParameterTestFixture):
  """Get CURVE addressed to ALL_SUB_DEVICES."""
  PID = 'CURVE'


# CURVE_DESCRIPTION
# -----------------------------------------------------------------------------
class GetCurveDescription(TestMixins.GetSettingDescriptionsRangeMixin,
                          OptionalParameterTestFixture):
  """Get the CURVE_DESCRIPTION for all known curves."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'CURVE_DESCRIPTION'
  REQUIRES = ['number_curves']
  EXPECTED_FIELDS = ['curve_number']
  DESCRIPTION_FIELD = 'curve_description'


class GetCurveDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                    OptionalParameterTestFixture):
  """Get CURVE_DESCRIPTION with no curve number specified."""
  PID = 'CURVE_DESCRIPTION'


class GetCurveDescriptionWithExtraData(TestMixins.GetWithDataMixin,
                                       OptionalParameterTestFixture):
  """Get CURVE_DESCRIPTION with extra data."""
  PID = 'CURVE_DESCRIPTION'


class GetZeroCurveDescription(TestMixins.GetZeroUInt8Mixin,
                              OptionalParameterTestFixture):
  """Get CURVE_DESCRIPTION for curve 0."""
  PID = 'CURVE_DESCRIPTION'


class GetOutOfRangeCurveDescription(TestMixins.GetOutOfRangeUInt8Mixin,
                                    OptionalParameterTestFixture):
  """Get CURVE_DESCRIPTION for an out-of-range curve."""
  PID = 'CURVE_DESCRIPTION'
  REQUIRES = ['number_curves']
  LABEL = 'curves'


class SetCurveDescription(TestMixins.UnsupportedSetMixin,
                          OptionalParameterTestFixture):
  """Set the CURVE_DESCRIPTION."""
  PID = 'CURVE_DESCRIPTION'


class SetCurveDescriptionWithData(TestMixins.UnsupportedSetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Attempt to SET CURVE_DESCRIPTION with data."""
  PID = 'CURVE_DESCRIPTION'


class AllSubDevicesGetCurveDescription(TestMixins.AllSubDevicesGetMixin,
                                       OptionalParameterTestFixture):
  """Get CURVE_DESCRIPTION addressed to ALL_SUB_DEVICES."""
  PID = 'CURVE_DESCRIPTION'
  DATA = [1]


# OUTPUT_RESPONSE_TIME
# -----------------------------------------------------------------------------
class GetOutputResponseTime(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get OUTPUT_RESPONSE_TIME."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = "OUTPUT_RESPONSE_TIME"
  PROVIDES = ['current_response_time', 'number_response_options']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      for key in self.PROVIDES:
        self.SetProperty(key, None)
      return

    self.SetPropertyFromDict(fields, 'current_response_time')
    self.SetPropertyFromDict(fields, 'number_response_options')

    if fields['current_response_time'] == 0:
      self.SetFailed('Output response times must be numbered from 1')
      return

    if fields['current_response_time'] > fields['number_response_options']:
      self.SetFailed(
          'Output response time %d exceeded number of response times %d' %
          (fields['current_response_time'],
           fields['number_response_options']))
      return


class GetOutputResponseTimeWithData(TestMixins.GetWithDataMixin,
                                    OptionalParameterTestFixture):
  """Get OUTPUT_RESPONSE_TIME with extra data."""
  PID = 'OUTPUT_RESPONSE_TIME'


class SetOutputResponseTime(OptionalParameterTestFixture):
  """Set OUTPUT_RESPONSE_TIME."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = "OUTPUT_RESPONSE_TIME"
  REQUIRES = ['current_response_time', 'number_response_options']

  def Test(self):
    times = self.Property('number_response_options')
    if times:
      self.output_response_times = [i + 1 for i in range(times)]
      self._SetOutputResponseTime()
    else:
      # Check we get a NR_UNKNOWN_PID
      self.AddExpectedResults(self.NackSetResult(RDMNack.NR_UNKNOWN_PID))
      self.current_response_time = 1  # Can use anything here really
      self.SendSet(ROOT_DEVICE, self.pid, [1])

  def _SetOutputResponseTime(self):
    if not self.output_response_times:
      # End of the list, we're done
      self.Stop()
      return

    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(ROOT_DEVICE, self.pid, [self.output_response_times[0]])

  def VerifySet(self):
    self.AddIfGetSupported(
      self.AckGetResult(
        field_values={'current_response_time': self.output_response_times[0]},
        action=self.NextOutputResponseTime))
    self.SendGet(ROOT_DEVICE, self.pid)

  def NextOutputResponseTime(self):
    self.output_response_times = self.output_response_times[1:]
    self._SetOutputResponseTime()

  def ResetState(self):
    if not self.PidSupported() or not self.Property('current_response_time'):
      return

    self.SendSet(ROOT_DEVICE, self.pid,
                 [self.Property('current_response_time')])
    self._wrapper.Run()


class SetZeroOutputResponseTime(TestMixins.SetZeroUInt8Mixin,
                                OptionalParameterTestFixture):
  """Set OUTPUT_RESPONSE_TIME to 0."""
  PID = 'OUTPUT_RESPONSE_TIME'


class SetOutOfRangeOutputResponseTime(TestMixins.SetOutOfRangeUInt8Mixin,
                                      OptionalParameterTestFixture):
  """Set OUTPUT_RESPONSE_TIME to an out-of-range value."""
  PID = 'OUTPUT_RESPONSE_TIME'
  REQUIRES = ['number_response_options']
  LABEL = 'output response times'


class SetOutputResponseTimeWithNoData(TestMixins.SetWithNoDataMixin,
                                      OptionalParameterTestFixture):
  """Set OUTPUT_RESPONSE_TIME without any data."""
  PID = 'OUTPUT_RESPONSE_TIME'


class SetOutputResponseTimeWithExtraData(TestMixins.SetWithDataMixin,
                                         OptionalParameterTestFixture):
  """Send a SET OUTPUT_RESPONSE_TIME command with extra data."""
  PID = 'OUTPUT_RESPONSE_TIME'


class AllSubDevicesGetOutputResponseTime(TestMixins.AllSubDevicesGetMixin,
                                         OptionalParameterTestFixture):
  """Get OUTPUT_RESPONSE_TIME addressed to ALL_SUB_DEVICES."""
  PID = 'OUTPUT_RESPONSE_TIME'


# OUTPUT_RESPONSE_TIME_DESCRIPTION
# -----------------------------------------------------------------------------
class GetOutputResponseTimeDescription(
        TestMixins.GetSettingDescriptionsRangeMixin,
        OptionalParameterTestFixture):
  """Get the OUTPUT_RESPONSE_TIME_DESCRIPTION for all response times."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'OUTPUT_RESPONSE_TIME_DESCRIPTION'
  REQUIRES = ['number_response_options']
  EXPECTED_FIELDS = ['response_time']
  DESCRIPTION_FIELD = 'response_time_description'


class GetOutputResponseTimeDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                                 OptionalParameterTestFixture):
  """Get OUTPUT_RESPONSE_TIME_DESCRIPTION with no response time number."""
  PID = 'OUTPUT_RESPONSE_TIME_DESCRIPTION'


class GetOutputResponseTimeDescriptionWithExtraData(
        TestMixins.GetWithDataMixin,
        OptionalParameterTestFixture):
  """Get OUTPUT_RESPONSE_TIME_DESCRIPTION with extra data."""
  PID = 'OUTPUT_RESPONSE_TIME_DESCRIPTION'


class GetZeroOutputResponseTimeDescription(TestMixins.GetZeroUInt8Mixin,
                                           OptionalParameterTestFixture):
  """Get OUTPUT_RESPONSE_TIME_DESCRIPTION for response_time 0."""
  PID = 'OUTPUT_RESPONSE_TIME_DESCRIPTION'


class GetOutOfRangeOutputResponseTimeDescription(
        TestMixins.GetOutOfRangeUInt8Mixin,
        OptionalParameterTestFixture):
  """Get OUTPUT_RESPONSE_TIME_DESCRIPTION for an out-of-range response time."""
  PID = 'OUTPUT_RESPONSE_TIME_DESCRIPTION'
  REQUIRES = ['number_response_options']
  LABEL = 'output response times'


class SetOutputResponseTimeDescription(TestMixins.UnsupportedSetMixin,
                                       OptionalParameterTestFixture):
  """SET OUTPUT_RESPONSE_TIME_DESCRIPTION."""
  PID = 'OUTPUT_RESPONSE_TIME_DESCRIPTION'


class SetOutputResponseTimeDescriptionWithData(
        TestMixins.UnsupportedSetWithDataMixin,
        OptionalParameterTestFixture):
  """Attempt to SET OUTPUT_RESPONSE_TIME_DESCRIPTION with data."""
  PID = 'OUTPUT_RESPONSE_TIME_DESCRIPTION'


class AllSubDevicesGetOutputResponseTimeDescription(
        TestMixins.AllSubDevicesGetMixin,
        OptionalParameterTestFixture):
  """Get OUTPUT_RESPONSE_TIME_DESCRIPTION addressed to ALL_SUB_DEVICES."""
  PID = 'OUTPUT_RESPONSE_TIME_DESCRIPTION'
  DATA = [1]


# MODULATION_FREQUENCY
# -----------------------------------------------------------------------------
class GetModulationFrequency(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get MODULATION_FREQUENCY."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = "MODULATION_FREQUENCY"
  PROVIDES = ['current_modulation_frequency', 'number_modulation_frequencies']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      for key in self.PROVIDES:
        self.SetProperty(key, None)
      return

    self.SetPropertyFromDict(fields, 'current_modulation_frequency')
    self.SetPropertyFromDict(fields, 'number_modulation_frequencies')

    if fields['current_modulation_frequency'] == 0:
      self.SetFailed('Modulation frequency must be numbered from 1')
      return

    if (fields['current_modulation_frequency'] >
        fields['number_modulation_frequencies']):
      self.SetFailed(
          'Modulation frequency %d exceeded number of modulation frequencies %d'
          % (fields['current_modulation_frequency'],
             fields['number_modulation_frequencies']))
      return


class GetModulationFrequencyWithData(TestMixins.GetWithDataMixin,
                                     OptionalParameterTestFixture):
  """Get MODULATION_FREQUENCY with extra data."""
  PID = 'MODULATION_FREQUENCY'


class SetModulationFrequency(OptionalParameterTestFixture):
  """Set MODULATION_FREQUENCY."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = "MODULATION_FREQUENCY"
  REQUIRES = ['current_modulation_frequency', 'number_modulation_frequencies']

  def Test(self):
    items = self.Property('number_modulation_frequencies')
    if items:
      self.frequencies = [i + 1 for i in range(items)]
      self._SetModulationFrequency()
    else:
      # Check we get a NR_UNKNOWN_PID
      self.AddExpectedResults(self.NackSetResult(RDMNack.NR_UNKNOWN_PID))
      self.frequency = 1  # Can use anything here really
      self.SendSet(ROOT_DEVICE, self.pid, [1])

  def _SetModulationFrequency(self):
    if not self.frequencies:
      # End of the list, we're done
      self.Stop()
      return

    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(ROOT_DEVICE, self.pid, [self.frequencies[0]])

  def VerifySet(self):
    self.AddIfGetSupported(
      self.AckGetResult(
        field_values={'current_modulation_frequency': self.frequencies[0]},
        action=self.NextModulationFrequency))
    self.SendGet(ROOT_DEVICE, self.pid)

  def NextModulationFrequency(self):
    self.frequencies = self.frequencies[1:]
    self._SetModulationFrequency()

  def ResetState(self):
    if (not self.PidSupported() or
        not self.Property('current_modulation_frequency')):
      return

    self.SendSet(ROOT_DEVICE, self.pid,
                 [self.Property('current_modulation_frequency')])
    self._wrapper.Run()


class SetZeroModulationFrequency(TestMixins.SetZeroUInt8Mixin,
                                 OptionalParameterTestFixture):
  """Set MODULATION_FREQUENCY with a frequency setting of 0."""
  PID = 'MODULATION_FREQUENCY'


class SetOutOfRangeModulationFrequency(TestMixins.SetOutOfRangeUInt8Mixin,
                                       OptionalParameterTestFixture):
  """Set MODULATION_FREQUENCY to an out-of-range value."""
  PID = 'MODULATION_FREQUENCY'
  REQUIRES = ['number_modulation_frequencies']
  LABEL = 'modulation frequencies'


class SetModulationFrequencyWithNoData(TestMixins.SetWithNoDataMixin,
                                       OptionalParameterTestFixture):
  """Set MODULATION_FREQUENCY without any data."""
  PID = 'MODULATION_FREQUENCY'


class SetModulationFrequencyWithExtraData(TestMixins.SetWithDataMixin,
                                          OptionalParameterTestFixture):
  """Send a SET MODULATION_FREQUENCY command with extra data."""
  PID = 'MODULATION_FREQUENCY'


class AllSubDevicesGetModulationFrequency(TestMixins.AllSubDevicesGetMixin,
                                          OptionalParameterTestFixture):
  """Get MODULATION_FREQUENCY addressed to ALL_SUB_DEVICES."""
  PID = 'MODULATION_FREQUENCY'


# MODULATION_FREQUENCY_DESCRIPTION
# -----------------------------------------------------------------------------
class GetModulationFrequencyDescription(
        TestMixins.GetSettingDescriptionsRangeMixin,
        OptionalParameterTestFixture):
  """Get the MODULATION_FREQUENCY_DESCRIPTION for all frequencies."""
  CATEGORY = TestCategory.DIMMER_SETTINGS
  PID = 'MODULATION_FREQUENCY_DESCRIPTION'
  REQUIRES = ['number_modulation_frequencies']
  EXPECTED_FIELDS = ['modulation_frequency']
  DESCRIPTION_FIELD = 'modulation_frequency_description'


class GetModulationFrequencyDescriptionWithNoData(TestMixins.GetWithNoDataMixin,
                                                  OptionalParameterTestFixture):
  """Get MODULATION_FREQUENCY_DESCRIPTION with no frequency number."""
  PID = 'MODULATION_FREQUENCY_DESCRIPTION'


class GetModulationFrequencyDescriptionWithExtraData(
        TestMixins.GetWithDataMixin,
        OptionalParameterTestFixture):
  """Get MODULATION_FREQUENCY_DESCRIPTION with extra data."""
  PID = 'MODULATION_FREQUENCY_DESCRIPTION'


class GetZeroModulationFrequencyDescription(TestMixins.GetZeroUInt8Mixin,
                                            OptionalParameterTestFixture):
  """Get MODULATION_FREQUENCY_DESCRIPTION for frequency 0."""
  PID = 'MODULATION_FREQUENCY_DESCRIPTION'


class GetOutOfRangeModulationFrequencyDescription(
        TestMixins.GetOutOfRangeUInt8Mixin,
        OptionalParameterTestFixture):
  """Get MODULATION_FREQUENCY_DESCRIPTION for an out-of-range frequency."""
  PID = 'MODULATION_FREQUENCY_DESCRIPTION'
  REQUIRES = ['number_modulation_frequencies']
  LABEL = 'modulation frequencies'


class SetModulationFrequencyDescription(TestMixins.UnsupportedSetMixin,
                                        OptionalParameterTestFixture):
  """SET MODULATION_FREQUENCY_DESCRIPTION."""
  PID = 'MODULATION_FREQUENCY_DESCRIPTION'


class SetModulationFrequencyDescriptionWithData(
        TestMixins.UnsupportedSetWithDataMixin,
        OptionalParameterTestFixture):
  """Attempt to SET MODULATION_FREQUENCY_DESCRIPTION with data."""
  PID = 'MODULATION_FREQUENCY_DESCRIPTION'


class AllSubDevicesGetModulationFrequencyDescription(
        TestMixins.AllSubDevicesGetMixin,
        OptionalParameterTestFixture):
  """Get MODULATION_FREQUENCY_DESCRIPTION addressed to ALL_SUB_DEVICES."""
  PID = 'MODULATION_FREQUENCY_DESCRIPTION'
  DATA = [1]


# PRESET_INFO
# -----------------------------------------------------------------------------
class GetPresetInfo(TestMixins.GetMixin, OptionalParameterTestFixture):
  """Get PRESET_INFO."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_INFO'
  PROVIDES = ['preset_info', 'max_scene_number']
  REQUIRES = ['supported_parameters']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      self.SetProperty('preset_info', {})
      self.SetProperty('max_scene_number', None)
      return

    self.CheckBounds(fields, 'preset_fade_time')
    self.CheckBounds(fields, 'preset_wait_time')
    self.CheckBounds(fields, 'fail_delay_time')
    self.CheckBounds(fields, 'fail_hold_time')
    self.CheckBounds(fields, 'startup_delay_time')
    self.CheckBounds(fields, 'startup_hold_time')

    if fields['max_scene_number'] == 0xffff:
      self.AddWarning('PRESET_INFO had max_scene_number of 0xffff')

    self.CrossCheckPidSupportIsZero('DMX_FAIL_MODE', fields,
                                    'fail_infinite_hold_supported')
    self.CrossCheckPidSupportIsZero('DMX_FAIL_MODE', fields,
                                    'fail_infinite_delay_supported')
    self.CrossCheckPidSupportIsZero('DMX_STARTUP_MODE', fields,
                                    'startup_infinite_hold_supported')

    self.CrossCheckPidSupportIsMax('DMX_FAIL_MODE', fields,
                                   'fail_delay_time')
    self.CrossCheckPidSupportIsMax('DMX_FAIL_MODE', fields,
                                   'fail_hold_time')
    self.CrossCheckPidSupportIsMax('DMX_STARTUP_MODE', fields,
                                   'startup_delay_time')
    self.CrossCheckPidSupportIsMax('DMX_STARTUP_MODE', fields,
                                   'startup_hold_time')

    self.SetProperty('preset_info', fields)
    self.SetProperty('max_scene_number', fields['max_scene_number'])

  def CrossCheckPidSupportIsZero(self, pid_name, fields, field_name):
    if not (self.IsSupported(pid_name) or fields[field_name] is False):
      self.AddWarning('%s not supported, but %s in PRESET_INFO is non-0' %
                      (pid_name, field_name))

  def CrossCheckPidSupportIsMax(self, pid_name, fields, base_name):
    for field_name in ['min_%s' % base_name, 'max_%s' % base_name]:
      if not (self.IsSupported(pid_name) or
              fields[field_name] == self.pid.GetResponseField(
                  RDM_GET, field_name).DisplayValue(0xffff)):
        self.AddWarning(
            '%s not supported, but %s in PRESET_INFO is not 0xffff' %
            (pid_name, field_name))

  def IsSupported(self, pid_name):
    pid = self.LookupPid(pid_name)
    return pid.value in self.Property('supported_parameters')

  def CheckBounds(self, fields, key):
    min_key = 'min_%s' % key
    max_key = 'max_%s' % key
    if fields[min_key] > fields[max_key]:
      self.AddAdvisory('%s (%d) > %s (%d)'
                       % (min_key, fields[min_key], max_key, fields[max_key]))


class GetPresetInfoWithData(TestMixins.GetWithDataMixin,
                            OptionalParameterTestFixture):
  """Get PRESET_INFO with extra data."""
  PID = 'PRESET_INFO'


class SetPresetInfo(TestMixins.UnsupportedSetMixin,
                    OptionalParameterTestFixture):
  """Set PRESET_INFO."""
  PID = 'PRESET_INFO'


class SetPresetInfoWithData(TestMixins.UnsupportedSetWithDataMixin,
                            OptionalParameterTestFixture):
  """Attempt to SET PRESET_INFO with data."""
  PID = 'PRESET_INFO'


class AllSubDevicesGetPresetInfo(TestMixins.AllSubDevicesGetMixin,
                                 OptionalParameterTestFixture):
  """Get PRESET_INFO addressed 0to ALL_SUB_DEVICES."""
  PID = 'PRESET_INFO'


# PRESET_STATUS
# -----------------------------------------------------------------------------
class GetPresetStatusPresetOff(OptionalParameterTestFixture):
  """Get the PRESET_STATUS for PRESET_PLAYBACK_OFF."""
  # AKA GetZeroPresetStatus
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PRESET_STATUS'

  def Test(self):
    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    data = struct.pack('!H', 0)
    self.SendRawGet(ROOT_DEVICE, self.pid, data)


class GetPresetStatusPresetScene(OptionalParameterTestFixture):
  """Get the PRESET_STATUS for PRESET_PLAYBACK_SCENE."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PRESET_STATUS'

  def Test(self):
    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    data = struct.pack('!H', 0xffff)
    self.SendRawGet(ROOT_DEVICE, self.pid, data)


class GetOutOfRangePresetStatus(OptionalParameterTestFixture):
  """Get the PRESET_STATUS for max_scene + 1."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PRESET_STATUS'
  REQUIRES = ['max_scene_number']

  def Test(self):
    max_scene = self.Property('max_scene_number')
    if max_scene is None:
      # Set a default value, PID likely isn't supported anyway
      max_scene = 0x0000
    elif max_scene == 0xfffe:
      self.SetNotRun('Device supports all scenes')
      return

    self.AddIfGetSupported(self.NackGetResult(RDMNack.NR_DATA_OUT_OF_RANGE))
    self.SendGet(ROOT_DEVICE, self.pid, [max_scene + 1])


class GetPresetStatus(OptionalParameterTestFixture):
  """Get the PRESET_STATUS for all scenes."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_STATUS'
  REQUIRES = ['max_scene_number', 'preset_info']
  PROVIDES = ['scene_writable_states']
  NOT_PROGRAMMED = 0
  PROGRAMMED = 1
  READ_ONLY = 2

  def Test(self):
    self.scene_writable_states = {}
    self.index = 0
    self.max_scene = self.Property('max_scene_number')
    preset_info = self.Property('preset_info')
    self.min_fade = preset_info.get('min_preset_fade_time', 0)
    self.max_fade = preset_info.get(
        'max_preset_fade_time',
        self.pid.GetResponseField(RDM_GET, 'up_fade_time').RawValue(0xffff))
    self.min_wait = preset_info.get('min_preset_wait_time', 0)
    self.max_wait = preset_info.get(
        'max_preset_wait_time',
        self.pid.GetResponseField(RDM_GET, 'wait_time').RawValue(0xffff))

    if self.max_scene is None or self.max_scene == 0:
      self.SetNotRun('No scenes supported')
      return

    self.FetchNextScene()

  def FetchNextScene(self):
    self.index += 1
    if self.index > self.max_scene:
      self.SetProperty('scene_writable_states', self.scene_writable_states)
      self.Stop()
      return

    self.AddIfGetSupported(self.AckGetResult(action=self.FetchNextScene))
    self.SendGet(ROOT_DEVICE, self.pid, [self.index])

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      return

    if fields['scene_number'] != self.index:
      self.SetFailed('Scene number mismatch, expected %d, got %d' %
                     (self.index, fields['scene_number']))

    if fields['programmed'] == self.NOT_PROGRAMMED:
      # Assume that NOT_PROGRAMMED means that it's writable.
      self.scene_writable_states[self.index] = True
      self.CheckFieldIsZero(fields, 'down_fade_time')
      self.CheckFieldIsZero(fields, 'up_fade_time')
      self.CheckFieldIsZero(fields, 'wait_time')
      return
    elif fields['programmed'] == self.READ_ONLY:
      self.scene_writable_states[self.index] = False
    else:
      self.scene_writable_states[self.index] = True

    for key in ['up_fade_time', 'down_fade_time']:
      self.CheckFieldIsBetween(fields, key, self.min_fade, self.max_fade)

    self.CheckFieldIsBetween(fields, 'wait_time', self.min_wait, self.max_wait)

  def CheckFieldIsZero(self, fields, key):
    if fields[key] != 0:
      self.AddWarning(
          '%s for scene %d was not zero, value is %d' %
          (key, self.index, fields[key]))

  def CheckFieldIsBetween(self, fields, key, min_value, max_value):
    if fields[key] < min_value:
      self.AddWarning(
          '%s for scene %d (%d s) is less than the min of %s' %
          (key, self.index, fields[key], min_value))
    if fields[key] > max_value:
      self.AddWarning(
          '%s for scene %d (%d s) is more than the min of %s' %
          (key, self.index, fields[key], max_value))


class GetPresetStatusWithNoData(TestMixins.GetWithNoDataMixin,
                                OptionalParameterTestFixture):
  """Get the PRESET_STATUS with no preset number specified."""
  PID = 'PRESET_STATUS'


class GetPresetStatusWithExtraData(TestMixins.GetWithDataMixin,
                                   OptionalParameterTestFixture):
  """GET PRESET_STATUS with more than 2 bytes of data."""
  PID = 'PRESET_STATUS'


class SetPresetStatusWithNoData(TestMixins.SetWithNoDataMixin,
                                OptionalParameterTestFixture):
  """Set PRESET_STATUS without any data."""
  PID = 'PRESET_STATUS'


class SetPresetStatusWithExtraData(TestMixins.SetWithDataMixin,
                                   OptionalParameterTestFixture):
  """Send a SET PRESET_STATUS command with extra data."""
  PID = 'PRESET_STATUS'


class SetPresetStatusPresetOff(TestMixins.SetOutOfRangePresetStatusMixin,
                               OptionalParameterTestFixture):
  """Set the PRESET_STATUS for PRESET_PLAYBACK_OFF."""
  # AKA SetZeroPresetStatus
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def PresetStatusSceneNumber(self):
    return 0


class SetPresetStatusPresetScene(TestMixins.SetOutOfRangePresetStatusMixin,
                                 OptionalParameterTestFixture):
  """Set the PRESET_STATUS for PRESET_PLAYBACK_SCENE."""
  CATEGORY = TestCategory.ERROR_CONDITIONS

  def PresetStatusSceneNumber(self):
    return 0xffff


class SetOutOfRangePresetStatus(TestMixins.SetOutOfRangePresetStatusMixin,
                                OptionalParameterTestFixture):
  """Set the PRESET_STATUS for max_scene + 1."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  REQUIRES = (['max_scene_number'] +
              TestMixins.SetOutOfRangePresetStatusMixin.REQUIRES)

  def PresetStatusSceneNumber(self):
    max_scene = self.Property('max_scene_number')
    if max_scene is None:
      # Set a default value, PID likely isn't supported anyway
      max_scene = 0x0000
    elif max_scene == 0xfffe:
      self.SetNotRun('Device supports all scenes')
      return None

    return (max_scene + 1)


class ClearReadOnlyPresetStatus(OptionalParameterTestFixture):
  """Attempt to clear a read only preset."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'PRESET_STATUS'
  REQUIRES = ['scene_writable_states', 'preset_info']

  def Test(self):
    self.scene = None
    scene_writable_states = self.Property('scene_writable_states')
    if scene_writable_states is not None:
      for scene_number, is_writeable in scene_writable_states.items():
        if not is_writeable:
          self.scene = scene_number
          break

    if self.scene is None:
      self.SetNotRun('No read-only scenes found')
      return

    preset_info = self.Property('preset_info')
    fade_time = 0
    wait_time = 0
    if preset_info:
      fade_time = preset_info['min_preset_fade_time']
      wait_time = preset_info['min_preset_wait_time']

    # Don't use AddIfSetSupported here, because we don't want to log an
    # advisory for NR_WRITE_PROTECT
    if self.PidSupported():
      results = self.NackSetResult(RDMNack.NR_WRITE_PROTECT)
    else:
      results = self.NackSetResult(RDMNack.NR_UNKNOWN_PID)
    self.AddExpectedResults(results)

    self.SendSet(ROOT_DEVICE, self.pid,
                 [self.scene, fade_time, fade_time, wait_time, True])


class SetPresetStatus(OptionalParameterTestFixture):
  """Set the PRESET_STATUS."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_STATUS'
  REQUIRES = ['scene_writable_states', 'preset_info']

  def Test(self):
    self.scene = None
    scene_writable_states = self.Property('scene_writable_states')
    if scene_writable_states is not None:
      for scene_number, is_writeable in scene_writable_states.items():
        if is_writeable:
          self.scene = scene_number
          break

    if self.scene is None:
      self.SetNotRun('No writeable scenes found')
      return

    self.max_fade = round(self.pid.GetRequestField(
        RDM_SET, 'up_fade_time').RawValue(0xffff), 1)
    self.max_wait = round(self.pid.GetRequestField(
        RDM_SET, 'wait_time').RawValue(0xffff), 1)
    preset_info = self.Property('preset_info')
    if preset_info is not None:
      self.max_fade = round(preset_info['max_preset_fade_time'], 1)
      self.max_wait = round(preset_info['max_preset_wait_time'], 1)

    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(ROOT_DEVICE, self.pid,
                 [self.scene, self.max_fade, self.max_fade, self.max_wait,
                  False])

  def VerifySet(self):
    self.AddExpectedResults(self.AckGetResult(field_values={
      'up_fade_time': self.max_fade,
      'wait_time': self.max_wait,
      'scene_number': self.scene,
      'down_fade_time': self.max_fade,
    }))
    self.SendGet(ROOT_DEVICE, self.pid, [self.scene])


class ClearPresetStatus(OptionalParameterTestFixture):
  """Set the PRESET_STATUS with clear preset = 1"""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_STATUS'
  REQUIRES = ['scene_writable_states', 'preset_info']

  def Test(self):
    self.scene = None
    scene_writable_states = self.Property('scene_writable_states')
    if scene_writable_states is not None:
      for scene_number, is_writeable in scene_writable_states.items():
        if is_writeable:
          self.scene = scene_number
          break

    if self.scene is None:
      self.SetNotRun('No writeable scenes found')
      return

    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    # We use made up values here to check that the device doesn't use them
    self.SendSet(ROOT_DEVICE, self.pid, [self.scene, 10, 10, 20, True])

  def VerifySet(self):
    self.AddExpectedResults(self.AckGetResult(field_values={
      'up_fade_time': 0.0,
      'wait_time': 0.0,
      'scene_number': self.scene,
      'programmed': 0,
      'down_fade_time': 0.0,
    }))
    self.SendGet(ROOT_DEVICE, self.pid, [self.scene])


class AllSubDevicesGetPresetStatus(TestMixins.AllSubDevicesGetMixin,
                                   OptionalParameterTestFixture):
  """Get PRESET_STATUS addressed to ALL_SUB_DEVICES."""
  PID = 'PRESET_STATUS'
  DATA = [1]


# PRESET_MERGEMODE
# -----------------------------------------------------------------------------
class GetPresetMergeMode(TestMixins.GetMixin,
                         OptionalParameterTestFixture):
  """Get PRESET_MERGEMODE."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_MERGEMODE'
  PROVIDES = ['preset_mergemode']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      self.SetProperty('preset_mergemode', None)
      return

    self.SetProperty('preset_mergemode', fields['merge_mode'])


class GetPresetMergeModeWithData(TestMixins.GetWithDataMixin,
                                 OptionalParameterTestFixture):
  """Get PRESET_MERGEMODE with extra data."""
  PID = 'PRESET_MERGEMODE'


class SetPresetMergeMode(OptionalParameterTestFixture):
  """Set PRESET_MERGEMODE."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_MERGEMODE'
  REQUIRES = ['preset_mergemode']
  PROVIDES = ['set_preset_mergemode_supported']

  def Test(self):
    self.value = self.Property('preset_mergemode')
    if self.value is None:
      self.value = 0

    self.in_set = True
    self.AddIfSetSupported([
      self.AckSetResult(action=self.VerifySet),
      self.NackSetResult(
        RDMNack.NR_UNSUPPORTED_COMMAND_CLASS,
        advisory='SET for %s returned unsupported command class' %
                 self.pid.name),
    ])
    self.SendSet(ROOT_DEVICE, self.pid, [self.value])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(field_values={'merge_mode': self.value}))
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if self.in_set:
      self.SetProperty(self.PROVIDES[0], response.WasAcked())
      self.in_set = False


class SetAllPresetMergeModes(OptionalParameterTestFixture):
  """Set PRESET_MERGEMODE to each of the defined values."""
  CATEGORY = TestCategory.CONTROL
  PID = 'PRESET_MERGEMODE'
  REQUIRES = ['preset_mergemode', 'set_preset_mergemode_supported']
  MODES = [0, 1, 2, 3, 0xff]

  def Test(self):
    if not self.Property('set_preset_mergemode_supported'):
      self.SetNotRun('SET PRESET_MERGEMODE not supported')
      return

    self.old_value = self.Property('preset_mergemode')
    self.merge_modes = [m for m in self.MODES if m != self.old_value]
    # PerformSet pop's the last value, so we add a dummy value to the end of
    # the list.
    self.merge_modes.append(self.old_value)
    self.PerformSet()

  def PerformSet(self):
    self.merge_modes.pop()
    if not self.merge_modes:
      self.Stop()
      return

    self.AddIfSetSupported([
      self.AckSetResult(action=self.VerifySet),
      self.NackSetResult(RDMNack.NR_DATA_OUT_OF_RANGE, action=self.PerformSet),
    ])
    self.SendSet(ROOT_DEVICE, self.pid, [self.merge_modes[-1]])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(field_values={'merge_mode': self.merge_modes[-1]},
                        action=self.PerformSet))
    self.SendGet(ROOT_DEVICE, self.pid)

  def ResetState(self):
    self.AddExpectedResults(self.AckSetResult())
    self.SendSet(ROOT_DEVICE, self.pid, [self.old_value])
    self._wrapper.Run()


class SetPresetMergeModeWithNoData(TestMixins.SetWithNoDataMixin,
                                   OptionalParameterTestFixture):
  """Set PRESET_MERGEMODE without any data."""
  PID = 'PRESET_MERGEMODE'


class SetPresetMergemodeWithExtraData(TestMixins.SetWithDataMixin,
                                      OptionalParameterTestFixture):
  """Send a SET PRESET_MERGEMODE command with extra data."""
  PID = 'PRESET_MERGEMODE'


class AllSubDevicesGetPresetMergeMode(TestMixins.AllSubDevicesGetMixin,
                                      OptionalParameterTestFixture):
  """Get PRESET_MERGEMODE addressed to ALL_SUB_DEVICES."""
  PID = 'PRESET_MERGEMODE'


# LIST_INTERFACES
# -----------------------------------------------------------------------------
class GetListInterfaces(TestMixins.GetMixin,
                        OptionalParameterTestFixture):
  """Get LIST_INTERFACES."""
  CATEGORY = TestCategory.IP_DNS_CONFIGURATION
  PID = 'LIST_INTERFACES'
  PROVIDES = ['interface_list']

  def Test(self):
    self.AddIfGetSupported(self.AckGetResult())
    self.SendGet(ROOT_DEVICE, self.pid)

  def VerifyResult(self, response, fields):
    if not response.WasAcked():
      self.SetProperty(self.PROVIDES[0], [])
      return

    interfaces = []

    for interface in fields['interfaces']:
      interface_id = interface['interface_identifier']
      if (interface_id < RDM_INTERFACE_INDEX_MIN or
          interface_id > RDM_INTERFACE_INDEX_MAX):
        self.AddWarning('Interface index %d is outside allowed range (%d to '
                        '%d)' % (interface_id,
                                 RDM_INTERFACE_INDEX_MIN,
                                 RDM_INTERFACE_INDEX_MAX))
      else:
        interfaces.append(interface_id)
      if (interface['interface_hardware_type'] !=
          INTERFACE_HARDWARE_TYPE_ETHERNET):
        self.AddAdvisory('Possible error, found unusual hardware type %d for '
                         'interface %d' %
                         (interface['interface_hardware_type'], interface_id))

    self.SetProperty(self.PROVIDES[0], interfaces)


class GetListInterfacesWithData(TestMixins.GetWithDataMixin,
                                OptionalParameterTestFixture):
  """Get LIST_INTERFACES with extra data."""
  PID = 'LIST_INTERFACES'


class SetListInterfaces(TestMixins.UnsupportedSetMixin,
                        OptionalParameterTestFixture):
  """Attempt to SET list interfaces."""
  PID = 'LIST_INTERFACES'


class SetListInterfacesWithData(TestMixins.UnsupportedSetWithDataMixin,
                                OptionalParameterTestFixture):
  """Attempt to SET LIST_INTERFACES with data."""
  PID = 'LIST_INTERFACES'


class AllSubDevicesGetListInterfaces(TestMixins.AllSubDevicesGetMixin,
                                     OptionalParameterTestFixture):
  """Send a get LIST_INTERFACES to ALL_SUB_DEVICES."""
  PID = 'LIST_INTERFACES'


# DNS_HOSTNAME
# -----------------------------------------------------------------------------
class GetDNSHostname(TestMixins.GetStringMixin,
                     OptionalParameterTestFixture):
  """GET the DNS hostname."""
  CATEGORY = TestCategory.IP_DNS_CONFIGURATION
  PID = 'DNS_HOSTNAME'
  EXPECTED_FIELDS = ['dns_hostname']
  ALLOWED_NACKS = [RDMNack.NR_HARDWARE_FAULT]
  MIN_LENGTH = RDM_MIN_HOSTNAME_LENGTH
  MAX_LENGTH = RDM_MAX_HOSTNAME_LENGTH


class GetDNSHostnameWithData(TestMixins.GetWithDataMixin,
                             OptionalParameterTestFixture):
  """Get DNS hostname with param data."""
  PID = 'DNS_HOSTNAME'


# TODO(Peter): Need to test set
# class SetDNSHostname(TestMixins.,
#                      OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'DNS_HOSTNAME'


class SetDNSHostnameWithNoData(TestMixins.SetWithNoDataMixin,
                               OptionalParameterTestFixture):
  """Set DNS_HOSTNAME command with no data."""
  PID = 'DNS_HOSTNAME'


class SetDNSHostnameWithExtraData(TestMixins.SetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Send a SET DNS_HOSTNAME command with extra data."""
  PID = 'DNS_HOSTNAME'


class AllSubDevicesGetDNSHostname(TestMixins.AllSubDevicesGetMixin,
                                  OptionalParameterTestFixture):
  """Send a Get DNS_HOSTNAME to ALL_SUB_DEVICES."""
  PID = 'DNS_HOSTNAME'


# DNS_DOMAIN_NAME
# -----------------------------------------------------------------------------
class GetDNSDomainName(TestMixins.GetStringMixin,
                       OptionalParameterTestFixture):
  """GET the DNS domain name."""
  CATEGORY = TestCategory.IP_DNS_CONFIGURATION
  PID = 'DNS_DOMAIN_NAME'
  EXPECTED_FIELDS = ['dns_domain_name']
  ALLOWED_NACKS = [RDMNack.NR_HARDWARE_FAULT]
  MAX_LENGTH = RDM_MAX_DOMAIN_NAME_LENGTH


class GetDNSDomainNameWithData(TestMixins.GetWithDataMixin,
                               OptionalParameterTestFixture):
  """Get DNS domain name with param data."""
  PID = 'DNS_DOMAIN_NAME'


# TODO(Peter): Need to test set
# class SetDNSDomainName(TestMixins.,
#                        OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'DNS_DOMAIN_NAME'


class SetDNSDomainNameWithNoData(TestMixins.SetWithNoDataMixin,
                                 OptionalParameterTestFixture):
  """Set DNS_DOMAIN_NAME command with no data."""
  PID = 'DNS_DOMAIN_NAME'


class SetDNSDomainNameWithExtraData(TestMixins.SetWithDataMixin,
                                    OptionalParameterTestFixture):
  """Send a SET DNS_DOMAIN_NAME command with extra data."""
  PID = 'DNS_DOMAIN_NAME'


class AllSubDevicesGetDNSDomainName(TestMixins.AllSubDevicesGetMixin,
                                    OptionalParameterTestFixture):
  """Send a Get DNS_DOMAIN_NAME to ALL_SUB_DEVICES."""
  PID = 'DNS_DOMAIN_NAME'


# DNS_IPV4_NAME_SERVER
# -----------------------------------------------------------------------------
class AllSubDevicesGetDNSIPv4NameServer(TestMixins.AllSubDevicesGetMixin,
                                        OptionalParameterTestFixture):
  """Send a get DNS_IPV4_NAME_SERVER to ALL_SUB_DEVICES."""
  PID = 'DNS_IPV4_NAME_SERVER'
  DATA = [0]


# class GetDNSIPv4NameServer(TestMixins.,
#                            OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'DNS_IPV4_NAME_SERVER'
# TODO(Peter): Test get


class GetDNSIPv4NameServerWithNoData(TestMixins.GetWithNoDataMixin,
                                     OptionalParameterTestFixture):
  """GET DNS_IPV4_NAME_SERVER with no argument given."""
  PID = 'DNS_IPV4_NAME_SERVER'


class GetDNSIPv4NameServerWithExtraData(TestMixins.GetWithDataMixin,
                                        OptionalParameterTestFixture):
  """GET DNS_IPV4_NAME_SERVER with more than 1 byte of data."""
  PID = 'DNS_IPV4_NAME_SERVER'


# class SetDNSIPv4NameServer(TestMixins.,
#                            OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'DNS_IPV4_NAME_SERVER'
# TODO(Peter): Test set


class SetDNSIPv4NameServerWithNoData(TestMixins.SetWithNoDataMixin,
                                     OptionalParameterTestFixture):
  """Set DNS_IPV4_NAME_SERVER command with no data."""
  PID = 'DNS_IPV4_NAME_SERVER'


class SetDNSIPv4NameServerWithExtraData(TestMixins.SetWithDataMixin,
                                        OptionalParameterTestFixture):
  """Send a SET DNS_IPV4_NAME_SERVER command with extra data."""
  PID = 'DNS_IPV4_NAME_SERVER'
  DATA = 'foobar'


# IPV4_DEFAULT_ROUTE
# -----------------------------------------------------------------------------
class GetIPv4DefaultRoute(TestMixins.GetMixin,
                          OptionalParameterTestFixture):
  """GET the IPv4 default route."""
  # TODO(Peter): Check interface identifier is a valid interface
  CATEGORY = TestCategory.IP_DNS_CONFIGURATION
  PID = 'IPV4_DEFAULT_ROUTE'
  EXPECTED_FIELDS = ['ipv4_address', 'interface_identifier']


class GetIPv4DefaultRouteWithData(TestMixins.GetWithDataMixin,
                                  OptionalParameterTestFixture):
  """Get IPv4 default route with param data."""
  PID = 'IPV4_DEFAULT_ROUTE'


# TODO(Peter): Need to restrict these somehow so we don't saw off the branch
# class SetIPv4DefaultRoute(TestMixins.,
#                           OptionalParameterTestFixture):
#   CATEGORY = TestCategory.
#   PID = 'IPV4_DEFAULT_ROUTE'


class SetIPv4DefaultRouteWithNoData(TestMixins.SetWithNoDataMixin,
                                    OptionalParameterTestFixture):
  """Set IPV4_DEFAULT_ROUTE command with no data."""
  PID = 'IPV4_DEFAULT_ROUTE'


class SetIPv4DefaultRouteWithExtraData(TestMixins.SetWithDataMixin,
                                       OptionalParameterTestFixture):
  """Send a SET IPV4_DEFAULT_ROUTE command with extra data."""
  PID = 'IPV4_DEFAULT_ROUTE'
  DATA = 'foobarbaz'


class AllSubDevicesGetIPv4DefaultRoute(TestMixins.AllSubDevicesGetMixin,
                                       OptionalParameterTestFixture):
  """Send a Get IPV4_DEFAULT_ROUTE to ALL_SUB_DEVICES."""
  PID = 'IPV4_DEFAULT_ROUTE'


# IPV4_DHCP_MODE
# -----------------------------------------------------------------------------
class AllSubDevicesGetIPv4DHCPMode(TestMixins.AllSubDevicesGetMixin,
                                   OptionalParameterTestFixture):
  """Send a get IPV4_DHCP_MODE to ALL_SUB_DEVICES."""
  PID = 'IPV4_DHCP_MODE'
  DATA = [0x00000001]


# class GetIPv4DHCPMode(TestMixins.,
#                       OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'IPV4_DHCP_MODE'
# TODO(Peter): Test get


class GetZeroIPv4DHCPMode(TestMixins.GetZeroUInt32Mixin,
                          OptionalParameterTestFixture):
  """GET IPV4_DHCP_MODE for interface identifier 0."""
  PID = 'IPV4_DHCP_MODE'


class GetIPv4DHCPModeWithNoData(TestMixins.GetWithNoDataMixin,
                                OptionalParameterTestFixture):
  """GET IPV4_DHCP_MODE with no argument given."""
  PID = 'IPV4_DHCP_MODE'


class GetIPv4DHCPModeWithExtraData(TestMixins.GetWithDataMixin,
                                   OptionalParameterTestFixture):
  """GET IPV4_DHCP_MODE with more than 4 bytes of data."""
  PID = 'IPV4_DHCP_MODE'
  DATA = 'foobar'


# class SetIPv4DHCPMode(TestMixins.,
#                       OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'IPV4_DHCP_MODE'
# TODO(Peter): Test set


class SetZeroIPv4DHCPMode(TestMixins.SetZeroMixin,
                          OptionalParameterTestFixture):
  """SET IPV4_DHCP_MODE to interface identifier 0."""
  PID = 'IPV4_DHCP_MODE'
  DATA = struct.pack('!IB', 0x00000000, 0x00)


class SetIPv4DHCPModeWithNoData(TestMixins.SetWithNoDataMixin,
                                OptionalParameterTestFixture):
  """Set IPV4_DHCP_MODE command with no data."""
  PID = 'IPV4_DHCP_MODE'


class SetIPv4DHCPModeWithExtraData(TestMixins.SetWithDataMixin,
                                   OptionalParameterTestFixture):
  """Send a SET IPV4_DHCP_MODE command with extra data."""
  PID = 'IPV4_DHCP_MODE'
  DATA = 'foobar'


# IPV4_ZEROCONF_MODE
# -----------------------------------------------------------------------------
class AllSubDevicesGetIPv4ZeroconfMode(TestMixins.AllSubDevicesGetMixin,
                                       OptionalParameterTestFixture):
  """Send a get IPV4_ZEROCONF_MODE to ALL_SUB_DEVICES."""
  PID = 'IPV4_ZEROCONF_MODE'
  DATA = [0x00000001]


# class GetIPv4ZeroconfMode(TestMixins.,
#                           OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'IPV4_ZEROCONF_MODE'
# TODO(Peter): Test get


class GetZeroIPv4ZeroconfMode(TestMixins.GetZeroUInt32Mixin,
                              OptionalParameterTestFixture):
  """GET IPV4_ZEROCONF_MODE for interface identifier 0."""
  PID = 'IPV4_ZEROCONF_MODE'


class GetIPv4ZeroconfModeWithNoData(TestMixins.GetWithNoDataMixin,
                                    OptionalParameterTestFixture):
  """GET IPV4_ZEROCONF_MODE with no argument given."""
  PID = 'IPV4_ZEROCONF_MODE'


class GetIPv4ZeroconfModeWithExtraData(TestMixins.GetWithDataMixin,
                                       OptionalParameterTestFixture):
  """GET IPV4_ZEROCONF_MODE with more than 4 bytes of data."""
  PID = 'IPV4_ZEROCONF_MODE'
  DATA = 'foobar'


# class SetIPv4ZeroconfMode(TestMixins.,
#                           OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'IPV4_ZEROCONF_MODE'
# TODO(Peter): Test set


class SetZeroIPv4ZeroconfMode(TestMixins.SetZeroMixin,
                              OptionalParameterTestFixture):
  """SET IPV4_ZEROCONF_MODE to interface identifier 0."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'IPV4_ZEROCONF_MODE'
  DATA = struct.pack('!IB', 0x00000000, 0x00)


class SetIPv4ZeroconfModeWithNoData(TestMixins.SetWithNoDataMixin,
                                    OptionalParameterTestFixture):
  """Set IPV4_ZEROCONF_MODE command with no data."""
  PID = 'IPV4_ZEROCONF_MODE'


class SetIPv4ZeroconfModeWithExtraData(TestMixins.SetWithDataMixin,
                                       OptionalParameterTestFixture):
  """Send a SET IPV4_ZEROCONF_MODE command with extra data."""
  PID = 'IPV4_ZEROCONF_MODE'
  DATA = 'foobar'


# IPV4_CURRENT_ADDRESS
# -----------------------------------------------------------------------------
class AllSubDevicesGetIPv4CurrentAddress(TestMixins.AllSubDevicesGetMixin,
                                         OptionalParameterTestFixture):
  """Send a get IPV4_CURRENT_ADDRESS to ALL_SUB_DEVICES."""
  PID = 'IPV4_CURRENT_ADDRESS'
  DATA = [0x00000001]


# class GetIPv4CurrentAddress(TestMixins.,
#                             OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'IPV4_CURRENT_ADDRESS'
# TODO(Peter): Test get


class GetZeroIPv4CurrentAddress(TestMixins.GetZeroUInt32Mixin,
                                OptionalParameterTestFixture):
  """GET IPV4_CURRENT_ADDRESS for interface identifier 0."""
  PID = 'IPV4_CURRENT_ADDRESS'


class GetIPv4CurrentAddressWithNoData(TestMixins.GetWithNoDataMixin,
                                      OptionalParameterTestFixture):
  """GET IPV4_CURRENT_ADDRESS with no argument given."""
  PID = 'IPV4_CURRENT_ADDRESS'


class GetIPv4CurrentAddressWithExtraData(TestMixins.GetWithDataMixin,
                                         OptionalParameterTestFixture):
  """GET IPV4_CURRENT_ADDRESS with more than 4 bytes of data."""
  PID = 'IPV4_CURRENT_ADDRESS'
  DATA = 'foobar'


class SetIPv4CurrentAddress(TestMixins.UnsupportedSetMixin,
                            OptionalParameterTestFixture):
  """Attempt to SET IPV4_CURRENT_ADDRESS."""
  PID = 'IPV4_CURRENT_ADDRESS'


class SetIPv4CurrentAddressWithData(TestMixins.UnsupportedSetWithDataMixin,
                                    OptionalParameterTestFixture):
  """Attempt to SET IPV4_CURRENT_ADDRESS with data."""
  PID = 'IPV4_CURRENT_ADDRESS'


# IPV4_STATIC_ADDRESS
# -----------------------------------------------------------------------------
class AllSubDevicesGetIPv4StaticAddress(TestMixins.AllSubDevicesGetMixin,
                                        OptionalParameterTestFixture):
  """Send a get IPV4_STATIC_ADDRESS to ALL_SUB_DEVICES."""
  PID = 'IPV4_STATIC_ADDRESS'
  DATA = [0x00000001]


# class GetIPv4StaticAddress(TestMixins.,
#                            OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'IPV4_STATIC_ADDRESS'
# TODO(Peter): Test get


class GetZeroIPv4StaticAddress(TestMixins.GetZeroUInt32Mixin,
                               OptionalParameterTestFixture):
  """GET IPV4_STATIC_ADDRESS for interface identifier 0."""
  PID = 'IPV4_STATIC_ADDRESS'


class GetIPv4StaticAddressWithNoData(TestMixins.GetWithNoDataMixin,
                                     OptionalParameterTestFixture):
  """GET IPV4_STATIC_ADDRESS with no argument given."""
  PID = 'IPV4_STATIC_ADDRESS'


class GetIPv4StaticAddressWithExtraData(TestMixins.GetWithDataMixin,
                                        OptionalParameterTestFixture):
  """GET IPV4_STATIC_ADDRESS with more than 4 bytes of data."""
  PID = 'IPV4_STATIC_ADDRESS'
  DATA = 'foobar'


# class SetIPv4StaticAddress(TestMixins.,
#                            OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'IPV4_STATIC_ADDRESS'
# TODO(Peter): Test set


class SetZeroIPv4StaticAddress(TestMixins.SetZeroMixin,
                               OptionalParameterTestFixture):
  """SET IPV4_STATIC_ADDRESS to interface identifier 0."""
  CATEGORY = TestCategory.ERROR_CONDITIONS
  PID = 'IPV4_STATIC_ADDRESS'
  # 192.168.0.1
  DATA = struct.pack('!IIB', 0x00000000, 0xc0a80001, 24)


class SetIPv4StaticAddressWithNoData(TestMixins.SetWithNoDataMixin,
                                     OptionalParameterTestFixture):
  """Set IPV4_STATIC_ADDRESS command with no data."""
  PID = 'IPV4_STATIC_ADDRESS'


class SetIPv4StaticAddressWithExtraData(TestMixins.SetWithDataMixin,
                                        OptionalParameterTestFixture):
  """Send a SET IPV4_STATIC_ADDRESS command with extra data."""
  PID = 'IPV4_STATIC_ADDRESS'
  DATA = 'foobarbazqux'


# INTERFACE_RENEW_DHCP
# -----------------------------------------------------------------------------
class AllSubDevicesGetInterfaceRenewDHCP(
        TestMixins.AllSubDevicesUnsupportedGetMixin,
        OptionalParameterTestFixture):
  """Attempt to send a get INTERFACE_RENEW_DHCP to ALL_SUB_DEVICES."""
  PID = 'INTERFACE_RENEW_DHCP'


class GetInterfaceRenewDHCP(TestMixins.UnsupportedGetMixin,
                            OptionalParameterTestFixture):
  """Attempt to GET INTERFACE_RENEW_DHCP."""
  PID = 'INTERFACE_RENEW_DHCP'


class GetInterfaceRenewDHCPWithData(TestMixins.UnsupportedGetWithDataMixin,
                                    OptionalParameterTestFixture):
  """GET INTERFACE_RENEW_DHCP with data."""
  PID = 'INTERFACE_RENEW_DHCP'


# class SetInterfaceRenewDHCP(TestMixins.,
#                             OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'INTERFACE_RENEW_DHCP'
# TODO(Peter): Test set


class SetZeroInterfaceRenewDHCP(TestMixins.SetZeroUInt32Mixin,
                                OptionalParameterTestFixture):
  """SET INTERFACE_RENEW_DHCP to interface identifier 0."""
  PID = 'INTERFACE_RENEW_DHCP'


class SetInterfaceRenewDHCPWithNoData(TestMixins.SetWithNoDataMixin,
                                      OptionalParameterTestFixture):
  """Set INTERFACE_RENEW_DHCP command with no data."""
  PID = 'INTERFACE_RENEW_DHCP'


class SetInterfaceRenewDHCPWithExtraData(TestMixins.SetWithDataMixin,
                                         OptionalParameterTestFixture):
  """Send a SET INTERFACE_RENEW_DHCP command with extra data."""
  PID = 'INTERFACE_RENEW_DHCP'
  DATA = 'foobar'


# INTERFACE_RELEASE_DHCP
# -----------------------------------------------------------------------------
class AllSubDevicesGetInterfaceReleaseDHCP(
        TestMixins.AllSubDevicesUnsupportedGetMixin,
        OptionalParameterTestFixture):
  """Attempt to send a get INTERFACE_RELEASE_DHCP to ALL_SUB_DEVICES."""
  PID = 'INTERFACE_RELEASE_DHCP'


class GetInterfaceReleaseDHCP(TestMixins.UnsupportedGetMixin,
                              OptionalParameterTestFixture):
  """Attempt to GET INTERFACE_RELEASE_DHCP."""
  PID = 'INTERFACE_RELEASE_DHCP'


class GetInterfaceReleaseDHCPWithData(TestMixins.UnsupportedGetWithDataMixin,
                                      OptionalParameterTestFixture):
  """GET INTERFACE_RELEASE_DHCP with data."""
  PID = 'INTERFACE_RELEASE_DHCP'


# class SetInterfaceReleaseDHCP(TestMixins.,
#                               OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'INTERFACE_RELEASE_DHCP'
# TODO(Peter): Test set


class SetZeroInterfaceReleaseDHCP(TestMixins.SetZeroUInt32Mixin,
                                  OptionalParameterTestFixture):
  """SET INTERFACE_RELEASE_DHCP to interface identifier 0."""
  PID = 'INTERFACE_RELEASE_DHCP'


class SetInterfaceReleaseDHCPWithNoData(TestMixins.SetWithNoDataMixin,
                                        OptionalParameterTestFixture):
  """Set INTERFACE_RELEASE_DHCP command with no data."""
  PID = 'INTERFACE_RELEASE_DHCP'


class SetInterfaceReleaseDHCPWithExtraData(TestMixins.SetWithDataMixin,
                                           OptionalParameterTestFixture):
  """Send a SET INTERFACE_RELEASE_DHCP command with extra data."""
  PID = 'INTERFACE_RELEASE_DHCP'
  DATA = 'foobar'


# INTERFACE_APPLY_CONFIGURATION
# -----------------------------------------------------------------------------
class AllSubDevicesGetInterfaceApplyConfiguration(
        TestMixins.AllSubDevicesUnsupportedGetMixin,
        OptionalParameterTestFixture):
  """Attempt to send a get INTERFACE_APPLY_CONFIGURATION to ALL_SUB_DEVICES."""
  PID = 'INTERFACE_APPLY_CONFIGURATION'


class GetInterfaceApplyConfiguration(TestMixins.UnsupportedGetMixin,
                                     OptionalParameterTestFixture):
  """Attempt to GET INTERFACE_APPLY_CONFIGURATION."""
  PID = 'INTERFACE_APPLY_CONFIGURATION'


class GetInterfaceApplyConfigurationWithData(
        TestMixins.UnsupportedGetWithDataMixin,
        OptionalParameterTestFixture):
  """GET INTERFACE_APPLY_CONFIGURATION with data."""
  PID = 'INTERFACE_APPLY_CONFIGURATION'


# class SetInterfaceApplyConfiguration(TestMixins.,
#                                      OptionalParameterTestFixture):
#   CATEGORY = TestCategory.IP_DNS_CONFIGURATION
#   PID = 'INTERFACE_APPLY_CONFIGURATION'
# TODO(Peter): Test set


class SetZeroInterfaceApplyConfiguration(TestMixins.SetZeroUInt32Mixin,
                                         OptionalParameterTestFixture):
  """SET INTERFACE_APPLY_CONFIGURATION to interface identifier 0."""
  PID = 'INTERFACE_APPLY_CONFIGURATION'


class SetInterfaceApplyConfigurationWithNoData(TestMixins.SetWithNoDataMixin,
                                               OptionalParameterTestFixture):
  """Set INTERFACE_APPLY_CONFIGURATION command with no data."""
  PID = 'INTERFACE_APPLY_CONFIGURATION'


class SetInterfaceApplyConfigurationWithExtraData(TestMixins.SetWithDataMixin,
                                                  OptionalParameterTestFixture):
  """Send a SET INTERFACE_APPLY_CONFIGURATION command with extra data."""
  PID = 'INTERFACE_APPLY_CONFIGURATION'
  DATA = 'foobar'


# Interface label
# -----------------------------------------------------------------------------
class GetInterfaceLabels(TestMixins.GetSettingDescriptionsListMixin,
                         OptionalParameterTestFixture):
  """Get the interface labels for all defined interfaces."""
  CATEGORY = TestCategory.IP_DNS_CONFIGURATION
  PID = 'INTERFACE_LABEL'
  REQUIRES = ['interface_list']
  EXPECTED_FIELDS = ['interface_identifier']
  DESCRIPTION_FIELD = 'interface_label'


class GetInterfaceLabelWithNoData(TestMixins.GetWithNoDataMixin,
                                  OptionalParameterTestFixture):
  """Get the interface label with no interface id specified."""
  PID = 'INTERFACE_LABEL'


class GetInterfaceLabelWithExtraData(TestMixins.GetWithDataMixin,
                                     OptionalParameterTestFixture):
  """Get the interface label with more than 4 bytes of data."""
  PID = 'INTERFACE_LABEL'
  DATA = 'foobar'


class GetZeroInterfaceLabel(TestMixins.GetZeroUInt32Mixin,
                            OptionalParameterTestFixture):
  """GET INTERFACE_LABEL for interface 0."""
  PID = 'INTERFACE_LABEL'


class SetInterfaceLabel(TestMixins.UnsupportedSetMixin,
                        OptionalParameterTestFixture):
  """Attempt to SET the interface label with no data."""
  PID = 'INTERFACE_LABEL'


class SetInterfaceLabelWithData(TestMixins.UnsupportedSetWithDataMixin,
                                OptionalParameterTestFixture):
  """SET the interface label with data."""
  PID = 'INTERFACE_LABEL'


class AllSubDevicesGetInterfaceLabel(TestMixins.AllSubDevicesGetMixin,
                                     OptionalParameterTestFixture):
  """Send a get INTERFACE_LABEL to ALL_SUB_DEVICES."""
  PID = 'INTERFACE_LABEL'
  DATA = [0x00000001]


# Interface hardware address type 1
# -----------------------------------------------------------------------------
class AllSubDevicesGetInterfaceHardwareAddressType1(
        TestMixins.AllSubDevicesGetMixin,
        OptionalParameterTestFixture):
  """Send a get INTERFACE_HARDWARE_ADDRESS_TYPE1 to ALL_SUB_DEVICES."""
  PID = 'INTERFACE_HARDWARE_ADDRESS_TYPE1'
  DATA = [0x00000001]


# class GetInterfaceHardwareAddressType1(TestMixins.,
#                                        OptionalParameterTestFixture):
# TODO(Peter): Test get


class GetInterfaceHardwareAddressType1WithNoData(TestMixins.GetWithNoDataMixin,
                                                 OptionalParameterTestFixture):
  """GET INTERFACE_HARDWARE_ADDRESS_TYPE1 with no argument given."""
  PID = 'INTERFACE_HARDWARE_ADDRESS_TYPE1'


class GetInterfaceHardwareAddressType1WithExtraData(
        TestMixins.GetWithDataMixin,
        OptionalParameterTestFixture):
  """GET INTERFACE_HARDWARE_ADDRESS_TYPE1 with more than 4 bytes of data."""
  PID = 'INTERFACE_HARDWARE_ADDRESS_TYPE1'
  DATA = 'foobar'


class GetZeroInterfaceHardwareAddressType1(TestMixins.GetZeroUInt32Mixin,
                                           OptionalParameterTestFixture):
  """GET INTERFACE_HARDWARE_ADDRESS_TYPE1 for interface 0."""
  PID = 'INTERFACE_HARDWARE_ADDRESS_TYPE1'


class SetInterfaceHardwareAddressType1(TestMixins.UnsupportedSetMixin,
                                       OptionalParameterTestFixture):
  """SET INTERFACE_HARDWARE_ADDRESS_TYPE1."""
  PID = 'INTERFACE_HARDWARE_ADDRESS_TYPE1'


class SetInterfaceHardwareAddressType1WithData(
        TestMixins.UnsupportedSetWithDataMixin,
        OptionalParameterTestFixture):
  """Attempt to SET INTERFACE_HARDWARE_ADDRESS_TYPE1 with data."""
  PID = 'INTERFACE_HARDWARE_ADDRESS_TYPE1'


# Cross check the control fields with various other properties
# -----------------------------------------------------------------------------
class SubDeviceControlField(TestFixture):
  """Check that the sub device control field is correct."""
  CATEGORY = TestCategory.CORE
  REQUIRES = ['mute_control_fields', 'sub_device_count']

  def PidRequired(self):
    return False

  def Test(self):
    sub_device_field = self.Property('mute_control_fields') & 0x02
    if self.Property('sub_device_count') > 0:
      if sub_device_field == 0:
        self.SetFailed('Sub devices reported but control field not set')
        return
    else:
      if sub_device_field:
        self.SetFailed('No Sub devices reported but control field is set')
        return
    self.SetPassed()


class ProxiedDevicesControlField(OptionalParameterTestFixture):
  """Check that the proxied devices control field is correct."""
  PID = 'PROXIED_DEVICES'
  CATEGORY = TestCategory.CORE
  REQUIRES = ['mute_control_fields']

  def Test(self):
    managed_proxy_field = self.Property('mute_control_fields') & 0x01

    if self.PidSupported() and managed_proxy_field == 0:
      self.AddWarning(
          "Support for PROXIED_DEVICES declared but the managed "
          "proxy control field isn't set")
      return
    elif not self.PidSupported() and managed_proxy_field == 1:
      self.SetFailed(
          "Managed proxy control bit is set, but proxied devices isn't "
          "supported")
      return
    self.SetPassed()
