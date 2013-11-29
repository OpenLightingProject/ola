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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ClockTest.cpp
 * Unittest for String functions.
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>
#include <string>

#include "ola/Clock.h"
#include "ola/testing/TestUtils.h"



class ClockTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ClockTest);
  CPPUNIT_TEST(testTimeStamp);
  CPPUNIT_TEST(testTimeInterval);
  CPPUNIT_TEST(testTimeIntervalMutliplication);
  CPPUNIT_TEST(testClock);
  CPPUNIT_TEST(testMockClock);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testTimeStamp();
    void testTimeInterval();
    void testTimeIntervalMutliplication();
    void testClock();
    void testMockClock();
};


CPPUNIT_TEST_SUITE_REGISTRATION(ClockTest);

using ola::Clock;
using ola::MockClock;
using ola::TimeStamp;
using ola::TimeInterval;
using std::string;


/*
 * Test the TimeStamp class
 */
void ClockTest::testTimeStamp() {
  Clock clock;
  TimeStamp timestamp, timestamp2;
  OLA_ASSERT_FALSE(timestamp.IsSet());
  OLA_ASSERT_FALSE(timestamp2.IsSet());

  // test assignment & copy constructor
  clock.CurrentTime(&timestamp);
  OLA_ASSERT_TRUE(timestamp.IsSet());
  timestamp2 = timestamp;
  OLA_ASSERT_TRUE(timestamp2.IsSet());
  TimeStamp timestamp3(timestamp);
  OLA_ASSERT_TRUE(timestamp3.IsSet());
  OLA_ASSERT_EQ(timestamp, timestamp2);
  OLA_ASSERT_EQ(timestamp, timestamp3);

  // test equalities
  // Windows only seems to have ms resolution, to make the tests pass we need
  // to sleep here
  usleep(1000);
  clock.CurrentTime(&timestamp3);
  OLA_ASSERT_NE(timestamp3, timestamp);
  OLA_ASSERT_GT(timestamp3, timestamp);
  OLA_ASSERT_LT(timestamp, timestamp3);

  // test intervals
  TimeInterval interval = timestamp3 - timestamp;

  // test subtraction / addition
  timestamp2 = timestamp + interval;
  OLA_ASSERT_EQ(timestamp2, timestamp3);
  timestamp2 -= interval;
  OLA_ASSERT_EQ(timestamp, timestamp2);

  // test toString and AsInt
  TimeInterval one_point_five_seconds(1500000);
  OLA_ASSERT_EQ(string("1.500000"), one_point_five_seconds.ToString());
  OLA_ASSERT_EQ(static_cast<int64_t>(1500000),
                       one_point_five_seconds.AsInt());
  OLA_ASSERT_EQ(static_cast<int64_t>(1500),
                       one_point_five_seconds.InMilliSeconds());
}


/*
 * test time intervals
 */
void ClockTest::testTimeInterval() {
  // test intervals
  TimeInterval interval(500000);  // 0.5s
  TimeInterval interval2 = interval;
  TimeInterval interval3(interval);
  OLA_ASSERT_EQ(interval, interval2);
  OLA_ASSERT_EQ(interval, interval3);

  TimeInterval interval4(1, 500000);  // 1.5s
  OLA_ASSERT_NE(interval, interval4);
  OLA_ASSERT_LT(interval, interval4);
  TimeInterval interval5(1, 600000);  // 1.6s
  OLA_ASSERT_NE(interval4, interval5);
  OLA_ASSERT_LT(interval4, interval5);
}


/*
 * Test multiplication of TimeIntervals.
 */
void ClockTest::testTimeIntervalMutliplication() {
  TimeInterval half_second(500000);  // 0.5s
  TimeInterval zero_seconds = half_second * 0;
  OLA_ASSERT_EQ((int64_t) 0, zero_seconds.InMilliSeconds());

  TimeInterval another_half_second = half_second * 1;
  OLA_ASSERT_EQ((int64_t) 500, another_half_second.InMilliSeconds());

  TimeInterval two_seconds = half_second * 4;
  OLA_ASSERT_EQ((int64_t) 2000, two_seconds.InMilliSeconds());

  TimeInterval twenty_seconds = half_second * 40;
  OLA_ASSERT_EQ((int64_t) 20000, twenty_seconds.InMilliSeconds());
}


/**
 * test the clock
 */
void ClockTest::testClock() {
  Clock clock;
  TimeStamp first;
  clock.CurrentTime(&first);
  sleep(1);

  TimeStamp second;
  clock.CurrentTime(&second);
  OLA_ASSERT_LT(first, second);
}


/**
 * test the Mock Clock
 */
void ClockTest::testMockClock() {
  MockClock clock;

  TimeStamp first;
  clock.CurrentTime(&first);

  TimeInterval one_second(1, 0);
  clock.AdvanceTime(one_second);

  TimeStamp second;
  clock.CurrentTime(&second);
  OLA_ASSERT_LT(first, second);
  OLA_ASSERT_TRUE(one_second <= (second - first));

  TimeInterval ten_point_five_seconds(10, 500000);
  clock.AdvanceTime(10, 500000);

  TimeStamp third;
  clock.CurrentTime(&third);
  OLA_ASSERT_LT(second, third);
  OLA_ASSERT_TRUE(ten_point_five_seconds <= (third - second));
}
