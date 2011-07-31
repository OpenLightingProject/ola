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
#include "ola/OlaThread.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"

using ola::network::SelectServer;
using ola::network::UdpSocket;
using ola::ThreadId;


class TestThread: public ola::OlaThread {
  public:
    TestThread(SelectServer *ss,
               ThreadId ss_thread_id)
        : m_ss(ss),
          m_ss_thread_id(ss_thread_id) {
    }

    void *Run() {
      m_ss->Execute(
          NewSingleCallback(this, &TestThread::TestCallback));
      return NULL;
    }

    void TestCallback() {
      CPPUNIT_ASSERT_EQUAL(m_ss_thread_id, ola::OlaThread::Self());
      m_ss->Terminate();
    }

  private:
    SelectServer *m_ss;
    ThreadId m_ss_thread_id;
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
  TestThread test_thread(&m_ss, ola::OlaThread::Self());
  m_ss.Execute(
      NewSingleCallback(&test_thread, &TestThread::TestCallback));
  m_ss.Run();
}


/*
 * Check that a callback from a different thread is executed in the
 * SelectServer thread.
 */
void SelectServerThreadTest::testDifferentThreadCallback() {
  TestThread test_thread(&m_ss, ola::OlaThread::Self());
  test_thread.Start();
  m_ss.Run();
  test_thread.Join();
}
