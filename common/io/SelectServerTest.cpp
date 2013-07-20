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
 * SelectServerTest.cpp
 * Test fixture for the Socket classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <sstream>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/testing/TestUtils.h"

using ola::ExportMap;
using ola::IntegerVariable;
using ola::TimeStamp;
using ola::io::LoopbackDescriptor;
using ola::io::SelectServer;
using ola::network::UDPSocket;

/*
 * For some of the tests we need precise control over the timing.
 */
class CustomMockClock: public ola::Clock {
  public:
    explicit CustomMockClock(TimeStamp *timestamp)
        : m_timestamp(timestamp) {
    }

    void CurrentTime(TimeStamp *timestamp) const {
      *timestamp = *m_timestamp;
    }

  private:
    TimeStamp *m_timestamp;
};

class SelectServerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SelectServerTest);
  CPPUNIT_TEST(testAddRemoveReadDescriptor);
  CPPUNIT_TEST(testTimeout);
  CPPUNIT_TEST(testOffByOneTimeout);
  CPPUNIT_TEST(testLoopCallbacks);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testAddRemoveReadDescriptor();
    void testTimeout();
    void testOffByOneTimeout();
    void testLoopCallbacks();

    void FatalTimeout() {
      OLA_FAIL("Fatal Timeout");
    }

    void TerminateTimeout() {
      if (m_ss) { m_ss->Terminate(); }
    }

    void SingleIncrementTimeout() {
      m_timeout_counter++;
    }

    void ReentrantTimeout(SelectServer *ss) {
      ss->RegisterSingleTimeout(
          0,
          ola::NewSingleCallback(this,
                                 &SelectServerTest::SingleIncrementTimeout));
      ss->RegisterSingleTimeout(
          5,
          ola::NewSingleCallback(this,
                                 &SelectServerTest::SingleIncrementTimeout));
    }

    bool IncrementTimeout() {
      if (m_ss && m_ss->IsRunning())
        m_timeout_counter++;
      return true;
    }

    void IncrementLoopCounter() { m_loop_counter++; }

  private:
    unsigned int m_timeout_counter;
    unsigned int m_loop_counter;
    ExportMap *m_map;
    SelectServer *m_ss;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SelectServerTest);


void SelectServerTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_map = new ExportMap();
  m_ss = new SelectServer(m_map);
  m_timeout_counter = 0;
  m_loop_counter = 0;
}


void SelectServerTest::tearDown() {
  delete m_ss;
  delete m_map;
}


/*
 * Check AddReadDescriptor/RemoveReadDescriptor works correctly and that the
 * export map is updated.
 */
void SelectServerTest::testAddRemoveReadDescriptor() {
  LoopbackDescriptor bad_socket;
  IntegerVariable *connected_socket_count =
    m_map->GetIntegerVar(SelectServer::K_CONNECTED_DESCRIPTORS_VAR);
  IntegerVariable *socket_count =
    m_map->GetIntegerVar(SelectServer::K_READ_DESCRIPTOR_VAR);
  OLA_ASSERT_EQ(0, connected_socket_count->Get());
  OLA_ASSERT_EQ(0, socket_count->Get());
  // adding and removin a non-connected socket should fail
  OLA_ASSERT_FALSE(m_ss->AddReadDescriptor(&bad_socket));
  OLA_ASSERT_FALSE(m_ss->RemoveReadDescriptor(&bad_socket));

  LoopbackDescriptor loopback_socket;
  loopback_socket.Init();
  OLA_ASSERT_EQ(0, connected_socket_count->Get());
  OLA_ASSERT_EQ(0, socket_count->Get());
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&loopback_socket));
  // Adding a second time should fail
  OLA_ASSERT_FALSE(m_ss->AddReadDescriptor(&loopback_socket));
  OLA_ASSERT_EQ(1, connected_socket_count->Get());
  OLA_ASSERT_EQ(0, socket_count->Get());

  // Add a udp socket
  UDPSocket udp_socket;
  OLA_ASSERT_TRUE(udp_socket.Init());
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&udp_socket));
  OLA_ASSERT_FALSE(m_ss->AddReadDescriptor(&udp_socket));
  OLA_ASSERT_EQ(1, connected_socket_count->Get());
  OLA_ASSERT_EQ(1, socket_count->Get());

  // Check remove works
  OLA_ASSERT_TRUE(m_ss->RemoveReadDescriptor(&loopback_socket));
  OLA_ASSERT_EQ(0, connected_socket_count->Get());
  OLA_ASSERT_EQ(1, socket_count->Get());
  OLA_ASSERT_TRUE(m_ss->RemoveReadDescriptor(&udp_socket));
  OLA_ASSERT_EQ(0, connected_socket_count->Get());
  OLA_ASSERT_EQ(0, socket_count->Get());

  // Remove again should fail
  OLA_ASSERT_FALSE(m_ss->RemoveReadDescriptor(&loopback_socket));
}


/*
 * Timeout tests
 */
void SelectServerTest::testTimeout() {
  // check a single timeout
  m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::SingleIncrementTimeout));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->Run();
  OLA_ASSERT_EQ(1u, m_timeout_counter);

  // now check a timeout that adds another timeout
  m_timeout_counter = 0;

  m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::ReentrantTimeout, m_ss));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->Run();
  OLA_ASSERT_EQ(2u, m_timeout_counter);

  // check repeating timeouts
  // Some systems (VMs in particular) can't do 10ms resolution so we go for
  // larger numbers here.
  m_timeout_counter = 0;
  m_ss->RegisterRepeatingTimeout(
      100,
      ola::NewCallback(this, &SelectServerTest::IncrementTimeout));
  m_ss->RegisterSingleTimeout(
      980,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->Run();
  // This seems to go as low as 7
  std::stringstream str;
  str << "Timeout counter was " << m_timeout_counter;
  OLA_ASSERT_TRUE_MSG(m_timeout_counter >= 5 && m_timeout_counter <= 9,
                      str.str());

  // check timeouts are removed correctly
  ola::thread::timeout_id timeout1 = m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::FatalTimeout));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->RemoveTimeout(timeout1);
  m_ss->Run();
}

/*
 * Test that timeouts aren't skipped
 */
void SelectServerTest::testOffByOneTimeout() {
  TimeStamp now;
  ola::Clock actual_clock;
  actual_clock.CurrentTime(&now);

  CustomMockClock clock(&now);;
  SelectServer ss(NULL, &clock);

  ss.RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::SingleIncrementTimeout));

  now += ola::TimeInterval(0, 10000);
  ss.CheckTimeouts(now);
  OLA_ASSERT_EQ(m_timeout_counter, 1u);
}


/*
 * Check that the loop closures are called
 */
void SelectServerTest::testLoopCallbacks() {
  m_ss->SetDefaultInterval(ola::TimeInterval(0, 100000));  // poll every 100ms
  m_ss->RunInLoop(ola::NewCallback(this,
                                   &SelectServerTest::IncrementLoopCounter));
  m_ss->RegisterSingleTimeout(
      500,
      ola::NewSingleCallback(this, &SelectServerTest::TerminateTimeout));
  m_ss->Run();
  // we should have at least 5 calls to IncrementLoopCounter
  OLA_ASSERT_TRUE(m_loop_counter >= 5);
}
