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
# ExpectedResultsTest.py
# Copyright (C) 2021 Peter Newman

import unittest
from collections import namedtuple

# Keep this import relative to simplify the testing
from ExpectedResults import (BroadcastResult, DUBResult, InvalidResponse,
                             SuccessfulResult, TimeoutResult,
                             UnsupportedResult)
from ola.OlaClient import OlaClient

"""Test cases for ExpectedResults classes."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class ExpectedResultsTest(unittest.TestCase):
  MockResponse = namedtuple('MockResponse', ['response_code'])
  MockBroadcastResponse = MockResponse(OlaClient.RDM_WAS_BROADCAST)
  MockTimeoutResponse = MockResponse(OlaClient.RDM_TIMEOUT)
  MockInvalidResponse = MockResponse(OlaClient.RDM_INVALID_RESPONSE)
  MockUnsupportedResult = MockResponse(
      OlaClient.RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED)
  MockDUBResult = MockResponse(OlaClient.RDM_DUB_RESPONSE)
  MockSuccessfulResult = MockResponse(OlaClient.RDM_COMPLETED_OK)

  def testBroadcastResult(self):
    br = BroadcastResult()
    self.assertTrue(br.Matches(self.MockBroadcastResponse, {}))
    self.assertFalse(br.Matches(self.MockTimeoutResponse, {}))
    self.assertFalse(br.Matches(self.MockInvalidResponse, {}))
    self.assertFalse(br.Matches(self.MockUnsupportedResult, {}))
    self.assertFalse(br.Matches(self.MockDUBResult, {}))
    self.assertFalse(br.Matches(self.MockSuccessfulResult, {}))

  def testTimeoutResult(self):
    tr = TimeoutResult()
    self.assertFalse(tr.Matches(self.MockBroadcastResponse, {}))
    self.assertTrue(tr.Matches(self.MockTimeoutResponse, {}))
    self.assertFalse(tr.Matches(self.MockInvalidResponse, {}))
    self.assertFalse(tr.Matches(self.MockUnsupportedResult, {}))
    self.assertFalse(tr.Matches(self.MockDUBResult, {}))
    self.assertFalse(tr.Matches(self.MockSuccessfulResult, {}))

  def testInvalidResponse(self):
    ir = InvalidResponse()
    self.assertFalse(ir.Matches(self.MockBroadcastResponse, {}))
    self.assertFalse(ir.Matches(self.MockTimeoutResponse, {}))
    self.assertTrue(ir.Matches(self.MockInvalidResponse, {}))
    self.assertFalse(ir.Matches(self.MockUnsupportedResult, {}))
    self.assertFalse(ir.Matches(self.MockDUBResult, {}))
    self.assertFalse(ir.Matches(self.MockSuccessfulResult, {}))

  def testUnsupportedResult(self):
    ur = UnsupportedResult()
    self.assertFalse(ur.Matches(self.MockBroadcastResponse, {}))
    self.assertFalse(ur.Matches(self.MockTimeoutResponse, {}))
    self.assertFalse(ur.Matches(self.MockInvalidResponse, {}))
    self.assertTrue(ur.Matches(self.MockUnsupportedResult, {}))
    self.assertFalse(ur.Matches(self.MockDUBResult, {}))
    self.assertFalse(ur.Matches(self.MockSuccessfulResult, {}))

  def testDUBResult(self):
    dr = DUBResult()
    self.assertFalse(dr.Matches(self.MockBroadcastResponse, {}))
    self.assertFalse(dr.Matches(self.MockTimeoutResponse, {}))
    self.assertFalse(dr.Matches(self.MockInvalidResponse, {}))
    self.assertFalse(dr.Matches(self.MockUnsupportedResult, {}))
    self.assertTrue(dr.Matches(self.MockDUBResult, {}))
    self.assertFalse(dr.Matches(self.MockSuccessfulResult, {}))

  def testSuccessfulResult(self):
    sr = SuccessfulResult()
    self.assertFalse(sr.Matches(self.MockBroadcastResponse, {}))
    self.assertFalse(sr.Matches(self.MockTimeoutResponse, {}))
    self.assertFalse(sr.Matches(self.MockInvalidResponse, {}))
    self.assertFalse(sr.Matches(self.MockUnsupportedResult, {}))
    self.assertFalse(sr.Matches(self.MockDUBResult, {}))
    self.assertTrue(sr.Matches(self.MockSuccessfulResult, {}))


if __name__ == '__main__':
  unittest.main()
