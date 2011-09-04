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
 * OlaThreadTest.cpp
 * Test fixture for the OlaThread class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/OlaThread.h"

using ola::ConditionVariable;
using ola::Mutex;
using ola::MutexLocker;
using ola::OlaThread;

class OlaThreadTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OlaThreadTest);
  CPPUNIT_TEST(testOlaThread);
  CPPUNIT_TEST(testConditionVariable);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testOlaThread();
    void testConditionVariable();
};


class MockThread: public OlaThread {
  public:
    MockThread()
        : OlaThread(),
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


CPPUNIT_TEST_SUITE_REGISTRATION(OlaThreadTest);


/*
 * Check that basic thread functionality works.
 */
void OlaThreadTest::testOlaThread() {
  MockThread thread;
  CPPUNIT_ASSERT(!thread.HasRan());
  thread.Start();
  CPPUNIT_ASSERT(thread.IsRunning());
  CPPUNIT_ASSERT(thread.Join());
  CPPUNIT_ASSERT(thread.HasRan());
}


class MockConditionThread: public OlaThread {
  public:
    MockConditionThread(
        Mutex *mutex,
        ConditionVariable *condition)
        : m_mutex(mutex),
          m_condition(condition) {}

    void *Run() {
      m_mutex->Lock();
      i = 10;
      m_mutex->Unlock();
      m_condition->Signal();
      return NULL;
    }

    int i;

  private:
    Mutex *m_mutex;
    ConditionVariable *m_condition;
};


/**
 * Check that a condition variable works
 */
void OlaThreadTest::testConditionVariable() {
  Mutex mutex;
  ConditionVariable condition;
  MockConditionThread thread(&mutex, &condition);
  thread.Start();

  mutex.Lock();
  condition.Wait(&mutex);
  CPPUNIT_ASSERT_EQUAL(10, thread.i);
  mutex.Unlock();

  thread.Join();
}
