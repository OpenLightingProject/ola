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

def GetUnsupportedNacks(pid):
  """Repsonders use either NR_UNSUPPORTED_COMMAND_CLASS or NR_UNKNOWN_PID."""
  return [
    ExpectedResult.NackResponse(pid.value,
                                RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
    ExpectedResult.NackResponse(pid.value, RDMNack.NR_UNKNOWN_PID),
  ]


# Generic Get / Set Mixins
# These don't care about the format of the message.
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
    self.AddExpectedResults(GetUnsupportedNacks(self.pid))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class GetWithData(ResponderTest):
  """GET a PID with random param data."""
  DATA = 'foobarbaz'

  def Test(self):
    self.AddIfSupported([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
      ExpectedResult.AckResponse(
        self.pid.value,
        warning='Get %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class SetWithData(ResponderTest):
  """SET a PID with random param data."""
  DATA = 'foobarbaz'

  def Test(self):
    self.AddIfSupported([
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR),
      ExpectedResult.AckResponse(
        self.pid.value,
        warning='Set %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, 'foo')


# Generic Label Mixins
# These all work in conjunction with the IsSupportedMixin
#------------------------------------------------------------------------------
class GetLabelMixin(object):
  """Fetch a PID, and make sure we get an ACK response."""
  def Test(self):
    self.AddIfSupported([
      ExpectedResult.AckResponse(self.pid.value, ['label'])
    ])
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetLabelMixin(object):
  """Set a PID and make sure the value is saved."""
  TEST_LABEL = 'test label'

  def ExpectedResults(self):
    return [
      ExpectedResult.NackResponse(self.pid.value,
                                  RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet)
    ]

  def Test(self):
    self.verify_result = False
    self.AddIfSupported(self.ExpectedResults())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.TEST_LABEL])

  def VerifySet(self):
    self.verify_result = True
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_names=['label']))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  def VerifyResult(self, status, fields):
    if not self.verify_result:
      return

    new_label = fields['label']
    if self.TEST_LABEL == new_label:
      return

    if (len(new_label) < len(self.TEST_LABEL) and
        self.TEST_LABEL.startswith(new_label)):
      self.AddAdvisory('Label for %s was truncated to %d characters' %
                       (self.pid, len(new_label)))
    else:
      self.SetFailed('Labels didn\'t match, expected %s, got %s' %
                     (self.TEST_LABEL, self.new_label))

  def ResetState(self):
    self.AddExpectedResults(ExpectedResult.AckResponse(self.pid.value))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.OldValue()])
    self._wrapper.Run()


class SetOversizedLabelMixin(object):
  """Send an over-sized SET label command."""
  LONG_STRING = 'this is a string which is more than 32 characters'

  def Test(self):
    self.verify_result = False
    self.AddIfSupported([
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
        self.AddWarning(
            'Setting an oversized %s set the first %d characters' % (
            self.PID, len(fields['label'])))

# Generic Bool Mixins
# These all work in conjunction with the IsSupportedMixin
#------------------------------------------------------------------------------
class GetBoolMixin(object):
  """Get a pid and expect a bool (8 bit field) as the result."""
  def Test(self):
    self.AddIfSupported(
      ExpectedResult.AckResponse(self.pid.value, [self.EXPECTED_FIELD]))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetBoolMixin(object):
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

  def Test(self):
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.NewValue()])

  def VerifySet(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={self.EXPECTED_FIELD: self.NewValue()}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  #TODO(simon): add a back out method here


class SetBoolNoDataMixin(object):
  """Set a bool field with no data."""
  def Test(self):
    self.AddIfSupported(
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, '')

  # TODO(simon): add a method to check this didn't change the value


# Generic UInt8 Mixins
# These all work in conjunction with the IsSupportedMixin
#------------------------------------------------------------------------------
class GetUInt8Mixin(object):
  """Get a pid and expect uint8 as the result."""
  def Test(self):
    self.AddIfSupported(
      ExpectedResult.AckResponse(self.pid.value, [self.EXPECTED_FIELD]))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetUInt8Mixin(object):
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

  def Test(self):
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.NewValue()])

  def VerifySet(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={self.EXPECTED_FIELD: self.NewValue()}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  #TODO(simon): add a back out method here

class SetUInt8NoDataMixin(object):
  """Set a uint8 field with no data."""
  def Test(self):
    self.AddIfSupported(
      ExpectedResult.NackResponse(self.pid.value, RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, '')

  # TODO(simon): add a method to check this didn't change the value


# Generic UInt32 Mixins
# These all work in conjunction with the IsSupportedMixin
#------------------------------------------------------------------------------
class GetUInt32Mixin(object):
  """Get a pid and expect a uint32 as the result."""
  def Test(self):
    self.AddIfSupported(
      ExpectedResult.AckResponse(self.pid.value, [self.EXPECTED_FIELD]))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)


class SetUInt32Mixin(object):
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

  def Test(self):
    self.AddIfSupported(
        ExpectedResult.AckResponse(self.pid.value, action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.NewValue()])

  def VerifySet(self):
    self.AddExpectedResults(
      ExpectedResult.AckResponse(
        self.pid.value,
        field_values={self.EXPECTED_FIELD: self.NewValue()}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  #TODO(simon): add a back out method here


class SetUInt32NoDataMixin(object):
  """Set a uint32 field with no data."""

  def Test(self):
    self.AddIfSupported(self.NackResponse(RDMNack.NR_FORMAT_ERROR))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, '')

  # TODO(simon): add a method to check this didn't change the value
