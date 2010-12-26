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
# PidStore.py
# Copyright (C) 2010 Simon Newton
# Holds all the information about RDM PIDs

"""The PID Store."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import struct
import sys
# not very Pythonesque but meh
from ola import PidStoreLocation

# Various sub device enums
ROOT_DEVICE = 0
MAX_VALID_SUB_DEVICE = 0x0200;
ALL_SUB_DEVICES = 0xffff

RDM_GET, RDM_SET = range(2)

class Pid(object):
  """A class that describes everything about a PID."""
  def __init__(self, name, value, get_request = None, get_response = None,
               set_request = None, set_response = None,
               get_validators = [], set_validators = []):
    """Create a new PID.

    Args:
      name: the human readable name
      value: the 2 byte PID value
      get_format:
      set_format:
    """
    self._name = name
    self._value = value
    self._get_request = get_request
    self._get_response = get_response
    self._set_request = set_request
    self._set_response = set_response
    self._get_validators = get_validators
    self._set_validators = set_validators

  @property
  def name(self):
    return self._name

  @property
  def value(self):
    return self._value

  def RequestSupported(self, request_type):
    if request_type == RDM_SET:
      return self._set_request is not None
    else:
      return self._get_request is not None

  def CheckArgs(self, args, request_type):
    """Check the args against the list of atoms.

    Args:
      args: A list of strings
      request_type: either RDM_GET or RDM_SET

    Returns:
      None if the args failed verification, otherwise a list of arguments,
      converted to the correct types.
    """
    if request_type == RDM_SET:
      atoms = self._set_request
    else:
      atoms = self._get_request

    if len(atoms) > len(args):
      print >> sys.stderr, 'Insufficient number of arguments'
      return None
    elif len(atoms) < len(args):
      print >> sys.stderr, 'Too many arguments'
      return None

    atoms_and_args = zip(atoms, args)
    sanitized_args = []
    for atom, arg in atoms_and_args:
      santizied_arg = atom.ValidArg(arg)
      if santizied_arg is None:
        print >> sys.stderr, (
            'Invalid value `%s` for arg `%s`' % (arg, atom.name))
        return None
      sanitized_args.append(santizied_arg)
    return sanitized_args

  def ValidateAddressing(self, args, request_type):
    if request_type == RDM_SET:
      validators = self._set_validators
    else:
      validators = self._get_validators

    args['pid'] = self
    for validator in validators:
      if not validator(args):
        return False
    return True

  def __cmp__(self, other):
    return cmp(self._value, other._value)

  def __str__(self):
    return '%s (0x%04hx)' % (self.name, self.value)

  def __hash__(self):
    return self._value

  def Pack(self, args, request_type):
    if request_type == RDM_SET:
      descriptor = self._set_request
    else:
      descriptor = self._get_request

    format = self._GenerateFormatString(descriptor)

    try:
      data = struct.pack(format, *args)
    except struct.error:
      return None
    return data

  def Unpack(self, data, request_type):
    """Unpack a message."""
    def GenerateDictFromValues(atoms, values):
      obj = {}
      for atom, value in zip(descriptor, values):
        obj[atom.name] = value
      return obj


    data_size = len(data)
    if request_type == RDM_SET:
      descriptor = self._set_response
    else:
      descriptor = self._get_response

    if isinstance(descriptor, RepeatedGroup):
      format = self._GenerateFormatString(descriptor)
      group_size = struct.calcsize(format)
      if data_size % group_size:
        return None

      groups = data_size / group_size
      if descriptor.min is not None and groups < descriptor.min:
        return None
      if descriptor.max is not None and groups > descriptor.max:
        return None

      objects = []
      i = 0;
      while i < data_size:
        values = struct.unpack_from(format, data, i)
        objects.append(GenerateDictFromValues(descriptor, values))
        i += group_size
      return objects
    else:
      format = self._GenerateFormatString(descriptor, data_size)
      try:
        values =  struct.unpack(format, data)
      except struct.error:
        return None

      return GenerateDictFromValues(descriptor, values)

  def _GenerateFormatString(self, atoms, data_size = None):
    """Generate the format string from a list of Atoms."""
    format_chars = []
    size = 0
    for atom in atoms:
      size += atom.size
      format_chars.append((atom.size, atom.char))

    format = ''.join(['!'] + ['%d%c' % f for f in format_chars])

    # This is a hack to deal with the fact that strings are variable size
    blob_size = struct.calcsize(format)
    if (data_size is not None and blob_size != data_size and
        isinstance(atoms[-1], String)):
      full_size, char = format_chars[-1]
      delta = blob_size - data_size
      if (delta < full_size):
        format_chars = (format_chars[0:-1] +
                        [(full_size - (blob_size - data_size), char)])

      format = ''.join(['!'] + ['%d%c' % f for f in format_chars])
    return format


# The following classes are used to describe RDM messages
class Atom(object):
  """The basic field in an RDM message."""
  def __init__(self, name, format_char, size = 1):
    self._name = name
    self._char = format_char
    self._size = size

  @property
  def name(self):
    return self._name

  @property
  def char(self):
    return self._char

  @property
  def size(self):
    return self._size

class Bool(Atom):
  def __init__(self, name):
    # once we have 2.6 use ? here
    super(Bool, self).__init__(name, 'c')

  def ValidArg(self, arg):
    return bool(arg)

class UInt8(Atom):
  """A single unsigned byte field."""
  def __init__(self, name):
    super(UInt8, self).__init__(name, 'B')

  def ValidArg(self, arg):
    try:
      value = int(arg)
    except ValueError:
      return None
    if value >= 0 and value <= 255:
      return value
    return None

class UInt16(Atom):
  """A two-byte unsigned field."""
  def __init__(self, name):
    super(UInt16, self).__init__(name, 'h')

  def ValidArg(self, arg):
    try:
      value = int(arg)
    except ValueError:
      return None
    if value >= 0 and value <= 0xffff:
      return value
    return None

class UInt32(Atom):
  """A four-byte unsigned field."""
  def __init__(self, name):
    super(UInt32, self).__init__(name, 'I')

  def ValidArg(self, arg):
    try:
      value = int(arg)
    except ValueError:
      return None
    if value >= 0 and value <= 0xffffffffff:
      return value
    return None

class String(Atom):
  """A string field."""
  def __init__(self, name, size = 32):
    super(String, self).__init__(name, 's', size)

  def ValidArg(self, arg):
    if len(arg) <= self.size:
      return arg
    return None



class RepeatedGroup(list):
  """A repeated group of atoms."""
  def __init__(self, atoms, **kwargs):
    super(RepeatedGroup, self).__init__(atoms)
    self._min = kwargs.get('min')
    self._max = kwargs.get('max')

  @property
  def min(self):
    return self._min

  @property
  def max(self):
    return self._max


# These are validators which can be applied before a request is sent
def RootDeviceValidator(args):
  """Ensure the sub device is the root device."""
  if args.get('sub_device') != ROOT_DEVICE:
    print >> sys.stderr, (
        "Can't send GET %s to non root sub devices" % args['pid'].name)
    return False
  return True


def SubDeviceValidator(args):
  """Ensure the sub device is in the range 0 - 512 or 0xffff."""
  sub_device = args.get('sub_device')
  if (sub_device is None or
      (sub_device > MAX_VALID_SUB_DEVICE and sub_device != ALL_SUB_DEVICES)):
    print >> sys.stderr, (
        "%s isn't a valid sub device" % sub_device)
    return False
  return True

def NonBroadcastSubDeviceValiator(args):
  """Ensure the sub device is in the range 0 - 512."""
  sub_device = args.get('sub_device')
  if (sub_device is None or sub_device > MAX_VALID_SUB_DEVICE):
    print >> sys.stderr, (
        "Sub device %s needs to be between 0 and 512" % sub_device)
    return False
  return True


def SpecificSubDeviceValidator(args):
  """Ensure the sub device is in the range 1 - 512."""
  sub_device = args.get('sub_device')
  if (sub_device is None or sub_device == ROOT_DEVICE or
      sub_device > MAX_VALID_SUB_DEVICE):
    print >> sys.stderr, (
        "Sub device %s needs to be between 1 and 512" % sub_device)
    return False
  return True


class PidStore(object):
  """The class which holds information about all the PIDs."""
  def __init__(self):
    self._pids = {}
    self._name_to_pid = {}
    self._manufacturer_pids = {}
    self._manufacturer_names_to_pids = {}

  def Load(self, file, validate = True):
    """Load a PidStore from a file.

    Args:
      file: The path to the pid store file
      validate: When True, enable strict checking.
    """
    globals = {
      'Bool': Bool,
      'Pid': Pid,
      'RepeatedGroup': RepeatedGroup,
      'UInt8': UInt8,
      'UInt16': UInt16,
      'UInt32': UInt32,
      'String': String,
      'RootDevice': RootDeviceValidator,
      'ValidSubDevice': SubDeviceValidator,
      'SpecificSubDevice': SpecificSubDeviceValidator,
      'NonBroadcastSubDevice': NonBroadcastSubDeviceValiator,
    }
    locals = {}
    execfile(file, globals, locals)
    for pid in locals['PIDS']:
      if validate:
        assert (pid.value < 0x8000 or pid.value > 0xffe0), (
          '%0x04hx between 0x8000 and 0xffdf' % pid.value)
        assert pid.value not in self._pids, (
          '0x%04hx listed more than once in %s' % (pid.value, file))
        assert pid.name not in self._name_to_pid, (
          '%s listed more than once in %s' % (pid.name, file))
      self._pids[pid.value] = pid
      self._name_to_pid[pid.name] = pid

    for manufacturer in locals['MANUFACTURER_PIDS']:
      pid_dict = self._manufacturer_pids.setdefault(manufacturer, {})
      name_dict = self._manufacturer_names_to_pids.setdefault(manufacturer, {})

      for pid in locals['MANUFACTURER_PIDS'][manufacturer]:
        if validate:
          assert (pid.value >= 0x8000 and pid.value <= 0xffdf), (
            'manufacturer pid %0x04hx not between 0x8000 and 0xffdf' %
            pid.value)
          assert pid.value not in pid_dict, (
            '0x%04hx listed more than once for 0x%04hx in %s' % (
              pid.value, manufacturer, file))
          assert pid.name not in name_dict, (
            '%s listed more than once for 0x%04hx in %s' % (
              pid.name, manufacturer, file))
        pid_dict[pid.value] = pid
        name_dict[pid.name] = pid


  def Pids(self):
    return self._pids.values()

  def ManufacturerPids(self, esta_id):
    return self._manufacturer_pids.get(esta_id, {}).values()

  def GetPid(self, pid_value, uid):
    pid = self._pids.get(pid_value, None)
    if not pid:
      pid = self._manufacturer_pids.get(uid.manufacturer_id, {}).get(
          pid_value, None)
    return pid

  def GetName(self, pid_name, uid):
    pid = self._name_to_pid.get(pid_name)
    if not pid:
      pid = self._manufacturer_names_to_pids.get(uid.manufacturer_id, {}).get(
          pid_name, None)
    return pid

  def NameToValue(self, name):
    pid = self.GetName(name)
    if pid:
      return pid.value
    return pid

  def Names(self):
    return self._name_to_pid.keys()

  def __contains__(self, pid):
    return pid in self._pids


_pid_store = None


def GetStore(location = None):
  """Get the instance of the PIDStore.

  Args:
    location: The location to load the store from. If not specified it uses the
    location defined in PidStoreLocation.py

  Returns:
    An instance of PidStore.
  """
  global _pid_store
  if not _pid_store:
    _pid_store = PidStore()
    if not location:
      location = PidStoreLocation.location
    _pid_store.Load(location)
  return _pid_store

