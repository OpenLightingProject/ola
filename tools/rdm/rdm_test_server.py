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
import json
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
}

class Error(Exception):
  """Base exception class."""


class ServerException(Error):
  """Indicates a problem handling the request."""

class TestLoggerException(Error):
  """Indicates a problem with the log reader."""


class TestLogger(object):
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


class HTTPRequest(object):
  """Represents a HTTP Request."""
  def __init__(self, environ):
    self._environ = environ
    self._params = None

  def Path(self):
    """Return the path for the request."""
    return self._environ['PATH_INFO']

  def GetParam(self, param):
    """This only returns the first value for each param.

    Args:
      param: the name of the url parameter.

    Returns:
      The value of the url param, or None if it wasn't present.
    """
    if self._params is None:
      self._params = {}
      get_params = urlparse.parse_qs(self._environ['QUERY_STRING'])
      for p in get_params:
        self._params[p] = get_params[p][0]
    return self._params.get(param, None)


class HTTPResponse(object):
  """Represents a HTTP Response."""
  OK = '200 OK'
  ERROR = '500 Error'
  DENIED = '403 Denied'
  NOT_FOUND = '404 Not Found'
  PERMANENT_REDIRECT = '301 Moved Permanently'

  def __init__(self):
    self._status = None
    self._headers = {};
    self._content_type = None
    self._data = []

  def SetStatus(self, status):
    self._status = status

  def GetStatus(self):
    return self._status

  def SetHeader(self, header, value):
    self._headers[header] = value

  def GetHeaders(self):
    headers = []
    for header, value in self._headers.iteritems():
      headers.append((header, value))
    return headers

  def AppendData(self, data):
    self._data.append(data)

  def Data(self):
    return self._data

class RequestHandler(object):
  """The base request handler class."""
  def HandleRequest(self, request, response):
    pass


class RedirectHandler(RequestHandler):
  """Serve a 301 redirect."""
  def __init__(self, new_location):
    self._new_location = new_location

  def HandleRequest(self, request, response):
    response.SetStatus(HTTPResponse.PERMANENT_REDIRECT)
    response.SetHeader('Location', self._new_location)


class StaticFileHandler(RequestHandler):
  """A class which handles requests for static files."""
  PREFIX = '/static/'

  def __init__(self, static_dir):
    self._static_dir = static_dir

  def HandleRequest(self, request, response):
    path = request.Path()
    if not path.startswith(self.PREFIX):
      response.SetStatus(HTTPResponse.NOT_FOUND)
      return

    # strip off /static
    path = path[len(self.PREFIX):]
    # This is important as it ensures we can't access arbitary files
    filename = os.path.abspath(os.path.join(self._static_dir, path))
    if (not filename.startswith(self._static_dir) or
         not os.path.exists(filename) or
         not os.path.isfile(filename)):
      response.SetStatus(HTTPResponse.NOT_FOUND)
      return
    elif not os.access(filename, os.R_OK):
      response.SetStatus(HTTPResponse.DENIED)
      return
    else:
      mimetype, encoding = mimetypes.guess_type(filename)
      if mimetype:
        response.SetHeader('Content-type', mimetype)
      if encoding:
        response.SetHeader('Content-encoding', encoding)

      stats = os.stat(filename)
      response.SetStatus(HTTPResponse.OK)
      response.SetHeader('Content-length', str(stats.st_size))
      response.AppendData(open(filename, 'rb').read())


class JsonRequestHandler(RequestHandler):
  """A class which handles Json requests."""
  def HandleRequest(self, request, response):
    response.SetHeader('Cache-Control', 'no-cache')
    response.SetHeader('Content-type', 'application/json')

    try:
      json_data = self.GetJson(request, response)
      response.AppendData(json.dumps(json_data, sort_keys = True))
    except ServerException as e:
      # for json requests, rather than returning 500s we return the error as
      # json
      response.SetStatus(HTTPResponse.OK)
      json_data = {
          'status': False,
          'error': str(e),
      }
      response.AppendData(json.dumps(json_data, sort_keys = True))

  def RaiseExceptionIfMissing(self, request, param):
    """Helper method to raise an exception if the param is missing."""
    value = request.GetParam(param)
    if value is None:
      raise ServerException('Missing parameter: %s' % param)
    return value

  def GetJson(self, request, response):
    """Subclasses implement this."""
    pass


class OLAServerRequestHandler(JsonRequestHandler):
  """Catches OLADNotRunningException and handles them gracefully."""
  def __init__(self, wrapper):
    self._wrapper = wrapper;

  def Wrapper(self):
    return self._wrapper

  def HandleRequest(self, request, response):
    try:
      super(OLAServerRequestHandler, self).HandleRequest(request, response)
    except OLADNotRunningException as e:
      response.SetStatus(HTTPResponse.OK)
      json_data = {
          'status': False,
          'error': 'The OLA Server is no longer running',
      }
      response.AppendData(json.dumps(json_data, sort_keys = True))


class TestCategoriesHandler(JsonRequestHandler):
  """Return a JSON list of test Categories."""
  def GetJson(self, request, response):
    response.SetStatus(HTTPResponse.OK)
    return {
      'Categories': sorted(c.__str__() for c in TestCategory.Categories()),
      'status': True,
    }


class TestDefinitionsHandler(JsonRequestHandler):
  """Return a JSON list of test definitions."""
  def GetJson(self, request, response):
    response.SetStatus(HTTPResponse.OK)
    tests = [t.__name__ for t in TestRunner.GetTestClasses(TestDefinitions)]
    return {
      'test_defs': tests,
      'status': True,
    }


def MakeSyncCall(wrapper, method, *method_args):
  """Turns an async call into a sync (blocking one).

  Args:
    wrapper: the ClientWrapper object
    method: the method to call
    *method_args: Any arguments to pass to the method

  Returns:
    The arguments that would have been passed to the callback function.
  """
  global args_result
  def Callback(*args, **kwargs):
    global args_result
    args_result = args
    wrapper.Stop()

  method(*method_args, callback=Callback)
  wrapper.Run()
  wrapper.Reset()
  return args_result


class GetUniversesHandler(OLAServerRequestHandler):
  """Return a JSON list of universes."""
  def GetJson(self, request, response):
    wrapper = self.Wrapper()
    status, universes = MakeSyncCall(wrapper, wrapper.Client().FetchUniverses)
    if not status.Succeeded():
      raise ServerException('Failed to fetch universes from server')

    response.SetStatus(HTTPResponse.OK)
    return {
      'universes': [u.__dict__ for u in universes],
      'status': True,
    }


class GetDevicesHandler(OLAServerRequestHandler):
  """Return a JSON list of RDM devices."""

  def GetJson(self, request, response):
    universe_param = request.GetParam('u')
    if universe_param is None:
      raise ServerException('Missing universe parameter: u')

    try:
      universe = int(universe_param)
    except ValueError:
      raise ServerException('Invalid universe parameter: u')

    wrapper = self.Wrapper()
    status, uids = MakeSyncCall(wrapper,
                                wrapper.Client().FetchUIDList,
                                universe)
    if not status.Succeeded():
      raise ServerException('Invalid universe ID!')

    response.SetStatus(HTTPResponse.OK)
    return {
      'uids':  [str(u) for u in uids],
      'status': True,
    }


class RunDiscoveryHandler(OLAServerRequestHandler):
  """Runs the RDM Discovery process."""

  def GetJson(self, request, response):
    universe_param = request.GetParam('u')
    if universe_param is None:
      raise ServerException('Missing universe parameter: u')

    try:
      universe = int(universe_param)
    except ValueError:
      raise ServerException('Invalid universe parameter: u')

    wrapper = self.Wrapper()
    status, uids = MakeSyncCall(wrapper,
                                wrapper.Client().RunRDMDiscovery,
                                universe, True)

    if not status.Succeeded():
      raise ServerException('Invalid universe ID!')

    response.SetStatus(HTTPResponse.OK)
    return {
      'uids':  [str(u) for u in uids],
      'status': True,
    }


class DownloadResultsHandler(RequestHandler):
  """A class which handles requests to download test results."""

  def HandleRequest(self, request, response):
    uid = request.GetParam('uid')
    if uid is None:
      raise ServerException('Missing uid parameter: uid')

    timestamp = request.GetParam('timestamp')
    if timestamp is None:
      raise ServerException('Missing timestamp parameter: timestamp')

    reader = TestLogger(settings['log_directory'])
    try:
      output, filename = reader.ReadContents(uid, timestamp)
    except TestLoggerException as e:
      raise ServerException(e)

    response.SetStatus(HTTPResponse.OK)
    response.SetHeader('Content-disposition',
                       'attachment; filename="%s"' % filename)
    response.SetHeader('Content-type', 'test/plain')
    response.SetHeader('Content-length', '%d' % len(output))
    response.AppendData(output)


class RunTestsHandler(OLAServerRequestHandler):
  """Run the RDM tests."""
  def __init__(self, wrapper, pid_location, logs_directory):
    super(RunTestsHandler, self).__init__(wrapper)
    self._pid_location = pid_location
    self._logs_directory = logs_directory

  def GetJson(self, request, response):
    universe_param = self.RaiseExceptionIfMissing(request, 'u')

    try:
      universe = int(universe_param)
    except ValueError:
      raise ServerException('Invalid universe parameter: u')

    wrapper = self.Wrapper()
    status, universes = MakeSyncCall(wrapper, wrapper.Client().FetchUniverses)
    if not status.Succeeded():
      raise ServerException('Failed to fetch universes from server')

    if universe not in [u.id for u in universes]:
      raise ServerException("Universe %d doesn't exist" % universe)

    uid_param = self.RaiseExceptionIfMissing(request, 'uid')
    uid = UID.FromString(uid_param)
    if uid is None:
      raise ServerException('Invalid uid: %s' % uid_param)

    # the tests to run, None means all
    test_filter = request.GetParam('t')
    if test_filter is not None:
      if test_filter == 'all':
        test_filter = None
      else:
        test_filter = set(test_filter.split(','))

    broadcast_write_delay = request.GetParam('w')
    if broadcast_write_delay is None:
      broadcast_write_delay = 0
    try:
      broadcast_write_delay = int(broadcast_write_delay)
    except ValueError:
      raise ServerException('Invalid broadcast write delay')

    slot_count = request.GetParam('c')
    if slot_count is None:
      slot_count = 0
    try:
      slot_count = int(slot_count)
    except ValueError:
      raise ServerException('Invalid slot count')
    if slot_count not in range(1, 513):
      raise ServerException('Slot count not in range 0..512')

    dmx_frame_rate = request.GetParam('f')
    if dmx_frame_rate is None:
      dmx_frame_rate = 0
    try:
      dmx_frame_rate = int(dmx_frame_rate)
    except ValueError:
      raise ServerException('Invalid DMX frame rate')

    runner = TestRunner.TestRunner(universe,
                                   uid,
                                   broadcast_write_delay,
                                   self._pid_location,
                                   wrapper)

    for test in TestRunner.GetTestClasses(TestDefinitions):
      runner.RegisterTest(test)

    dmx_sender = None
    if dmx_frame_rate > 0 and slot_count > 0:
      dmx_sender = DMXSender(wrapper, universe, dmx_frame_rate, slot_count)

    tests, device = runner.RunTests(test_filter, False)

    if dmx_sender is not None:
      dmx_sender.Stop()

    timestamp = int(time())
    json_data = {
      'UID': str(uid),
      'logs_disabled': False,
      'timestamp': timestamp,
      'status': True,
    }

    self.FormatTestResults(tests, json_data)

    log_saver = TestLogger(self._logs_directory)
    try:
      log_saver.SaveLog(uid, timestamp, tests)
    except TestLoggerException:
      json_data['logs_disabled'] = True
    response.SetStatus(HTTPResponse.OK)
    return json_data

  def FormatTestResults(self, tests, json_data):
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

    json_data.update({
      'test_results': results,
      'stats': stats,
      'stats_by_catg': stats_by_catg,
    })


class Application(object):
  """Creates a new Application."""
  def __init__(self):
    # dict of path to handler
    self._handlers = {}
    self._regex_handlers = []

  def RegisterHandler(self, path, handler):
    self._handlers[path] = handler

  def RegisterRegex(self, path_regex, handler):
    self._regex_handlers.append((path_regex, handler))

  def HandleRequest(self, environ, start_response):
    """Create a new TestServerApplication, passing in the OLA Wrapper."""
    request = HTTPRequest(environ)
    response = HTTPResponse()
    self.DispatchRequest(request, response)
    start_response(response.GetStatus(), response.GetHeaders())
    return response.Data()

  def DispatchRequest(self, request, response):
    path = request.Path()
    if path in self._handlers:
      self._handlers[path](request, response)
      return
    else:
      for pattern, handler in self._regex_handlers:
        if re.match(pattern, path):
          handler(request, response)
          return
    response.SetStatus(HTTPResponse.NOT_FOUND)


def BuildApplication(wrapper, settings):
  """Construct the application and add the handlers."""
  app = Application()
  app.RegisterHandler('/',
      RedirectHandler('/static/rdmtests.html').HandleRequest)
  app.RegisterHandler('/GetTestCategories',
      TestCategoriesHandler().HandleRequest)
  app.RegisterHandler('/GetTestDefs',
      TestDefinitionsHandler().HandleRequest)
  app.RegisterHandler('/GetUnivInfo',
      GetUniversesHandler(wrapper).HandleRequest)
  app.RegisterHandler('/GetDevices',
      GetDevicesHandler(wrapper).HandleRequest)
  app.RegisterHandler('/RunDiscovery',
      RunDiscoveryHandler(wrapper).HandleRequest)
  app.RegisterHandler('/DownloadResults',
      DownloadResultsHandler().HandleRequest)
  app.RegisterHandler('/RunTests',
      RunTestsHandler(wrapper, settings['pid_store'],
                      settings['log_directory']).HandleRequest)
  app.RegisterRegex('/static/.*',
      StaticFileHandler(settings['www_dir']).HandleRequest)
  return app


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

  app = BuildApplication(ola_wrapper, settings)
  httpd = make_server('', settings['PORT'], app.HandleRequest)
  print "Running RDM Tests Server on %s:%s" % ('127.0.0.1', httpd.server_port)
  httpd.serve_forever()

if __name__ == '__main__':
  main()
