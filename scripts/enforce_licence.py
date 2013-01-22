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
# enforce_licence.py
# Copyright (C) 2013 Simon Newton

import difflib
import getopt
import glob
import os
import pprint
import re
import sys
import textwrap

CPP, PYTHON = xrange(2)

IGNORED_FILES = [
  'examples/ola-dmxconsole.cpp',
  'examples/ola-dmxmonitor.cpp',
  'include/ola/gen_callbacks.py',
  'ola/common.h',
  'olad/OlaVersion.h',
  'tools/ola_trigger/config.tab.cpp',
  'tools/ola_trigger/config.tab.h',
  'tools/ola_trigger/lex.yy.cpp',
]

def Usage(arg0):
  print textwrap.dedent("""\
  Usage: %s

  Walk the directory tree from the current directory, and make sure all .cpp
  and .h files have the appropriate Licence. The licence is determined from the
  LICENCE file in each branch of the directory tree.

    --diff               Print the diffs.
    --fix                Fix the files.
    --help               Display this message.""" % arg0)

def ParseArgs():
  """Extract the options."""
  try:
    opts, args = getopt.getopt(sys.argv[1:], '',
                               ['diff', 'fix', 'help'])
  except getopt.GetoptError, err:
    print str(err)
    Usage(sys.argv[0])
    sys.exit(2)

  help = False
  fix = False
  diff = False
  for o, a in opts:
    if o in ('--diff'):
      diff = True
    elif o in ('--fix'):
      fix = True
    elif o in ('-h', '--help'):
      Usage(sys.argv[0])
      sys.exit()

  if help:
    Usage(sys.argv[0])
    sys.exit(0)
  return diff, fix

def IgnoreFile(file_name):
  for ignored_file in IGNORED_FILES:
    if file_name.endswith(ignored_file):
      return True
  return False

def TransformLicence(licence):
  """Wrap a licence in C++ style comments,"""
  output = []
  output.append('/*')
  for l in licence:
    l = l.strip()
    if l:
      output.append(' * %s' % l)
    else:
      output.append(' *')
  output.append(' *')
  return '\n'.join(output)

def TransformCppToPythonLicence(licence):
  """Change a C++ licence to Python style"""
  lines = licence.split('\n')
  output = []
  for l in lines[1:]:
    output.append('#%s' % l[2:])
  return '\n'.join(output)

def ReplaceHeader(file_name, new_header, lang):
  f = open(file_name)
  breaks = 0
  line = f.readline()
  while line != '':
    if lang == CPP and re.match(r'^ \*\s*\n$', line):
      breaks += 1
    if lang == PYTHON and re.match(r'^#\s*\n$', line):
      breaks += 1
    if breaks == 3:
      break
    line = f.readline()

  remainder = f.read()
  f.close()

  f = open(file_name, 'w')
  f.write(new_header)
  f.write('\n')
  f.write(remainder)
  f.close()

def GetDirectoryLicences(root_dir):
  """Walk the directory tree and determine the licence for each directory."""
  LICENCE_FILE = 'LICENCE'
  licences = {}

  for dir_name, subdirs, files in os.walk(root_dir):
    # skip the root_dir since the licence file is different there
    if dir_name == root_dir:
      continue

    # don't descend into hidden dirs like .libs and .deps
    subdirs[:] = [d for d in subdirs if not d.startswith('.')]

    if LICENCE_FILE in files:
      f = open(os.path.join(dir_name, LICENCE_FILE))
      lines = f.readlines()
      f.close()
      licences[dir_name] = TransformLicence(lines)
      print 'Adding LICENCE for %s' % dir_name

    # use this licence for all subdirs
    licence = licences.get(dir_name)
    if licence is not None:
      for sub_dir in subdirs:
        licences[os.path.join(dir_name, sub_dir)] = licence
  return licences

def CheckLicenceForDir(dir_name, licence, diff, fix):
  """Check all files in a directory contain the correct licence."""
  # glob doesn't support { } so we iterate instead
  for match in ['*.h', '*.cpp']:
    for file_name in glob.glob(os.path.join(dir_name, match)):
      # skip the generated protobuf code
      if '.pb.' in file_name:
        continue
      CheckLicenceForFile(file_name, licence, CPP, diff, fix)

  for file_name in glob.glob(os.path.join(dir_name, '*.py')):
    # skip the generated protobuf code
    if file_name.endswith('__init__.py') or file_name.endswith('pb2.py'):
      continue
    python_licence = TransformCppToPythonLicence(licence)
    CheckLicenceForFile(file_name, python_licence, PYTHON, diff, fix)

def CheckLicenceForFile(file_name, licence, lang, diff, fix):
  """Check a file contains the correct licence."""
  if IgnoreFile(file_name):
    return

  f = open(file_name)
  header_size = len(licence)
  first_line = None
  if lang == PYTHON:
    first_line = f.readline()
  header = f.read(header_size)
  f.close()
  if header == licence:
    return

  if fix:
    print 'Fixing %s' % file_name
    if lang == PYTHON:
      licence = first_line + licence
    ReplaceHeader(file_name, licence, lang)
  else:
    print "File does not start with %s" % (file_name)
    if diff:
      d = difflib.Differ()
      result = list(d.compare(header.splitlines(1), licence.splitlines(1)))
      pprint.pprint(result)

def main():
  diff, fix = ParseArgs()
  licences = GetDirectoryLicences(os.getcwd())
  for dir_name, licence in licences.iteritems():
    CheckLicenceForDir(dir_name, licence, diff=diff, fix=fix)

if __name__ == '__main__':
  main()
