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
# TestMixins.py
# Copyright (C) 2010 Simon Newton

'''Mixins used by the test definitions.

This module contains classes which can be inherited from to simplify writing
test definitions.
'''

__author__ = 'nomis52@gmail.com (Simon Newton)'

from ola import PidStore
from ola.OlaClient import RDMNack
from ResponderTest import ResponderTest, ExpectedResult

MAX_LABEL_SIZE = 32

# Generic Get / Set Mixins
#------------------------------------------------------------------------------
class UnsupportedGetMixin(object):
  """Check that Get fails with NR_UNSUPPORTED_COMMAND_CLASS."""
  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid)


class UnsupportedSetMixin(object):
  """Check that SET fails with NR_UNSUPPORTED_COMMAND_CLASS."""
  DATA = ''

  def Test(self):
    self.AddExpectedResults(
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


# Generic Label Mixins
#------------------------------------------------------------------------------
class GetLabelMixin(object):
  """Fetch a PID, and make sure we get either a UNKNOWN_PID or ACK response."""
  def Test(self):
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_UNKNOWN_PID),
      ExpectedResult.AckResponse(self.pid.value, ['label'])
    ])
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class GetLabelWithDataMixin(object):
  """Fetch a PID with some random data."""
  DATA = 'foobarbaz'

  def Test(self):
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_UNKNOWN_PID),
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
      ExpectedResult.AckResponse(self.pid.value, ['label'])
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class SetLabelMixin(object):
  """Set a PID and make sure the value is saved."""
  TEST_LABEL = 'test label'

  def Test(self):
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet)
    ])
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.TEST_LABEL])

  def VerifySet(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={'label': self.TEST_LABEL}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetEmptyLabelMixin(object):
  """Send an empty SET label command."""
  def Test(self):
    self.test_label = ''
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet)
    ])
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.test_label])

  def VerifySet(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={'label': self.test_label}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetOversizedLabelMixin(object):
  """Send an over-sized SET label command."""
  LONG_STRING = 'this is a string which is more than 32 characters'

  def Test(self):
    self.verify_result = False
    self.AddExpectedResults([
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
      ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet)
    ])
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.LONG_STRING)

  def VerifySet(self):
    """If we got an ACK back, we send a GET to check what the result was."""
    self.verify_result = True
    self.AddExpectedResults(
      ExpectedResult.AckResponse(self.pid.value, ['label']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if not self.verify_result:
      return

    if 'label' not in fields:
      self.SetFailed('Missing label field in response')
    else:
      if fields['label'] != self.LONG_STRING[0:MAX_LABEL_SIZE]:
        self.AddWarning('Oversized %s returned %s' % (
          self.PID, fields['label']))
