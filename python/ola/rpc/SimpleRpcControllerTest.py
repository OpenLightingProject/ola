#!/usr/bin/env python
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
# SimpleRpcControllerTest.py
# Copyright (C) 2005 Simon Newton

import unittest

from SimpleRpcController import SimpleRpcController

"""Test cases for the SimpleRpcController."""

__author__ = 'nomis52@gmail.com (Simon Newton)'


class SimpleRpcControllerTest(unittest.TestCase):

  def setUp(self):
    self.callback_run = False

  def Callback(self):
    self.callback_run = True

  def testSimpleRpcController(self):
    controller = SimpleRpcController()
    self.assertFalse(controller.Failed())
    self.assertFalse(controller.IsCanceled())
    self.assertEqual(None, controller.ErrorText())
    self.assertFalse(self.callback_run)

    # cancel
    controller.NotifyOnCancel(self.Callback)
    controller.StartCancel()
    self.assertFalse(controller.Failed())
    self.assertFalse(not controller.IsCanceled())
    self.assertEqual(None, controller.ErrorText())
    self.assertFalse(not self.callback_run)

    self.callback_run = False
    controller.Reset()
    self.assertFalse(controller.Failed())
    self.assertFalse(controller.IsCanceled())
    self.assertEqual(None, controller.ErrorText())

    # fail
    failure_string = 'foo'
    controller.SetFailed(failure_string)
    self.assertFalse(not controller.Failed())
    self.assertFalse(controller.IsCanceled())
    self.assertEqual(failure_string, controller.ErrorText())

    controller.Reset()
    self.assertFalse(controller.Failed())
    self.assertFalse(controller.IsCanceled())
    self.assertEqual(None, controller.ErrorText())
    self.assertFalse(self.callback_run)


if __name__ == '__main__':
  unittest.main()
