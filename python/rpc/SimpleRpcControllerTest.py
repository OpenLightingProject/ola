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
# SimpleRpcControllerTest.py
# Copyright (C) 2005-2009 Simon Newton

"""Test cases for the SimpleRpcController."""

__author__ = 'nomis52@gmail.com (Simon Newton)'

import unittest
from SimpleRpcController import SimpleRpcController

class SimpleRpcControllerTest(unittest.TestCase):

  def setUp(self):
    self.callback_run = False

  def Callback(self):
    self.callback_run = True

  def testSimpleRpcController(self):
    controller = SimpleRpcController()
    self.assertFalse(controller.Failed())
    self.assertFalse(controller.IsCanceled())
    self.assertEquals(None, controller.ErrorText())
    self.assertFalse(self.callback_run)

    # cancel
    controller.NotifyOnCancel(self.Callback)
    controller.StartCancel()
    self.assertFalse(controller.Failed())
    self.assertFalse(not controller.IsCanceled())
    self.assertEquals(None, controller.ErrorText())
    self.assertFalse(not self.callback_run)

    self.callback_run = False
    controller.Reset()
    self.assertFalse(controller.Failed())
    self.assertFalse(controller.IsCanceled())
    self.assertEquals(None, controller.ErrorText())

    # fail
    failure_string = 'foo'
    controller.SetFailed(failure_string)
    self.assertFalse(not controller.Failed())
    self.assertFalse(controller.IsCanceled())
    self.assertEquals(failure_string, controller.ErrorText())

    controller.Reset()
    self.assertFalse(controller.Failed())
    self.assertFalse(controller.IsCanceled())
    self.assertEquals(None, controller.ErrorText())
    self.assertFalse(self.callback_run)

if __name__ == '__main__':
  unittest.main()
