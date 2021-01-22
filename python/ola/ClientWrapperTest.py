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
# ClientWrapperTest.py
# Copyright (C) 2019 Bruce Lowekamp

import array
import binascii
import datetime
import socket
# import timeout_decorator
try:
  # Python 2.6 needs backport
  import unittest2 as unittest
except ImportError:
  import unittest
from ola.ClientWrapper import ClientWrapper
from ola.ClientWrapper import _Event


"""Test cases for the Event and Event loop of ClientWrapper class."""

__author__ = 'bruce@lowekamp.net (Bruce Lowekamp)'


# functions used for event comparison tests
def a_func():
  pass


def b_func():
  pass


def c_func():
  pass


class ClientWrapperTest(unittest.TestCase):

  def testEventCmp(self):
    a = _Event(0.5, a_func)
    # b/b2/b3/c have same timing to test comparisons of
    # callback (b==b2 and b!=b3)
    b = _Event(1, b_func)
    b2 = _Event(1, b_func)
    b2._run_at = b._run_at
    b3 = _Event(1, a_func)
    b3._run_at = b._run_at
    c = _Event(1, c_func)
    c._run_at = b._run_at

    self.assertEqual(b, b2)
    self.assertNotEqual(b, b3)
    self.assertTrue(a != b)
    self.assertFalse(b != b2)
    self.assertTrue(b == b2)
    self.assertTrue(a < b)
    self.assertTrue(a <= b)
    self.assertTrue(b > a)
    self.assertTrue(b >= a)

    self.assertFalse(a == b2)
    self.assertFalse(b < a)
    self.assertFalse(b <= a)
    self.assertFalse(a > b)
    self.assertFalse(a >= b)
    self.assertTrue(b3 < c)
    self.assertFalse(c < b3)

    s = sorted([c, a, b, b3])
    self.assertEqual(s, [a, b3, b, c])

    s = "teststring"
    self.assertNotEqual(a, s)
    self.assertEqual(a.__lt__(s), NotImplemented)
    self.assertEqual(a.__ne__(s), True)
    self.assertEqual(a.__eq__(s), False)

  def testEventHash(self):
    a = _Event(0.5, None)
    b = _Event(1, None)
    b2 = _Event(1, None)
    b2._run_at = b._run_at
    b3 = _Event(1, 2)
    b3._run_at = b._run_at
    c = _Event(1, 4)
    c._run_at = b._run_at

    self.assertEqual(hash(a), hash(a))
    self.assertNotEqual(hash(a), hash(b))
    self.assertEqual(hash(b), hash(b2))
    self.assertNotEqual(hash(b), hash(b3))
    self.assertNotEqual(hash(b), hash(c))

  # @timeout_decorator.timeout(2)
  def testBasic(self):
    sockets = socket.socketpair()
    wrapper = ClientWrapper(sockets[0])

    class results:
      a_called = False
      b_called = False

    def a():
      results.a_called = True

    def b():
      results.b_called = True
      wrapper.Stop()

    wrapper.AddEvent(0, a)
    wrapper.AddEvent(0, b)
    self.assertFalse(results.a_called)
    wrapper.Run()
    self.assertTrue(results.a_called)
    self.assertTrue(results.b_called)

    sockets[0].close()
    sockets[1].close()

  # @timeout_decorator.timeout(2)
  def testEventLoop(self):
    sockets = socket.socketpair()
    wrapper = ClientWrapper(sockets[0])

    class results:
      a_called = None
      b_called = None
      c_called = None
      d_called = None

    def a():
      self.assertIsNone(results.a_called)
      self.assertIsNone(results.b_called)
      self.assertIsNone(results.c_called)
      self.assertIsNone(results.d_called)
      results.a_called = datetime.datetime.now()

    def b():
      self.assertIsNotNone(results.a_called)
      self.assertIsNone(results.b_called)
      self.assertIsNone(results.c_called)
      self.assertIsNone(results.d_called)
      results.b_called = datetime.datetime.now()

    def c():
      self.assertIsNotNone(results.a_called)
      self.assertIsNotNone(results.b_called)
      self.assertIsNone(results.c_called)
      self.assertIsNone(results.d_called)
      results.c_called = datetime.datetime.now()

    def d():
      self.assertIsNotNone(results.a_called)
      self.assertIsNotNone(results.b_called)
      self.assertIsNotNone(results.c_called)
      self.assertIsNone(results.d_called)
      results.d_called = datetime.datetime.now()
      wrapper.AddEvent(0, wrapper.Stop)

    self.start = datetime.datetime.now()
    wrapper.AddEvent(0, a)
    wrapper.AddEvent(15, d)
    wrapper.AddEvent(datetime.timedelta(milliseconds=5), b)
    wrapper.AddEvent(10, c)

    self.assertIsNone(results.a_called)
    self.assertIsNone(results.b_called)
    self.assertIsNone(results.c_called)
    self.assertIsNone(results.d_called)

    self.start = datetime.datetime.now()
    wrapper.Run()

    self.assertIsNotNone(results.a_called)
    self.assertIsNotNone(results.b_called)
    self.assertIsNotNone(results.c_called)
    self.assertIsNotNone(results.d_called)

    self.assertTrue(results.a_called - self.start <
                    datetime.timedelta(milliseconds=5))
    self.assertTrue(results.b_called - self.start >=
                    datetime.timedelta(milliseconds=5))
    self.assertTrue(results.c_called - self.start >=
                    datetime.timedelta(milliseconds=10))
    self.assertTrue(results.d_called - self.start >=
                    datetime.timedelta(milliseconds=15))

    sockets[0].close()
    sockets[1].close()

  # @timeout_decorator.timeout(2)
  def testSend(self):
    """tests that data goes out on the wire with SendDMX"""
    sockets = socket.socketpair()
    wrapper = ClientWrapper(sockets[0])

    class results:
      gotdata = False

    def DataCallback(self):
      data = sockets[1].recv(4096)
      expected = binascii.unhexlify(
        "7d000010080110001a0d557064617465446d784461746122680801126400000"
        "000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000"
        "000000")
      self.assertEqual(data, expected,
                       msg="Regression check failed. If protocol change "
                       "was intended set expected to: " +
                       str(binascii.hexlify(data)))
      results.gotdata = True
      wrapper.AddEvent(0, wrapper.Stop)

    wrapper._ss.AddReadDescriptor(sockets[1], lambda: DataCallback(self))
    self.assertTrue(wrapper.Client().SendDmx(1, array.array('B', [0] * 100),
                                             None))

    wrapper.Run()

    self.assertTrue(results.gotdata)


if __name__ == '__main__':
  unittest.main()
