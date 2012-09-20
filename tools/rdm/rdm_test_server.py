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
# rdm_test_server.py
# Copyright (C) 2012 Ravindra Nath Kakarla

import cgi
import inspect
import json
import math
import mimetypes
import os
import pickle
import re
import sys
import textwrap
import traceback
import urlparse
from time import time
from optparse import OptionParser, OptionGroup, OptionValueError
from wsgiref.simple_server import make_server
from ola.UID import UID
from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OLADNotRunningException
from ola import PidStore
from ola.testing.rdm.DMXSender import DMXSender
from ola.testing.rdm import DataLocation
from ola.testing.rdm import ResponderTest
from ola.testing.rdm import TestDefinitions
from ola.testing.rdm import TestRunner
from ola.testing.rdm.TestCategory import TestCategory
from ola.testing.rdm.TestState import TestState


__author__ = 'ravindhranath@gmail.com (Ravindra Nath Kakarla)'


settings = {
  'PORT': 9099,
}

status = {
  '200': '200 OK',
  '403': '403 Forbidden',
  '404': '404 Not Found',
  '500': '500 Internal Server Error',
}

paths = {
  '/RunTests': 'run_tests',
  '/GetDevices': 'get_devices',
  '/GetUnivInfo': 'get_univ_info',
  '/GetTestDefs': 'get_test_definitions',
  '/RunDiscovery': 'run_discovery',
  '/GetTestCategories': 'get_test_categories',
  '/DownloadResults': 'download_results',
}

class Error(Exception):
  """Base exception class."""


class TestLoggerException(Error):
  """Indicates a problem with the log reader."""


class TestLogger:
  """Reads previously saved test results from a file."""
  FILE_NAME_RE = r'[0-9a-f]{4}:[0-9a-f]{8}\.[0-9]{10}\.log$'

  def __init__(self, log_dir):
    self._log_dir = log_dir

  def SaveLog(self, uid, timestamp, tests):
    """Log the results to a file.

    Args:
      uid: the UID
      timestamp: the timestamp for the logs
      tests: The list of Test objects

    Returns:
      True if we wrote the logfile, false otherwise.
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

    output = {'test_results':  test_results}
    filename = '%s.%d.log' % (uid, timestamp)
    filename = os.path.join(self._log_dir, filename)

    try:
      log_file = open(filename, 'w')
    except IOError as e:
      raise TestLoggerException(
          'Failed to write to %s: %s' % (filename, e.message))

    pickle.dump(output, log_file)
    print 'Wrote log file %s' % (log_file.name)
    log_file.close()

  def ReadContents(self, uid, timestamp):
    """

    Returns:
      A tuple in the form (contents, filename)
    """
    log_name = "%s.%s.log" % (uid, timestamp)
    if not self._CheckFilename(log_name):
      raise TestLoggerException('Invalid log file requested!')

    filename = os.path.abspath(
        os.path.join(settings['log_directory'], log_name))
    if not os.path.isfile(filename):
      raise TestLoggerException('Missing log file! Please re-run tests')

    try:
      f = open(filename, 'rb')
    except IOError as e:
      raise TestLoggerException(e)

    test_data = pickle.load(f)
    return self._FormatData(test_data), log_name

  def _CheckFilename(self, filename):
    return re.match(self.FILE_NAME_RE, filename) is not None

  def _FormatData(self, test_data):
    results_log = []
    warnings = []
    advisories = []
    count_by_category = {}
    broken = 0
    failed = 0
    not_run = 0
    passed = 0

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

      results_log.append('%s: %s' % (test['definition'], test['state'].upper()))
      results_log.append(str(test['doc']))
      results_log.extend(str(l) for l in test.get('debug', []))
      results_log.append('')
      warnings.extend(str(s) for s in test.get('warnings', []))
      advisories.extend(str(s) for s in test.get('advisories', []))

    results_log.append("------------------- Warnings --------------------")
    results_log.extend(warnings)
    results_log.append("------------------ Advisories -------------------")
    results_log.extend(advisories)
    results_log.append("----------------- By Category -------------------")

    for category, counts in sorted(count_by_category.items()):
      cat_passed = counts['passed']
      cat_total = counts['total']
      try:
        percent = cat_passed / cat_total * 100
      except ZeroDivisionError:
        percent = '-'
      results_log.append(' %26s:   %3d / %3d    %s%%' %
                         (category, cat_passed, cat_total, percent))

    results_log.append("-------------------------------------------------")
    results_log.append('%d / %d tests run, %d passed, %d failed, %d broken' % (
      total - not_run, total, passed, failed, broken))
    return '\n'.join(results_log)


"""
  An instance of this class is created to serve every request.
"""
class TestServerApplication(object):
  def __init__(self, client_wrapper, environ, start_response):
    self.environ = environ
    self.start = start_response
    self.get_params = {}
    self.response = {}
    self.output = None
    self.headers = []
    self.is_static_request = False
    self.wrapper = client_wrapper
    try:
      self.__request_handler()
    except OLADNotRunningException:
      self.status = status['500']
      self.__set_response_status(False)
      self.__set_response_message(
          'Error creating connection with olad. Is it running?')
      print traceback.print_exc()

  def __set_response_status(self, bool_value):
    if type(bool_value) == bool:
      self.response.update({'status': bool_value})

  def __set_response_message(self, message):
    if type(message) == str:
      self.response.update({'message': message})

  def __request_handler(self):
    self.request = self.environ['PATH_INFO']

    if self.request.startswith('/static/'):
      self.is_static_request = True
      self.status = status['200']
    elif self.request == '/':
      self.is_static_request = True
      self.request = '/static/rdmtests.html'
      self.status = status['200']
    elif self.request not in paths.keys():
      self.status = status['404']
    else:
      self.status = status['200']
      self.get_params = urlparse.parse_qs(self.environ['QUERY_STRING'])
      for param in self.get_params:
        self.get_params[param] = self.get_params[param][0]

    return self.__response_handler()

  def __response_handler(self):
    if self.status == status['404']:
      self.__set_response_status(False)
      self.__set_response_message('Invalid request!')

    elif self.status == status['200']:
      if self.is_static_request:
        """
          Remove the first '/' (Makes it easy to partition)
          static/foo/bar partitions to ('static', '/', 'foo/bar')
        """
        resource = self.request[1:]
        resource = resource.partition('/')[2]
        self.__static_content_handler(resource)
      else:
        try:
          self.__getattribute__(paths[self.request])(self.get_params)
        except AttributeError:
          self.status = status['500']
          self.__response_handler()
          print traceback.print_exc()

    elif self.status == status['500']:
      self.__set_response_status(False)
      self.__set_response_message('Error 500: Internal failure')

  def __static_content_handler(self, resource):
    filename = os.path.abspath(os.path.join(settings['www_dir'], resource))

    if not filename.startswith(settings['www_dir']):
      self.status = status['403']
      self.output = 'OLA TestServer: 403: Access denied!'
    elif not os.path.exists(filename) or not os.path.isfile(filename):
      self.status = status['404']
      self.output = 'OLA TestServer: 404: File does not exist!'
    elif not os.access(filename, os.R_OK):
      self.status = status['403']
      self.output = 'OLA TestServer: 403: You do not have permission to access this file.'
    else:
      mimetype, encoding = mimetypes.guess_type(filename)
      if mimetype:
        self.headers.append(('Content-type', mimetype))
      if encoding:
        self.headers.append(('Content-encoding', encoding))

      stats = os.stat(filename)
      self.headers.append(('Content-length', str(stats.st_size)))

      self.output = open(filename, 'rb').read()

  def get_test_categories(self, params):
    self.__set_response_status(True)
    self.response.update({
      'Categories': sorted(c.__str__() for c in TestCategory.Categories()),
    })

  def run_discovery(self, params):
    global UIDs
    def discovery_results(state, uids):
      global UIDs
      if state.Succeeded():
        UIDs = [uid.__str__() for uid in uids]
      else:
        UIDs = False
      self.wrapper.Stop()

    self.wrapper.Client().RunRDMDiscovery(int(params['u']), True, discovery_results)
    self.wrapper.Run()
    self.wrapper.Reset()
    if UIDs:
      self.__set_response_status(True)
      self.response.update({'uids': UIDs})
    else:
      self.__set_response_status(False)
      self.__set_response_message('Invalid Universe ID or no devices patched!')

  def __get_universes(self):
    global univs
    def format_univ_info(state, universes):
      global univs
      if state.Succeeded():
        univs = [univ.__dict__ for univ in universes]

      self.wrapper.Stop()

    self.wrapper.Client().FetchUniverses(format_univ_info)
    self.wrapper.Run()
    self.wrapper.Reset()
    return univs

  def get_univ_info(self, params):
    universes = self.__get_universes()
    self.__set_response_status(True)
    self.response.update({'universes': universes})

  def get_test_definitions(self, params):
    self.__set_response_status(True)
    tests_defs = [test.__name__ for test in TestRunner.GetTestClasses(TestDefinitions)]
    self.response.update({'test_defs': tests_defs})

  def run_tests(self, params):
    test_filter = None

    defaults = {
                'u': 0,
                'uid': None,
                'w': 0,
                'f': 0,
                'c': 128,
                't': None,
    }
    defaults.update(params)

    if defaults['uid'] is None:
      self.__set_response_status(False)
      self.__set_response_message('Missing parameter: uid')
      return

    universe = int(defaults['u'])
    if not universe in [univ['_id'] for univ in self.__get_universes()]:
      self.__set_response_status(False)
      self.__set_response_message('Universe %d doesn\'t exist' % (universe))
      return

    uid = UID.FromString(defaults['uid'])
    broadcast_write_delay = int(defaults['w'])
    dmx_frame_rate = int(defaults['f'])
    slot_count = int(defaults['c'])

    if slot_count not in range(1, 513):
      self.__set_response_status(False)
      self.__set_response_message('Invalid number of slots (expected [1-512])')
      return

    if defaults['t'] is not None:
      if defaults['t'] == 'all':
        test_filter = None
      else:
        test_filter = set(defaults['t'].split(','))

    runner = TestRunner.TestRunner(universe,
                                   uid,
                                   broadcast_write_delay,
                                   settings['pid_store'],
                                   self.wrapper)

    for test in TestRunner.GetTestClasses(TestDefinitions):
      runner.RegisterTest(test)

    dmx_sender = DMXSender(self.wrapper,
                           universe,
                           dmx_frame_rate,
                           slot_count)

    tests, device = runner.RunTests(test_filter, False)
    self.__format_test_results(tests)
    self.response.update({'UID': str(uid)})

    log_saver = TestLogger(settings['log_directory'])
    try:
      timestamp = int(time())
      log_saver.SaveLog(uid, timestamp, tests)
      self.response.update({
        'logs_disabled': False,
        'timestamp': timestamp
      })
    except TestLoggerException:
      self.response.update({'logs_disabled': True})

  def download_results(self, params):
    uid = params.get('uid', '')
    timestamp = params.get('timestamp', '')

    reader = TestLogger(settings['log_directory'])
    try:
      self.output, filename = reader.ReadContents(uid, timestamp)
    except Error as e:
      self.__set_response_status(False)
      self.__set_response_message(e.message)
      return

    size = len(self.output)
    self.is_static_request = True
    self.headers.append(('Content-type', 'text/plain'))
    self.headers.append(('Content-length', '%d' % size))
    self.headers.append(
        ('Content-disposition', 'attachment; filename="%s"' % filename))

  def __format_test_results(self, tests):
    results = []
    stats_by_catg = {}
    passed = 0
    failed = 0
    broken = 0
    not_run = 0
    for test in tests:
      state = test.state.__str__()
      category = test.category.__str__()

      stats_by_catg.setdefault(category, {'passed': 0, 'total': 0})

      if test.state == TestState.PASSED:
        passed += 1
        stats_by_catg[category]['passed'] += 1
        stats_by_catg[category]['total'] += 1

      elif test.state == TestState.FAILED:
        failed += 1
        stats_by_catg[category]['total'] += 1

      elif test.state == TestState.BROKEN:
        broken += 1
        stats_by_catg[category]['total'] += 1

      elif test.state == TestState.NOT_RUN:
        not_run += 1

      results.append({
          'definition': test.__str__(),
          'state': state,
          'category': category,
          'warnings': [cgi.escape(w) for w in test.warnings],
          'advisories': [cgi.escape(a) for a in test.advisories],
          'debug': [cgi.escape(d) for d in test._debug],
          'doc': cgi.escape(test.__doc__),
        }
      )

    stats = {
      'total': len(tests),
      'passed': passed,
      'failed': failed,
      'broken': broken,
      'not_run': not_run,
    }

    self.__set_response_status(True)
    self.response.update({
      'test_results': results,
      'stats': stats,
      'stats_by_catg': stats_by_catg,
    })

  def get_devices(self, params):
    def format_uids(state, uids):
      if state.Succeeded():
        self.__set_response_status(True)
        self.response.update({'uids': [str(uid) for uid in uids]})
      else:
        self.__set_response_status(False)
        self.__set_response_message('Invalid Universe id!')

      self.wrapper.Stop()

    try:
      universe = int(params['u'])
    except ValueError:
      self.__set_response_status(False)
      self.__set_response_message('Invalid Universe id!')
      return;

    self.wrapper.Client().FetchUIDList(universe, format_uids)
    self.wrapper.Run()
    self.wrapper.Reset()

  def __iter__(self):
    if self.is_static_request:
      self.start(self.status, self.headers)
      yield(self.output)
    else:
      self.headers.append(('Content-type', 'application/json'))
      self.start(self.status, self.headers)
      json_response = json.dumps(self.response, sort_keys = True)
      yield(json_response)

def parse_options():
  """
    Parse Command Line options
  """
  usage = 'Usage: %prog [options]'
  description = textwrap.dedent("""\
    Starts the TestServer (A simple Web Server) which run a series of tests on
    a RDM responder and displays the results in a Web UI.
    This requires the OLA server to be running, and the RDM device to have been
    detected. You can confirm this by running ola_rdm_discover -u
    UNIVERSE. This will send SET commands to the broadcast UIDs which means
    the start address, device label etc. will be changed for all devices
    connected to the responder. Think twice about running this on your
    production lighting rig.
  """)
  parser = OptionParser(usage, description=description)
  parser.add_option('-p', '--pid_store', metavar='FILE',
                    help='The file to load the PID definitions from.')
  parser.add_option('-d', '--www_dir', default=DataLocation.location,
                    help='The root directory to serve static files.')
  parser.add_option('-l', '--log_directory',
                    default=os.path.abspath('/tmp/ola-rdm-logs'),
                    help='The directory to store log files.')

  options, args = parser.parse_args()

  return options


class RequestHandler:
  """Creates a new TestServerApplication to handle each request."""
  def __init__(self, ola_wrapper):
    self._wrapper = ola_wrapper

  def HandleRequest(self, environ, start_response):
    """Create a new TestServerApplication, passing in the OLA Wrapper."""
    return TestServerApplication(self._wrapper, environ, start_response)


def main():
  options = parse_options()
  settings.update(options.__dict__)
  settings['pid_store'] = PidStore.GetStore(options.pid_store, ('pids.proto'))

  # Setup the log dir, or display an error
  if not os.path.exists(options.log_directory):
    try:
      os.makedirs(options.log_directory)
    except OSError:
      print ('Failed to create %s for RDM logs. Logging will be disabled.' %
             options.log_directory)
  elif not os.path.isdir(options.log_directory):
    print ('Log directory invalid: %s. Logging will be disabled.' %
           options.log_directory)
  elif not os.access(options.log_directory, os.W_OK):
    print ('Unable to write to log directory: %s. Logging will be disabled.' %
           options.log_directory)

  #Check olad status
  ola_wrapper = None
  print 'Checking olad status'
  try:
    ola_wrapper = ClientWrapper()
  except OLADNotRunningException:
    print 'Error creating connection with olad. Is it running?'
    sys.exit(127)

  request_handler = RequestHandler(ola_wrapper)
  httpd = make_server('', settings['PORT'], request_handler.HandleRequest)
  print "Running RDM Tests Server on %s:%s" % ('127.0.0.1', httpd.server_port)
  httpd.serve_forever()

if __name__ == '__main__':
  main()
