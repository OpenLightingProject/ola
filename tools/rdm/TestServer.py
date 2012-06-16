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

import json
import sys
import textwrap
import urlparse
import inspect
import ResponderTest
import TestDefinitions
import TestRunner
import traceback
from wsgiref.simple_server import make_server
from ola import PidStore
from DMXSender import DMXSender
from ola.ClientWrapper import ClientWrapper
from TestState import TestState
from ola.UID import UID
from optparse import OptionParser, OptionGroup, OptionValueError


__author__ = 'ravindhranath@gmail.com (Ravindra Nath Kakarla)'


settings = {
  'PORT': 9999,
  'headers': [('Content-type', 'application/json')],
}

status = {
  '200': '200 OK',
  '404': '404 Not Found',
  '500': '500 Internal Server Error',
}

paths = {
  '/RunTests': 'run_tests',
  '/GetDevices': 'get_devices',
  '/GetUnivInfo': 'get_univ_info',
}

"""
  An instance of this class is created to serve every request.
"""
class TestServerApplication(object):
  def __init__(self, environ, start_response):
    self.environ = environ
    self.start = start_response
    self.get_params = {}
    self.response = {}
    try:
      self.wrapper = ClientWrapper()
      self.__request_handler()
    except:
      self.status = status['500']
      self.__set_response_status(False)
      self.__set_response_message('Error creating connection with olad. Is it running?')
      print traceback.print_exc()

  def __set_response_status(self, bool_value):
    if type(bool_value) == bool:
      self.response.update({'status': bool_value})

  def __set_response_message(self, message):
    if type(message) == str:
      self.response.update({'message': message})

  def __request_handler(self):
    self.request = self.environ['PATH_INFO']

    if self.request not in paths.keys():
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
      try:
        self.__getattribute__(paths[self.request])(self.get_params)
      except AttributeError:
        self.status = status['500']
        self.__response_handler()
        print traceback.print_exc()

    elif self.status == status['500']:
      self.__set_response_status(False)
      self.__set_response_message('Error 500: Internal failure')

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

  def run_tests(self, params):
    test_filter = None

    defaults = {
                'u': 0,
                'uid': None,
                'w': 0,
                'f': 0,
                'c': 10,
                't': None,
    }
    defaults.update(params)

    universe = int(defaults['u'])
    if not universe in [univ['_id'] for univ in self.__get_universes()]:
      self.__set_response_status(False)
      self.__set_response_message('Universe %d doesn\'t exist' % (universe))
      return

    if defaults['uid'] is None:
      self.__set_response_status(False)
      self.__set_response_message('Missing parameter: uid')
      return

    uid = UID.FromString(defaults['uid'])
    broadcast_write_delay = int(defaults['w'])
    dmx_frame_rate = int(defaults['f'])
    slot_count = int(defaults['c'])

    if defaults['t'] is not None:
      test_filter = set(defaults['t'].split(','))

    runner = TestRunner.TestRunner(universe,
                                   uid,
                                   broadcast_write_delay,
                                   settings['pid_store'],
                                   self.wrapper)

    for symbol in dir(TestDefinitions):
      obj = getattr(TestDefinitions, symbol)
      if not inspect.isclass(obj):
        continue
      if (obj == ResponderTest.ResponderTestFixture or
          obj == ResponderTest.OptionalParameterTestFixture):
        continue
      if issubclass(obj, ResponderTest.ResponderTestFixture):
        runner.RegisterTest(obj)

      dmx_sender = DMXSender(self.wrapper,
                          universe,
                          dmx_frame_rate,
                          slot_count)

    tests, device = runner.RunTests(test_filter, False)
    self.__format_test_results(tests)

  def __format_test_results(self, tests):
    tests_by_category = {}
    for test in tests:
      tests_by_category.setdefault(test.category.__str__(), []) \
                                 .append([test.__str__(), test.state.__str__()])

    self.__set_response_status(True)
    self.response.update({'test_results': tests_by_category})

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
    self.start(self.status, settings['headers'])
    yield(json.dumps(self.response))

def parse_options():
  """
    Parse Command Line options
  """
  usage = 'Usage: %prog [options]'
  description = textwrap.dedent("""\
    Starts the TestServer (A simple Web Server) which run a series of tests on a RDM responder and returns the results to Web UI.
    This requires the OLA server to be running, and the RDM device to have been
    detected. You can confirm this by running ola_rdm_discover -u
    UNIVERSE. This will send SET commands to the broadcast UIDs which means
    the start address, device label etc. will be changed for all devices
    connected to the responder. Think twice about running this on your
    production lighting rig.
  """)
  parser = OptionParser(usage, description=description)
  parser.add_option('-p', '--pid_file', metavar='FILE',
                    help='The file to load the PID definitions from.')

  options, args = parser.parse_args()

  return options

def main():
  options = parse_options()
  settings.update(options.__dict__)
  settings['pid_store'] = PidStore.GetStore(options.pid_file)
  httpd = make_server('', settings['PORT'], TestServerApplication)
  httpd.serve_forever()

if __name__ == '__main__':
  main()
