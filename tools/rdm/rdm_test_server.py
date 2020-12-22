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
# rdm_test_server.py
# Copyright (C) 2012 Ravindra Nath Kakarla & Simon Newton

from __future__ import print_function
import cgi
import json
import logging
import mimetypes
import os
import pprint
import re
import stat
import sys
import textwrap
import traceback
import urlparse

from datetime import datetime
from optparse import OptionParser
from threading import Condition, Event, Lock, Thread
from time import time
from wsgiref.simple_server import make_server
from ola.UID import UID
from ola.ClientWrapper import ClientWrapper, SelectServer
from ola.OlaClient import OlaClient, OLADNotRunningException
from ola import PidStore
from ola.testing.rdm.DMXSender import DMXSender
from ola.testing.rdm import DataLocation
from ola.testing.rdm import TestDefinitions
from ola.testing.rdm import TestLogger
from ola.testing.rdm import TestRunner
from ola.testing.rdm.ModelCollector import ModelCollector
from ola.testing.rdm.TestState import TestState


__author__ = 'ravindhranath@gmail.com (Ravindra Nath Kakarla)'


settings = {
  'PORT': 9099,
}


class Error(Exception):
  """Base exception class."""


class ServerException(Error):
  """Indicates a problem handling the request."""


class OLAFuture(object):
  def __init__(self):
    self._event = Event()
    self._data = None

  def set(self, data):
    self._data = data
    self._event.set()

  def wait(self):
    self._event.wait()

  def result(self):
    return self._data


class OLAThread(Thread):
  """The thread which runs the OLA Client."""
  def __init__(self, ola_client):
    super(OLAThread, self).__init__()
    self._client = ola_client
    self._ss = None  # Created in run()

  def run(self):
    self._ss = SelectServer()
    self._ss.AddReadDescriptor(self._client.GetSocket(),
                               self._client.SocketReady)
    self._ss.Run()
    logging.info('OLA thread finished')

  def Stop(self):
    if self._ss is None:
      logging.critical('OLAThread.Stop() called before thread was running')
      return

    logging.info('Stopping OLA thread')
    self._ss.Terminate()

  def Execute(self, cb):
    self._ss.Execute(cb)

  def FetchUniverses(self):
    return self.MakeSyncClientCall(self._client.FetchUniverses)

  def FetchUIDList(self, *args):
    return self.MakeSyncClientCall(self._client.FetchUIDList, *args)

  def RunRDMDiscovery(self, *args):
    return self.MakeSyncClientCall(self._client.RunRDMDiscovery, *args)

  def MakeSyncClientCall(self, method, *method_args):
    """Turns an async call into a sync (blocking one).

    Args:
      wrapper: the ClientWrapper object
      method: the method to call
      *method_args: Any arguments to pass to the method

    Returns:
      The arguments that would have been passed to the callback function.
    """
    future = OLAFuture()

    def Callback(*args, **kwargs):
      future.set(args)

    def RunMethod():
      method(*method_args, callback=Callback)

    self._ss.Execute(RunMethod)
    future.wait()
    return future.result()


class RDMTestThread(Thread):
  """The RDMResponder tests are closely coupled to the Wrapper (yuck!). So we
     need to run this all in a separate thread. This is all a bit of a hack and
     you'll get into trouble if multiple things are running at once...
  """
  RUNNING, COMPLETED, ERROR = range(3)
  TESTS, COLLECTOR = range(2)

  def __init__(self, pid_store, logs_directory):
    super(RDMTestThread, self).__init__()
    self._pid_store = pid_store
    self._logs_directory = logs_directory
    self._terminate = False
    self._request = None
    # Guards _terminate and _request
    self._cv = Condition()
    self._wrapper = None
    self._test_state_lock = Lock()  # Guards _test_state
    self._test_state = {}

  def Stop(self):
    self._cv.acquire()
    self._terminate = True
    self._cv.notify()
    self._cv.release()

  def ScheduleTests(self, universe, uid, test_filter, broadcast_write_delay,
                    inter_test_delay, dmx_frame_rate, slot_count):
    """Schedule the tests to be run. Callable from any thread. Callbable by any
       thread.

    Returns:
      An error message, or None if the tests were scheduled.
    """
    if not self._CheckIfConnected():
      return 'Lost connection to OLAD'

    self._cv.acquire()
    if self._request is not None:
      self._cv.release()
      return 'Existing request pending'

    self._request = lambda: self._RunTests(universe, uid, test_filter,
                                            broadcast_write_delay,
                                            inter_test_delay, dmx_frame_rate,
                                            slot_count)
    self._cv.notify()
    self._cv.release()
    return None

  def ScheduleCollector(self, universe, skip_queued_messages):
    """Schedule the collector to run on a universe. Callable by any thread.

    Returns:
      An error message, or None if the collection was scheduled.
    """
    if not self._CheckIfConnected():
      return 'Lost connection to OLAD'

    self._cv.acquire()
    if self._request is not None:
      self._cv.release()
      return 'Existing request pending'

    self._request = lambda: self._RunCollector(universe, skip_queued_messages)
    self._cv.notify()
    self._cv.release()
    return None

  def Stat(self):
    """Check the state of the tests. Callable by any thread.

    Returns:
      The status of the tests.
    """
    self._test_state_lock.acquire()
    state = dict(self._test_state)
    self._test_state_lock.release()
    return state

  def run(self):
    self._wrapper = ClientWrapper()
    self._collector = ModelCollector(self._wrapper, self._pid_store)
    while True:
      self._cv.acquire()
      if self._terminate:
        logging.info('quitting test thread')
        self._cv.release()
        return

      if self._request is not None:
        request = self._request
        self._request = None
        self._cv.release()
        request()
        continue
      # Nothing to do, go into the wait
      self._cv.wait()
      self._cv.release()

  def _UpdateStats(self, tests_completed, total_tests):
    self._test_state_lock.acquire()
    self._test_state['tests_completed'] = tests_completed
    self._test_state['total_tests'] = total_tests
    self._test_state_lock.release()

  def _RunTests(self, universe, uid, test_filter, broadcast_write_delay,
                inter_test_delay, dmx_frame_rate, slot_count):
    self._test_state_lock.acquire()
    self._test_state = {
      'action': self.TESTS,
      'tests_completed': 0,
      'total_tests': None,
      'state': self.RUNNING,
      'duration': 0,
    }
    start_time = datetime.now()
    self._test_state_lock.release()

    runner = TestRunner.TestRunner(universe, uid, broadcast_write_delay,
                                   inter_test_delay, self._pid_store,
                                   self._wrapper)

    for test in TestRunner.GetTestClasses(TestDefinitions):
      runner.RegisterTest(test)

    dmx_sender = None
    if dmx_frame_rate > 0 and slot_count > 0:
      logging.info('Starting DMXSender with slot count %d and FPS of %d' %
                   (slot_count, dmx_frame_rate))
      dmx_sender = DMXSender(self._wrapper, universe, dmx_frame_rate,
                             slot_count)

    try:
      tests, device = runner.RunTests(test_filter, False, self._UpdateStats)
    except Exception as e:
      self._test_state_lock.acquire()
      self._test_state['state'] = self.ERROR
      self._test_state['exception'] = str(e)
      self._test_state['traceback'] = traceback.format_exc()
      self._test_state_lock.release()
      return
    finally:
      if dmx_sender is not None:
        dmx_sender.Stop()

    timestamp = int(time())
    end_time = datetime.now()
    test_parameters = {
      'broadcast_write_delay': broadcast_write_delay,
      'inter_test_delay': inter_test_delay,
      'dmx_frame_rate': dmx_frame_rate,
      'dmx_slot_count': slot_count,
    }
    log_saver = TestLogger.TestLogger(self._logs_directory)
    logs_saved = True
    try:
      log_saver.SaveLog(uid, timestamp, end_time, tests, device,
                        test_parameters)
    except TestLogger.TestLoggerException:
      logs_saved = False

    self._test_state_lock.acquire()
    # We can't use total_seconds() since it requires Python 2.7
    time_delta = end_time - start_time
    self._test_state['duration'] = (
        time_delta.seconds + time_delta.days * 24 * 3600)
    self._test_state['state'] = self.COMPLETED
    self._test_state['tests'] = tests
    self._test_state['logs_saved'] = logs_saved
    self._test_state['timestamp'] = timestamp
    self._test_state['uid'] = uid
    self._test_state_lock.release()

  def _RunCollector(self, universe, skip_queued_messages):
    """Run the device model collector for a universe."""
    logging.info('Collecting for universe %d' % universe)
    self._test_state_lock.acquire()
    self._test_state = {
      'action': self.COLLECTOR,
      'state': self.RUNNING,
    }
    self._test_state_lock.release()

    try:
      output = self._collector.Run(universe, skip_queued_messages)
    except Exception as e:
      self._test_state_lock.acquire()
      self._test_state['state'] = self.ERROR
      self._test_state['exception'] = str(e)
      self._test_state['traceback'] = traceback.format_exc()
      self._test_state_lock.release()
      return

    self._test_state_lock.acquire()
    self._test_state['state'] = self.COMPLETED
    self._test_state['output'] = output
    self._test_state_lock.release()

  def _CheckIfConnected(self):
    """Check if the client is connected to olad.

    Returns:
      True if connected, False otherwise.
    """
    # TODO(simon): add this check, remember it needs locking.
    return True


class HTTPRequest(object):
  """Represents a HTTP Request."""
  def __init__(self, environ):
    self._environ = environ
    self._params = None
    self._post_params = None

  def Path(self):
    """Return the path for the request."""
    return self._environ['PATH_INFO']

  def GetParam(self, param, default=None):
    """This only returns the first value for each param.

    Args:
      param: the name of the url parameter.
      default: the value to return if the parameter wasn't supplied.

    Returns:
      The value of the url param, or None if it wasn't present.
    """
    if self._params is None:
      self._params = {}
      get_params = urlparse.parse_qs(self._environ['QUERY_STRING'])
      for p in get_params:
        self._params[p] = get_params[p][0]
    return self._params.get(param, default)

  def PostParam(self, param, default=None):
    """Lookup the value of a POST parameter.

    Args:
      param: the name of the post parameter.
      default: the value to return if the parameter wasn't supplied.

    Returns:
      The value of the post param, or None if it wasn't present.
    """
    if self._post_params is None:
      self._post_params = {}
      try:
        request_body_size = int(self._environ.get('CONTENT_LENGTH', 0))
      except (ValueError):
        request_body_size = 0

      request_body = self._environ['wsgi.input'].read(request_body_size)
      post_params = urlparse.parse_qs(request_body)
      for p in post_params:
        self._post_params[p] = post_params[p][0]
    return self._post_params.get(param, default)


class HTTPResponse(object):
  """Represents a HTTP Response."""
  OK = '200 OK'
  ERROR = '500 Error'
  DENIED = '403 Denied'
  NOT_FOUND = '404 Not Found'
  PERMANENT_REDIRECT = '301 Moved Permanently'

  def __init__(self):
    self._status = None
    self._headers = {}
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

    # Strip off /static
    path = path[len(self.PREFIX):]
    # This is important as it ensures we can't access arbitrary files
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
  """A class which handles JSON requests."""
  def HandleRequest(self, request, response):
    response.SetHeader('Cache-Control', 'no-cache')
    response.SetHeader('Content-type', 'application/json')

    try:
      json_data = self.GetJson(request, response)
      response.AppendData(json.dumps(json_data, sort_keys=True))
    except ServerException as e:
      # For JSON requests, rather than returning 500s we return the error as
      # JSON
      response.SetStatus(HTTPResponse.OK)
      json_data = {
          'status': False,
          'error': str(e),
      }
      response.AppendData(json.dumps(json_data, sort_keys=True))

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
  def __init__(self, ola_thread, pid_store):
    self._thread = ola_thread
    self._pid_store = pid_store

  def GetThread(self):
    return self._thread

  def GetPidStore(self):
    return self._pid_store

  def HandleRequest(self, request, response):
    try:
      super(OLAServerRequestHandler, self).HandleRequest(request, response)
    except OLADNotRunningException:
      response.SetStatus(HTTPResponse.OK)
      json_data = {
          'status': False,
          'error': 'The OLA Server instance is no longer running',
      }
      response.AppendData(json.dumps(json_data, sort_keys=True))


class TestDefinitionsHandler(JsonRequestHandler):
  """Return a JSON list of test definitions."""
  def GetJson(self, request, response):
    response.SetStatus(HTTPResponse.OK)
    tests = [t.__name__ for t in TestRunner.GetTestClasses(TestDefinitions)]
    return {
      'test_defs': tests,
      'status': True,
    }


class GetUniversesHandler(OLAServerRequestHandler):
  """Return a JSON list of universes."""
  def GetJson(self, request, response):
    def UniverseToJson(u):
      return {
        '_id': u.id,
        '_name': u.name,
        '_merge_mode': u.merge_mode,
      }

    status, universes = self.GetThread().FetchUniverses()
    if not status.Succeeded():
      raise ServerException('Failed to fetch universes from server')

    response.SetStatus(HTTPResponse.OK)
    return {
      'universes': [UniverseToJson(u) for u in universes],
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

    status, uids = self.GetThread().FetchUIDList(universe)
    if not status.Succeeded():
      raise ServerException('Invalid universe ID!')

    response.SetStatus(HTTPResponse.OK)
    return {
      'uids': [str(u) for u in uids],
      'nameduids': dict(
        (str(u),
         self.GetPidStore().ManufacturerIdToName(u.manufacturer_id))
        for u in uids),
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

    status, uids = self.GetThread().RunRDMDiscovery(universe, True)
    if not status.Succeeded():
      raise ServerException('Invalid universe ID!')

    response.SetStatus(HTTPResponse.OK)
    return {
      'uids': [str(u) for u in uids],
      'nameduids': dict(
        (str(u),
         self.GetPidStore().ManufacturerIdToName(u.manufacturer_id))
        for u in uids),
      'status': True,
    }


class DownloadModelDataHandler(RequestHandler):
  """Take the data in the form and return it as a downloadable file."""

  def HandleRequest(self, request, response):
    print(dir(request))
    model_data = request.PostParam('model_data') or ''
    logging.info(model_data)

    filename = 'model-data.%s.txt' % int(time())
    response.SetStatus(HTTPResponse.OK)
    response.SetHeader('Content-disposition',
                       'attachment; filename="%s"' % filename)
    response.SetHeader('Content-type', 'text/plain')
    response.SetHeader('Content-length', '%d' % len(model_data))
    response.AppendData(model_data)


class DownloadResultsHandler(RequestHandler):
  """A class which handles requests to download test results."""

  def HandleRequest(self, request, response):
    uid_param = request.GetParam('uid') or ''
    uid = UID.FromString(uid_param)
    if uid is None:
      raise ServerException('Missing uid parameter: uid')

    timestamp = request.GetParam('timestamp')
    if timestamp is None:
      raise ServerException('Missing timestamp parameter: timestamp')

    include_debug = request.GetParam('debug')
    include_description = request.GetParam('description')
    category = request.GetParam('category')
    test_state = request.GetParam('state')

    reader = TestLogger.TestLogger(settings['log_directory'])
    try:
      output = reader.ReadAndFormat(uid, timestamp, category,
                                    test_state, include_debug,
                                    include_description)
    except TestLogger.TestLoggerException as e:
      raise ServerException(e)

    filename = ('%04x-%08x.%s.txt' %
                (uid.manufacturer_id, uid.device_id, timestamp))
    response.SetStatus(HTTPResponse.OK)
    response.SetHeader('Content-disposition',
                       'attachment; filename="%s"' % filename)
    response.SetHeader('Content-type', 'text/plain')
    response.SetHeader('Content-length', '%d' % len(output))
    response.AppendData(output)


class RunTestsHandler(OLAServerRequestHandler):
  """Run the RDM tests."""
  def __init__(self, ola_thread, test_thread, pid_store):
    super(RunTestsHandler, self).__init__(ola_thread, pid_store)
    self._test_thread = test_thread

  def GetJson(self, request, response):
    """Check if this is a RunTests or StatTests request."""
    path = request.Path()
    if path == '/RunTests':
      return self.RunTests(request, response)
    if path == '/RunCollector':
      return self.RunCollector(request, response)
    elif path == '/StatTests':
      return self.StatTests(request, response)
    elif path == '/StatCollector':
      return self.StatCollector(request, response)
    else:
      logging.error('Got invalid request for %s' % path)
      raise ServerException('Invalid request')

  def StatTests(self, request, response):
    """Return the status of the running tests."""
    response.SetStatus(HTTPResponse.OK)
    status = self._test_thread.Stat()
    if status is None:
      return {}

    json_data = {'status': True}
    if status['state'] == RDMTestThread.COMPLETED:
      json_data['UID'] = str(status['uid'])
      json_data['duration'] = status['duration']
      json_data['completed'] = True
      json_data['logs_disabled'] = not status['logs_saved']
      json_data['timestamp'] = status['timestamp'],
      self._FormatTestResults(status['tests'], json_data)
    elif status['state'] == RDMTestThread.ERROR:
      json_data['completed'] = True
      json_data['exception'] = status['exception']
      json_data['traceback'] = status.get('traceback', '')
    else:
      json_data['completed'] = False
      json_data['tests_completed'] = status['tests_completed']
      json_data['total_tests'] = status['total_tests']
    return json_data

  def StatCollector(self, request, response):
    """Return the status of the running collector process."""
    response.SetStatus(HTTPResponse.OK)
    status = self._test_thread.Stat()
    if status is None:
      return {}

    json_data = {'status': True}
    if status['state'] == RDMTestThread.COMPLETED:
      json_data['completed'] = True
      json_data['output'] = pprint.pformat(status['output'])
    elif status['state'] == RDMTestThread.ERROR:
      json_data['completed'] = True
      json_data['exception'] = status['exception']
      json_data['traceback'] = status.get('traceback', '')
    else:
      json_data['completed'] = False
    return json_data

  def RunCollector(self, request, response):
    """Handle a /RunCollector request."""
    universe = self._CheckValidUniverse(request)

    skip_queued = request.GetParam('skip_queued')
    if skip_queued is None or skip_queued.lower() == 'false':
      skip_queued = False
    else:
      skip_queued = True

    ret = self._test_thread.ScheduleCollector(universe, skip_queued)
    if ret is not None:
      raise ServerException(ret)

    response.SetStatus(HTTPResponse.OK)
    return {'status': True}

  def RunTests(self, request, response):
    """Handle a /RunTests request."""
    universe = self._CheckValidUniverse(request)

    uid_param = self.RaiseExceptionIfMissing(request, 'uid')
    uid = UID.FromString(uid_param)
    if uid is None:
      raise ServerException('Invalid uid: %s' % uid_param)

    # The tests to run, None means all
    test_filter = request.GetParam('t')
    if test_filter is not None:
      if test_filter == 'all':
        test_filter = None
      else:
        test_filter = set(test_filter.split(','))

    broadcast_write_delay = request.GetParam('broadcast_write_delay')
    if broadcast_write_delay is None:
      broadcast_write_delay = 0
    try:
      broadcast_write_delay = int(broadcast_write_delay)
    except ValueError:
      raise ServerException('Invalid broadcast write delay')

    inter_test_delay = request.GetParam('inter_test_delay')
    if inter_test_delay is None:
      inter_test_delay = 0
    try:
      inter_test_delay = int(inter_test_delay)
    except ValueError:
      raise ServerException('Invalid inter-test delay')

    slot_count = request.GetParam('slot_count')
    if slot_count is None:
      slot_count = 0
    try:
      slot_count = int(slot_count)
    except ValueError:
      raise ServerException('Invalid slot count')
    if slot_count not in range(0, 513):
      raise ServerException('Slot count not in range 0..512')

    dmx_frame_rate = request.GetParam('dmx_frame_rate')
    if dmx_frame_rate is None:
      dmx_frame_rate = 0
    try:
      dmx_frame_rate = int(dmx_frame_rate)
    except ValueError:
      raise ServerException('Invalid DMX frame rate')

    ret = self._test_thread.ScheduleTests(universe, uid, test_filter,
                                          broadcast_write_delay,
                                          inter_test_delay, dmx_frame_rate,
                                          slot_count)
    if ret is not None:
      raise ServerException(ret)

    response.SetStatus(HTTPResponse.OK)
    return {'status': True}

  def _CheckValidUniverse(self, request):
    """Check that the universe parameter is present and refers to a valid
       universe.

    Args:
      request: the HTTPRequest object.

    Returns:
      The santitized universe id.

    Raises:
      ServerException if the universe isn't valid or doesn't exist.
    """
    universe_param = self.RaiseExceptionIfMissing(request, 'u')

    try:
      universe = int(universe_param)
    except ValueError:
      raise ServerException('Invalid universe parameter: u')

    status, universes = self.GetThread().FetchUniverses()
    if not status.Succeeded():
      raise ServerException('Failed to fetch universes from server')

    if universe not in [u.id for u in universes]:
      raise ServerException("Universe %d doesn't exist" % universe)
    return universe

  def _FormatTestResults(self, tests, json_data):
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


def BuildApplication(ola_thread, test_thread, pid_store):
  """Construct the application and add the handlers."""
  app = Application()
  app.RegisterHandler('/',
                      RedirectHandler('/static/rdmtests.html').HandleRequest)
  app.RegisterHandler(
      '/favicon.ico',
      RedirectHandler('/static/images/favicon.ico').HandleRequest)
  app.RegisterHandler('/GetTestDefs',
                      TestDefinitionsHandler().HandleRequest)
  app.RegisterHandler('/GetUnivInfo',
                      GetUniversesHandler(ola_thread, pid_store).HandleRequest)
  app.RegisterHandler('/GetDevices',
                      GetDevicesHandler(ola_thread, pid_store).HandleRequest)
  app.RegisterHandler('/RunDiscovery',
                      RunDiscoveryHandler(ola_thread, pid_store).HandleRequest)
  app.RegisterHandler('/DownloadResults',
                      DownloadResultsHandler().HandleRequest)
  app.RegisterHandler('/DownloadModelData',
                      DownloadModelDataHandler().HandleRequest)

  run_tests_handler = RunTestsHandler(ola_thread, test_thread, pid_store)
  app.RegisterHandler('/RunCollector', run_tests_handler.HandleRequest)
  app.RegisterHandler('/RunTests', run_tests_handler.HandleRequest)
  app.RegisterHandler('/StatCollector', run_tests_handler.HandleRequest)
  app.RegisterHandler('/StatTests', run_tests_handler.HandleRequest)
  app.RegisterRegex(r'/static/.*',
                    StaticFileHandler(settings['www_dir']).HandleRequest)
  return app


def parse_options():
  """Parse Command Line options"""
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
  parser.add_option('-p', '--pid-location', metavar='DIR',
                    help='The directory to load the PID definitions from.')
  parser.add_option('-d', '--www-dir', default=DataLocation.location,
                    help='The root directory to serve static files, this must '
                         'be absolute.')
  parser.add_option('-l', '--log-directory',
                    default=os.path.abspath('/tmp/ola-rdm-logs'),
                    help='The directory to store log files.')
  parser.add_option('--world-writeable',
                    action="store_true",
                    help='Make the log directory world writeable.')

  options, args = parser.parse_args()

  return options


def SetupLogDirectory(options):
  """Setup the log dir."""
  # Setup the log dir, or display an error
  log_directory = options.log_directory
  if not os.path.exists(log_directory):
    try:
      os.makedirs(log_directory)
      if options.world_writeable:
        stat_result = os.stat(log_directory)
        os.chmod(log_directory, stat_result.st_mode | stat.S_IWOTH)
    except OSError:
      logging.error(
          'Failed to create %s for RDM logs. Logging will be disabled.' %
          options.log_directory)
  elif not os.path.isdir(options.log_directory):
    logging.error('Log directory invalid: %s. Logging will be disabled.' %
                  options.log_directory)
  elif not os.access(options.log_directory, os.W_OK):
    logging.error(
        'Unable to write to log directory: %s. Logging will be disabled.' %
        options.log_directory)


def main():
  options = parse_options()
  settings.update(options.__dict__)
  pid_store = PidStore.GetStore(options.pid_location,
                                ('pids.proto', 'draft_pids.proto',
                                 'manufacturer_names.proto'))

  logging.basicConfig(level=logging.INFO, format='%(message)s')

  SetupLogDirectory(options)

  # Check olad status
  logging.info('Checking olad status')
  try:
    ola_client = OlaClient()
  except OLADNotRunningException:
    logging.error('Error creating connection with olad. Is it running?')
    sys.exit(127)

  ola_thread = OLAThread(ola_client)
  ola_thread.start()
  test_thread = RDMTestThread(pid_store, settings['log_directory'])
  test_thread.start()
  app = BuildApplication(ola_thread, test_thread, pid_store)

  httpd = make_server('', settings['PORT'], app.HandleRequest)
  logging.info('Running RDM Tests Server on http://%s:%s' %
               ('127.0.0.1', httpd.server_port))

  try:
    httpd.serve_forever()
  except KeyboardInterrupt:
    pass
  ola_thread.Stop()
  test_thread.Stop()
  ola_thread.join()
  test_thread.join()


if __name__ == '__main__':
  main()
