/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DmxSourceTest.cpp
 * Test fixture for the DmxSource classes
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/Clock.h"
#include "ola/DmxBuffer.h"
#include "olad/DmxSource.h"


class DmxSourceTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DmxSourceTest);
  CPPUNIT_TEST(testDmxSource);
  CPPUNIT_TEST(testIsActive);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testDmxSource();
    void testIsActive();
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
  Clock::CurrentTime(&timestamp);

  DmxSource source(buffer, timestamp, 100);
  CPPUNIT_ASSERT(source.IsSet());
  CPPUNIT_ASSERT(buffer == source.Data());
  CPPUNIT_ASSERT_EQUAL(timestamp, source.Timestamp());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 100, source.Priority());

  DmxBuffer buffer2("987654321");
  TimeStamp timestamp2;
  Clock::CurrentTime(&timestamp2);
  CPPUNIT_ASSERT(timestamp != timestamp2);

  source.UpdateData(buffer2, timestamp2, 120);
  CPPUNIT_ASSERT(buffer2 == source.Data());
  CPPUNIT_ASSERT_EQUAL(timestamp2, source.Timestamp());
  CPPUNIT_ASSERT_EQUAL((uint8_t) 120, source.Priority());

  DmxSource empty_source;
  CPPUNIT_ASSERT(!empty_source.IsSet());
}


/*
 * Test the time based checks
 */
void DmxSourceTest::testIsActive() {
  DmxBuffer buffer("123456789");
  TimeStamp timestamp;
  Clock::CurrentTime(&timestamp);

  DmxSource source(buffer, timestamp, 100);
  CPPUNIT_ASSERT(source.IsSet());

  CPPUNIT_ASSERT(source.IsActive(timestamp));
  TimeInterval interval(1000000);
  TimeStamp later = timestamp + interval;
  CPPUNIT_ASSERT(source.IsActive(later));

  later = timestamp + TimeInterval(2500000);
  CPPUNIT_ASSERT(!source.IsActive(later));
}
