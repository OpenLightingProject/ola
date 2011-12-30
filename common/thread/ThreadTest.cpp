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
 * ThreadTest.cpp
 * Test fixture for the Thread class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/thread/Thread.h"

using ola::thread::ConditionVariable;
using ola::thread::Mutex;
using ola::thread::MutexLocker;
using ola::thread::Thread;

class ThreadTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ThreadTest);
  CPPUNIT_TEST(testThread);
  CPPUNIT_TEST(testConditionVariable);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testThread();
    void testConditionVariable();
};


class MockThread: public Thread {
  public:
    MockThread()
        : Thread(),
          m_thread_ran(false),
          m_mutex() {
    }
    ~MockThread() {}

    void *Run() {
      MutexLocker locker(&m_mutex);
      m_thread_ran = true;
      return NULL;
    }

    bool HasRan() {
      MutexLocker locker(&m_mutex);
      return m_thread_ran;
    }

  private:
    bool m_thread_ran;
    Mutex m_mutex;
};


CPPUNIT_TEST_SUITE_REGISTRATION(ThreadTest);


/*
 * Check that basic thread functionality works.
 */
void ThreadTest::testThread() {
  MockThread thread;
  CPPUNIT_ASSERT(!thread.HasRan());
  CPPUNIT_ASSERT(thread.Start());
  // starting twice must fail
  CPPUNIT_ASSERT(!thread.Start());
  CPPUNIT_ASSERT(thread.IsRunning());
  CPPUNIT_ASSERT(thread.Join());
  CPPUNIT_ASSERT(!thread.IsRunning());
  CPPUNIT_ASSERT(thread.HasRan());
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


/**
 * Check that a condition variable works
 */
void ThreadTest::testConditionVariable() {
  Mutex mutex;
  ConditionVariable condition;
  MockConditionThread thread(&mutex, &condition);
  thread.Start();

  mutex.Lock();
  if (thread.i != MockConditionThread::EXPECTED)
    condition.Wait(&mutex);
  CPPUNIT_ASSERT_EQUAL(10, thread.i);
  mutex.Unlock();

  thread.Join();
}
