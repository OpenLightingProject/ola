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
# PidStore.py
# Copyright (C) 2010 Simon Newton
# Holds all the information about RDM PIDs

from __future__ import print_function

import binascii
import math
import os
import socket
import struct
import sys

from google.protobuf import text_format
from ola.MACAddress import MACAddress
from ola.UID import UID

from ola import Pids_pb2, PidStoreLocation, RDMConstants

"""The PID Store."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


# magic filenames
OVERRIDE_FILE_NAME = "overrides.proto"
MANUFACTURER_NAMES_FILE_NAME = "manufacturer_names.proto"

# Various sub device enums
ROOT_DEVICE = 0
MAX_VALID_SUB_DEVICE = 0x0200
ALL_SUB_DEVICES = 0xffff

# The different types of commands classes
RDM_GET, RDM_SET, RDM_DISCOVERY = range(3)


class Error(Exception):
  """Base error class."""


class InvalidPidFormat(Error):
  """Indicates the PID data file was invalid."""


class PidStructureException(Error):
  """Raised if the PID structure isn't valid."""


class ArgsValidationError(Error):
  """Raised if the arguments don't match the expected frame format."""


class UnpackException(Error):
  """Raised if we can't unpack the data correctly."""


class MissingPLASAPIDs(Error):
  """Raises if the files did not contain the ESTA (PLASA) PIDs."""


class Pid(object):
  """A class that describes everything about a PID."""
  def __init__(self, name, value,
               discovery_request=None,
               discovery_response=None,
               get_request=None,
               get_response=None,
               set_request=None,
               set_response=None,
               discovery_validators=[],
               get_validators=[],
               set_validators=[]):
    """Create a new PID.

    Args:
      name: the human readable name
      value: the 2 byte PID value
      discovery_request: A Group object, or None if DISCOVERY isn't supported
      discovery_response: A Group object, or None if DISCOVERY isn't supported
      get_request: A Group object, or None if GET isn't supported
      get_response:
      set_request: A Group object, or None if SET isn't supported
      set_response:
      discovery_validators:
      get_validators:
      set_validators:
    """
    self._name = name
    self._value = value
    self._requests = {
      RDM_GET: get_request,
      RDM_SET: set_request,
      RDM_DISCOVERY: discovery_request,
    }
    self._responses = {
      RDM_GET: get_response,
      RDM_SET: set_response,
      RDM_DISCOVERY: discovery_response,
    }
    self._validators = {
      RDM_GET: get_validators,
      RDM_SET: set_validators,
      RDM_DISCOVERY: discovery_validators,
    }

  @property
  def name(self):
    return self._name

  @property
  def value(self):
    return self._value

  def RequestSupported(self, command_class):
    """Check if this PID allows a command class."""
    return self._requests.get(command_class) is not None

  def GetRequest(self, command_class):
    return self._requests.get(command_class)

  def GetRequestField(self, command_class, field_name):
    fields = self.GetRequest(command_class).GetAtoms()
    fields = [f for f in fields if f.name == field_name]
    return fields[0] if fields else None

  def ResponseSupported(self, command_class):
    """Check if this PID responds to a command class."""
    return self._requests.get(command_class) is not None

  def GetResponse(self, command_class):
    return self._responses.get(command_class)

  def GetResponseField(self, command_class, field_name):
    fields = self.GetResponse(command_class).GetAtoms()
    fields = [f for f in fields if f.name == field_name]
    return fields[0] if fields else None

  def ValidateAddressing(self, args, command_class):
    """Run the validators."""
    validators = self._validators.get(command_class)
    if validators is None:
      return False

    args['pid'] = self
    for validator in validators:
      if not validator(args):
        return False
    return True

  def _GroupCmp(self, a, b):
    def setToDescription(x):
      def descOrNone(y):
        d = y.GetDescription() if y is not None else "None"
        return '::'.join(d) if isinstance(d, tuple) else d
      return ':'.join([descOrNone(x[f])
                       for f in [RDM_GET, RDM_SET, RDM_DISCOVERY]])
    ax = setToDescription(a)
    bx = setToDescription(b)
    return (ax > bx) - (ax < bx)  # canonical for cmp(a,b)

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self.value == other.value

  def __lt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self._value < other._value

  # These 4 can be replaced with functools:total_ordering when 2.6 is dropped
  def __le__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return self < other or self == other

  def __gt__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self <= other

  def __ge__(self, other):
    if not isinstance(other, self.__class__):
      return NotImplemented
    return not self < other

  def __ne__(self, other):
    return not self == other

  def __str__(self):
    return '%s (0x%04hx)' % (self.name, self.value)

  def __hash__(self):
    return hash(self._value)

  def Pack(self, args, command_class):
    """Pack args

    Args:
      args: A list of arguments of the right types.
      command_class: RDM_GET or RDM_SET or RDM_DISCOVERY

    Returns:
      Binary data which can be used as the Param Data.
    """
    group = self._requests.get(command_class)
    blob, args_used = group.Pack(args)
    return blob

  def Unpack(self, data, command_class):
    """Unpack a message.

    Args:
      data: The raw data
      command_class: RDM_GET or RDM_SET or RDM_DISCOVERY
    """
    group = self._responses.get(command_class)
    if group is None:
      raise UnpackException('Response contained data (hex): %s' %
                            binascii.b2a_hex(data))
    output = group.Unpack(data)[0]
    return output

  def GetRequestDescription(self, command_class):
    """Get a help string that describes the format of the request.

    Args:
      command_class: RDM_GET or RDM_SET or RDM_DISCOVERY

    Returns:
      A tuple of two strings, the first is a list of the names
      and the second is the descriptions, one per line.
    """
    group = self._requests.get(command_class)
    return group.GetDescription()


# The following classes are used to describe RDM messages
class Atom(object):
  """The basic field in an RDM message."""
  def __init__(self, name):
    self._name = name

  @property
  def name(self):
    return self._name

  def CheckForSingleArg(self, args):
    if len(args) < 1:
      raise ArgsValidationError('Missing argument for %s' % self.name)

  def __str__(self):
    return '%s, %s' % (self.__class__, self._name)

  def __repr__(self):
    return '%s, %s' % (self.__class__, self._name)

  def GetDescription(self, indent=0):
    return str(self)

  @staticmethod
  def HasRanges():
    return False


class FixedSizeAtom(Atom):
  def __init__(self, name, format_char):
    super(FixedSizeAtom, self).__init__(name)
    self._char = format_char

  @property
  def size(self):
    return struct.calcsize(self._FormatString())

  def FixedSize(self):
    """Returns true if the size of this atom doesn't vary."""
    return True

  def Unpack(self, data):
    format_string = self._FormatString()
    try:
      values = struct.unpack(format_string, data)
    except struct.error as e:
      raise UnpackException(e)
    return values[0]

  def Pack(self, args):
    format_string = self._FormatString()
    try:
      data = struct.pack(format_string, args[0])
    except struct.error as e:
      raise ArgsValidationError("Can't pack data: %s" % e)
    return data, 1

  def _FormatString(self):
    return '!%s' % self._char


class Bool(FixedSizeAtom):
  BOOL_MAP = {
    'true': 1,
    'false': 0,
  }

  def __init__(self, name):
    # once we have 2.6 use ? here
    super(Bool, self).__init__(name, 'B')

  def Pack(self, args):
    self.CheckForSingleArg(args)
    arg = args[0]
    if isinstance(arg, str):
      arg = args[0].lower()
      if arg not in self.BOOL_MAP:
        raise ArgsValidationError('Argument should be true or false')
      arg = self.BOOL_MAP[arg]
    return super(Bool, self).Pack([arg])

  def Unpack(self, value):
    return bool(super(Bool, self).Unpack(value))

  def GetDescription(self, indent=0):
    indent = ' ' * indent
    return '%s%s: <true|false>' % (indent, self.name)


class Range(object):
  """A range of allowed int values."""
  def __init__(self, min, max):
    self.min = min
    self.max = max

  def Matches(self, value):
    return value >= self.min and value <= self.max

  def __str__(self):
    if self.min == self.max:
      return '%d' % self.min
    else:
      return '[%d, %d]' % (self.min, self.max)


class IntAtom(FixedSizeAtom):
  def __init__(self, name, char, max_value, **kwargs):
    super(IntAtom, self).__init__(name, char)

    # About Labels & Ranges:
    # If neither labels nor ranges are specified, the valid values is the range
    #   of the data type.
    # If labels are specified, and ranges aren't, the valid values are the
    #   labels
    # If ranges are specified, the valid values are those which fall into the
    #   range (inclusive).
    # If both are specified, the enum values must fall into the specified
    #   ranges.

    # ranges limit the allowed values for a field
    self._ranges = kwargs.get('ranges', [])[:]
    self._multiplier = kwargs.get('multiplier', 0)

    # labels provide a user friendly way of referring to data values
    self._labels = {}
    for value, label in kwargs.get('labels', []):
      self._labels[label.lower()] = value
      if not kwargs.get('ranges', []):
        # Add the labels to the list of allowed values
        self._ranges.append(Range(value, value))

    if not self._ranges:
      self._ranges.append(Range(0, max_value))

  def ValidateRawValueInRange(self, value):
    for valid_range in self._ranges:
      if valid_range.Matches(value):
        return True
    return False

  def Pack(self, args):
    self.CheckForSingleArg(args)
    arg = args[0]
    if isinstance(arg, str):
      arg = arg.lower()
    value = self._labels.get(arg)

    # not a labeled value
    if value is None:
      value = self._AccountForMultiplierPack(args[0])

    for valid_range in self._ranges:
      if valid_range.Matches(value):
        break
    else:
      raise ArgsValidationError('Param %d out of range, must be one of %s' %
                                (value, self._GetAllowedRanges()))

    return super(IntAtom, self).Pack([value])

  def Unpack(self, data):
    return self._AccountForMultiplierUnpack(super(IntAtom, self).Unpack(data))

  def GetDescription(self, indent=0):
    indent = ' ' * indent
    increment = ''
    if self._multiplier:
      increment = ', increment %s' % (10 ** self._multiplier)

    return ('%s%s: <%s>%s' % (indent, self.name, self._GetAllowedRanges(),
                              increment))

  def DisplayValue(self, value):
    """Converts a raw value, e.g. UInt16 (as opposed to an array of bytes) into
    the value it would be displayed as, e.g. float to 1 D.P.

    This takes into account any multipliers set for the field.
    """
    return self._AccountForMultiplierUnpack(value)

  def RawValue(self, value):
    """Converts a display value, e.g. float to 1 D.P. into a raw value UInt16
    (as opposed to an array of bytes) it would be transmitted as.

    This takes into account any multipliers set for the field.
    """
    return self._AccountForMultiplierPack(value)

  def HasRanges(self):
    return (len(self._ranges) > 0)

  def _GetAllowedRanges(self):
    values = list(self._labels.keys())

    for valid_range in self._ranges:
      if valid_range.min == valid_range.max:
        values.append(str(self._AccountForMultiplierUnpack(valid_range.min)))
      else:
        values.append('[%s, %s]' %
                      (self._AccountForMultiplierUnpack(valid_range.min),
                       self._AccountForMultiplierUnpack(valid_range.max)))

    return ('%s' % ', '.join(values))

  def _AccountForMultiplierUnpack(self, value):
    new_value = value * (10 ** self._multiplier)
    if self._multiplier < 0:
      new_value = round(new_value, abs(self._multiplier))
    return new_value

  def _AccountForMultiplierPack(self, value):
    if self._multiplier >= 0:
      try:
        new_value = int(value)
      except ValueError as e:
        raise ArgsValidationError(e)

      multiplier = 10 ** self._multiplier
      if new_value % multiplier:
        raise ArgsValidationError(
            'Conversion will lose data: %d -> %d' %
            (new_value, (new_value / multiplier * multiplier)))
      new_value = int(new_value / multiplier)

    else:
      try:
        new_value = float(value)
      except ValueError as e:
        raise ArgsValidationError(e)

      scaled_value = new_value * 10 ** abs(self._multiplier)

      fraction, int_value = math.modf(scaled_value)

      if fraction:
        raise ArgsValidationError(
            'Conversion will lose data: %s -> %s' %
            (new_value, int_value * (10.0 ** self._multiplier)))
      new_value = int(int_value)
    return new_value


class Int8(IntAtom):
  """A single signed byte field."""
  def __init__(self, name, **kwargs):
    super(Int8, self).__init__(name, 'b', 0xff, **kwargs)


class UInt8(IntAtom):
  """A single unsigned byte field."""
  def __init__(self, name, **kwargs):
    super(UInt8, self).__init__(name, 'B', 0xff, **kwargs)


class Int16(IntAtom):
  """A two-byte signed field."""
  def __init__(self, name, **kwargs):
    super(Int16, self).__init__(name, 'h', 0xffff, **kwargs)


class UInt16(IntAtom):
  """A two-byte unsigned field."""
  def __init__(self, name, **kwargs):
    super(UInt16, self).__init__(name, 'H', 0xffff, **kwargs)


class Int32(IntAtom):
  """A four-byte signed field."""
  def __init__(self, name, **kwargs):
    super(Int32, self).__init__(name, 'i', 0xffffffff, **kwargs)


class UInt32(IntAtom):
  """A four-byte unsigned field."""
  def __init__(self, name, **kwargs):
    super(UInt32, self).__init__(name, 'I', 0xffffffff, **kwargs)


class Int64(IntAtom):
  """An eight-byte signed field."""
  def __init__(self, name, **kwargs):
    super(Int64, self).__init__(name, 'q', 0xffffffffffffffff, **kwargs)


class UInt64(IntAtom):
  """An eight-byte unsigned field."""
  def __init__(self, name, **kwargs):
    super(UInt64, self).__init__(name, 'Q', 0xffffffffffffffff, **kwargs)


class IPV4(IntAtom):
  """A four-byte IPV4 address."""
  def __init__(self, name, **kwargs):
    super(IPV4, self).__init__(name, 'I', 0xffffffff, **kwargs)

  def Unpack(self, data):
    try:
      return socket.inet_ntoa(data)
    except socket.error as e:
      raise ArgsValidationError("Can't unpack data: %s" % e)

  def Pack(self, args):
    # TODO(Peter): This currently allows some rather quirky values as per
    # inet_aton, we may want to restrict that in future
    try:
      value = struct.unpack("!I", socket.inet_aton(args[0]))
    except socket.error as e:
      raise ArgsValidationError("Can't pack data: %s" % e)
    return super(IntAtom, self).Pack(value)


class IPV6Atom(FixedSizeAtom):
  """A sixteen-byte IPV6 address."""
  def __init__(self, name, **kwargs):
    super(IPV6Atom, self).__init__(name, 'BBBBBBBBBBBBBBBB')

  def Unpack(self, data):
    try:
      return socket.inet_ntop(socket.AF_INET6, data)
    except socket.error as e:
      raise ArgsValidationError("Can't unpack data: %s" % e)

  def Pack(self, args):
    # TODO(Peter): This currently allows some rather quirky values as per
    # inet_pton, we may want to restrict that in future
    format_string = self._FormatString()
    try:
      data = struct.pack(format_string,
                         socket.inet_pton(socket.AF_INET6, args[0]))
    except socket.error as e:
      raise ArgsValidationError("Can't pack data: %s" % e)
    return data, 1


class MACAtom(FixedSizeAtom):
  """A MAC address."""
  def __init__(self, name, **kwargs):
    super(MACAtom, self).__init__(name, 'BBBBBB')

  def Unpack(self, data):
    format_string = self._FormatString()
    try:
      values = struct.unpack(format_string, data)
    except struct.error as e:
      raise UnpackException(e)
    return MACAddress(bytearray([values[0],
                                 values[1],
                                 values[2],
                                 values[3],
                                 values[4],
                                 values[5]]))

  def Pack(self, args):
    mac = None
    if isinstance(args[0], MACAddress):
      mac = args[0]
    else:
      mac = MACAddress.FromString(args[0])

    if mac is None:
      raise ArgsValidationError("Invalid MAC Address: %s" % args)

    format_string = self._FormatString()
    try:
      data = struct.pack(format_string,
                         mac.mac_address[0],
                         mac.mac_address[1],
                         mac.mac_address[2],
                         mac.mac_address[3],
                         mac.mac_address[4],
                         mac.mac_address[5])
    except struct.error as e:
      raise ArgsValidationError("Can't pack data: %s" % e)
    return data, 1


class UIDAtom(FixedSizeAtom):
  """A UID."""
  def __init__(self, name, **kwargs):
    super(UIDAtom, self).__init__(name, 'HI')

  def Unpack(self, data):
    format_string = self._FormatString()
    try:
      values = struct.unpack(format_string, data)
    except struct.error as e:
      raise UnpackException(e)
    return UID(values[0], values[1])

  def Pack(self, args):
    uid = None
    if isinstance(args[0], UID):
      uid = args[0]
    else:
      uid = UID.FromString(args[0])

    if uid is None:
      raise ArgsValidationError("Invalid UID: %s" % args)

    format_string = self._FormatString()
    try:
      data = struct.pack(format_string, uid.manufacturer_id, uid.device_id)
    except struct.error as e:
      raise ArgsValidationError("Can't pack data: %s" % e)
    return data, 1


class String(Atom):
  """A string field."""
  def __init__(self, name, **kwargs):
    super(String, self).__init__(name)
    self._min = kwargs.get('min_size', 0)
    self._max = kwargs.get('max_size', 32)

  @property
  def min(self):
    return self._min

  @property
  def max(self):
    return self._max

  @property
  def size(self):
    # only valid if FixedSize() == True
    return self.min

  def FixedSize(self):
    return self.min == self.max

  def Pack(self, args):
    self.CheckForSingleArg(args)
    arg = args[0]
    arg_size = len(arg)

    # Handle the fact a UTF-8 character could be multi-byte
    if sys.version_info >= (3, 2):
      arg_size = max(arg_size, len(bytes(arg, 'utf-8')))
    else:
      arg_size = max(arg_size, len(arg.encode('utf-8')))

    if self.max is not None and arg_size > self.max:
      raise ArgsValidationError('%s can be at most %d,' %
                                (self.name, self.max))

    if self.min is not None and arg_size < self.min:
      raise ArgsValidationError('%s must be more than %d,' %
                                (self.name, self.min))

    try:
      if sys.version_info >= (3, 2):
        data = struct.unpack('%ds' % arg_size, bytes(arg, 'utf-8'))
      else:
        data = struct.unpack('%ds' % arg_size, arg.encode('utf-8'))
    except struct.error as e:
      raise ArgsValidationError("Can't pack data: %s" % e)
    return data[0], 1

  def Unpack(self, data):
    data_size = len(data)
    if self.min and data_size < self.min:
      raise UnpackException('%s too short, required %d, got %d' %
                            (self.name, self.min, data_size))

    if self.max and data_size > self.max:
      raise UnpackException('%s too long, required %d, got %d' %
                            (self.name, self.max, data_size))

    try:
      value = struct.unpack('%ds' % data_size, data)
    except struct.error as e:
      raise UnpackException(e)

    try:
      value = value[0].rstrip(b'\x00').decode('utf-8')
    except UnicodeDecodeError as e:
      raise UnpackException(e)

    return value

  def GetDescription(self, indent=0):
    indent = ' ' * indent
    return ('%s%s: <string, [%d, %d] bytes>' % (
      indent, self.name, self.min, self.max))

  def __str__(self):
    return 'String(%s, min=%s, max=%s)' % (self.name, self.min, self.max)


class Group(Atom):
  """A repeated group of atoms."""
  def __init__(self, name, atoms, **kwargs):
    """Create a group of atoms.

    Args:
      name: The name of the group
      atoms: The list of atoms the group contains

    Raises:
      PidStructureException: if the structure of this group is invalid.
    """
    super(Group, self).__init__(name)
    self._atoms = atoms
    self._min = kwargs.get('min_size')
    self._max = kwargs.get('max_size')

    # None for variable sized groups
    self._group_size = self._VerifyStructure()

  def HasAtoms(self):
    return (len(self._atoms) > 0)

  def GetAtoms(self):
    return self._atoms

  @property
  def min(self):
    return self._min

  @property
  def max(self):
    return self._max

  def _VerifyStructure(self):
    """Verify that we can pack & unpack this group.

      We need to make sure we have enough known information to pack & unpack a
      group. We don't support repeated groups of variable length data, nor
      nested, repeated groups.

      For now we support the following cases:
       - Fixed size group. This is easy to unpack
       - Groups of variable size. We enforce two conditions for these, i) the
         variable sized field MUST be the last one ii) Only a single occurrence
         is allowed. This means you can't do things like:

           [(string, int)]   # variable sized types must be last
           [(int, string)]   # assuming string is variable sized
           [(int, [(bool,)]] # no way to tell where the group barriers are

      Returns:
        The number of bytes this group uses, or None if it's variable sized
    """
    variable_sized_atoms = []
    group_size = 0
    for atom in self._atoms:
      if atom.FixedSize():
        group_size += atom.size
      else:
        variable_sized_atoms.append(atom)

    if len(variable_sized_atoms) > 1:
      raise PidStructureException(
        'More than one variable size field in %s: %s' %
        (self.name, variable_sized_atoms))

    if not variable_sized_atoms:
      # The group is of a fixed size, this means we don't care how many times
      # it's repeated.
      return group_size

    # for now we only support the case where the variable sized field is the
    # last one
    if variable_sized_atoms[0] != self._atoms[-1]:
      raise PidStructureException(
        'The variable sized field %s must be the last one' %
        variable_sized_atoms[0].name)

    # It's impossible to unpack groups of variable length data without more
    # information.
    if self.min != 1 and self.max != 1:
      raise PidStructureException(
        "Repeated groups can't contain variable length data")
    return None

  def FixedSize(self):
    """This is true if we know the exact size of the group and min == max.

      Obviously this is unlikely.
    """
    can_determine_size = True
    for atom in self._atoms:
      if not atom.FixedSize():
        can_determine_size = False
        break
    return (can_determine_size and self._min is not None and
            self._min == self._max)

  @property
  def size(self):
    # only valid if FixedSize() == True
    return self.min

  def Pack(self, args):
    """Pack the args into binary data.

    Args:
      args: A list of string.

    Returns:
      binary data
    """
    if self._group_size is None:
      # variable length data, work out the fixed length portion first
      data = []
      arg_offset = 0
      for atom in self._atoms[0:-1]:
        chunk, args_consumed = atom.Pack(args[arg_offset:])
        data.append(chunk)
        arg_offset += args_consumed

      # what remains is for the variable length section
      chunk, args_used = self._atoms[-1].Pack(args[arg_offset:])
      arg_offset += args_used
      data.append(chunk)

      if arg_offset < len(args):
        raise ArgsValidationError('Too many arguments, expected %d, got %d' %
                                  (arg_offset, len(args)))

      return b''.join(data), arg_offset

    elif self._group_size == 0:
      return b'', 0
    else:
      # this could be groups of fields, but we don't support that yet
      data = []
      arg_offset = 0
      for atom in self._atoms:
        chunk, args_consumed = atom.Pack(args[arg_offset:])
        data.append(chunk)
        arg_offset += args_consumed

      if arg_offset < len(args):
        raise ArgsValidationError('Too many arguments, expected %d, got %d' %
                                  (arg_offset, len(args)))
      return b''.join(data), arg_offset

  def Unpack(self, data):
    """Unpack binary data.

    Args:
      data: The binary data

    Returns:
      A list of dicts.
    """
    # we've already performed checks in _VerifyStructure so we can rely on
    # self._group_size
    data_size = len(data)
    if self._group_size is None:
      total_size = 0
      for atom in self._atoms[0:-1]:
        total_size += atom.size

      if data_size < total_size:
          raise UnpackException(
            'Response too small, required %d, only got %d' % (
              total_size, data_size))

      output, used = self._UnpackFixedLength(self._atoms[0:-1], data)

      # what remains is for the variable length section
      variable_sized_atom = self._atoms[-1]
      data = data[used:]
      output[variable_sized_atom.name] = variable_sized_atom.Unpack(data)
      return [output]
    elif self._group_size == 0:
      if data_size > 0:
        raise UnpackException('Expected 0 bytes but got %d' % data_size)
      return [{}]
    else:
      # groups of fixed length data
      if data_size % self._group_size:
        raise UnpackException(
            'Data size issue for %s (%s), data size %d, "%s" group size %d' %
            (self.name, self.__str__, data_size, data, self._group_size))

      group_count = data_size / self._group_size
      if self.max is not None and group_count > self.max:
        raise UnpackException(
            'Too many repeated group_count for %s (%s), limit is %d, found %d' %
            (self.name, self.__str__, self.max, group_count))

      if self.min is not None and group_count < self.min:
        raise UnpackException(
            'Too few repeated group_count for %s (%s), limit is %d, found %d' %
            (self.name, self.__str__, self.min, group_count))

      offset = 0
      groups = []
      while offset + self._group_size <= data_size:
        group = self._UnpackFixedLength(
            self._atoms,
            data[offset:offset + self._group_size])[0]
        groups.append(group)
        offset += self._group_size
      return groups

  def GetDescription(self, indent=0):
    names = []
    output = []

    for atom in self._atoms:
      names.append('<%s>' % atom.name)
      output.append(atom.GetDescription(indent=2))

    return ' '.join(names), '\n'.join(output)

  def _UnpackFixedLength(self, atoms, data):
    """Unpack a list of atoms of a known, fixed size.

    Args:
      atoms: A list of atoms, must all have FixedSize() == True.
      data: The binary data.

    Returns:
      A tuple in the form (output_dict, data_consumed)
    """
    output = {}
    offset = 0
    for atom in atoms:
      size = atom.size
      output[atom.name] = atom.Unpack(data[offset:offset + size])
      offset += size
    return output, offset

  def __str__(self):
    return ('Group: atoms: %s, [%s, %s]' %
            (str(self._atoms), self.min, self.max))


# These are validators which can be applied before a request is sent
def RootDeviceValidator(args):
  """Ensure the sub device is the root device."""
  if args.get('sub_device') != ROOT_DEVICE:
    print("Can't send GET %s to non root sub devices" % args['pid'].name,
          file=sys.stderr)
    return False
  return True


def SubDeviceValidator(args):
  """Ensure the sub device is in the range 0 - 512 or 0xffff."""
  sub_device = args.get('sub_device')
  if (sub_device is None or
      (sub_device > MAX_VALID_SUB_DEVICE and sub_device != ALL_SUB_DEVICES)):
    print("%s isn't a valid sub device" % sub_device, file=sys.stderr)
    return False
  return True


def NonBroadcastSubDeviceValidator(args):
  """Ensure the sub device is in the range 0 - 512."""
  sub_device = args.get('sub_device')
  if (sub_device is None or sub_device > MAX_VALID_SUB_DEVICE):
    print("Sub device %s needs to be between 0 and 512" % sub_device,
          file=sys.stderr)
    return False
  return True


def SpecificSubDeviceValidator(args):
  """Ensure the sub device is in the range 1 - 512."""
  sub_device = args.get('sub_device')
  if (sub_device is None or sub_device == ROOT_DEVICE or
      sub_device > MAX_VALID_SUB_DEVICE):
    print("Sub device %s needs to be between 1 and 512" % sub_device,
          file=sys.stderr)
    return False
  return True


class PidStore(object):
  """The class which holds information about all the PIDs."""
  def __init__(self):
    self._pid_store = Pids_pb2.PidStore()
    self._pids = {}
    self._name_to_pid = {}
    self._manufacturer_pids = {}
    self._manufacturer_names_to_pids = {}
    self._manufacturer_id_to_name = {}

  def Load(self, pid_files, validate=True):
    """Load a PidStore from a list of files.

    Args:
      pid_files: A list of PID files on disk to load.  If
      a file "overrides.proto" is in the list it will be loaded
      last and its PIDs override any previously loaded.
      validate: When True, enable strict checking.
    """
    self._pid_store.Clear()

    raw_list = pid_files
    pid_files = []
    override_file = None

    for f in raw_list:
      if os.path.basename(f) == OVERRIDE_FILE_NAME:
        override_file = f
        continue
      if os.path.basename(f) == MANUFACTURER_NAMES_FILE_NAME:
        continue
      pid_files.append(f)

    for pid_file in pid_files:
      self.LoadFile(pid_file, validate)
    if override_file is not None:
      self.LoadFile(override_file, validate, True)

  def LoadFile(self, pid_file_name, validate, override=False):
    """Load a pid file."""
    pid_file = open(pid_file_name, 'r')
    lines = pid_file.readlines()
    pid_file.close()

    try:
      text_format.Merge('\n'.join(lines), self._pid_store)
    except text_format.ParseError as e:
      raise InvalidPidFormat(str(e))
    for pid_pb in self._pid_store.pid:
      if override:  # if processing overrides delete any conflicting entry
        old = self._pids.pop(pid_pb.value, None)
        if old is not None:
          del self._name_to_pid[old.name]
      if validate:
        if ((pid_pb.value >= RDMConstants.RDM_MANUFACTURER_PID_MIN) and
            (pid_pb.value <= RDMConstants.RDM_MANUFACTURER_PID_MAX)):
          raise InvalidPidFormat('0x%04hx between 0x%04hx and 0x%04hx in %s' %
                                 (pid_pb.value,
                                  RDMConstants.RDM_MANUFACTURER_PID_MIN,
                                  RDMConstants.RDM_MANUFACTURER_PID_MAX,
                                  pid_file_name))
        if pid_pb.value in self._pids:
          raise InvalidPidFormat('0x%04hx listed more than once in %s' %
                                 (pid_pb.value, pid_file_name))
        if pid_pb.name in self._name_to_pid:
          raise InvalidPidFormat('%s listed more than once in %s' %
                                 (pid_pb.name, pid_file_name))

      pid = self._PidProtoToObject(pid_pb)
      self._pids[pid.value] = pid
      self._name_to_pid[pid.name] = pid

    for manufacturer in self._pid_store.manufacturer:
      pid_dict = self._manufacturer_pids.setdefault(
          manufacturer.manufacturer_id,
          {})
      name_dict = self._manufacturer_names_to_pids.setdefault(
          manufacturer.manufacturer_id,
          {})

      self._manufacturer_id_to_name[manufacturer.manufacturer_id] = (
          manufacturer.manufacturer_name)

      for pid_pb in manufacturer.pid:
        if override:  # if processing overrides delete any conflicting entry
          old = pid_dict.pop(pid_pb.value, None)
          if old is not None:
            del name_dict[old.name]
        if validate:
          if ((pid_pb.value < RDMConstants.RDM_MANUFACTURER_PID_MIN) or
              (pid_pb.value > RDMConstants.RDM_MANUFACTURER_PID_MAX)):
            raise InvalidPidFormat(
              'Manufacturer pid 0x%04hx not between 0x%04hx and 0x%04hx' %
              (pid_pb.value,
               RDMConstants.RDM_MANUFACTURER_PID_MIN,
               RDMConstants.RDM_MANUFACTURER_PID_MAX))
          if pid_pb.value in pid_dict:
            raise InvalidPidFormat(
                '0x%04hx listed more than once for 0x%04hx in %s' % (
                  pid_pb.value, manufacturer.manufacturer_id, pid_file_name))
          if pid_pb.name in name_dict:
            raise InvalidPidFormat(
                '%s listed more than once for %s in %s' % (
                  pid_pb.name, manufacturer, pid_file_name))
        pid = self._PidProtoToObject(pid_pb)
        pid_dict[pid.value] = pid
        name_dict[pid.name] = pid

    # we no longer need the protobuf representation
    self._pid_store.Clear()

  def Pids(self):
    """Returns a list of all PIDs. Manufacturer PIDs aren't included.

    Returns:
      A list of Pid objects.
    """
    return list(self._pids.values())

  def ManufacturerPids(self, esta_id):
    """Return a list of all Manufacturer PIDs for a given esta_id.

    Args:
      esta_id: The 2-byte esta / manufacturer ID.

    Returns:
      A list of Pid objects.
    """
    return list(self._manufacturer_pids.get(esta_id, {}).values())

  def GetPid(self, pid_value, esta_id=None):
    """Look up a PIDs by the 2-byte PID value.

    Args:
      pid_value: The 2-byte PID value, e.g. 0x8000
      esta_id: The 2-byte esta / manufacturer ID.

    Returns:
      A Pid object, or None if no PID was found.
    """
    pid = self._pids.get(pid_value, None)
    if not pid:
      pid = self._manufacturer_pids.get(esta_id, {}).get(
          pid_value, None)
    return pid

  def GetName(self, pid_name, esta_id=None):
    """Look up a PIDs by name.

    Args:
      pid_name: The name of the PID, e.g. 'DEVICE_INFO'
      esta_id: The 2-byte esta / manufacturer ID.

    Returns:
      A Pid object, or None if no PID was found.
    """
    pid = self._name_to_pid.get(pid_name)
    if not pid:
      pid = self._manufacturer_names_to_pids.get(esta_id, {}).get(
          pid_name, None)
    return pid

  def NameToValue(self, pid_name, esta_id=None):
    """A helper method to convert a PID name to a PID value

    Args:
      pid_name: The name of the PID, e.g. 'DEVICE_INFO'
      esta_id: The 2-byte esta / manufacturer ID.

    Returns:
      The value for this PID, or None if it wasn't found.
    """
    pid = self.GetName(pid_name, esta_id)
    if pid:
      return pid.value
    return pid

  def ManufacturerIdToName(self, esta_id):
    """A helper method to convert a manufacturer ID to a name

    Args:
      esta_id: The 2-byte esta / manufacturer ID.

    Returns:
      The name of the manufacturer, or None if it wasn't found.
    """
    return self._manufacturer_id_to_name.get(esta_id, None)

  def _PidProtoToObject(self, pid_pb):
    """Convert the protobuf representation of a PID to a PID object.

    Args:
      pid_pb: The protobuf version of the pid

    Returns:
      A PIDStore.PID object.
  """
    def BuildList(field_name):
      if not pid_pb.HasField(field_name):
        return None

      try:
        group = self._FrameFormatToGroup(getattr(pid_pb, field_name))
      except PidStructureException as e:
        raise PidStructureException(
            "The structure for the %s in %s isn't valid: %s" %
            (field_name, pid_pb.name, e))
      return group

    discovery_request = BuildList('discovery_request')
    discovery_response = BuildList('discovery_response')
    get_request = BuildList('get_request')
    get_response = BuildList('get_response')
    set_request = BuildList('set_request')
    set_response = BuildList('set_response')

    discovery_validators = []
    if pid_pb.HasField('discovery_sub_device_range'):
      discovery_validators.append(self._SubDeviceRangeToValidator(
        pid_pb.discovery_sub_device_range))
    get_validators = []
    if pid_pb.HasField('get_sub_device_range'):
      get_validators.append(self._SubDeviceRangeToValidator(
        pid_pb.get_sub_device_range))
    set_validators = []
    if pid_pb.HasField('set_sub_device_range'):
      set_validators.append(self._SubDeviceRangeToValidator(
        pid_pb.set_sub_device_range))

    return Pid(pid_pb.name,
               pid_pb.value,
               discovery_request,
               discovery_response,
               get_request,
               get_response,
               set_request,
               set_response,
               discovery_validators,
               get_validators,
               set_validators)

  def _FrameFormatToGroup(self, frame_format):
    """Convert a frame format to a group."""
    atoms = []
    for field in frame_format.field:
      atoms.append(self._FieldToAtom(field))
    return Group('', atoms, min_size=1, max_size=1)

  def _FieldToAtom(self, field):
    """Convert a PID proto field message into an atom."""
    field_name = str(field.name)
    args = {'labels': [],
            'ranges': [],
            }
    if field.HasField('max_size'):
      args['max_size'] = field.max_size
    if field.HasField('min_size'):
      args['min_size'] = field.min_size
    if field.HasField('multiplier'):
      args['multiplier'] = field.multiplier

    for label in field.label:
      args['labels'].append((label.value, label.label))

    for allowed_value in field.range:
      args['ranges'].append(Range(allowed_value.min, allowed_value.max))

    if field.type == Pids_pb2.BOOL:
      return Bool(field_name)
    elif field.type == Pids_pb2.INT8:
      return Int8(field_name, **args)
    elif field.type == Pids_pb2.UINT8:
      return UInt8(field_name, **args)
    elif field.type == Pids_pb2.INT16:
      return Int16(field_name, **args)
    elif field.type == Pids_pb2.UINT16:
      return UInt16(field_name, **args)
    elif field.type == Pids_pb2.INT32:
      return Int32(field_name, **args)
    elif field.type == Pids_pb2.UINT32:
      return UInt32(field_name, **args)
    elif field.type == Pids_pb2.INT64:
      return Int64(field_name, **args)
    elif field.type == Pids_pb2.UINT64:
      return UInt64(field_name, **args)
    elif field.type == Pids_pb2.IPV4:
      return IPV4(field_name, **args)
    elif field.type == Pids_pb2.IPV6:
      return IPV6Atom(field_name, **args)
    elif field.type == Pids_pb2.MAC:
      return MACAtom(field_name, **args)
    elif field.type == Pids_pb2.UID:
      return UIDAtom(field_name, **args)
    elif field.type == Pids_pb2.GROUP:
      if not field.field:
        raise InvalidPidFormat('Missing child fields for %s' % field_name)
      atoms = []
      for child_field in field.field:
        atoms.append(self._FieldToAtom(child_field))
      return Group(field_name, atoms, **args)
    elif field.type == Pids_pb2.STRING:
      return String(field_name, **args)

  def _SubDeviceRangeToValidator(self, range):
    """Convert a sub device range to a validator."""
    if range == Pids_pb2.ROOT_DEVICE:
      return RootDeviceValidator
    elif range == Pids_pb2.ROOT_OR_ALL_SUBDEVICE:
      return SubDeviceValidator
    elif range == Pids_pb2.ROOT_OR_SUBDEVICE:
      return NonBroadcastSubDeviceValidator
    elif range == Pids_pb2.ONLY_SUBDEVICES:
      return SpecificSubDeviceValidator


_pid_store = None


def GetStore(location=None, only_files=()):
  """Get the instance of the PIDStore.

  Args:
    location: The location to load the store from. If not specified it uses the
    location defined in PidStoreLocation.py
    only_files: Load a subset of the files in the location.

  Returns:
    An instance of PidStore.
  """
  global _pid_store
  if not _pid_store:
    _pid_store = PidStore()
    if not location:
      location = PidStoreLocation.location
    pid_files = []
    for file_name in os.listdir(location):
      if not file_name.endswith('.proto'):
        continue
      if only_files and file_name not in only_files:
        continue
      pid_files.append(os.path.join(location, file_name))
    _pid_store.Load(pid_files)

  REQUIRED_PIDS = [
      'DEVICE_INFO',
      'QUEUED_MESSAGE',
      'SUPPORTED_PARAMETERS'
  ]
  for pid in REQUIRED_PIDS:
    if not _pid_store.GetName(pid):
      raise MissingPLASAPIDs(
          'Could not find %s in PID datastore, check the directory contains '
          'the ESTA (PLASA) PIDs.' % pid)

  return _pid_store
