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
#include <sys/file.h>

#include "ola/io/Serial.h"
#include "ola/io/IOUtils.h"
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
  int fd;
  pid_t our_pid = getpid();

  std::stringstream str;
  str << "serialLockTestFile." << our_pid;
  const std::string path = str.str();

  OLA_ASSERT_FALSE_MSG(ola::io::FileExists(path),
                       "Test file already exists");

  fd = open(path.c_str(), O_CREAT | O_RDWR,
             S_IRUSR | S_IWUSR
#ifndef _WIN32
             | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#endif  // !_WIN32
             );  // NOLINT(whitespace/parens)
  OLA_ASSERT_FALSE_MSG(fd < 0, "Couldn't create test file");

#ifdef HAVE_FLOCK
  OLA_ASSERT_TRUE(flock(fd, LOCK_EX | LOCK_NB) != -1);
#else

#ifdef UUCP_LOCKING
  // flock() is not available, but UUCP locking was selected.  OK.
#else
  OLA_FAIL("Not using UUCP locking, and flock() is not available");
#endif

#endif

  close(fd);

  OLA_ASSERT_FALSE_MSG(unlink(path.c_str()),
                       "Couldn't delete test file");
}
