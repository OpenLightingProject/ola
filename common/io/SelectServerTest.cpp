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
 * SelectServerTest.cpp
 * Test fixture for the Socket classes
 * Copyright (C) 2005 Simon Newton
 */

#ifdef _WIN32
#include <Winsock2.h>
#endif

#include <cppunit/extensions/HelperMacros.h>
#include <sstream>

#include "common/io/PollerInterface.h"
#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/ExportMap.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/testing/TestUtils.h"

using ola::ExportMap;
using ola::IntegerVariable;
using ola::io::ConnectedDescriptor;
using ola::io::LoopbackDescriptor;
using ola::io::PollerInterface;
using ola::io::SelectServer;
using ola::io::UnixSocket;
using ola::network::UDPSocket;
using ola::NewSingleCallback;
using ola::TimeStamp;
using std::auto_ptr;

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
  CPPUNIT_TEST(testAddInvalidDescriptor);
  CPPUNIT_TEST(testDoubleAddAndRemove);
  CPPUNIT_TEST(testAddRemoveReadDescriptor);
  CPPUNIT_TEST(testRemoteEndClose);
  CPPUNIT_TEST(testRemoteEndCloseWithDelete);
  CPPUNIT_TEST(testRemoteEndCloseWithRemoveAndDelete);
#ifndef _WIN32
  CPPUNIT_TEST(testReadWriteInteraction);
#endif
  CPPUNIT_TEST(testShutdownWithActiveDescriptors);
  CPPUNIT_TEST(testTimeout);
  CPPUNIT_TEST(testOffByOneTimeout);
  CPPUNIT_TEST(testLoopCallbacks);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void tearDown();
    void testAddInvalidDescriptor();
    void testDoubleAddAndRemove();
    void testAddRemoveReadDescriptor();
    void testRemoteEndClose();
    void testRemoteEndCloseWithDelete();
    void testRemoteEndCloseWithRemoveAndDelete();
    void testReadWriteInteraction();
    void testShutdownWithActiveDescriptors();
    void testTimeout();
    void testOffByOneTimeout();
    void testLoopCallbacks();

    void FatalTimeout() {
      OLA_FAIL("Fatal Timeout");
    }

    void Terminate() {
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

    void RemoveFromSelectServer(ConnectedDescriptor *descriptor) {
      if (m_ss) {
        m_ss->RemoveReadDescriptor(descriptor);
        m_ss->Terminate();
      }
    }

    void IncrementLoopCounter() { m_loop_counter++; }

 private:
    unsigned int m_timeout_counter;
    unsigned int m_loop_counter;
    ExportMap m_map;
    IntegerVariable *connected_read_descriptor_count;
    IntegerVariable *read_descriptor_count;
    IntegerVariable *write_descriptor_count;
    SelectServer *m_ss;
};


CPPUNIT_TEST_SUITE_REGISTRATION(SelectServerTest);

void SelectServerTest::setUp() {
  connected_read_descriptor_count = m_map.GetIntegerVar(
      PollerInterface::K_CONNECTED_DESCRIPTORS_VAR);
  read_descriptor_count = m_map.GetIntegerVar(
      PollerInterface::K_READ_DESCRIPTOR_VAR);
  write_descriptor_count = m_map.GetIntegerVar(
      PollerInterface::K_WRITE_DESCRIPTOR_VAR);

  m_ss = new SelectServer(&m_map);
  m_timeout_counter = 0;
  m_loop_counter = 0;

#if _WIN32
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2, 0), &wsa_data);
  OLA_ASSERT_EQ(result, 0);
#endif
}

void SelectServerTest::tearDown() {
  delete m_ss;

#ifdef _WIN32
  WSACleanup();
#endif
}

/*
 * Confirm we can't add invalid descriptors to the SelectServer
 */
void SelectServerTest::testAddInvalidDescriptor() {
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());  // internal socket
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());

  // Adding and removing a uninitialized socket should fail
  LoopbackDescriptor bad_socket;
  OLA_ASSERT_FALSE(m_ss->AddReadDescriptor(&bad_socket));
  OLA_ASSERT_FALSE(m_ss->AddWriteDescriptor(&bad_socket));
  m_ss->RemoveReadDescriptor(&bad_socket);
  m_ss->RemoveWriteDescriptor(&bad_socket);

  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());  // internal socket
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());
}

/*
 * Confirm we can't add the same descriptor twice.
 */
void SelectServerTest::testDoubleAddAndRemove() {
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());  // internal socket
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());

  LoopbackDescriptor loopback_socket;
  loopback_socket.Init();

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&loopback_socket));
  OLA_ASSERT_EQ(2, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());

  OLA_ASSERT_TRUE(m_ss->AddWriteDescriptor(&loopback_socket));
  OLA_ASSERT_EQ(2, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(1, write_descriptor_count->Get());

  m_ss->RemoveReadDescriptor(&loopback_socket);
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(1, write_descriptor_count->Get());

  m_ss->RemoveWriteDescriptor(&loopback_socket);
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());

  // Trying to remove a second time shouldn't crash
  m_ss->RemoveReadDescriptor(&loopback_socket);
  m_ss->RemoveWriteDescriptor(&loopback_socket);
}


/*
 * Check AddReadDescriptor/RemoveReadDescriptor works correctly and that the
 * export map is updated.
 */
void SelectServerTest::testAddRemoveReadDescriptor() {
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());

  LoopbackDescriptor bad_socket;
  // adding and removing a non-connected socket should fail
  OLA_ASSERT_FALSE(m_ss->AddReadDescriptor(&bad_socket));
  m_ss->RemoveReadDescriptor(&bad_socket);

  LoopbackDescriptor loopback_socket;
  loopback_socket.Init();

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&loopback_socket));
  OLA_ASSERT_EQ(2, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());

  // Add a udp socket
  UDPSocket udp_socket;
  OLA_ASSERT_TRUE(udp_socket.Init());
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&udp_socket));
  OLA_ASSERT_EQ(2, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(1, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());

  // Check remove works
  m_ss->RemoveReadDescriptor(&loopback_socket);
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(1, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());

  m_ss->RemoveReadDescriptor(&udp_socket);
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, write_descriptor_count->Get());
}

/*
 * Confirm we correctly detect the remote end closing the connection.
 */
void SelectServerTest::testRemoteEndClose() {
  LoopbackDescriptor loopback_socket;
  loopback_socket.Init();
  loopback_socket.SetOnClose(NewSingleCallback(
      this, &SelectServerTest::RemoveFromSelectServer,
      reinterpret_cast<ConnectedDescriptor*>(&loopback_socket)));

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&loopback_socket));
  OLA_ASSERT_EQ(2, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());

  // now the Write end closes
  loopback_socket.CloseClient();

  m_ss->Run();
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
}

/*
 * Confirm we correctly detect the remote end closing the connection.
 * This uses the delete_on_close option.
 */
void SelectServerTest::testRemoteEndCloseWithDelete() {
  // Ownership is transferred to the SelectServer.
  LoopbackDescriptor *loopback_socket = new LoopbackDescriptor();
  loopback_socket->Init();
  loopback_socket->SetOnClose(NewSingleCallback(
      this, &SelectServerTest::Terminate));

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(loopback_socket, true));
  OLA_ASSERT_EQ(2, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());

  // Now the Write end closes
  loopback_socket->CloseClient();

  m_ss->Run();
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
}

/*
 * This is a tricky case to test reentrancy. We set an OnClose handler that also
 * removes the LoopbackDescriptor from the SelectServer. We also set
 * delete_on_close to true.
 *
 * This makes sure that the delete_on_close code handles the descriptor being
 * removed from within the OnClose handler.
 */
void SelectServerTest::testRemoteEndCloseWithRemoveAndDelete() {
  // Ownership is transferred to the SelectServer.
  LoopbackDescriptor *loopback_socket = new LoopbackDescriptor();
  loopback_socket->Init();
  loopback_socket->SetOnClose(NewSingleCallback(
      this, &SelectServerTest::RemoveFromSelectServer,
      reinterpret_cast<ConnectedDescriptor*>(loopback_socket)));

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(loopback_socket, true));
  OLA_ASSERT_EQ(2, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());

  // Now the Write end closes
  loopback_socket->CloseClient();

  m_ss->Run();
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
}


/*
 * Test the interaction between read and write descriptor.
 */
void SelectServerTest::testReadWriteInteraction() {
  UnixSocket socket;
  socket.Init();
  socket.SetOnClose(NewSingleCallback(this, &SelectServerTest::Terminate));

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&socket));
  OLA_ASSERT_TRUE(m_ss->AddWriteDescriptor(&socket));
  m_ss->RemoveWriteDescriptor(&socket);

  // now the Write end closes
  auto_ptr<UnixSocket> other_end(socket.OppositeEnd());
  other_end->CloseClient();

  m_ss->RegisterSingleTimeout(
      100, ola::NewSingleCallback(this, &SelectServerTest::FatalTimeout));
  m_ss->Run();
  m_ss->RemoveReadDescriptor(&socket);
  OLA_ASSERT_EQ(1, connected_read_descriptor_count->Get());
  OLA_ASSERT_EQ(0, read_descriptor_count->Get());
}

/*
 * Confirm we don't leak memory when the SelectServer is destroyed without all
 * the descriptors being removed.
 */
void SelectServerTest::testShutdownWithActiveDescriptors() {
  LoopbackDescriptor loopback_socket;
  loopback_socket.Init();

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&loopback_socket));
  OLA_ASSERT_TRUE(m_ss->AddWriteDescriptor(&loopback_socket));
}

/*
 * Timeout tests
 */
void SelectServerTest::testTimeout() {
  // Check a single timeout
  m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::SingleIncrementTimeout));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::Terminate));
  m_ss->Run();
  OLA_ASSERT_EQ(1u, m_timeout_counter);

  // Now check a timeout that adds another timeout
  m_timeout_counter = 0;

  m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::ReentrantTimeout, m_ss));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::Terminate));
  m_ss->Run();
  OLA_ASSERT_EQ(2u, m_timeout_counter);

  // Check repeating timeouts
  // Some systems (VMs in particular) can't do 10ms resolution so we go for
  // larger numbers here.
  m_timeout_counter = 0;
  m_ss->RegisterRepeatingTimeout(
      100,
      ola::NewCallback(this, &SelectServerTest::IncrementTimeout));
  m_ss->RegisterSingleTimeout(
      980,
      ola::NewSingleCallback(this, &SelectServerTest::Terminate));
  m_ss->Run();
  // This seems to go as low as 7
  std::ostringstream str;
  str << "Timeout counter was " << m_timeout_counter;
  OLA_ASSERT_TRUE_MSG(m_timeout_counter >= 5 && m_timeout_counter <= 9,
                      str.str());

  // Confirm timeouts are removed correctly
  ola::thread::timeout_id timeout1 = m_ss->RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::FatalTimeout));
  m_ss->RegisterSingleTimeout(
      20,
      ola::NewSingleCallback(this, &SelectServerTest::Terminate));
  m_ss->RemoveTimeout(timeout1);
  m_ss->Run();
}

/*
 * Test that timeouts aren't skipped.
 */
void SelectServerTest::testOffByOneTimeout() {
  TimeStamp now;
  ola::Clock actual_clock;
  actual_clock.CurrentTime(&now);

  CustomMockClock clock(&now);
  SelectServer ss(NULL, &clock);

  ss.RegisterSingleTimeout(
      10,
      ola::NewSingleCallback(this, &SelectServerTest::SingleIncrementTimeout));

  now += ola::TimeInterval(0, 10000);
  ss.m_timeout_manager->ExecuteTimeouts(&now);
  OLA_ASSERT_EQ(m_timeout_counter, 1u);
}


/*
 * Check that the loop closures are called.
 */
void SelectServerTest::testLoopCallbacks() {
  m_ss->SetDefaultInterval(ola::TimeInterval(0, 100000));  // poll every 100ms
  m_ss->RunInLoop(
      ola::NewCallback(this, &SelectServerTest::IncrementLoopCounter));
  m_ss->RegisterSingleTimeout(
      500,
      ola::NewSingleCallback(this, &SelectServerTest::Terminate));
  m_ss->Run();
  // we should have at least 5 calls to IncrementLoopCounter
  OLA_ASSERT_TRUE(m_loop_counter >= 5);
}
