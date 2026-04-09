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
# rdm_compare.py
# Copyright (C) 2012 Simon Newton

import getopt
import os
import pickle
import sys
import tempfile
import textwrap
import webbrowser

from ola.UID import UID

'''Compare the RDM configurations saves with rdm_snapshot.py'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


class Error(Exception):
  """Base exception class."""


class LoadException(Error):
  """Raised when we can't write to the output file."""


def Usage():
  print(textwrap.dedent("""\
  Usage: rdm_compare.py <file1> <file2>

  Compare two RDM configurations saved with rdm_snapshot.

  Flags:
    -h, --help    Display this help message and exit."""))


def ReadFile(filename):
  try:
    f = open(filename, 'rb')
  except IOError as e:
    raise LoadException(e)

  raw_data = pickle.load(f)
  f.close()
  data = {}
  for uid, settings in raw_data.items():
    data[UID.FromString(uid)] = settings
  return data


def TransformKey(key):
  tokens = key.split('_')
  output = []
  for t in tokens:
    if t == 'dmx':
      output.append('DMX')
    else:
      output.append(t.capitalize())
  return ' '.join(output)


def Diff(configuration1, configuration2):
  """Return the added and removed devices."""
  added = set()
  changed = []
  removed = set()
  for uid, config1 in configuration1.items():
    config2 = configuration2.get(uid)
    if config2 is None:
      removed.add(uid)
      continue

    fields = ['label', 'dmx_start_address', 'personality']
    for field in fields:
      value1 = config1.get(field)
      value2 = config2.get(field)
      if value1 != value2:
        changed.append((uid, TransformKey(field), value1, value2))

  for uid in configuration2:
    if uid not in configuration1:
      added.add(uid)
  return added, changed, removed


def AddList(output, uids, title):
  if not uids:
    return
  output.append('    <h4>%s</h4>    <ul>' % title)
  for uid in uids:
    output.append('    <li>%s</li>' % uid)
  output.append('    </ul>')


def DiffInBrowser(configuration1, configuration2):
  """Diff the configurations and output a HTML page."""
  added, changed, removed = Diff(configuration1, configuration2)

  output = []
  output.append(textwrap.dedent("""\
      <html>
       <head>
        <title>RDM Comparator</title>
        <style type='text/css'>
          table {
            padding: 0px;
            margin: 0px;
            border-spacing: 0px;
            border-collapse: collapse;
          }

          table td {
            border: 1px solid #000000;
            padding: 3px 10px 3px 10px;
          }
        </style>
       </head>
       <body>
        <h3>RDM Comparator</h3>
  """))

  AddList(output, added, 'Devices Added')
  AddList(output, removed, 'Devices Removed')

  if changed:
    output.append('    <h4>Device Changes</h4>')
    output.append('    <table>')
    output.append('     <tr><th>UID</th><th>Field</th><th>Old</th><th>New</th>'
                  '</tr>')
    for row in changed:
      output.append('     </tr>')
      output.append('<td>%s</td><td>%s</td><td>%s</td><td>%s</td>' % row)
      output.append('     </tr>')
    output.append('    </table>')

  output.append(textwrap.dedent("""\
      </body>
     </html>
  """))

  fd, filename = tempfile.mkstemp('.html')
  f = os.fdopen(fd, 'w')
  f.write('\n'.join(output))
  f.close()

  # open in new tab if possible
  webbrowser.open('file://%s' % filename, new=2)


def DiffToStdout(configuration1, configuration2):
  """Diff the configurations and write to STDOUT."""
  added, changed, removed = Diff(configuration1, configuration2)
  for uid in added:
    print('Device %s was added' % uid)
  for uid in removed:
    print('Device %s was removed' % uid)

  for uid, human_field, value1, value2 in changed:
    print('%s: %s changed from %s to %s' %
          (uid, human_field, value1, value2))


def main():
  try:
    opts, args = getopt.getopt(
        sys.argv[1:],
        'hb',
        ['help', 'browser'])
  except getopt.GetoptError as err:
    print(str(err))
    Usage()
    sys.exit(2)

  use_browser = False
  if len(args) != 2:
    Usage()
    sys.exit()

  for o, a in opts:
    if o in ('-h', '--help'):
      Usage()
      sys.exit()
    elif o in ('-b', '--browser'):
      use_browser = True

  configuration1 = ReadFile(args[0])
  configuration2 = ReadFile(args[1])

  if use_browser:
    DiffInBrowser(configuration1, configuration2)
  else:
    DiffToStdout(configuration1, configuration2)


if __name__ == '__main__':
  main()
