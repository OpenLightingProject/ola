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
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

CPP, JS, PROTOBUF, PYTHON = xrange(4)

IGNORED_FILES = [
  'common/rdm/testdata/duplicate_manufacturer.proto',
  'common/rdm/testdata/duplicate_pid_name.proto',
  'common/rdm/testdata/duplicate_pid_value.proto',
  'common/rdm/testdata/inconsistent_pid.proto',
  'common/rdm/testdata/invalid_esta_pid.proto',
  'common/rdm/testdata/test_pids.proto',
  'common/rdm/testdata/pids/pids1.proto',
  'common/rdm/testdata/pids/pids2.proto',
  'examples/ola-dmxconsole.cpp',
  'examples/ola-dmxmonitor.cpp',
  'include/ola/gen_callbacks.py',
  'olad/www/mobile.js',
  'olad/www/ola.js',
  'python/ola/PidStoreLocation.py',
  'python/ola/Version.py',
  'tools/ola_trigger/config.tab.cpp',
  'tools/ola_trigger/config.tab.h',
  'tools/ola_trigger/lex.yy.cpp',
  'tools/rdm/DataLocation.py',
  'tools/rdm/static/jquery-1.7.2.min.js',
  'tools/rdm/static/jquery-ui-1.8.21.custom.min.js',
  'tools/rdm/static/ui.multiselect.js',
]

def Usage(arg0):
  print textwrap.dedent("""\
  Usage: %s

  Walk the directory tree from the current directory, and make sure all .cpp,
  .h, .js, .proto and .py files have the appropriate Licence. The licence is
  determined from the LICENCE file in each branch of the directory tree.

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

def TransformCppToJsLicence(licence):
  """Change a C++ licence to JS style"""
  lines = licence.split('\n')
  output = []
  output.append('/**')
  for l in lines[1:]:
    l = l[2:].strip()
    if l:
      output.append(' * %s' % l)
    else:
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
    if (lang == CPP or lang == JS or lang == PROTOBUF) and \
       re.match(r'^ \*\s*\n$', line):
      breaks += 1
    if lang == PYTHON and re.match(r'^#\s*\n$', line):
      breaks += 1
    if breaks == 3:
      break
    line = f.readline()

  if breaks < 3:
    print "Couldn't find header for %s so couldn't fix it" % file_name
    f.close()
    return

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
      print 'Found LICENCE for directory %s' % dir_name

    # use this licence for all subdirs
    licence = licences.get(dir_name)
    if licence is not None:
      for sub_dir in subdirs:
        licences[os.path.join(dir_name, sub_dir)] = licence
  return licences

def CheckLicenceForDir(dir_name, licence, diff, fix):
  """Check all files in a directory contain the correct licence."""
  errors = 0
  # glob doesn't support { } so we iterate instead
  for match in ['*.h', '*.cpp']:
    for file_name in glob.glob(os.path.join(dir_name, match)):
      # skip the generated protobuf code
      if '.pb.' in file_name:
        continue
      errors += CheckLicenceForFile(file_name, licence, CPP, diff, fix)

  js_licence = TransformCppToJsLicence(licence)
  for file_name in glob.glob(os.path.join(dir_name, '*.js')):
    errors += CheckLicenceForFile(file_name, js_licence, JS, diff, fix)

  for file_name in glob.glob(os.path.join(dir_name, '*.proto')):
    errors += CheckLicenceForFile(file_name, licence, PROTOBUF, diff, fix)

  python_licence = TransformCppToPythonLicence(licence)
  for file_name in glob.glob(os.path.join(dir_name, '*.py')):
    # skip the generated protobuf code
    if file_name.endswith('__init__.py') or file_name.endswith('pb2.py'):
      continue
    errors += CheckLicenceForFile(file_name, python_licence, PYTHON, diff, fix)

  return errors

def CheckLicenceForFile(file_name, licence, lang, diff, fix):
  """Check a file contains the correct licence."""
  if IgnoreFile(file_name):
    return 0

  f = open(file_name)
  header_size = len(licence)
  first_line = None
  if lang == PYTHON:
    first_line = f.readline()
    if not first_line.startswith('#!') and not first_line.startswith('# !'):
      # First line isn't a shebang, ignore it.
      f.seek(0, os.SEEK_SET)
      first_line = None
  header = f.read(header_size)
  f.close()
  if header == licence:
    return 0

  if fix:
    print 'Fixing %s' % file_name
    if lang == PYTHON and first_line is not None:
      licence = first_line + licence
    ReplaceHeader(file_name, licence, lang)
    return 1
  else:
    print "File %s does not start with \"%s...\"" % (
        file_name,
        licence.split('\n')[(0 if (lang == PYTHON) else 1)])
    if diff:
      d = difflib.Differ()
      result = list(d.compare(header.splitlines(1), licence.splitlines(1)))
      pprint.pprint(result)
    return 1

def main():
  diff, fix = ParseArgs()
  licences = GetDirectoryLicences(os.getcwd())
  errors = 0
  for dir_name, licence in licences.iteritems():
    errors += CheckLicenceForDir(dir_name, licence, diff=diff, fix=fix)
  print 'Found %d files with incorrect licences' % errors
  if errors > 0:
    sys.exit(1)
  else:
    sys.exit()

if __name__ == '__main__':
  main()
