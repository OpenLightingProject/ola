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
# TestLogger.py
# Copyright (C) 2012 Simon Newton

import logging
import os
import pickle
import re

from ola import Version
from ola.testing.rdm.TestState import TestState


__author__ = 'nomis52@gmail.com (Simon Newton)'


class Error(Exception):
  """Base exception class."""


class TestLoggerException(Error):
  """Indicates a problem with the log reader."""


class TestLogger(object):
  """Reads/saves test results to/from files."""
  FILE_NAME_RE = r'[0-9a-f]{4}-[0-9a-f]{8}\.[0-9]{10}\.log$'

  def __init__(self, log_dir):
    """

    Args:
      log_dir: where to store the logs
    """
    self._log_dir = log_dir

  def UIDToString(self, uid):
    """Converts a UID to a string in the form <manufacturer>-<device>.

    This is different from the __str__() method in UID since the : causes
    problems on some filesystems.
    """
    return '%04x-%08x' % (uid.manufacturer_id, uid.device_id)

  def SaveLog(self, uid, timestamp, end_time, tests, device, test_parameters):
    """Log the results to a file.

    Args:
      uid: the UID
      timestamp: the timestamp for the logs as an int
      end_time: the datetime object when the tests completed.
      tests: The list of Test objects
      device: The device properties
      test_parameters: a dict of values representing the parameters for the
        test. Can contain things like the broadcast_write_delay, timing info
        etc.

    Raises:
      TestLoggerException if we failed to write the file.
    """
    test_results = []
    for test in tests:
      test_results.append({
          'advisories': test.advisories,
          'category': test.category.__str__(),
          'debug': test._debug,
          'definition': test.__str__(),
          'doc': test.__doc__,
          'state': test.state.__str__(),
          'warnings': test.warnings,
      })

    output = dict(test_parameters)
    output['test_results'] = test_results
    output['timestamp'] = end_time.strftime('%F %r %z')
    output['uid'] = uid
    output['version'] = Version.version
    output['properties'] = device.AsDict()
    filename = '%s.%d.log' % (self.UIDToString(uid), timestamp)
    filename = os.path.join(self._log_dir, filename)

    try:
      log_file = open(filename, 'w')
    except IOError as e:
      raise TestLoggerException(
          'Failed to write to %s: %s' % (filename, e.message))

    pickle.dump(output, log_file)
    logging.info('Wrote log file %s' % (log_file.name))
    log_file.close()

  def ReadLog(self, uid, timestamp):
    """Load the test data for this particular responder & time.

    Args:
      uid:
      timestamp:

    Returns:
      The formatted data. Don't rely on the format of this data being the same.
    """
    log_name = "%s.%s.log" % (self.UIDToString(uid), timestamp)
    if not self._CheckFilename(log_name):
      raise TestLoggerException('Invalid log file requested!')

    filename = os.path.abspath(
        os.path.join(self._log_dir, log_name))
    if not os.path.isfile(filename):
      raise TestLoggerException('Missing log file! Please re-run tests')

    try:
      f = open(filename, 'rb')
    except IOError as e:
      raise TestLoggerException(e)

    return pickle.load(f)

  def ReadAndFormat(self, uid, timestamp, category, test_state,
                    include_debug=True, include_description=True,
                    include_summary=True):
    """Read the data from the log and produce a text report.

    Args:
      uid:
      timestamp: the timestamp for the logs

    Returns:
      The text version of the results.
    """
    test_data = self.ReadLog(uid, timestamp)
    formatted_output = self._FormatData(test_data, category, test_state,
                                        include_debug, include_description,
                                        include_summary)
    return formatted_output

  def _CheckFilename(self, filename):
    """Check if a test log filename is valid."""
    return re.match(self.FILE_NAME_RE, filename) is not None

  def _FormatData(self, test_data, requested_category, requested_test_state,
                  include_debug, include_description, include_summary):
    """Format the data nicely."""
    results_log = []
    warnings = []
    advisories = []
    count_by_category = {}
    broken = 0
    failed = 0
    not_run = 0
    passed = 0

    results_log.append('OLA RDM Responder Tests\n')

    if requested_category is None or requested_category.lower() == 'all':
      requested_category = None
    if requested_test_state is None or requested_test_state.lower() == 'all':
      requested_test_state = None

    tests = test_data.get('test_results', [])
    total = len(tests)

    for test in tests:
      category = test['category']
      state = test['state']
      counts = count_by_category.setdefault(category, {'passed': 0, 'total': 0})

      if state == str(TestState.PASSED):
        counts['passed'] += 1
        counts['total'] += 1
        passed += 1
      elif state == str(TestState.NOT_RUN):
        not_run += 1
      elif state == str(TestState.FAILED):
        counts['total'] += 1
        failed += 1
      elif state == str(TestState.BROKEN):
        counts['total'] += 1
        broken += 1

      if requested_category is not None and requested_category != category:
        continue
      if requested_test_state is not None and requested_test_state != state:
        continue

      results_log.append('%s: %s' % (test['definition'], test['state'].upper()))
      if include_description:
        results_log.append(str(test['doc']))
      if include_debug:
        results_log.extend(str(s) for s in test.get('debug', []))
      results_log.append('')
      warnings.extend(str(s) for s in test.get('warnings', []))
      advisories.extend(str(s) for s in test.get('advisories', []))

    results_log.append('------------------- Summary --------------------')
    results_log.append('OLA Version: %s' % test_data['version'])
    results_log.append('Test Run: %s' % test_data['timestamp'])
    results_log.append('UID: %s' % test_data['uid'])

    manufacturer_label = test_data['properties'].get('manufacturer_label', None)
    if manufacturer_label:
      results_log.append('Manufacturer: %s' %
                         manufacturer_label.encode('string-escape'))

    model_description = test_data['properties'].get('model_description', None)
    if model_description:
      results_log.append('Model Description: %s' %
                         model_description.encode('string-escape'))

    software_version = test_data['properties'].get('software_version', None)
    if software_version:
      results_log.append('Software Version: %s' % software_version)

    results_log.append("------------------- Warnings --------------------")
    results_log.extend(warnings)
    results_log.append("------------------ Advisories -------------------")
    results_log.extend(advisories)

    if include_summary:
      results_log.append("----------------- By Category -------------------")

      for category, counts in sorted(count_by_category.items()):
        cat_passed = counts['passed']
        cat_total = counts['total']
        if cat_total == 0:
          continue
        try:
          percent = int(round(100.0 * cat_passed / cat_total))
        except ZeroDivisionError:
          percent = ' - '
        results_log.append(' %26s:   %3d / %3d    %s%%' %
                           (category, cat_passed, cat_total, percent))

      results_log.append("-------------------------------------------------")
      results_log.append('%d / %d tests run, %d passed, %d failed, %d broken' %
                         (total - not_run, total, passed, failed, broken))
    return '\n'.join(results_log)
