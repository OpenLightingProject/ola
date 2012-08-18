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
# TestServer.py
# Copyright (C) 2012 Ravindra Nath Kakarla

import cgi
import inspect
import json
import math
import mimetypes
import os
import pickle
import pprint
import re
import sys
import textwrap
import traceback
import urlparse
from time import time
from optparse import OptionParser, OptionGroup, OptionValueError
from wsgiref.simple_server import make_server
from wsgiref.headers import Headers
from ola.UID import UID
from ola.ClientWrapper import ClientWrapper
from ola.OlaClient import OLADNotRunningException
from ola import PidStore
from ola.testing.rdm.DMXSender import DMXSender
from ola.testing.rdm import ResponderTest
from ola.testing.rdm import TestDefinitions
from ola.testing.rdm import TestRunner
from ola.testing.rdm.TestState import TestState
from ola.testing.rdm.TestCategory import TestCategory


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
    if not self.log_results(str(uid), int(time())):
      self.response.update({'logs_disabled': True})
    else:
      self.response.update({'logs_disabled': False})

  def __is_valid_log_file(self, filename):
    regex = re.compile('[0-9a-f]{4}:[0-9a-f]{8}\.[0-9]{10}\.log$')
    if regex.match(filename) is not None:
      return True
    else:
      return False

  def download_results(self, params):
    uid = params['uid']
    timestamp = params['timestamp']
    log_name = "%s.%s.log" % (uid, timestamp)
    try:
      if not self.__is_valid_log_file(log_name):
        self.__set_response_status(False)
        self.__set_response_message('Invalid log file requested!')
      else:
        filename = os.path.abspath(os.path.join(settings['log_directory'], log_name))
        if not os.path.isfile(filename):
          self.__set_response_status(False)
          self.__set_response_message('Missing log file! Please re-run tests')
        else:
          self.is_static_request = True
          mimetype, encoding = mimetypes.guess_type(filename)
          if mimetype:
            self.headers.append(('Content-type', mimetype))
          if encoding:
            self.headers.append(('Content-encoding', encoding))

          #Force downloading of file instead of showing it on the browser
          headers = Headers(self.headers)
          headers.add_header('Content-disposition', 'attachment', filename=log_name)

          response = pickle.load(open(filename, 'rb'))
          self.output = self.format_log_data(response)
          stats = len(self.output)
          self.headers.append(('Content-length', str(stats)))
    except:
      print traceback.print_exc()

  def format_log_data(self, response):
    results_log = ''
    for result in response['test_results']:
      results_log += '%s: %s\n' % (result['definition'], result['state'])

    results_log += "\n------------------- Warnings --------------------\n\n"

    for result in response['test_results']:
      results_log += '%s: %s\n' % (result['definition'], result['warnings'])

    results_log += "\n------------------- Advisories --------------------\n\n"

    for result in response['test_results']:
      results_log += '%s: %s\n' % (result['definition'], result['advisories'])

    results_log += "\n------------------- By Category --------------------\n\n"

    stats_by_catg = response['stats_by_catg']
    for result in stats_by_catg:
      passed = int(stats_by_catg[result]['passed'])
      total = int(stats_by_catg[result]['total'])
      try:
        percent = str(passed / total * 100)
      except ZeroDivisionError:
        percent = '-'
      results_log += '\t%s: %d / %d %s%%\n' % (result, passed, total, percent)

    results_log += "-------------------------------------------------\n\n"

    stats = response['stats']

    for result in sorted(stats.keys()):
      results_log += '%d %s  ' % (stats[result], result)

    return results_log


  def log_results(self, uid, timestamp):
    """Log the results to a file.

    Args:
      uid: the UID
      timestamp: the timestamp for the logs

    Returns:
      True if we wrote the logfile, false otherwise.
    """
    filename = '%s.%d.log' % (uid, timestamp)
    filename = os.path.join(dir, settings['log_directory'], filename)

    try:
      log_file = open(filename, 'w')
    except IOError as e:
      print 'Failed to open %s: %s' % (filename, e)
      return False

    pickle.dump(self.response, log_file)
    print 'Written log file %s' % (log_file.name)
    log_file.close()
    return True

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

      stats_by_catg.setdefault(category, {})
      stats_by_catg[category].setdefault('passed', 0)
      stats_by_catg[category].setdefault('total', 0)

      if test.state == TestState.PASSED:
        passed += 1
        stats_by_catg[category]['passed'] = (1 +
          stats_by_catg[category].get('passed', 0))

        stats_by_catg[category]['total'] = (1 +
          stats_by_catg[category].get('total', 0))

      elif test.state == TestState.FAILED:
        failed += 1
        stats_by_catg[category]['total'] = (1 +
          stats_by_catg[category].get('total', 0))

      elif test.state == TestState.BROKEN:
        broken += 1
        stats_by_catg[category]['total'] = (1 +
          stats_by_catg[category].get('total', 0))

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

    universe = int(params['u'])
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
      self.response.update({'timestamp': int(time())})
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
  parser.add_option('-d', '--www_dir', default=os.path.abspath('static/'),
                    help='The root directory to serve static files.')
  parser.add_option('-l', '--log_directory',
                    default=os.path.abspath('static/logs/'),
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
