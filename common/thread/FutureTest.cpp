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
 * FutureTest.cpp
 * Test fixture for the Future class
 * Copyright (C) 2013 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/thread/Future.h"
#include "ola/thread/Thread.h"
#include "ola/testing/TestUtils.h"


using ola::thread::ConditionVariable;
using ola::thread::Mutex;
using ola::thread::MutexLocker;
using ola::thread::Future;

class FutureTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(FutureTest);
  CPPUNIT_TEST(testSingleThreadedFuture);
  CPPUNIT_TEST(testSingleThreadedVoidFuture);
  CPPUNIT_TEST(testMultithreadedFuture);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testSingleThreadedFuture();
    void testSingleThreadedVoidFuture();
    void testMultithreadedFuture();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(FutureTest);

class AdderThread: public ola::thread::Thread {
 public:
    AdderThread(int i, int j, Future<int> *future)
        : Thread(),
          i(i),
          j(j),
          future(future) {
    }
    ~AdderThread() {}

    void *Run() {
      future->Set(i + j);
      return NULL;
    }


 private:
    const int i, j;
    Future<int> *future;
};

/*
 * Check that single threaded Future functionality works.
 */
void FutureTest::testSingleThreadedFuture() {
  Future<bool> f1;
  OLA_ASSERT_FALSE(f1.IsComplete());
  f1.Set(true);
  OLA_ASSERT_EQ(true, f1.Get());

  // now test a copy-constructor
  Future<bool> f2;
  Future<bool> f3(f2);
  OLA_ASSERT_FALSE(f2.IsComplete());
  OLA_ASSERT_FALSE(f3.IsComplete());
  f2.Set(true);
  OLA_ASSERT_EQ(true, f2.Get());
  OLA_ASSERT_EQ(true, f3.Get());

  // now test an assignment, this is more expensive than the copy-constructor
  Future<bool> f4;
  Future<bool> f5 = f4;
  OLA_ASSERT_FALSE(f4.IsComplete());
  OLA_ASSERT_FALSE(f5.IsComplete());
  f5.Set(false);
  OLA_ASSERT_EQ(false, f4.Get());
  OLA_ASSERT_EQ(false, f5.Get());
}

/*
 * Check that single threaded Future functionality works.
 */
void FutureTest::testSingleThreadedVoidFuture() {
  Future<void> f1;
  OLA_ASSERT_FALSE(f1.IsComplete());
  f1.Set();
  f1.Get();

  // now test a copy-constructor
  Future<void> f2;
  Future<void> f3(f2);
  OLA_ASSERT_FALSE(f2.IsComplete());
  OLA_ASSERT_FALSE(f3.IsComplete());
  f2.Set();
  f2.Get();
  f3.Get();

  // now test an assignment, this is more expensive than the copy-constructor
  Future<void> f4;
  Future<void> f5 = f4;
  OLA_ASSERT_FALSE(f4.IsComplete());
  OLA_ASSERT_FALSE(f5.IsComplete());
  f5.Set();
  f4.Get();
  f5.Get();
}

/*
 * Check that Futures work in a multithreaded environment
 */
void FutureTest::testMultithreadedFuture() {
  Future<int> f1;
  AdderThread thread(3, 5, &f1);
  OLA_ASSERT_FALSE(f1.IsComplete());
  thread.Run();
  OLA_ASSERT_EQ(8, f1.Get());
}
