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
# rdm_compare.py
# Copyright (C) 2012 Simon Newton

'''Compare the RDM configurations saves with rdm_snapshot.py'''

__author__ = 'nomis52@gmail.com (Simon Newton)'


import getopt
import logging
import pickle
import sys
import textwrap
from ola.UID import UID

class Error(Exception):
  """Base exception class."""

class LoadException(Error):
  """Raised when we can't write to the output file."""

def Usage():
  print textwrap.dedent("""\
  Usage: rdm_compare.py <file1> <file2>

  Compare two RDM configurations saved with rdm_snapshot.

  Flags:
    -h, --help    Display this help message and exit.""")


def ReadFile(filename):
  try:
    f = open(filename, 'rb')
  except IOError as e:
    raise LoadException(e)

  raw_data = pickle.load(f)
  f.close()
  data = {}
  for uid, settings in raw_data.iteritems():
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


def main():
  try:
    opts, args = getopt.getopt(
        sys.argv[1:],
        'h',
        ['help'])
  except getopt.GetoptError, err:
    print str(err)
    Usage()
    sys.exit(2)

  if len(args) != 2:
    Usage()
    sys.exit()

  for o, a in opts:
    if o in ('-h', '--help'):
      Usage()
      sys.exit()

  configuration1 = ReadFile(args[0])
  configuration2 = ReadFile(args[1])

  for uid, config1 in configuration1.iteritems():
    if uid in configuration2:
      config2 = configuration2[uid]

      fields = ['label', 'dmx_start_address', 'personality']
      for field in fields:
        human_field = TransformKey(field)
        value1 = config1.get(field)
        value2 = config2.get(field)
        if value1 != value2:
          print ('%s: %s changed from %s to %s' %
                 (uid, human_field, value1, value2))
    else:
      print 'Device %s was removed' % uid

  for uid in configuration2.keys():
    if uid not in configuration1:
      print 'Device %s was added' % uid


if __name__ == '__main__':
  main()
