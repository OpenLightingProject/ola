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
# list_rdm_tests.py
# Copyright (C) 2016 Peter Newman

import getopt
import getpass
import logging
import pprint
import sys
import textwrap
from ola import PidStore
from ola.ClientWrapper import ClientWrapper

'''List RDM tests and autogenerate test code.'''

__author__ = 'Peter Newman'


def Usage():
  print textwrap.dedent("""\
  Usage: list_rdm_tests.py

  Generate RDM test names or code.

    -d, --debug               Print extra debug info.
    -h, --help                Display this help message and exit.
    -p, --pid-location        The directory to read PID definitions from.
    --include-draft           Include draft PIDs.
    --names                   Just output the test names.""")


def generate_dummy_data(size):
  if size <= 2:
    return 'foo'
  elif size <= 5:
    return 'foobar'
  elif size <= 8:
    return 'foobarbaz'
  elif size <= 11:
    return 'foobarbazqux'
  else:
    return None


def generate_class_header(commented, class_prefix, class_name, class_suffix, parent_classes):
  name = class_prefix + class_name + class_suffix
  comment = ''
  if commented:
    comment = '# '
  end = ','
  if len(parent_classes) == 1:
    end = '):'
  max_parent = max(parent_classes, key=len)
  max_end = ','
  if max_parent == parent_classes[-1]:
    max_end = '):'
  if len('%sclass %s(%s%s' % (comment, name, parent_classes[0], max_end)) <= 80:
    print('%sclass %s(%s%s' % (comment, name, parent_classes[0], end))
    if len(parent_classes) > 1:
      for parent_class in parent_classes[1:-1]:
        print('%s      %s %s,' % (comment, ' ' * len(name), parent_class))
      print('%s      %s %s):' % (comment, ' ' * len(name), parent_classes[-1]))
  else:
    print('%sclass %s(' % (comment, name))
    print('%s        %s%s' % (comment, parent_classes[0], end))
    if len(parent_classes) > 1:
      for parent_class in parent_classes[1:-1]:
        print('%s        %s,' % (comment, parent_class))
      print('%s        %s):' % (comment, parent_classes[-1]))


def main():
  try:
    opts, args = getopt.getopt(
        sys.argv[1:],
        'dhnp:',
        ['debug', 'help', 'names', 'pid-location=', 'include-draft'])
  except getopt.GetoptError, err:
    print str(err)
    Usage()
    sys.exit(2)

  universe = None
  pid_location = None
  level = logging.INFO
  names = False
  include_draft = False
  for o, a in opts:
    if o in ('-d', '--debug'):
      level = logging.DEBUG
    elif o in ('-h', '--help'):
      Usage()
      sys.exit()
    elif o in ('-n', '--names'):
      names = True
    elif o in ('--include-draft'):
      include_draft = True
    elif o in ('-p', '--pid-location',):
      pid_location = a

  logging.basicConfig(
      level=level,
      format='%(message)s')

  pid_files = ['pids.proto']
  if include_draft:
    pid_files.append('draft_pids.proto')

  pid_store = PidStore.GetStore(pid_location, pid_files)
  for pid in pid_store.Pids():
    #print(pid.name)
    #if (pid.name != 'RESET_DEVICE') and (pid.name != 'SUPPORTED_PARAMETERS'):
    #if ((pid.name != 'DMX_FAIL_MODE') and
    #    (pid.name != 'DMX_PERSONALITY') and
    #    (pid.name != 'DMX_PERSONALITY_DESCRIPTION') and
    #    (pid.name != 'INTERFACE_HARDWARE_ADDRESS_TYPE1')):
    #if (pid.name != 'INTERFACE_HARDWARE_ADDRESS_TYPE1'):
    #if (pid.name != 'IPV4_STATIC_ADDRESS'):
    #  print('Not %s' % (pid.name))
    #  continue
    #if (pid.name != 'PRESET_STATUS') and (pid.name != 'SELF_TEST_DESCRIPTION'):
    #  print('Not %s' % (pid.name))
    #  continue
    #if pid.RequestSupported(PidStore.RDM_DISCOVERY):
    #  continue

    pid_test_base_name = pid.name.lower().title().replace('_','')

    get_size = 0
    if ((pid.RequestSupported(PidStore.RDM_GET)) and
        (pid.GetRequest(PidStore.RDM_GET).HasAtoms())):
      get_size = pid.GetRequest(PidStore.RDM_GET).GetAtoms()[0].size
      #print('# Get requires %d bytes' % (get_size))

    if names:
      print('AllSubDevicesGet%s' % (pid_test_base_name))
    else:
      if pid.RequestSupported(PidStore.RDM_GET):
        generate_class_header(False, 'AllSubDevicesGet', pid_test_base_name, '',
                              ['TestMixins.AllSubDevicesGetMixin',
                               'OptionalParameterTestFixture'])
        print('  """Send a get %s to ALL_SUB_DEVICES."""' % (pid.name))
      else:
        generate_class_header(False, 'AllSubDevicesGet', pid_test_base_name, '',
                              ['TestMixins.AllSubDevicesUnsupportedGetMixin',
                               'OptionalParameterTestFixture'])
        print('  """Attempt to send a get %s to ALL_SUB_DEVICES."""' % (pid.name))
      print('  PID = \'%s\'' % (pid.name))
      if get_size > 0:
        print('  #DATA = []  # TODO(%s): Specify some suitable data, %d byte%s' % (getpass.getuser(), get_size, 's' if get_size > 1 else ''))
      print('')
      print('')

    if names:
      print('Get%s' % (pid_test_base_name))
    else:
      if pid.RequestSupported(PidStore.RDM_GET):
        generate_class_header(True, 'Get', pid_test_base_name, '',
                              ['TestMixins.',
                               'OptionalParameterTestFixture'])
        print('#   CATEGORY = TestCategory.')
        print('#   PID = \'%s\'' % (pid.name))
        print('# TODO(%s): Test get' % (getpass.getuser()))
      else:
        generate_class_header(False, 'Get', pid_test_base_name, '',
                              ['TestMixins.UnsupportedGetMixin',
                               'OptionalParameterTestFixture'])
        print('  """Attempt to GET %s."""' % (pid.name))
        print('  PID = \'%s\'' % (pid.name))
      print('')
      print('')

    if ((pid.RequestSupported(PidStore.RDM_GET)) and
        (pid.GetRequest(PidStore.RDM_GET).HasAtoms())):
      first_atom = pid.GetRequest(PidStore.RDM_GET).GetAtoms()[0]

      if (first_atom.HasRanges() and (not first_atom.ValidateRawValueInRange(0) and
          first_atom.ValidateRawValueInRange(1))):
        if names:
          print('GetZero%s' % (pid_test_base_name))
        else:
          if len(pid.GetRequest(PidStore.RDM_GET).GetAtoms()) > 1:
            generate_class_header(True, 'GetZero', pid_test_base_name, '',
                                  ['TestMixins.',
                                   'OptionalParameterTestFixture'])
            print('#   """GET %s for %s 0."""' % (pid.name, first_atom.name.replace('_',' ')))
            print('#   CATEGORY = TestCategory.ERROR_CONDITIONS')
            print('#   PID = \'%s\'' % (pid.name))
            print('# TODO(%s): Test get zero' % (getpass.getuser()))
          else:
            generate_class_header(False, 'GetZero', pid_test_base_name, '',
                                  ['TestMixins.GetZero%sMixin' % (first_atom.__class__.__name__),
                                   'OptionalParameterTestFixture'])
            print('  """GET %s for %s 0."""' % (pid.name, first_atom.name.replace('_',' ')))
            print('  PID = \'%s\'' % (pid.name))
          print('')
          print('')

      if names:
        print('Get%sWithNoData' % (pid_test_base_name))
      else:
        generate_class_header(False, 'Get', pid_test_base_name, 'WithNoData',
                              ['TestMixins.GetWithNoDataMixin',
                               'OptionalParameterTestFixture'])
        print('  """GET %s with no argument given."""' % (pid.name))
        print('  PID = \'%s\'' % (pid.name))
        print('')
        print('')

      if names:
        print('Get%sWithExtraData' % (pid_test_base_name))
      else:
        generate_class_header(False, 'Get', pid_test_base_name, 'WithExtraData',
                              ['TestMixins.GetWithDataMixin',
                               'OptionalParameterTestFixture'])
        print('  """GET %s with more than %d byte%s of data."""' % (pid.name, get_size, 's' if get_size > 1 else ''))
        print('  PID = \'%s\'' % (pid.name))
        dummy_data = generate_dummy_data(get_size)
        if dummy_data is None:
          print("  #DATA = 'foo' # TODO(%s): Specify extra data if this isn't enough" % (getpass.getuser()))
        elif dummy_data != 'foo':
          # Doesn't match default
          print("  DATA = '%s'" % (dummy_data))
        print('')
        print('')

    else:
      if names:
        print('Get%sWithData' % (pid_test_base_name))
      else:
        if pid.RequestSupported(PidStore.RDM_GET):
          generate_class_header(False, 'Get', pid_test_base_name, 'WithData',
                                ['TestMixins.GetWithDataMixin',
                                 'OptionalParameterTestFixture'])
        else:
          generate_class_header(False, 'Get', pid_test_base_name, 'WithData',
                                ['TestMixins.UnsupportedGetWithDataMixin',
                                 'OptionalParameterTestFixture'])
        print('  """GET %s with data."""' % (pid.name))
        print('  PID = \'%s\'' % (pid.name))
        print('')
        print('')

    if names:
      print('Set%s' % (pid_test_base_name))
    else:
      if pid.RequestSupported(PidStore.RDM_SET):
        generate_class_header(True, 'Set', pid_test_base_name, '',
                              ['TestMixins.',
                               'OptionalParameterTestFixture'])
        print('#   CATEGORY = TestCategory.')
        print('#   PID = \'%s\'' % (pid.name))
        print('# TODO(%s): Test set' % (getpass.getuser()))
      else:
        generate_class_header(False, 'Set', pid_test_base_name, '',
                              ['TestMixins.UnsupportedSetMixin',
                               'OptionalParameterTestFixture'])
        print('  """Attempt to SET %s."""' % (pid.name))
        print('  PID = \'%s\'' % (pid.name))
      print('')
      print('')

    set_size = 0;
    if ((pid.RequestSupported(PidStore.RDM_SET)) and
        (pid.GetRequest(PidStore.RDM_SET).HasAtoms())):
      for atom in pid.GetRequest(PidStore.RDM_SET).GetAtoms():
        set_size += atom.size
      #print('# Set requires %d bytes' % (set_size))

      first_atom = pid.GetRequest(PidStore.RDM_SET).GetAtoms()[0]

      if (first_atom.HasRanges() and (not first_atom.ValidateRawValueInRange(0) and
          first_atom.ValidateRawValueInRange(1))):
        if names:
          print('SetZero%s' % (pid_test_base_name))
        else:
          if len(pid.GetRequest(PidStore.RDM_SET).GetAtoms()) > 1:
            generate_class_header(True, 'SetZero', pid_test_base_name, '',
                                  ['TestMixins.',
                                   'OptionalParameterTestFixture'])
            print('#   """SET %s to %s 0."""' % (pid.name, first_atom.name.replace('_',' ')))
            print('#   CATEGORY = TestCategory.ERROR_CONDITIONS')
            print('#   PID = \'%s\'' % (pid.name))
            print('# TODO(%s): Test set zero' % (getpass.getuser()))
          else:
            generate_class_header(False, 'SetZero', pid_test_base_name, '',
                                  ['TestMixins.SetZero%sMixin' % (first_atom.__class__.__name__),
                                   'OptionalParameterTestFixture'])
            print('  """SET %s to %s 0."""' % (pid.name, first_atom.name.replace('_',' ')))
            print('  PID = \'%s\'' % (pid.name))
          print('')
          print('')

      if names:
        print('Set%sWithNoData' % (pid_test_base_name))
      else:
        generate_class_header(False, 'Set', pid_test_base_name, 'WithNoData',
                              ['TestMixins.SetWithNoDataMixin',
                               'OptionalParameterTestFixture'])
        print('  """Set %s command with no data."""' % (pid.name))
        print('  PID = \'%s\'' % (pid.name))
        print('')
        print('')

      if names:
        print('Set%sWithExtraData' % (pid_test_base_name))
      else:
        generate_class_header(False, 'Set', pid_test_base_name, 'WithExtraData',
                              ['TestMixins.SetWithDataMixin',
                               'OptionalParameterTestFixture'])
        print('  """Send a SET %s command with extra data."""' % (pid.name))
        print('  PID = \'%s\'' % (pid.name))
        dummy_data = generate_dummy_data(set_size)
        if dummy_data is None:
          print("  #DATA = 'foo' # TODO(%s): Specify extra data if this isn't enough" % (getpass.getuser()))
        elif dummy_data != 'foo':
          # Doesn't match default
          print("  DATA = '%s'" % (dummy_data))
        print('')
        print('')
    else:
      if names:
        print('Set%sWithData' % (pid_test_base_name))
      else:
        if pid.RequestSupported(PidStore.RDM_SET):
          generate_class_header(False, 'Set', pid_test_base_name, 'WithData',
                                ['TestMixins.SetWithDataMixin',
                                 'OptionalParameterTestFixture'])
          print('  """Send a SET %s command with unnecessary data."""' % (pid.name))
        else:
          generate_class_header(False, 'Set', pid_test_base_name, 'WithData',
                                ['TestMixins.UnsupportedSetWithDataMixin',
                                 'OptionalParameterTestFixture'])
          print('  """Attempt to SET %s with data."""' % (pid.name))
        print('  PID = \'%s\'' % (pid.name))
        print('')
        print('')

  sys.exit(0)

if __name__ == '__main__':
  main()
