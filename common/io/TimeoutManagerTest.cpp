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
 * TimeoutManagerTest.cpp
 * Test fixture for the TimeoutManager class.
 * Copyright (C) 2013 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include <map>

#include "common/io/TimeoutManager.h"
#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"

using ola::ExportMap;
using ola::MockClock;
using ola::NewSingleCallback;
using ola::NewCallback;
using ola::TimeInterval;
using ola::TimeStamp;
using ola::io::TimeoutManager;
using ola::thread::timeout_id;

class TimeoutManagerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TimeoutManagerTest);
  CPPUNIT_TEST(testSingleTimeouts);
  CPPUNIT_TEST(testRepeatingTimeouts);
  CPPUNIT_TEST(testAbortedRepeatingTimeouts);
  CPPUNIT_TEST(testPendingEventShutdown);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testSingleTimeouts();
    void testRepeatingTimeouts();
    void testAbortedRepeatingTimeouts();
    void testPendingEventShutdown();

    void HandleEvent(unsigned int event_id) {
      m_event_counters[event_id]++;
    }

    bool HandleRepeatingEvent(unsigned int event_id) {
      m_event_counters[event_id]++;
      return true;
    }

    // returns false after the second event.
    bool HandleAbortedEvent(unsigned int event_id) {
      m_event_counters[event_id]++;
      return m_event_counters[event_id] < 2;
    }

    unsigned int GetEventCounter(unsigned int event_id) {
      return m_event_counters[event_id];
    }

 private:
    ExportMap m_map;
    std::map<unsigned int, unsigned int> m_event_counters;
};


CPPUNIT_TEST_SUITE_REGISTRATION(TimeoutManagerTest);

/*
 * Check RegisterSingleTimeout works.
 */
void TimeoutManagerTest::testSingleTimeouts() {
  MockClock clock;
  TimeoutManager timeout_manager(&m_map, &clock);

  OLA_ASSERT_FALSE(timeout_manager.EventsPending());

  TimeInterval timeout_interval(1, 0);
  timeout_id id1 = timeout_manager.RegisterSingleTimeout(
      timeout_interval,
      NewSingleCallback(this, &TimeoutManagerTest::HandleEvent, 1u));
  OLA_ASSERT_NE(id1, ola::thread::INVALID_TIMEOUT);

  TimeStamp last_checked_time;

  clock.AdvanceTime(0, 1);  // Small offset to work around timer precision
  clock.CurrentMonotonicTime(&last_checked_time);
  TimeInterval next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_EQ(0u, GetEventCounter(1));
  OLA_ASSERT_LT(next, timeout_interval);

  clock.AdvanceTime(0, 500000);
  clock.CurrentMonotonicTime(&last_checked_time);
  next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_EQ(0u, GetEventCounter(1));
  OLA_ASSERT_LT(next, TimeInterval(0, 500000));

  clock.AdvanceTime(0, 500000);
  clock.CurrentMonotonicTime(&last_checked_time);
  next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_TRUE(next.IsZero());
  OLA_ASSERT_EQ(1u, GetEventCounter(1));

  OLA_ASSERT_FALSE(timeout_manager.EventsPending());

  // now add another timeout and then remove it
  timeout_id id2 = timeout_manager.RegisterSingleTimeout(
      timeout_interval,
      NewSingleCallback(this, &TimeoutManagerTest::HandleEvent, 2u));
  OLA_ASSERT_NE(id2, ola::thread::INVALID_TIMEOUT);
  OLA_ASSERT_TRUE(timeout_manager.EventsPending());
  OLA_ASSERT_EQ(0u, GetEventCounter(2));

  timeout_manager.CancelTimeout(id2);

  clock.AdvanceTime(1, 0);
  clock.CurrentMonotonicTime(&last_checked_time);
  next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_FALSE(timeout_manager.EventsPending());
  OLA_ASSERT_EQ(0u, GetEventCounter(2));
}

/*
 * Check RegisterRepeatingTimeout works.
 */
void TimeoutManagerTest::testRepeatingTimeouts() {
  MockClock clock;
  TimeoutManager timeout_manager(&m_map, &clock);

  OLA_ASSERT_FALSE(timeout_manager.EventsPending());

  TimeInterval timeout_interval(1, 0);
  timeout_id id1 = timeout_manager.RegisterRepeatingTimeout(
      timeout_interval,
      NewCallback(this, &TimeoutManagerTest::HandleRepeatingEvent, 1u));
  OLA_ASSERT_NE(id1, ola::thread::INVALID_TIMEOUT);

  TimeStamp last_checked_time;

  clock.AdvanceTime(0, 1);  // Small offset to work around timer precision
  clock.CurrentMonotonicTime(&last_checked_time);
  TimeInterval next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_EQ(0u, GetEventCounter(1));
  OLA_ASSERT_LT(next, timeout_interval);

  clock.AdvanceTime(0, 500000);
  clock.CurrentMonotonicTime(&last_checked_time);
  next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_EQ(0u, GetEventCounter(1));
  OLA_ASSERT_LT(next, TimeInterval(0, 500000));

  clock.AdvanceTime(0, 500000);
  clock.CurrentMonotonicTime(&last_checked_time);
  next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_LTE(next, timeout_interval);
  OLA_ASSERT_EQ(1u, GetEventCounter(1));

  OLA_ASSERT_TRUE(timeout_manager.EventsPending());

  // fire the event again
  clock.AdvanceTime(1, 0);
  clock.CurrentMonotonicTime(&last_checked_time);
  next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_LTE(next, timeout_interval);
  OLA_ASSERT_EQ(2u, GetEventCounter(1));

  // cancel the event
  timeout_manager.CancelTimeout(id1);
  clock.AdvanceTime(1, 0);
  clock.CurrentMonotonicTime(&last_checked_time);
  next = timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_TRUE(next.IsZero());
  OLA_ASSERT_EQ(2u, GetEventCounter(1));
}


/*
 * Check returning false from a repeating timeout cancels the timeout.
 */
void TimeoutManagerTest::testAbortedRepeatingTimeouts() {
  MockClock clock;
  TimeoutManager timeout_manager(&m_map, &clock);

  OLA_ASSERT_FALSE(timeout_manager.EventsPending());

  TimeInterval timeout_interval(1, 0);
  timeout_id id1 = timeout_manager.RegisterRepeatingTimeout(
      timeout_interval,
      NewCallback(this, &TimeoutManagerTest::HandleAbortedEvent, 1u));
  OLA_ASSERT_NE(id1, ola::thread::INVALID_TIMEOUT);

  TimeStamp last_checked_time;

  clock.AdvanceTime(0, 1);  // Small offset to work around timer precision
  clock.AdvanceTime(1, 0);
  clock.CurrentMonotonicTime(&last_checked_time);
  timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_EQ(1u, GetEventCounter(1));

  clock.AdvanceTime(1, 0);
  clock.CurrentMonotonicTime(&last_checked_time);
  timeout_manager.ExecuteTimeouts(&last_checked_time);
  OLA_ASSERT_EQ(2u, GetEventCounter(1));

  OLA_ASSERT_FALSE(timeout_manager.EventsPending());
}

/*
 * Check we don't leak if there are events pending when the manager is
 * destroyed.
 */
void TimeoutManagerTest::testPendingEventShutdown() {
  MockClock clock;
  TimeoutManager timeout_manager(&m_map, &clock);

  OLA_ASSERT_FALSE(timeout_manager.EventsPending());

  TimeInterval timeout_interval(1, 0);
  timeout_id id1 = timeout_manager.RegisterSingleTimeout(
      timeout_interval,
      NewSingleCallback(this, &TimeoutManagerTest::HandleEvent, 1u));
  OLA_ASSERT_NE(id1, ola::thread::INVALID_TIMEOUT);

  timeout_id id2 = timeout_manager.RegisterRepeatingTimeout(
      timeout_interval,
      NewCallback(this, &TimeoutManagerTest::HandleRepeatingEvent, 1u));
  OLA_ASSERT_NE(id2, ola::thread::INVALID_TIMEOUT);

  OLA_ASSERT_TRUE(timeout_manager.EventsPending());
}
