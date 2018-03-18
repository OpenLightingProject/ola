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
 * SelectServerThreadTest.cpp
 * Test fixture for the SelectServer class that ensures Execute works
 * correctly.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/testing/TestUtils.h"

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"
// On MinGW, Thread.h pulls in pthread.h which pulls in Windows.h, which needs
// to be after WinSock2.h, hence this order
#include "ola/thread/Thread.h"

using ola::io::SelectServer;
using ola::network::UDPSocket;
using ola::thread::ThreadId;

class TestThread: public ola::thread::Thread {
 public:
    TestThread(SelectServer *ss,
               ThreadId ss_thread_id)
        : m_ss(ss),
          m_ss_thread_id(ss_thread_id),
          m_callback_executed(false) {
    }

    void *Run() {
      m_ss->Execute(
          ola::NewSingleCallback(this, &TestThread::TestCallback));
      return NULL;
    }

    void TestCallback() {
      OLA_ASSERT_TRUE(
          pthread_equal(m_ss_thread_id, ola::thread::Thread::Self()));
      m_callback_executed = true;
      m_ss->Terminate();
    }

    bool CallbackRun() const { return m_callback_executed; }

 private:
    SelectServer *m_ss;
    ThreadId m_ss_thread_id;
    bool m_callback_executed;
};


class SelectServerThreadTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SelectServerThreadTest);
  CPPUNIT_TEST(testSameThreadCallback);
  CPPUNIT_TEST(testDifferentThreadCallback);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testSameThreadCallback();
  void testDifferentThreadCallback();

 private:
  SelectServer m_ss;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SelectServerThreadTest);

/**
 * Check that a callback from the SelectServer thread executes.
 */
void SelectServerThreadTest::testSameThreadCallback() {
  TestThread test_thread(&m_ss, ola::thread::Thread::Self());
  m_ss.Execute(
      ola::NewSingleCallback(&test_thread, &TestThread::TestCallback));
  OLA_ASSERT_FALSE(test_thread.CallbackRun());
  m_ss.Run();
  OLA_ASSERT_TRUE(test_thread.CallbackRun());
}


/*
 * Check that a callback from a different thread is executed in the
 * SelectServer thread.
 */
void SelectServerThreadTest::testDifferentThreadCallback() {
  TestThread test_thread(&m_ss, ola::thread::Thread::Self());
  test_thread.Start();
  OLA_ASSERT_FALSE(test_thread.CallbackRun());
  m_ss.Run();
  test_thread.Join();
  OLA_ASSERT_TRUE(test_thread.CallbackRun());
}
