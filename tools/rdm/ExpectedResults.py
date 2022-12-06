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
# ExpectedResults.py
# Copyright (C) 2011 Simon Newton
#
# Expected result classes are broken down as follows:
#
# BaseExpectedResult - the base class
#  BroadcastResult   - expects the request to be broadcast, hence no response
#  TimeoutResult     - expects the response to be a timeout
#  InvalidResult     - expects the response to be invalid
#  UnsupportedResult - expects RDM Discovery to be unsupported
#  DUBResult         - expects an RDM DUB response
#  SuccessfulResult  - expects a well formed response from the device
#   NackResult       - parent NACK class
#    NackDiscoveryResult - expects DISCOVERY_COMMAND_RESPONSE with a NACK
#    NackGetResult   - expects GET_COMMAND_RESPONSE with a NACK
#    NackSetResult   - expects SET_COMMAND_RESPONSE with a NACK
#   AckResult        - parent ACK class
#    AckDiscoveryResult - expects DISCOVERY_COMMAND_RESPONSE with an ACK
#    AckGetResult    - expects GET_COMMAND_RESPONSE with an ACK
#    AckSetResult    - expects SET_COMMAND_RESPONSE with an ACK
#   QueuedMessageResult - expects an ACK or NACK for any PID other than
#                         QUEUED_MESSAGE


from ola.OlaClient import OlaClient
from ola.PidStore import RDM_DISCOVERY, RDM_GET, RDM_SET, GetStore

COMMAND_CLASS_DICT = {
    RDM_GET: 'Get',
    RDM_SET: 'Set',
    RDM_DISCOVERY: 'Discovery',
}


def _CommandClassToString(command_class):
  return COMMAND_CLASS_DICT[command_class]


class BaseExpectedResult(object):
  """The base class for expected results."""
  def __init__(self,
               action=None,
               warning=None,
               advisory=None):
    """Create the base expected result object.

    Args:
      action: The action to run if this result matches
      warning: A warning message to log if this result matches
      advisory: An advisory message to log if this result matches
    """
    self._action = action
    self._warning_message = warning
    self._advisory_message = advisory

  @property
  def action(self):
    return self._action

  @property
  def warning(self):
    return self._warning_message

  @property
  def advisory(self):
    return self._advisory_message

  def Matches(self, response, unpacked_data):
    """Check if the response we received matches this object.

    Args:
      response: An RDMResponse object
      unpacked_data: A dict of field name : value mappings that were present in
        the response.
    """
    raise TypeError('Base method called')


class BroadcastResult(BaseExpectedResult):
  """This checks that the request was broadcast."""
  def __str__(self):
    return 'RDM_WAS_BROADCAST'

  def Matches(self, response, unpacked_data):
    return OlaClient.RDM_WAS_BROADCAST == response.response_code


class TimeoutResult(BaseExpectedResult):
  """This checks that the request timed out."""
  def __str__(self):
    return 'RDM_TIMEOUT'

  def Matches(self, response, unpacked_data):
    return OlaClient.RDM_TIMEOUT == response.response_code


class InvalidResponse(BaseExpectedResult):
  """This checks that we got an invalid response back."""
  def __str__(self):
    return 'RDM_INVALID_RESPONSE'

  def Matches(self, response, unpacked_data):
    return OlaClient.RDM_INVALID_RESPONSE == response.response_code


class UnsupportedResult(BaseExpectedResult):
  """This checks that the request was unsupported."""
  def __str__(self):
    return 'RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED'

  def Matches(self, response, unpacked_data):
    return (OlaClient.RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED ==
            response.response_code)


class DUBResult(BaseExpectedResult):
  """Checks that the result was a DUB response."""
  def __str__(self):
    return 'RDM_DUB_RESPONSE'

  def Matches(self, response, unpacked_data):
    return OlaClient.RDM_DUB_RESPONSE == response.response_code


class SuccessfulResult(BaseExpectedResult):
  """This checks that we received a valid response from the device.

  This doesn't check that the response was a certain type, but simply that the
  message was formed correctly. Other classes inherit from this and perform
  more specific checking.
  """
  def __str__(self):
    return 'RDM_COMPLETED_OK'

  def Matches(self, response, unpacked_data):
    return response.response_code == OlaClient.RDM_COMPLETED_OK


class QueuedMessageResult(SuccessfulResult):
  """This checks for a valid response to a QUEUED_MESSAGE request."""
  def __str__(self):
    return 'It\'s complicated'

  def Matches(self, response, unpacked_data):
    ok = super(QueuedMessageResult, self).Matches(response, unpacked_data)

    if not ok:
      return False

    pid_store = GetStore()
    queued_message_pid = pid_store.GetName('QUEUED_MESSAGE')

    return ((response.response_type == OlaClient.RDM_NACK_REASON or
             response.response_type == OlaClient.RDM_ACK) and
             response.pid != queued_message_pid.value)


class NackResult(SuccessfulResult):
  """This checks that the device nacked the request."""
  def __init__(self,
               command_class,
               pid_id,
               nack_reason,
               action=None,
               warning=None,
               advisory=None):
    """Create an NackResult object.

    Args:
      command_class: RDM_GET or RDM_SET
      pid_id: The pid id we expect to have been nack'ed
      nack_reason: The RDMNack object we expect.
      action: The action to run if this result matches
      warning: A warning message to log is this result matches
      advisory: An advisory message to log is this result matches
    """
    super(NackResult, self).__init__(action, warning, advisory)

    self._command_class = command_class
    self._pid_id = pid_id
    self._nack_reason = nack_reason

  def __str__(self):
    return ('CC: %s, PID 0x%04hx, NACK %s' %
            (_CommandClassToString(self._command_class),
             self._pid_id,
             self._nack_reason))

  def Matches(self, response, unpacked_data):
    ok = super(NackResult, self).Matches(response, unpacked_data)

    return (ok and
            response.response_type == OlaClient.RDM_NACK_REASON and
            response.command_class == self._command_class and
            response.pid == self._pid_id and
            response.nack_reason == self._nack_reason)


class NackDiscoveryResult(NackResult):
  """This checks that the device nacked a Discovery request."""
  def __init__(self,
               pid_id,
               nack_reason,
               action=None,
               warning=None,
               advisory=None):
    """Create an expected result object which is a NACK for a Discovery
      request.

    Args:
      pid_id: The pid id we expect to have been nack'ed
      nack_reason: The RDMNack object we expect.
      action: The action to run if this result matches
      warning: A warning message to log is this result matches
      advisory: An advisory message to log is this result matches
    """
    super(NackDiscoveryResult, self).__init__(RDM_DISCOVERY,
                                              pid_id,
                                              nack_reason,
                                              action,
                                              warning,
                                              advisory)


class NackGetResult(NackResult):
  """This checks that the device nacked a GET request."""
  def __init__(self,
               pid_id,
               nack_reason,
               action=None,
               warning=None,
               advisory=None):
    """Create an expected result object which is a NACK for a GET request.

    Args:
      pid_id: The pid id we expect to have been nack'ed
      nack_reason: The RDMNack object we expect.
      action: The action to run if this result matches
      warning: A warning message to log is this result matches
      advisory: An advisory message to log is this result matches
    """
    super(NackGetResult, self).__init__(RDM_GET,
                                        pid_id,
                                        nack_reason,
                                        action,
                                        warning,
                                        advisory)


class NackSetResult(NackResult):
  """This checks that the device nacked a SET request."""
  def __init__(self,
               pid_id,
               nack_reason,
               action=None,
               warning=None,
               advisory=None):
    """Create an expected result object which is a NACK for a SET request.

    Args:
      pid_id: The pid id we expect to have been nack'ed
      nack_reason: The RDMNack object we expect.
      action: The action to run if this result matches
      warning: A warning message to log is this result matches
      advisory: An advisory message to log is this result matches
    """
    super(NackSetResult, self).__init__(RDM_SET,
                                        pid_id,
                                        nack_reason,
                                        action,
                                        warning,
                                        advisory)


class AckResult(SuccessfulResult):
  """This checks that the device ack'ed the request."""
  def __init__(self,
               command_class,
               pid_id,
               field_names=[],
               field_values={},
               action=None,
               warning=None,
               advisory=None):
    """Create an expected result object that matches an ACK.

    Args:
      command_class: RDM_GET or RDM_SET
      pid_id: The pid id we expect
      field_names: Check that these fields are present in the response
      field_values: Check that fields & values are present in the response
      action: The action to run if this result matches
      warning: A warning message to log is this result matches
      advisory: An advisory message to log is this result matches
    """
    super(AckResult, self).__init__(action, warning, advisory)

    self._command_class = command_class
    self._pid_id = pid_id
    self._field_names = field_names
    self._field_values = field_values

  def __str__(self):
    return ('CC: %s, PID 0x%04hx, ACK, fields %s, values %s' % (
            _CommandClassToString(self._command_class),
            self._pid_id,
            self._field_names,
            self._field_values))

  def Matches(self, response, unpacked_data):
    ok = super(AckResult, self).Matches(response, unpacked_data)

    if (not ok or
        response.response_type != OlaClient.RDM_ACK or
        response.command_class != self._command_class or
        response.pid != self._pid_id):
      return False

    # unpacked_data may be either a list of dicts, or a dict
    if isinstance(unpacked_data, list):
      for item in unpacked_data:
        field_keys = set(item.keys())
        for field in self._field_names:
          if field not in field_keys:
            return False
    else:
      field_keys = set(unpacked_data.keys())
      for field in self._field_names:
        if field not in field_keys:
          return False

    for field, value in self._field_values.items():
      if field not in unpacked_data:
        return False
      if value != unpacked_data[field]:
        return False
    return True


class AckDiscoveryResult(AckResult):
  """This checks that the device ack'ed a DISCOVERY request."""
  def __init__(self,
               pid_id,
               field_names=[],
               field_values={},
               action=None,
               warning=None,
               advisory=None):
    """Create an expected result object which is an ACK for a DISCOVERY request.

    Args:
      pid_id: The pid id we expect
      field_names: Check that these fields are present in the response
      field_values: Check that fields & values are present in the response
      action: The action to run if this result matches
      warning: A warning message to log is this result matches
      advisory: An advisory message to log is this result matches
    """
    super(AckDiscoveryResult, self).__init__(RDM_DISCOVERY,
                                             pid_id,
                                             field_names,
                                             field_values,
                                             action,
                                             warning,
                                             advisory)


class AckGetResult(AckResult):
  """This checks that the device ack'ed a GET request."""
  def __init__(self,
               pid_id,
               field_names=[],
               field_values={},
               action=None,
               warning=None,
               advisory=None):
    """Create an expected result object which is an ACK for a GET request.

    Args:
      pid_id: The pid id we expect
      field_names: Check that these fields are present in the response
      field_values: Check that fields & values are present in the response
      action: The action to run if this result matches
      warning: A warning message to log is this result matches
      advisory: An advisory message to log is this result matches
    """
    super(AckGetResult, self).__init__(RDM_GET,
                                       pid_id,
                                       field_names,
                                       field_values,
                                       action,
                                       warning,
                                       advisory)


class AckSetResult(AckResult):
  """This checks that the device ack'ed a SET request."""
  def __init__(self,
               pid_id,
               field_names=[],
               field_values={},
               action=None,
               warning=None,
               advisory=None):
    """Create an expected result object which is an ACK for a SET request.

    Args:
      pid_id: The pid id we expect
      field_names: Check that these fields are present in the response
      field_values: Check that fields & values are present in the response
      action: The action to run if this result matches
      warning: A warning message to log is this result matches
      advisory: An advisory message to log is this result matches
    """
    super(AckSetResult, self).__init__(RDM_SET,
                                       pid_id,
                                       field_names,
                                       field_values,
                                       action,
                                       warning,
                                       advisory)
