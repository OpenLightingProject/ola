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
# verify_trees.py
# Copyright (C) 2015 Simon Newton

from __future__ import print_function

import os
import fnmatch
import textwrap
import sys

# File & directory patterns that differ between what's in the git repo and
# what's in the tarball.
IGNORE_PATTERNS = [
  '*.log',
  '*.pc',
  '*.swp',
  '*.trs',
  '*/.deps',
  '*/.dirstamp',
  '*/.libs',
  '*/LICENCE',
  '*/Makefile',
  '*_pb2.py',
  '*~',
  '.codespellignore*',
  '.flake8',
  '.git',
  '.git/*',
  '.github/*',
  '.gitignore',
  '.travis-ci.sh',
  '.travis.yml',
  'Doxyfile',
  'Makefile',
  'README.md',
  'autom4te.cache',
  'config.h',
  'config.status',
  'html/*',
  'include/ola/base/Version.h',
  'libtool',
  'man/generate-html.sh',
  'ola-*.tar.gz',
  'ola-*/*',
  'plugins/generate-html.sh',
  'plugins/pandoc-html-index.html',
  'plugins/kinet/kinet.cpp',
  'python/ola/PidStoreLocation.py',
  'python/ola/Version.py',
  'scripts/*',
  'stamp-h1',
  'tools/rdm/DataLocation.py',
]


def Usage(arg0):
  print(textwrap.dedent("""\
  Usage: %s <treeA> <treeB>

  Check for files that exist in treeA but aren't in treeB. This can be used to
  ensure we don't miss files from the tarball.""" % arg0))


def ShouldIgnore(path):
  for pattern in IGNORE_PATTERNS:
    if fnmatch.fnmatch(path, pattern):
      return True
  return False


def BuildTree(root):
  tree = set()
  for directory, sub_dirs, files in os.walk(root):
    rel_dir = os.path.relpath(directory, root)
    if rel_dir == '.':
      rel_dir = ''
    if ShouldIgnore(rel_dir):
      continue
    for f in files:
      path = os.path.join(rel_dir, f)
      if not ShouldIgnore(path):
        tree.add(path)

  return tree


def main():
  if len(sys.argv) != 3:
    Usage(sys.argv[0])
    sys.exit(1)

  tree_a_root = sys.argv[1]
  tree_b_root = sys.argv[2]
  for tree in (tree_a_root, tree_b_root):
    if not os.path.isdir(tree):
      print('Not a directory `%s`' % tree, file=sys.stderr)
      sys.exit(2)

  tree_a = BuildTree(tree_a_root)
  tree_b = BuildTree(tree_b_root)

  difference = tree_a.difference(tree_b)

  if difference:
    for file_path in difference:
      print('Missing from tarball %s' % file_path, file=sys.stderr)

    print('\n---------------------------------------', file=sys.stderr)
    print('Either add the missing files to the appropriate Makefile.mk\n'
          'or update IGNORE_PATTERNS in scripts/verify_trees.py',
          file=sys.stderr)
    print('---------------------------------------', file=sys.stderr)
    sys.exit(1)

  sys.exit()


if __name__ == '__main__':
  main()
