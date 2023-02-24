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
# launch_tests.py
# Copyright (C) 2012 Simon Newton

from __future__ import print_function
from optparse import OptionParser
import logging
import os
import setup_patch  # The Port Autopatcher
import shutil
import signal
import subprocess
import sys
import tempfile
import textwrap
import time

"""
Launch the OLA RDM test environment.

Under the hood this does the following:
  - creates a new temp directory for the configs
  - copies in the skeleton configs files from the skel directory
  - starts olad pointing at the config directory
  - runs the auto-patching script to setup the port / universe patchings
"""

__author__ = 'nomis52@gmail.com (Simon Newton)'


olad_process = None


def ParseOptions():
  usage = 'Usage: %prog [options] <uid>'
  description = textwrap.dedent("""\
    This starts the OLA RDM testing environment.
  """)
  parser = OptionParser(usage, description=description)
  parser.add_option('--skel', default='skel_config',
                    type='string',
                    help='The path to the skeleton config directory.')
  parser.add_option('--olad-log-level', default='2',
                    type='int',
                    help='The log level for olad.')

  options, args = parser.parse_args()
  return options


def SigINTHandler(signal, frame):
  global olad_process
  if olad_process:
    olad_process.terminate()


def main():
  options = ParseOptions()

  logging.basicConfig(
      level=logging.INFO,
      format='%(message)s')

  # create temp dir
  config_dir = tempfile.mkdtemp()

  if not os.access(config_dir, os.W_OK):
    print('%s is not writable' % config_dir)
    sys.exit()

  # copy the skeleton configs files over, no symlinks since we don't want to
  # change the originals when olad writes settings.
  skel_config = options.skel
  if not os.path.isdir(skel_config):
    print('%s is not a directory' % skel_config)
    sys.exit()

  for file_name in os.listdir(skel_config):
    shutil.copy(os.path.join(skel_config, file_name), config_dir)

  # ok, time to start olad, first install the signal handler
  signal.signal(signal.SIGINT, SigINTHandler)

  args = ['olad',
          '--config-dir', config_dir,
          '--log-level', '%d' % options.olad_log_level]
  global olad_process
  olad_process = subprocess.Popen(args, stdin=subprocess.PIPE)

  # there isn't a decent wait to wait until olad is running, instead we sleep
  # for a bit and then call poll
  time.sleep(1)
  if olad_process.poll():
    # ola exited
    logging.error('OLA exited with return code %d' % olad_process.returncode)
    sys.exit()

  # now patch the ports
  patch_results = setup_patch.PatchPorts()
  if not patch_results.status:
    logging.error('Failed to patch ports, check the olad logs')

  # wait for the signal, or olad to exit
  olad_process.wait()


if __name__ == '__main__':
    main()
