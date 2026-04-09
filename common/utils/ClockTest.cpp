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
 * ClockTest.cpp
 * Unittest for String functions.
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <ola/win/CleanWindows.h>
#endif  // _WIN32

#include "ola/Clock.h"
#include "ola/testing/TestUtils.h"

class ClockTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ClockTest);
  CPPUNIT_TEST(testTimeStamp);
  CPPUNIT_TEST(testTimeInterval);
  CPPUNIT_TEST(testTimeIntervalMultiplication);
  CPPUNIT_TEST(testClockMonotonic);
  CPPUNIT_TEST(testClockRealTime);
  CPPUNIT_TEST(testClockCurrentTime);
  CPPUNIT_TEST(testMockClockMonotonic);
  CPPUNIT_TEST(testMockClockRealTime);
  CPPUNIT_TEST(testMockClockCurrentTime);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testTimeStamp();
    void testTimeInterval();
    void testTimeIntervalMultiplication();
    void testClockMonotonic();
    void testClockRealTime();
    void testClockCurrentTime();
    void testMockClockMonotonic();
    void testMockClockRealTime();
    void testMockClockCurrentTime();
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
  clock.CurrentMonotonicTime(&timestamp);
  OLA_ASSERT_TRUE(timestamp.IsSet());
  timestamp2 = timestamp;
  OLA_ASSERT_TRUE(timestamp2.IsSet());
  TimeStamp timestamp3(timestamp);
  OLA_ASSERT_TRUE(timestamp3.IsSet());
  OLA_ASSERT_EQ(timestamp, timestamp2);
  OLA_ASSERT_EQ(timestamp, timestamp3);

  // test timespec assignment
  TimeStamp timestamp4;
  struct timespec ts1;
  const time_t test_secs = 1604140280;
  const int test_nsecs = 42000;
  ts1.tv_sec = test_secs;
  ts1.tv_nsec = test_nsecs;
  timestamp4 = ts1;
  OLA_ASSERT_TRUE(timestamp4.IsSet());
  OLA_ASSERT_EQ(test_secs, timestamp4.Seconds());
  OLA_ASSERT_EQ(test_nsecs/1000, timestamp4.MicroSeconds());
  // test timeval assignment
  TimeStamp timestamp5;
  struct timeval tv1;
  tv1.tv_sec = test_secs;
  tv1.tv_usec = test_nsecs/1000;
  timestamp5 = tv1;
  OLA_ASSERT_TRUE(timestamp5.IsSet());
  OLA_ASSERT_EQ(test_secs, timestamp5.Seconds());
  OLA_ASSERT_EQ(test_nsecs/1000, timestamp5.MicroSeconds());
  // test timespec copy constructor
  TimeStamp timestamp6(ts1);
  OLA_ASSERT_TRUE(timestamp6.IsSet());
  OLA_ASSERT_EQ(test_secs, timestamp6.Seconds());
  OLA_ASSERT_EQ(test_nsecs/1000, timestamp6.MicroSeconds());
  // test timeval copy constructor
  TimeStamp timestamp7(tv1);
  OLA_ASSERT_TRUE(timestamp7.IsSet());
  OLA_ASSERT_EQ(test_secs, timestamp7.Seconds());
  OLA_ASSERT_EQ(test_nsecs/1000, timestamp7.MicroSeconds());


  // test equalities
  // Windows only seems to have ms resolution, to make the tests pass we need
  // to sleep here; XP only has 16ms resolution, so sleep a bit longer
  usleep(20000);
  clock.CurrentMonotonicTime(&timestamp3);
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
void ClockTest::testTimeIntervalMultiplication() {
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
 * @brief Test the monotonic clock
 */
void ClockTest::testClockMonotonic() {
  Clock clock;
  TimeStamp first;
  clock.CurrentMonotonicTime(&first);
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif  // _WIN32

  TimeStamp second;
  clock.CurrentMonotonicTime(&second);
  OLA_ASSERT_LT(first, second);
}

/**
 * @brief Test the real time clock
 */
void ClockTest::testClockRealTime() {
  Clock clock;
  TimeStamp first;
  clock.CurrentRealTime(&first);
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif  // _WIN32

  TimeStamp second;
  clock.CurrentRealTime(&second);
  OLA_ASSERT_LT(first, second);
}

/**
 * @brief Test the CurrentTime wrapper method
 */
void ClockTest::testClockCurrentTime() {
  Clock clock;
  TimeStamp first;
  clock.CurrentTime(&first);
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif  // _WIN32

  TimeStamp second;
  clock.CurrentTime(&second);
  OLA_ASSERT_LT(first, second);
}

/**
 * @brief Test the mock monotonic clock
 */
void ClockTest::testMockClockMonotonic() {
  MockClock clock;

  TimeStamp first;
  clock.CurrentMonotonicTime(&first);

  TimeInterval one_second(1, 0);
  clock.AdvanceTime(one_second);

  TimeStamp second;
  clock.CurrentMonotonicTime(&second);
  OLA_ASSERT_LT(first, second);
  OLA_ASSERT_TRUE_MSG(
      one_second <= (second - first),
      "This test should not have failed. Was the time changed?");

  TimeInterval ten_point_five_seconds(10, 500000);
  clock.AdvanceTime(10, 500000);

  TimeStamp third;
  clock.CurrentMonotonicTime(&third);
  OLA_ASSERT_LT(second, third);
  OLA_ASSERT_TRUE_MSG(
      ten_point_five_seconds <= (third - second),
      "This test should not have failed. Was the time changed?");
}

/**
 * @brief Test the mock real time clock
 * 
 */
void ClockTest::testMockClockRealTime() {
  MockClock clock;

  TimeStamp first;
  clock.CurrentRealTime(&first);

  TimeInterval one_second(1, 0);
  clock.AdvanceTime(one_second);

  TimeStamp second;
  clock.CurrentRealTime(&second);
  OLA_ASSERT_LT(first, second);
  OLA_ASSERT_TRUE_MSG(
      one_second <= (second - first),
      "This test should not have failed. Was the time changed?");

  TimeInterval ten_point_five_seconds(10, 500000);
  clock.AdvanceTime(10, 500000);

  TimeStamp third;
  clock.CurrentRealTime(&third);
  OLA_ASSERT_LT(second, third);
  OLA_ASSERT_TRUE_MSG(
      ten_point_five_seconds <= (third - second),
      "This test should not have failed. Was the time changed?");
}

/**
 * @brief Test the mock CurrentTime wrapper method
 */
void ClockTest::testMockClockCurrentTime() {
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

