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
import pprint
import re
import sys
import textwrap

LGPL = """/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
"""

GPL = """/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
"""

def Usage(arg0):
  print textwrap.dedent("""\
  Usage: %s [files]

  Check that the files start with the appropriate licence

    --diff               Print the diffs.
    --fix                Fix the files.
    --help               Display this message.
    --license <GPL,LGPL> Type of license to check for.""" % arg0)

def ReplaceHeader(file_name, new_header):
  f = open(file_name)
  breaks = 0
  line = f.readline()
  while line != '':
    if re.match(r'^ \*\s*\n$', line):
      breaks += 1
    if breaks == 3:
      break
    line = f.readline()

  remainder = f.read()
  f.close()

  f = open(file_name, 'w')
  f.write(new_header)
  f.write(remainder)
  f.close()

LICENCES = {
  'GPL': GPL,
  'LGPL': LGPL
}

def main():
  try:
    opts, args = getopt.getopt(sys.argv[1:], '',
                               ['diff', 'fix', 'help', 'licence=', 'license='])
  except getopt.GetoptError, err:
    print str(err)
    Usage(sys.argv[0])
    sys.exit(2)

  help = False
  fix = False
  diff = False
  licence_name = 'GPL'
  for o, a in opts:
    if o in ('--diff'):
      diff = True
    elif o in ('--fix'):
      fix = True
    elif o in ('-h', '--help'):
      Usage(sys.argv[0])
      sys.exit()
    elif o in ('--licence', '--license'):
      licence_name = a

  if help:
    Usage(sys.argv[0])
    sys.exit(0)

  if not licence_name or licence_name.upper() not in LICENCES:
    Usage(sys.argv[0])
    sys.exit(0)

  licence = LICENCES[licence_name.upper()]
  header_size = len(licence)
  d = difflib.Differ()

  for file_name in args:
    f = open(file_name)
    header = f.read(header_size)
    f.close()
    if header == licence:
      continue

    if fix:
      print 'Fixing %s' % file_name
      ReplaceHeader(file_name, licence)
    else:
      print "File does not start with %s: %s" % (licence_name, file_name)
      if diff:
        result = list(d.compare(header.splitlines(1), licence.splitlines(1)))
        pprint.pprint(result)

    elif o in ('--licence', '--license'):
      licence_name = a

if __name__ == '__main__':
  main()
