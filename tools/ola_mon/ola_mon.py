#!/usr/bin/python
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
# ola_mon.py
# Copyright (C) 2010 Simon Newton

import getopt
import httplib
import rrdtool
import time
import os.path
import socket
import sys
import textwrap
import threading

DEFAULT_CONFIG = 'ola_mon.conf'
DEFAULT_PORT = 9090


class OlaFetcher(object):
  def __init__(self, host, port):
    self._host = host
    self._port = port

  def FetchVariables(self):
    """Fetch the variables from an OLAD instance.

    Returns:
      A dict of variable_name: value mappings
    """
    body = self._FetchDebug()
    if body:
      return self._ProcessDebug(body)
    return {}

  def _FetchDebug(self):
    """Fetch the contents of the debug page."""
    connection = httplib.HTTPConnection('%s:%d' % (self._host, self._port))
    try:
      connection.request('GET', '/debug')
    except socket.error:
      return None

    try:
      response = connection.getresponse()
      if response.status == 200:
        return response.read()
    except httplib.BadStatusLine:
      return None
    return None

  def _ProcessDebug(self, contents):
    """Process the contents of a debug page."""
    variables = {}

    for line in contents.split('\n'):
      if ':' not in line:
        continue
      var, data = line.split(':', 1)
      var = var.strip()
      data = data.strip()

      if data.startswith('map:'):
        # label, key_values = data.split(' ', 1)
        # _, label_name = label.split(':', 1)
        pass
      else:
        variables[var] = data
    return variables


class RRDStore(object):
  def __init__(self, variables, filename):
    self._filename = filename
    self._variables = variables

    data_sources = []
    for data_type, variable, _ in variables:
      data_sources.append('DS:%s:%s:30:0:U' % (variable, data_type))

    if not os.path.exists(filename):
      rrdtool.create(filename,
                     '--step=1',
                     data_sources,
                     'RRA:AVERAGE:0.5:1:300')

  def Update(self, timestamp, data):
    args = ['%d' % timestamp]
    for _, variable, _ in self._variables:
      value = data.get(variable, 'U')
      args.append(value)
    rrdtool.update(self._filename, ':'.join(args))


class Monitor(threading.Thread):
  def __init__(self, rrd_directory, host, port, variables):
    threading.Thread.__init__(self)
    self._fetcher = OlaFetcher(host, port)
    rrd_file = os.path.join(rrd_directory, '%s.rrd' % host)
    self._store = RRDStore(variables, rrd_file)
    self._terminate = False

  def Terminate(self):
    self._terminate = True

  def run(self):
    while not self._terminate:
      variables = self._fetcher.FetchVariables()
      now = time.time()

      self._store.Update(now, variables)
      time.sleep(1)


class Grapher(threading.Thread):
  def __init__(self, rrd_directory, output_directory, host, variables, cdefs):
    threading.Thread.__init__(self)
    self._directory = os.path.join(output_directory, host)
    if not os.path.exists(self._directory):
      os.makedirs(self._directory)

    self._rrd_file = os.path.join(rrd_directory, '%s.rrd' % host)
    self._variables = variables
    self._cdefs = cdefs
    self._terminate = False

  def Terminate(self):
    self._terminate = True

  def run(self):
    while not self._terminate:
      if os.path.exists(self._rrd_file):
        self._MakeGraphs()
      time.sleep(5)

  def _MakeGraphs(self):
    for data_type, variable, title in self._variables:
      output_file = os.path.join(self._directory, '%s.png' % variable)
      rrdtool.graph(output_file,
                    '--imgformat', 'PNG',
                    '--title', title,
                    '--start', 'end-30s',
                    'DEF:%s=%s:%s:AVERAGE' %
                      (variable, self._rrd_file, variable),
                    'LINE1:%s#FF0000' % variable)

    variables = set([x for _, x, _ in self._variables])
    for cdef_name, function, title in self._cdefs:
      output_file = os.path.join(self._directory, '%s.png' % cdef_name)
      values = function.split(',')
      used_variables = set(values).intersection(variables)
      defs = []
      for variable in used_variables:
        defs.append('DEF:%s=%s:%s:AVERAGE' %
                    (variable, self._rrd_file, variable))

      rrdtool.graph(output_file,
                    '--imgformat', 'PNG',
                    '--title', title,
                    '--start', 'end-30s',
                    defs,
                    'CDEF:%s=%s' %
                      (cdef_name, function),
                    'LINE1:%s#FF0000' % cdef_name)


def LoadConfig(config_file):
  """Load the config file.

  Args:
    config_file: path to the config

  Returns:
    A dict with the config parameters.
  """
  locals_dict = {}
  execfile(config_file, {}, locals_dict)

  keys = set(['OLAD_SERVERS', 'DATA_DIRECTORY', 'VARIABLES', 'WWW_DIRECTORY',
              'CDEFS'])
  if not keys.issubset(locals.keys()):
    print 'Invalid config file'
    sys.exit(2)

  if not len(locals['OLAD_SERVERS']):
    print 'No hosts defined'
    sys.exit(2)

  if not len(locals['VARIABLES']):
    print 'No variables defined'
    sys.exit(2)

  return locals


def Usage(binary):
  """Display the usage information."""
  print textwrap.dedent("""\
    Usage: %s [options]

    Start the OLAD monitoring system
      -h, --help   Display this help message
      -c, --config The config file to use
    """ % binary)


def main():
  try:
    opts, args = getopt.getopt(sys.argv[1:], "hc:v", ["help", "config="])
  except getopt.GetoptError, err:
    print str(err)
    Usage(sys.argv[0])
    sys.exit(2)

  config_file = DEFAULT_CONFIG
  for o, a in opts:
    if o in ("-h", "--help"):
      Usage(sys.argv[0])
      sys.exit()
    elif o in ("-c", "--config"):
      config_file = os.path.expanduser(a)
    else:
      assert False, "unhandled option"

  config = LoadConfig(config_file)
  rrd_directory = os.path.expanduser(config['DATA_DIRECTORY'])
  www_directory = os.path.expanduser(config['WWW_DIRECTORY'])
  variables = config['VARIABLES']
  cdefs = config['CDEFS']

  if not os.path.exists(rrd_directory):
    os.makedirs(rrd_directory)

  threads = []
  for host in config['OLAD_SERVERS']:
    port = DEFAULT_PORT
    if ':' in host:
      host, port = host.split(':')
    monitor = Monitor(rrd_directory, host, port, variables)
    monitor.start()
    threads.append(monitor)

    grapher = Grapher(rrd_directory, www_directory, host, variables, cdefs)
    grapher.start()
    threads.append(grapher)

  for thread in threads:
    thread.join()


if __name__ == "__main__":
  main()
