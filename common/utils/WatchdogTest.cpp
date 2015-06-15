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
 * WatchdogTest.cpp
 * Unittest for the Watchdog.
 * Copyright (C) 2015 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/util/Watchdog.h"
#include "ola/testing/TestUtils.h"

class WatchdogTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(WatchdogTest);
  CPPUNIT_TEST(testWatchdog);
  CPPUNIT_TEST_SUITE_END();

 public:
  WatchdogTest(): m_timeouts(0) {}

  void testWatchdog();

 private:
  unsigned int m_timeouts;

  void Timeout() {
    m_timeouts++;
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(WatchdogTest);

void WatchdogTest::testWatchdog() {
  m_timeouts = 0;
  ola::Watchdog watchdog(4, ola::NewCallback(this, &WatchdogTest::Timeout));

  // Not enabled yet
  for (unsigned int i = 0; i < 10; i++) {
    watchdog.Clock();
  }
  OLA_ASSERT_EQ(0u, m_timeouts);

  watchdog.Enable();
  // Test with regular kicks.
  for (unsigned int i = 0; i < 10; i++) {
    watchdog.Clock();
    if (i % 2) {
      watchdog.Kick();
    }
  }
  OLA_ASSERT_EQ(0u, m_timeouts);

  // test with non kicks
  for (unsigned int i = 0; i < 10; i++) {
    watchdog.Clock();
  }
  OLA_ASSERT_EQ(1u, m_timeouts);

  // Disable and re-enable.
  watchdog.Disable();
  watchdog.Enable();

  for (unsigned int i = 0; i < 3; i++) {
    watchdog.Clock();
  }
  OLA_ASSERT_EQ(1u, m_timeouts);

  // Disable triggers a reset of the count
  watchdog.Disable();
  watchdog.Enable();
  watchdog.Clock();
  watchdog.Clock();
  OLA_ASSERT_EQ(1u, m_timeouts);

  watchdog.Clock();
  watchdog.Clock();
  OLA_ASSERT_EQ(2u, m_timeouts);
}
