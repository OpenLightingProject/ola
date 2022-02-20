/*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * SerialLockTester.cpp
 * Test for serial port locking
 * Copyright (C) 2022 Thomas White
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <unistd.h>
#include <fcntl.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/io/Serial.h"
#include "ola/testing/TestUtils.h"

class SerialLockTest: public CppUnit::TestFixture {
 public:
  CPPUNIT_TEST_SUITE(SerialLockTest);
  CPPUNIT_TEST(testLock);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testLock();
};


CPPUNIT_TEST_SUITE_REGISTRATION(SerialLockTest);

void SerialLockTest::testLock() {
  bool r1, r2;
  int fd1, fd2;
  const std::string path = "ChangeLog";

  r1 = ola::io::AcquireLockAndOpenSerialPort(path, O_RDWR, &fd1);
  OLA_ASSERT_TRUE(r1);

  r2 = ola::io::AcquireLockAndOpenSerialPort(path, O_RDWR, &fd2);
  OLA_ASSERT_FALSE(r2);

  ola::io::ReleaseSerialPortLock(path);
  close(fd1);
}
