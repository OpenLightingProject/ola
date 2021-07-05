#!/usr/bin/env python
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
# PidStoreTest.py
# Copyright (C) 2020 Bruce Lowekamp

import binascii
import os
try:
  # Python 2.6 needs backport
  import unittest2 as unittest
except ImportError:
  import unittest
import ola.PidStore as PidStore
from ola.TestUtils import allNotEqual, allHashNotEqual

"""Test cases for the PidStore class.
   Relies on the PID data from RDM tests in the directory
   passed as TESTDATADIR envvar.  Otherwise PID data directory
   defaults to ../common/rdm/testdata.
"""

__author__ = 'bruce@lowekamp.net (Bruce Lowekamp)'

global path

OPEN_LIGHTING_ESTA_CODE = 0x7a70


class PidStoreTest(unittest.TestCase):

  def testBasic(self):
    store = PidStore.PidStore()
    store.Load([os.path.join(path, "test_pids.proto")])

    pid = store.GetPid(17)
    self.assertEqual(pid.name, "PROXIED_DEVICE_COUNT")
    self.assertEqual(pid.value, 17)

    self.assertEqual(len(store.Pids()), 70)

    pid = store.GetName("DEVICE_INFO")
    self.assertEqual(pid.value, 96)
    self.assertIsNotNone(pid.GetRequest(PidStore.RDM_GET))
    self.assertIsNotNone(pid.GetResponse(PidStore.RDM_GET))
    self.assertIsNone(pid.GetRequest(PidStore.RDM_SET))
    self.assertIsNone(pid.GetResponse(PidStore.RDM_SET))
    expected = (
      "<protocol_major> <protocol_minor> <device_model> "
      "<product_category> <software_version> <dmx_footprint> "
      "<current_personality> <personality_count> <dmx_start_address> "
      "<sub_device_count> <sensor_count>",
      "  protocol_major: <[0, 255]>\n  protocol_minor: <[0, 255]>\n  "
      "device_model: <[0, 65535]>\n  product_category: <[0, 65535]>\n  "
      "software_version: <[0, 4294967295]>\n  dmx_footprint: <[0, 65535]>\n  "
      "current_personality: <[0, 255]>\n  personality_count: <[0, 255]>\n  "
      "dmx_start_address: <[0, 65535]>\n  sub_device_count: <[0, 65535]>\n  "
      "sensor_count: <[0, 255]>")
    self.assertEqual(pid.GetResponse(PidStore.RDM_GET).GetDescription(),
                     expected)

    pid = store.GetName("CAPTURE_PRESET")
    expected = ("<scene> <fade_up_time> <fade_down_time> <wait_time>",
                "  scene: <[1, 65534]>\n"
                "  fade_up_time: <[0.0, 6553.5]>, increment 0.1\n"
                "  fade_down_time: <[0.0, 6553.5]>, increment 0.1\n"
                "  wait_time: <[0.0, 6553.5]>, increment 0.1")
    self.assertEqual(pid.GetRequest(PidStore.RDM_SET).GetDescription(),
                     expected)
    self.assertEqual(1, pid._GroupCmp(pid._requests, pid._responses))

  def testDirectoryAndSingleton(self):
    store = PidStore.GetStore(os.path.join(path, "pids"))
    self.assertIs(store, PidStore.GetStore(os.path.join(path, "pids")))

    self.assertEqual(len(store.Pids()), 6)
    self.assertEqual(len(store.ManufacturerPids(OPEN_LIGHTING_ESTA_CODE)), 1)

    # in pids1
    self.assertIsNotNone(store.GetPid(16))
    self.assertIsNotNone(store.GetPid(17))
    self.assertIsNotNone(store.GetName("DEVICE_INFO"))
    self.assertIsNotNone(store.GetName("PROXIED_DEVICES",
                                       OPEN_LIGHTING_ESTA_CODE))
    self.assertEqual(store.NameToValue("DEVICE_INFO"), 96)

    # in pids2
    self.assertIsNotNone(store.GetPid(80))
    self.assertIsNotNone(store.GetName("COMMS_STATUS"))

    self.assertEqual(store.GetName("PROXIED_DEVICES"),
                     store.GetPid(16, OPEN_LIGHTING_ESTA_CODE))

    self.assertIsNotNone(store.GetPid(32768, 161))

    # check override file
    self.assertIsNone(store.GetName("SERIAL_NUMBER", OPEN_LIGHTING_ESTA_CODE))
    self.assertIsNone(store.GetName("DEVICE_MODE", OPEN_LIGHTING_ESTA_CODE))
    pid = store.GetName("FOO_BAR", OPEN_LIGHTING_ESTA_CODE)
    self.assertEqual(pid.value, 32768)
    self.assertEqual(pid, store.GetPid(32768, OPEN_LIGHTING_ESTA_CODE))
    self.assertEqual(pid.name, "FOO_BAR")
    self.assertIsNotNone(pid.GetRequest(PidStore.RDM_GET))
    self.assertIsNotNone(pid.GetResponse(PidStore.RDM_GET))
    self.assertIsNone(pid.GetRequest(PidStore.RDM_SET))
    self.assertIsNone(pid.GetResponse(PidStore.RDM_SET))
    self.assertEqual(store.NameToValue("FOO_BAR", OPEN_LIGHTING_ESTA_CODE),
                     32768)
    self.assertIsNone(store.NameToValue("FOO_BAR", OPEN_LIGHTING_ESTA_CODE + 1))
    self.assertEqual(pid.GetResponse(PidStore.RDM_GET).GetDescription(),
                     ("<baz>", "  baz: <[0, 4294967295]>"))

  def testLoadMissing(self):
    store = PidStore.PidStore()
    with self.assertRaises(IOError):
      store.Load([os.path.join(path, "missing_file_pids.proto")])

  def testLoadDuplicateManufacturer(self):
    store = PidStore.PidStore()
    with self.assertRaises(PidStore.InvalidPidFormat):
      store.Load([os.path.join(path, "duplicate_manufacturer.proto")])

  def testLoadDuplicateValue(self):
    store = PidStore.PidStore()
    with self.assertRaises(PidStore.InvalidPidFormat):
      store.Load([os.path.join(path, "duplicate_pid_value.proto")])

  def testLoadDuplicateName(self):
    store = PidStore.PidStore()
    with self.assertRaises(PidStore.InvalidPidFormat):
      store.Load([os.path.join(path, "duplicate_pid_name.proto")])

  def testLoadInvalidEstaPid(self):
    store = PidStore.PidStore()
    with self.assertRaises(PidStore.InvalidPidFormat):
      store.Load([os.path.join(path, "invalid_esta_pid.proto")])

  def testInconsistentData(self):
    store = PidStore.PidStore()
    with self.assertRaises(PidStore.PidStructureException):
      store.Load([os.path.join(path, "inconsistent_pid.proto")])

  def testSort(self):
    store = PidStore.PidStore()
    store.Load([os.path.join(path, "test_pids.proto")])

    self.assertEqual(len(store.Pids()), 70)
    pids = [p.value for p in sorted(store.Pids())]
    self.assertEqual(pids,
                     [16, 17, 21, 32, 48, 49, 50, 51, 80, 81, 96, 112, 128,
                      129, 130, 144, 160, 176, 192, 193, 194, 224, 225, 240,
                      288, 289, 290, 512, 513, 514, 1024, 1025, 1026, 1027,
                      1028, 1029, 1280, 1281, 1536, 1537, 1538, 1539, 4096,
                      4097, 4112, 4128, 4129, 4144, 4145, 32688, 32689, 32690,
                      32691, 32692, 32752, 32753, 32754, 32755, 32756, 32757,
                      32758, 32759, 32760, 32761, 32762, 32763, 32764, 32765,
                      32766, 32767])
    allNotEqual(self, store.Pids())
    allHashNotEqual(self, store.Pids())

  def testCmp(self):
    p1 = PidStore.Pid("base", 42)
    p1a = PidStore.Pid("notbase", 42)

    g1getreq = PidStore.Group("grg", [PidStore.UInt16("ui16"),
                                      PidStore.Int32("i32")])
    g1setreq = PidStore.Group("srg", [PidStore.MACAtom("mac"),
                                      PidStore.Int8("i32")])
    p1b = PidStore.Pid("base", 42, None, None,
                       g1getreq, g1setreq)

    p2 = PidStore.Pid("base", 43, None, None,
                      g1getreq, g1setreq)
    p3 = PidStore.Pid("notbase", 43)

    self.assertEqual(p1, p1a)
    self.assertEqual(p1, p1b)
    self.assertEqual(p1a, p1b)
    self.assertEqual(p2, p3)
    self.assertNotEqual(p1, p2)
    self.assertNotEqual(p1a, p2)
    self.assertNotEqual(p1b, p2)
    self.assertNotEqual(p1a, p3)

    self.assertEqual(hash(p1), hash(p1a))
    self.assertEqual(hash(p1), hash(p1b))
    self.assertEqual(hash(p1a), hash(p1b))
    self.assertEqual(hash(p2), hash(p3))
    self.assertNotEqual(hash(p1), hash(p2))
    self.assertNotEqual(hash(p1a), hash(p2))
    self.assertNotEqual(hash(p1b), hash(p2))
    self.assertNotEqual(hash(p1a), hash(p3))

  def testPackUnpack(self):
    store = PidStore.PidStore()
    store.Load([os.path.join(path, "test_pids.proto")])

    pid = store.GetName("DMX_PERSONALITY_DESCRIPTION")

    # Pid.Pack only packs requests and Pid.Unpack only unpacks responses
    # so test in two halves
    args = ["42"]
    blob = pid.Pack(args, PidStore.RDM_GET)
    self.assertEqual(blob, binascii.unhexlify("2a"))
    decoded = pid._requests.get(PidStore.RDM_GET).Unpack(blob)[0]
    self.assertEqual(decoded['personality'], 42)

    args = ["42", "7", "UnpackTest"]
    blob = pid._responses.get(PidStore.RDM_GET).Pack(args)[0]
    self.assertEqual(blob, binascii.unhexlify("2a0007556e7061636b54657374"))
    decoded = pid.Unpack(blob, PidStore.RDM_GET)
    self.assertEqual(decoded['personality'], 42)
    self.assertEqual(decoded['slots_required'], 7)
    self.assertEqual(decoded['name'], "UnpackTest")

  def testPackRanges(self):
    store = PidStore.PidStore()
    store.Load([os.path.join(path, "test_pids.proto")])

    pid = store.GetName("REAL_TIME_CLOCK")

    # first check encoding of valid RTC data
    args = ["2020", "6", "20", "21", "22", "23"]
    blob = pid.Pack(args, PidStore.RDM_SET)
    self.assertEqual(blob, binascii.unhexlify("07e40614151617"))
    decoded = pid._requests.get(PidStore.RDM_SET).Unpack(blob)[0]
    self.assertEqual(decoded, {'year': 2020, 'month': 6,
                               'day': 20, 'hour': 21,
                               'minute': 22, 'second': 23})

    # next check that ranges are being enforced properly
    # invalid year (2002 < 2003)
    with self.assertRaises(PidStore.ArgsValidationError):
      args = ["2002", "6", "20", "20", "20", "20"]
      blob = pid.Pack(args, PidStore.RDM_SET)

    # invalid month < 1
    with self.assertRaises(PidStore.ArgsValidationError):
      args = ["2020", "0", "20", "20", "20", "20"]
      blob = pid.Pack(args, PidStore.RDM_SET)

    # invalid month > 12
    with self.assertRaises(PidStore.ArgsValidationError):
      args = ["2020", "13", "20", "20", "20", "20"]
      blob = pid.Pack(args, PidStore.RDM_SET)

    # invalid month > 255
    with self.assertRaises(PidStore.ArgsValidationError):
      args = ["2020", "256", "20", "20", "20", "20"]
      blob = pid.Pack(args, PidStore.RDM_SET)

    # invalid negative month
    with self.assertRaises(PidStore.ArgsValidationError):
      args = ["2020", "-1", "20", "20", "20", "20"]
      blob = pid.Pack(args, PidStore.RDM_SET)

    # tests for string with min=max=2
    pid = store.GetName("LANGUAGE_CAPABILITIES")
    args = ["en"]
    blob = pid._responses.get(PidStore.RDM_GET).Pack(args)[0]
    self.assertEqual(blob, binascii.unhexlify("656e"))
    decoded = pid.Unpack(blob, PidStore.RDM_GET)
    self.assertEqual(decoded, {'languages': [{'language': 'en'}]})

    with self.assertRaises(PidStore.ArgsValidationError):
      args = ["e"]
      blob = pid._responses.get(PidStore.RDM_GET).Pack(args)[0]

    with self.assertRaises(PidStore.ArgsValidationError):
      args = ["enx"]
      blob = pid._responses.get(PidStore.RDM_GET).Pack(args)[0]

    # valid empty string
    pid = store.GetName("STATUS_ID_DESCRIPTION")
    args = [""]
    blob = pid._responses.get(PidStore.RDM_GET).Pack(args)[0]
    self.assertEqual(len(blob), 0)
    decoded = pid.Unpack(blob, PidStore.RDM_GET)
    self.assertEqual(decoded['label'], "")

    # string too long
    with self.assertRaises(PidStore.ArgsValidationError):
      args = ["123456789012345678901234567890123"]
      blob = pid._responses.get(PidStore.RDM_GET).Pack(args)[0]


if __name__ == '__main__':
  path = (os.environ.get('TESTDATADIR', "../common/rdm/testdata"))
  unittest.main()
