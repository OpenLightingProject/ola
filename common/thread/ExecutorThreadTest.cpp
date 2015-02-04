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
 * ExecutorThreadTest.cpp
 * Test fixture for the ExecutorThread class
 * Copyright (C) 2015 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/thread/ExecutorThread.h"
#include "ola/thread/Future.h"
#include "ola/testing/TestUtils.h"

using ola::NewSingleCallback;
using ola::thread::Future;
using ola::thread::ExecutorThread;

namespace {
void SetFuture(Future<void>* f) {
  f->Set();
}
}  // namespace

class ExecutorThreadTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ExecutorThreadTest);
  CPPUNIT_TEST(test);
  CPPUNIT_TEST_SUITE_END();

 public:
  void test();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ExecutorThreadTest);

/*
 * Check that callbacks are executed correctly.
 */
void ExecutorThreadTest::test() {
  Future<void> f1;
  {
    ola::thread::Thread::Options options;
    ExecutorThread thread(options);
    OLA_ASSERT_TRUE(thread.Start());

    Future<void> f2;
    thread.Execute(NewSingleCallback(SetFuture, &f2));
    f2.Get();

    OLA_ASSERT_TRUE(thread.Stop());

    // Try stop a second time.
    OLA_ASSERT_FALSE(thread.Stop());

    // Confirm that all callbacks are run when the thread is destroyed.
    thread.Execute(NewSingleCallback(SetFuture, &f1));
  }
  f1.Get();
}
