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
 * ThreadTest.cpp
 * Test fixture for the Thread class
 * Copyright (C) 2010 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <cppunit/extensions/HelperMacros.h>
#include <errno.h>
#include <string.h>
#ifndef _WIN32
#include <sys/resource.h>
#endif  // _WIN32
#include <algorithm>

#include "ola/Logging.h"
#include "ola/system/Limits.h"
#include "ola/testing/TestUtils.h"
#include "ola/thread/Thread.h"
#include "ola/thread/Utils.h"


using ola::thread::ConditionVariable;
using ola::thread::Mutex;
using ola::thread::MutexLocker;
using ola::thread::Thread;

// A struct containing the scheduling parameters.
struct SchedulingParams {
  int policy;
  int priority;
};

// Get the current scheduling parameters.
SchedulingParams GetCurrentParams() {
  SchedulingParams our_params;

  struct sched_param param;
  pthread_getschedparam(pthread_self(), &our_params.policy, &param);
  our_params.priority = param.sched_priority;
  return our_params;
}

// Set the current scheduling parameters.
bool SetCurrentParams(const SchedulingParams &new_params) {
  struct sched_param param;
  param.sched_priority = new_params.priority;
  return ola::thread::SetSchedParam(pthread_self(), new_params.policy, param);
}


// A simple thread that runs, captures the scheduling parameters and exits.
class MockThread: public Thread {
 public:
  explicit MockThread(const Options &options = Options("MockThread"))
      : Thread(options),
        m_thread_ran(false),
        m_mutex() {
  }
  ~MockThread() {}

  void *Run() {
    MutexLocker locker(&m_mutex);
    m_thread_ran = true;
    m_scheduling_params = GetCurrentParams();
    return NULL;
  }

  bool HasRan() {
    MutexLocker locker(&m_mutex);
    return m_thread_ran;
  }

  SchedulingParams GetSchedulingParams() {
    MutexLocker locker(&m_mutex);
    return m_scheduling_params;
  }

 private:
  bool m_thread_ran;
  SchedulingParams m_scheduling_params;
  Mutex m_mutex;
};

bool RunThread(MockThread *thread) {
  return thread->Start() &&
         thread->Join() &&
         thread->HasRan();
}

class ThreadTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ThreadTest);
  CPPUNIT_TEST(testThread);
  CPPUNIT_TEST(testSchedulingOptions);
  CPPUNIT_TEST(testConditionVariable);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testThread();
  void testConditionVariable();
  void testSchedulingOptions();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ThreadTest);

/*
 * Check that basic thread functionality works.
 */
void ThreadTest::testThread() {
  MockThread thread;
  OLA_ASSERT_FALSE(thread.HasRan());
  OLA_ASSERT_TRUE(thread.Start());
  // starting twice must fail
  OLA_ASSERT_FALSE(thread.Start());
  OLA_ASSERT_TRUE(thread.IsRunning());
  OLA_ASSERT_TRUE(thread.Join());
  OLA_ASSERT_FALSE(thread.IsRunning());
  OLA_ASSERT_TRUE(thread.HasRan());
}

/*
 * Check that the scheduling options behave as expected.
 */
void ThreadTest::testSchedulingOptions() {
#ifndef _WIN32
#if HAVE_DECL_RLIMIT_RTPRIO
  struct rlimit rlim;
  OLA_ASSERT_TRUE(ola::system::GetRLimit(RLIMIT_RTPRIO, &rlim));

  if (rlim.rlim_cur == 0) {
    // A value of 0 means the user can't change policies.
    OLA_INFO << "Skipping testSchedulingOptions since RLIMIT_RTPRIO is 0";
    return;
  }

  const int max_priority = rlim.rlim_cur - 1;
  const int other_priority = std::min(1, max_priority - 1);
#else
  const int max_priority = 31;
  const int other_priority = 15;
#endif  // HAVE_DECL_RLIMIT_RTPRIO

  SchedulingParams default_params = GetCurrentParams();

  {
    // Default scheduling options.
    MockThread thread;
    OLA_ASSERT_TRUE(RunThread(&thread));
    OLA_ASSERT_EQ(default_params.policy, thread.GetSchedulingParams().policy);
    OLA_ASSERT_EQ(default_params.priority,
                  thread.GetSchedulingParams().priority);
  }

  {
    // A thread that explicitly sets scheduling params.
    Thread::Options options;
    options.name = "ExplicitSchedParamsFIFO";
    options.policy = SCHED_FIFO;
    options.priority = max_priority;
    MockThread thread(options);
    OLA_ASSERT_TRUE(RunThread(&thread));
    OLA_ASSERT_EQ(static_cast<int>(SCHED_FIFO),
                  thread.GetSchedulingParams().policy);
    OLA_ASSERT_EQ(max_priority, thread.GetSchedulingParams().priority);
  }

  // Set the current thread to something other than the default.
  // This allows us to check inheritance.
  SchedulingParams override_params;
  override_params.policy = SCHED_FIFO;
  override_params.priority = other_priority;
  OLA_ASSERT_TRUE(SetCurrentParams(override_params));

  {
    // Default scheduling options.
    MockThread thread;
    OLA_ASSERT_TRUE(RunThread(&thread));
    OLA_ASSERT_EQ(default_params.policy, thread.GetSchedulingParams().policy);
    OLA_ASSERT_EQ(default_params.priority,
                  thread.GetSchedulingParams().priority);
  }

  {
    // A thread that explicitly sets scheduling params.
    Thread::Options options;
    options.name = "ExplicitSchedParamsRR";
    options.policy = SCHED_RR;
    options.priority = max_priority;
    MockThread thread(options);
    OLA_ASSERT_TRUE(RunThread(&thread));
    OLA_ASSERT_EQ(static_cast<int>(SCHED_RR),
                  thread.GetSchedulingParams().policy);
    OLA_ASSERT_EQ(max_priority, thread.GetSchedulingParams().priority);
  }

  {
    // A thread that inherits scheduling params.
    Thread::Options options;
    options.name = "InheritSchedParams";
    options.inheritsched = PTHREAD_INHERIT_SCHED;
    MockThread thread(options);
    OLA_ASSERT_TRUE(RunThread(&thread));
    OLA_ASSERT_EQ(override_params.policy, thread.GetSchedulingParams().policy);
    OLA_ASSERT_EQ(override_params.priority,
                  thread.GetSchedulingParams().priority);
  }
#else
  OLA_WARN << "Scheduling options are not supported on Windows.";
#endif  // !_WIN32
}

class MockConditionThread: public Thread {
 public:
    MockConditionThread(
        Mutex *mutex,
        ConditionVariable *condition)
        : m_mutex(mutex),
          m_condition(condition) {}

    void *Run() {
      m_mutex->Lock();
      i = EXPECTED;
      m_mutex->Unlock();
      m_condition->Signal();
      return NULL;
    }

    int i;

    static const int EXPECTED = 10;

 private:
    Mutex *m_mutex;
    ConditionVariable *m_condition;
};

/*
 * Check that a condition variable works
 */
void ThreadTest::testConditionVariable() {
  Mutex mutex;
  ConditionVariable condition;
  MockConditionThread thread(&mutex, &condition);
  thread.Start();

  mutex.Lock();
  if (thread.i != MockConditionThread::EXPECTED) {
    condition.Wait(&mutex);
  }
  OLA_ASSERT_EQ(10, thread.i);
  mutex.Unlock();

  thread.Join();
}
