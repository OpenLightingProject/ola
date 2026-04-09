/*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DmxSourceTest.cpp
 * Test fixture for the DmxSource classes
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/Clock.h"
#include "ola/DmxBuffer.h"
#include "olad/DmxSource.h"
#include "ola/testing/TestUtils.h"



class DmxSourceTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DmxSourceTest);
  CPPUNIT_TEST(testDmxSource);
  CPPUNIT_TEST(testIsActive);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testDmxSource();
    void testIsActive();

 private:
    ola::Clock m_clock;
};


CPPUNIT_TEST_SUITE_REGISTRATION(DmxSourceTest);

using ola::Clock;
using ola::DmxBuffer;
using ola::DmxSource;
using ola::TimeInterval;
using ola::TimeStamp;


/*
 * Check that the base device class works correctly.
 */
void DmxSourceTest::testDmxSource() {
  DmxBuffer buffer("123456789");
  TimeStamp timestamp;
  m_clock.CurrentMonotonicTime(&timestamp);

  DmxSource source(buffer, timestamp, 100);
  OLA_ASSERT(source.IsSet());
  OLA_ASSERT_DMX_EQUALS(buffer, source.Data());
  OLA_ASSERT_EQ(timestamp, source.Timestamp());
  OLA_ASSERT_EQ((uint8_t) 100, source.Priority());

  DmxBuffer buffer2("987654321");
  TimeStamp timestamp2;
  m_clock.CurrentMonotonicTime(&timestamp2);
  OLA_ASSERT_TRUE(timestamp <= timestamp2);

  source.UpdateData(buffer2, timestamp2, 120);
  OLA_ASSERT_DMX_EQUALS(buffer2, source.Data());
  OLA_ASSERT_EQ(timestamp2, source.Timestamp());
  OLA_ASSERT_EQ((uint8_t) 120, source.Priority());

  DmxSource empty_source;
  OLA_ASSERT_FALSE(empty_source.IsSet());
}


/*
 * Test the time based checks
 */
void DmxSourceTest::testIsActive() {
  DmxBuffer buffer("123456789");
  TimeStamp timestamp;
  m_clock.CurrentMonotonicTime(&timestamp);

  DmxSource source(buffer, timestamp, 100);
  OLA_ASSERT(source.IsSet());

  OLA_ASSERT(source.IsActive(timestamp));
  TimeInterval interval(1000000);
  TimeStamp later = timestamp + interval;
  OLA_ASSERT(source.IsActive(later));

  later = timestamp + TimeInterval(2500000);
  OLA_ASSERT_FALSE(source.IsActive(later));
}
