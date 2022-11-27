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
# rdm_responder_test.py
# Copyright (C) 2010 Simon Newton

from __future__ import print_function

import datetime
import logging
import re
import sys
import textwrap
import time
from optparse import OptionParser

from ola.ClientWrapper import ClientWrapper
from ola.StringUtils import StringEscape
from ola.testing.rdm import TestDefinitions, TestRunner
from ola.testing.rdm.DMXSender import DMXSender
from ola.testing.rdm.TestState import TestState
from ola.testing.rdm.TimingStats import TimingStats
from ola.UID import UID

from ola import PidStore, Version

if sys.version_info >= (3, 0):
  try:
    raw_input
  except NameError:
    raw_input = input

'''Automated testing for RDM responders.'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


def ParseOptions():
  usage = 'Usage: %prog [options] <uid>'
  description = textwrap.dedent("""\
    Run a series of tests on a RDM responder to check the behaviour.
    This requires the OLA server to be running, and the RDM device to have been
    detected. You can confirm this by running ola_rdm_discover -u
    UNIVERSE.  This will send SET commands to the broadcast UIDs which means
    the start address, device label etc. will be changed for all devices
    connected to the responder. Think twice about running this on your
    production lighting rig.
  """)
  parser = OptionParser(usage, description=description)
  parser.add_option('-c', '--slot-count', default=10,
                    help='Number of slots to send when sending DMX.')
  parser.add_option('-d', '--debug', action='store_true',
                    help='Print debug information to assist in diagnosing '
                         'failures.')
  parser.add_option('-f', '--dmx-frame-rate', default=0,
                    type='int',
                    help='Send DMX frames at this rate in the background.')
  parser.add_option('-l', '--log', metavar='FILE',
                    help='Also log to the file named FILE.uid.timestamp.')
  parser.add_option('--list-tests', action='store_true',
                    help='Display a list of all tests')
  parser.add_option('-p', '--pid-location', metavar='DIR',
                    help='The location of the PID definitions.')
  parser.add_option('-s', '--skip-check', action='store_true',
                    help='Skip the check for multiple devices.')
  parser.add_option('-t', '--tests', metavar='TEST1,TEST2',
                    help='A comma separated list of tests to run.')
  parser.add_option('--timestamp', action='store_true',
                    help='Add timestamps to each test.')
  parser.add_option('--timing', action='store_true',
                    help='Display summary timing information')
  parser.add_option('--no-factory-defaults', action='store_true',
                    help="Don't run the SET factory defaults tests")
  parser.add_option('-w', '--broadcast-write-delay', default=0,
                    type='int',
                    help='The time in ms to wait after sending broadcast set '
                         'commands.')
  parser.add_option('-u', '--universe', default=0,
                    type='int',
                    help='The universe number to use, default is universe 0.')
  parser.add_option('--inter-test-delay', default=0,
                    type='int',
                    help='The delay in ms to wait between tests, defaults to '
                         '0.')

  options, args = parser.parse_args()

  if options.list_tests:
    return options

  if not args:
    parser.print_help()
    sys.exit(2)

  uid = UID.FromString(args[0])
  if uid is None:
    parser.print_usage()
    print('Invalid UID: %s' % args[0])
    sys.exit(2)

  options.uid = uid
  return options


class MyFilter(object):
  """Filter out the ascii coloring."""
  def filter(self, record):
    msg = record.msg
    # Use a raw string for the regex
    record.msg = re.sub(r'\x1b\[\d*m', '', str(msg))
    return True


def SetupLogging(options):
  """Setup the logging for test results."""
  level = logging.INFO

  if options.debug:
    level = logging.DEBUG

  logging.basicConfig(
      level=level,
      format='%(message)s')

  if options.log:
    file_name = '%s.%s.%d' % (options.log, options.uid, time.time())
    file_handler = logging.FileHandler(file_name, 'w')
    file_handler.addFilter(MyFilter())
    if options.debug:
      file_handler.setLevel(logging.DEBUG)
    logging.getLogger('').addHandler(file_handler)


def LogTimingParam(parameter, data):
  # if non-0 print the stats
  if data['max']:
    data['param'] = parameter
    logging.info('  %s:' % parameter)
    logging.info('    Mean: %(mean).1f, StdDev %(std).1f, Median %(median).1f'
                 % data)
    logging.info('    Min: %(min).1f, Max %(max).1f, 99%%: %(99).1f' % data)


def LogAllTimingParams(response_type, stats):
  if not stats.Count():
    return

  logging.info('%s [%d frames]' % (response_type, stats.Count()))
  LogTimingParam('Response Time', stats.ResponseTime())
  LogTimingParam('Break', stats.Break())
  LogTimingParam('Mark', stats.Mark())
  LogTimingParam('Data', stats.Data())


def DisplayTiming(timing_stats):
  """Print timing information."""
  logging.info('--------------- Response Timing ----------------')

  stats = timing_stats.GetStatsForType(TimingStats.GET)
  LogAllTimingParams('GET_RESPONSE', stats)
  stats = timing_stats.GetStatsForType(TimingStats.SET)
  LogAllTimingParams('SET_RESPONSE', stats)
  stats = timing_stats.GetStatsForType(TimingStats.DISCOVERY)
  LogAllTimingParams('DISCOVERY_RESPONSE', stats)
  stats = timing_stats.GetStatsForType(TimingStats.DUB)
  LogAllTimingParams('DISCOVERY_UNIQUE_BRANCH', stats)


def DisplaySummary(options, runner, tests, device):
  """Log a summary of the tests."""
  by_category = {}
  warnings = []
  advisories = []
  count_by_state = {}
  for test in tests:
    state = test.state
    count_by_state[state] = count_by_state.get(state, 0) + 1
    warnings.extend(test.warnings)
    advisories.extend(test.advisories)

    by_category.setdefault(test.category, {})
    by_category[test.category][state] = (
        1 + by_category[test.category].get(state, 0))

  total = sum(count_by_state.values())

  logging.info('------------------- Summary --------------------')
  now = datetime.datetime.now()
  logging.info('Test Run: %s' % now.strftime('%F %r %z'))
  logging.info('UID: %s' % options.uid)

  manufacturer_label = getattr(device, 'manufacturer_label', None)
  if manufacturer_label:
    logging.info('Manufacturer: %s' % StringEscape(manufacturer_label))

  model_description = getattr(device, 'model_description', None)
  if model_description:
    logging.info('Model Description: %s' % StringEscape(model_description))

  software_version = getattr(device, 'software_version', None)
  if software_version:
    logging.info('Software Version: %s' % software_version)

  if options.timing:
    timing_stats = runner.TimingStats()
    DisplayTiming(timing_stats)

  logging.info('------------------- Warnings --------------------')
  for warning in sorted(warnings):
    logging.info(warning)

  logging.info('------------------ Advisories -------------------')
  for advisory in sorted(advisories):
    logging.info(advisory)

  logging.info('------------------ By Category ------------------')

  for category, counts in by_category.items():
    passed = counts.get(TestState.PASSED, 0)
    total_run = (passed + counts.get(TestState.FAILED, 0))
    if total_run == 0:
      continue
    percent = 1.0 * passed / total_run
    logging.info(' %26s:   %3d / %3d     %.0f%%' %
                 (category, passed, total_run, percent * 100))

  logging.info('-------------------------------------------------')
  logging.info('%d / %d tests run, %d passed, %d failed, %d broken' % (
      total - count_by_state.get(TestState.NOT_RUN, 0),
      total,
      count_by_state.get(TestState.PASSED, 0),
      count_by_state.get(TestState.FAILED, 0),
      count_by_state.get(TestState.BROKEN, 0)))


def main():
  options = ParseOptions()

  test_classes = TestRunner.GetTestClasses(TestDefinitions)
  if len(test_classes) <= 0:
    print('Failed to find any tests to run')
    sys.exit(2)
  if options.list_tests:
    for test_name in sorted(c.__name__ for c in test_classes):
      print(test_name)
    sys.exit(0)

  SetupLogging(options)
  logging.info('OLA Responder Tests Version %s' % Version.version)
  pid_store = PidStore.GetStore(options.pid_location,
                                ('pids.proto', 'draft_pids.proto'))
  wrapper = ClientWrapper()

  global uid_ok
  uid_ok = False

  def UIDList(state, uids):
    wrapper.Stop()
    global uid_ok
    if not state.Succeeded():
      logging.error('Fetch failed: %s' % state.message)
      return

    for uid in uids:
      if uid == options.uid:
        logging.debug('Found UID %s' % options.uid)
        uid_ok = True

    if not uid_ok:
      logging.error('UID %s not found in universe %d' %
                    (options.uid, options.universe))
      return

    if len(uids) > 1:
      logging.info(
          'The following devices were detected and will be reconfigured')
      for uid in uids:
        logging.info(' %s' % uid)

      if not options.skip_check:
        logging.info('Continue ? [Y/n]')
        response = raw_input().strip().lower()
        uid_ok = response == 'y' or response == ''

  logging.debug('Fetching UID list from server')
  wrapper.Client().FetchUIDList(options.universe, UIDList)
  wrapper.Run()
  wrapper.Reset()

  if not uid_ok:
    sys.exit(1)

  test_filter = None
  if options.tests is not None:
    logging.info('Restricting tests to %s' % options.tests)
    test_filter = set(options.tests.split(','))

  logging.info(
      'Starting tests, universe %d, UID %s, broadcast write delay %dms, '
      'inter-test delay %dms' %
      (options.universe, options.uid, options.broadcast_write_delay,
       options.inter_test_delay))

  runner = TestRunner.TestRunner(options.universe,
                                 options.uid,
                                 options.broadcast_write_delay,
                                 options.inter_test_delay,
                                 pid_store,
                                 wrapper,
                                 options.timestamp)

  for test_class in test_classes:
    runner.RegisterTest(test_class)

  DMXSender(wrapper,
            options.universe,
            options.dmx_frame_rate,
            options.slot_count)

  tests, device = runner.RunTests(test_filter, options.no_factory_defaults)

  DisplaySummary(options, runner, tests, device)


if __name__ == '__main__':
  main()
