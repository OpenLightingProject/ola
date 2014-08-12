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
 * ThreadPoolTest.cpp
 * Test fixture for the ThreadPool class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/thread/Thread.h"
#include "ola/thread/ThreadPool.h"
#include "ola/testing/TestUtils.h"



using ola::thread::Mutex;
using ola::thread::MutexLocker;
using ola::thread::ThreadPool;


class ThreadPoolTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ThreadPoolTest);
  CPPUNIT_TEST(test1By10);
  CPPUNIT_TEST(test2By10);
  CPPUNIT_TEST(test10By100);
  CPPUNIT_TEST_SUITE_END();

 public:
    void test1By10() {
      RunThreads(1, 10);
    }
    void test2By10() {
      RunThreads(2, 10);
    }
    void test10By100() {
      RunThreads(10, 100);
    }

    void setUp() {
      m_counter = 0;
    }

 private:
    unsigned int m_counter;
    Mutex m_mutex;

    void IncrementCounter() {
      MutexLocker locker(&m_mutex);
      m_counter++;
    }

    void RunThreads(unsigned int threads, unsigned int actions);
};


CPPUNIT_TEST_SUITE_REGISTRATION(ThreadPoolTest);


/**
 * Run threads and add actions to the queue
 */
void ThreadPoolTest::RunThreads(unsigned int threads, unsigned int actions) {
  ThreadPool pool(threads);
  OLA_ASSERT_TRUE(pool.Init());

  for (unsigned int i = 0; i < actions; i++)
    pool.Execute(
        ola::NewSingleCallback(this, &ThreadPoolTest::IncrementCounter));

  pool.JoinAll();
  OLA_ASSERT_EQ(static_cast<unsigned int>(actions), m_counter);
}
