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
 * SelectServerThreadTest.cpp
 * Test fixture for the SelectServer class that ensures Execute works
 * correctly.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/thread/Thread.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"

using ola::io::SelectServer;
using ola::network::UdpSocket;
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
      CPPUNIT_ASSERT_EQUAL(m_ss_thread_id, ola::thread::Thread::Self());
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
    void setUp();
    void tearDown();
    void testSameThreadCallback();
    void testDifferentThreadCallback();

  private:
    SelectServer m_ss;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SelectServerThreadTest);


void SelectServerThreadTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
}


void SelectServerThreadTest::tearDown() {
}


/**
 * Check that a callback from the SelectServer thread executes.
 */
void SelectServerThreadTest::testSameThreadCallback() {
  TestThread test_thread(&m_ss, ola::thread::Thread::Self());
  m_ss.Execute(
      ola::NewSingleCallback(&test_thread, &TestThread::TestCallback));
  CPPUNIT_ASSERT(!test_thread.CallbackRun());
  m_ss.Run();
  CPPUNIT_ASSERT(test_thread.CallbackRun());
}


/*
 * Check that a callback from a different thread is executed in the
 * SelectServer thread.
 */
void SelectServerThreadTest::testDifferentThreadCallback() {
  TestThread test_thread(&m_ss, ola::thread::Thread::Self());
  test_thread.Start();
  CPPUNIT_ASSERT(!test_thread.CallbackRun());
  m_ss.Run();
  test_thread.Join();
  CPPUNIT_ASSERT(test_thread.CallbackRun());
}
