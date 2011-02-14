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
from ExpectedResults import *
from ResponderTest import ResponderTestFixture

MAX_LABEL_SIZE = 32

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
    self.AddExpectedResults(
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

  def VerifyResult(self, status, fields):
    if self.PROVIDES:
      value = None
      if status.WasAcked():
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

  def VerifyResult(self, status, fields):
    if status.WasAcked() and self.PROVIDES:
      self.SetProperty(self.PROVIDES[0], fields[self.EXPECTED_FIELD])


class GetWithDataMixin(ResponderTestFixture):
  """GET a PID with random param data."""
  DATA = 'foobarbaz'

  def Test(self):
    self.AddIfGetSupported([
      self.NackGetResult(RDMNack.NR_FORMAT_ERROR),
      self.AckGetResult(
        warning='Get %s with data returned an ack' % self.pid.name)
    ])
    self.SendRawGet(PidStore.ROOT_DEVICE, self.pid, 'foo')


class UnsupportedSetMixin(object):
  """Check that SET fails with NR_UNSUPPORTED_COMMAND_CLASS."""
  DATA = ''

  def Test(self):
    self.AddExpectedResults(UnsupportedSetNacks(self.pid))
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.DATA)


class SetWithDataMixin(ResponderTestFixture):
  """SET a PID with random param data."""
  DATA = 'foobarbaz'

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
  """Set a PID and make sure the value is saved."""
  TEST_LABEL = 'test label'

  def ExpectedResults(self):
    return [
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.AckSetResult(action=self.VerifySet)
    ]

  def Test(self):
    self.verify_result = False
    self.AddIfSetSupported(self.ExpectedResults())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.TEST_LABEL])

  def VerifySet(self):
    self.verify_result = True
    self.AddExpectedResults(self.AckGetResult(field_names=['label']))
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
    if not self.OldValue():
      return
    self.AddExpectedResults(self.AckSetResult())
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.OldValue()])
    self._wrapper.Run()


class SetOversizedLabelMixin(object):
  """Send an over-sized SET label command."""
  LONG_STRING = 'this is a string which is more than 32 characters'

  def Test(self):
    self.verify_result = False
    self.AddIfSetSupported([
      self.NackSetResult(RDMNack.NR_UNSUPPORTED_COMMAND_CLASS),
      self.NackSetResult(RDMNack.NR_FORMAT_ERROR),
      self.AckSetResult(action=self.VerifySet),
    ])
    self.SendRawSet(PidStore.ROOT_DEVICE, self.pid, self.LONG_STRING)

  def VerifySet(self):
    """If we got an ACK back, we send a GET to check what the result was."""
    self.verify_result = True
    self.AddExpectedResults(self.AckGetResult(field_names=['label']))
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
    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.NewValue()])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(field_values={self.EXPECTED_FIELD: self.NewValue()}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  #TODO(simon): add a back out method here


# Generic UInt8 Mixins
# These all work in conjunction with the IsSupportedMixin
#------------------------------------------------------------------------------
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
    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.NewValue()])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(field_values={self.EXPECTED_FIELD: self.NewValue()}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  #TODO(simon): add a back out method here


# Generic UInt32 Mixins
# These all work in conjunction with the IsSupportedMixin
#------------------------------------------------------------------------------
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
    self.AddIfSetSupported(self.AckSetResult(action=self.VerifySet))
    self.SendSet(PidStore.ROOT_DEVICE, self.pid, [self.NewValue()])

  def VerifySet(self):
    self.AddExpectedResults(
      self.AckGetResult(field_values={self.EXPECTED_FIELD: self.NewValue()}))
    self.SendGet(PidStore.ROOT_DEVICE, self.pid)

  #TODO(simon): add a back out method here
