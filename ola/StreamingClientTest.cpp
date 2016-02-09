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
 * StreamingClientTest.cpp
 * Test fixture for the StreamingClient class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <memory>

#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/StreamingClient.h"
#include "ola/base/Flags.h"
#include "ola/network/SocketAddress.h"
#include "ola/testing/TestUtils.h"
#include "ola/thread/Thread.h"
#include "olad/OlaDaemon.h"

DECLARE_uint16(rpc_port);

static unsigned int TEST_UNIVERSE = 1;

using ola::OlaDaemon;
using ola::StreamingClient;
using ola::network::GenericSocketAddress;
using ola::thread::ConditionVariable;
using ola::thread::Mutex;
using std::auto_ptr;

class StreamingClientTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StreamingClientTest);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void tearDown();
    void testSendDMX();

 private:
    class OlaServerThread *m_server_thread;
};


CPPUNIT_TEST_SUITE_REGISTRATION(StreamingClientTest);


/*
 * The thread that the OlaServer runs in.
 */
class OlaServerThread: public ola::thread::Thread {
 public:
    OlaServerThread() :
        Thread(),
        m_is_running(false) {
    }
    ~OlaServerThread() {}
    bool Setup();
    void *Run();
    void Terminate();
    void WaitForStart();
    GenericSocketAddress RPCAddress() const;

 private:
    auto_ptr<OlaDaemon> m_olad;
    bool m_is_running;
    Mutex m_mutex;
    ConditionVariable m_condition;

    void MarkAsStarted();
};


bool OlaServerThread::Setup() {
  FLAGS_rpc_port = 0;  // pick an unused port
  ola::OlaServer::Options ola_options;
  ola_options.http_enable = false;
  ola_options.http_localhost_only = false;
  ola_options.http_enable_quit = false;
  ola_options.http_port = 0;
  ola_options.http_data_dir = "";

  // pick an unused port
  auto_ptr<OlaDaemon> olad(new OlaDaemon(ola_options, NULL));
  if (olad->Init()) {
    m_olad.reset(olad.release());
    return true;
  } else {
    return false;
  }
}


/*
 * Run the ola Server
 */
void *OlaServerThread::Run() {
  if (m_olad.get()) {
    m_olad->GetSelectServer()->Execute(
        ola::NewSingleCallback(this, &OlaServerThread::MarkAsStarted));
    m_olad->Run();
    m_olad->Shutdown();
  }
  return NULL;
}


/*
 * Stop the OLA server
 */
void OlaServerThread::Terminate() {
  if (m_olad.get()) {
    m_olad->GetSelectServer()->Terminate();
  }
}


/**
 * Block until the OLA Server is running
 */
void OlaServerThread::WaitForStart() {
  m_mutex.Lock();
  if (!m_is_running) {
    m_condition.Wait(&m_mutex);
  }
  m_mutex.Unlock();
}


void OlaServerThread::MarkAsStarted() {
  m_mutex.Lock();
  m_is_running = true;
  m_mutex.Unlock();
  m_condition.Signal();
}


GenericSocketAddress OlaServerThread::RPCAddress() const {
  if (m_olad.get()) {
    return m_olad->RPCAddress();
  }
  return GenericSocketAddress();
}


/*
 * Startup the Ola server
 */
void StreamingClientTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_server_thread = new OlaServerThread();
  if (m_server_thread->Setup()) {
    m_server_thread->Start();
  } else {
    CPPUNIT_FAIL("Failed to setup OlaDaemon");
  }
}


/*
 * Stop the ola Server
 */
void StreamingClientTest::tearDown() {
  m_server_thread->Terminate();
  m_server_thread->Join();
  delete m_server_thread;
}


/*
 * Check that the SendDMX method works correctly.
 */
void StreamingClientTest::testSendDMX() {
  m_server_thread->WaitForStart();
  GenericSocketAddress server_address = m_server_thread->RPCAddress();
  OLA_ASSERT_EQ(static_cast<uint16_t>(AF_INET), server_address.Family());
  StreamingClient::Options options;
  options.auto_start = false;
  options.server_port = server_address.V4Addr().Port();
  StreamingClient ola_client(options);

  ola::DmxBuffer buffer;
  buffer.Blackout();

  // Setup the client, this connects to the server
  OLA_ASSERT_TRUE(ola_client.Setup());
  // Try it again to make sure it doesn't break
  OLA_ASSERT_FALSE(ola_client.Setup());

  OLA_ASSERT_TRUE(ola_client.SendDmx(TEST_UNIVERSE, buffer));
  ola_client.Stop();

  // Now reconnect
  OLA_ASSERT_TRUE(ola_client.Setup());
  OLA_ASSERT_TRUE(ola_client.SendDmx(TEST_UNIVERSE, buffer));
  ola_client.Stop();

  // Now Terminate the server mid flight
  OLA_ASSERT_TRUE(ola_client.Setup());
  OLA_ASSERT_TRUE(ola_client.SendDmx(TEST_UNIVERSE, buffer));
  m_server_thread->Terminate();
  m_server_thread->Join();

  OLA_ASSERT_FALSE(ola_client.SendDmx(TEST_UNIVERSE, buffer));
  ola_client.Stop();

  OLA_ASSERT_FALSE(ola_client.Setup());
}
