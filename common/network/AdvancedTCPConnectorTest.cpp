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
 * AdvancedTCPConnectorTest.cpp
 * Test fixture for the TCPConnector class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <string>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/AdvancedTCPConnector.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketAddress.h"
#include "ola/network/Socket.h"
#include "ola/network/TCPSocketFactory.h"
#include "ola/testing/TestUtils.h"


using ola::ExponentialBackoffPolicy;
using ola::LinearBackoffPolicy;
using ola::TimeInterval;
using ola::io::SelectServer;
using ola::network::AdvancedTCPConnector;
using ola::network::GenericSocketAddress;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::network::StringToAddress;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using std::auto_ptr;
using std::string;

// used to set a timeout which aborts the tests
static const int CONNECT_TIMEOUT_IN_MS = 500;
static const int ABORT_TIMEOUT_IN_MS = 2000;

class AdvancedTCPConnectorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(AdvancedTCPConnectorTest);

  CPPUNIT_TEST(testConnect);
  CPPUNIT_TEST(testPause);
  CPPUNIT_TEST(testBackoff);
  CPPUNIT_TEST(testEarlyDestruction);
  CPPUNIT_TEST_SUITE_END();

  public:
    AdvancedTCPConnectorTest()
        : CppUnit::TestFixture(),
          m_localhost(IPV4Address::Loopback()),
          m_server_address(m_localhost, 9010) {
    }

    void setUp();
    void tearDown();
    void testConnect();
    void testPause();
    void testBackoff();
    void testEarlyDestruction();

    // timing out indicates something went wrong
    void Timeout() {
      OLA_FAIL("timeout");
    }

    // Socket close actions
    void TerminateOnClose() {
      m_ss->Terminate();
    }

  private:
    ola::MockClock m_clock;
    SelectServer *m_ss;
    auto_ptr<ola::network::TCPSocketFactory> m_tcp_socket_factory;
    IPV4Address m_localhost;
    IPV4SocketAddress m_server_address;
    ola::thread::timeout_id m_timeout_id;
    TCPSocket *m_connected_socket;

    void ConfirmState(unsigned int line,
                      AdvancedTCPConnector &connector,
                      const IPV4SocketAddress &endpoint,
                      AdvancedTCPConnector::ConnectionState state,
                      unsigned int failed_attempts);
    void SetupListeningSocket(TCPAcceptingSocket *socket);
    void AcceptedConnection(TCPSocket *socket);
    void OnConnect(TCPSocket *socket);
};

CPPUNIT_TEST_SUITE_REGISTRATION(AdvancedTCPConnectorTest);


/*
 * Setup the select server
 */
void AdvancedTCPConnectorTest::setUp() {
  m_tcp_socket_factory.reset(new ola::network::TCPSocketFactory(
      ola::NewCallback(this, &AdvancedTCPConnectorTest::OnConnect)));
  m_connected_socket = NULL;
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  m_ss = new SelectServer(NULL, &m_clock);
  m_timeout_id = m_ss->RegisterSingleTimeout(
        ABORT_TIMEOUT_IN_MS,
        ola::NewSingleCallback(this, &AdvancedTCPConnectorTest::Timeout));
  OLA_ASSERT_TRUE(m_timeout_id);
}


/*
 * Cleanup the select server
 */
void AdvancedTCPConnectorTest::tearDown() {
  delete m_ss;
}


/*
 * Test that a TCP Connect works.
 */
void AdvancedTCPConnectorTest::testConnect() {
  ola::network::TCPSocketFactory socket_factory(
      ola::NewCallback(this, &AdvancedTCPConnectorTest::AcceptedConnection));
  TCPAcceptingSocket listening_socket(&socket_factory);
  SetupListeningSocket(&listening_socket);

  AdvancedTCPConnector connector(
      m_ss,
      m_tcp_socket_factory.get(),
      TimeInterval(0, CONNECT_TIMEOUT_IN_MS * 1000));

  // 5 per attempt, up to a max of 30
  LinearBackoffPolicy policy(TimeInterval(5, 0), TimeInterval(30, 0));
  connector.AddEndpoint(m_server_address, &policy);
  OLA_ASSERT_EQ(1u, connector.EndpointCount());

  m_ss->Run();
  OLA_ASSERT_EQ(1u, connector.EndpointCount());

  // confirm the status is correct
  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::CONNECTED, 0);

  // check our socket exists
  OLA_ASSERT_TRUE(m_connected_socket);
  m_connected_socket->Close();
  delete m_connected_socket;
  OLA_INFO << "disconnecting";
  connector.Disconnect(m_server_address, true);

  // state should be updated
  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::PAUSED, 0);

  // remove & shutdown
  connector.RemoveEndpoint(m_server_address);
  OLA_ASSERT_EQ(0u, connector.EndpointCount());
  m_ss->RemoveReadDescriptor(&listening_socket);
}


/**
 * Test that pausing a connection works.
 */
void AdvancedTCPConnectorTest::testPause() {
  ola::network::TCPSocketFactory socket_factory(
      ola::NewCallback(this, &AdvancedTCPConnectorTest::AcceptedConnection));
  TCPAcceptingSocket listening_socket(&socket_factory);
  SetupListeningSocket(&listening_socket);

  AdvancedTCPConnector connector(
      m_ss,
      m_tcp_socket_factory.get(),
      TimeInterval(0, CONNECT_TIMEOUT_IN_MS * 1000));

  // 5 per attempt, up to a max of 30
  LinearBackoffPolicy policy(TimeInterval(5, 0), TimeInterval(30, 0));
  // add endpoint, but make sure it's paused
  connector.AddEndpoint(m_server_address, &policy, true);
  OLA_ASSERT_EQ(1u, connector.EndpointCount());

  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::PAUSED, 0);

  m_ss->RunOnce(0, 500000);

  // now unpause
  connector.Resume(m_server_address);
  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::DISCONNECTED, 0);

  m_ss->Run();

  OLA_ASSERT_EQ(1u, connector.EndpointCount());
  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::CONNECTED, 0);

  // check our socket exists
  OLA_ASSERT_TRUE(m_connected_socket);
  m_connected_socket->Close();
  delete m_connected_socket;
  OLA_INFO << "disconnecting";
  connector.Disconnect(m_server_address, true);

  // state should be updated
  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::PAUSED, 0);

  // clean up
  connector.RemoveEndpoint(m_server_address);
  OLA_ASSERT_EQ(0u, connector.EndpointCount());

  m_ss->RemoveReadDescriptor(&listening_socket);
}


/**
 * Test that backoff works.
 * This is quite brittle and should be fixed at some stage.
 */
void AdvancedTCPConnectorTest::testBackoff() {
  // we advance the clock so remove the timeout closure
  m_ss->RemoveTimeout(m_timeout_id);
  m_timeout_id = ola::thread::INVALID_TIMEOUT;

  AdvancedTCPConnector connector(
      m_ss,
      m_tcp_socket_factory.get(),
      TimeInterval(0, CONNECT_TIMEOUT_IN_MS * 1000));

  // 5s per attempt, up to a max of 30
  LinearBackoffPolicy policy(TimeInterval(5, 0), TimeInterval(30, 0));
  connector.AddEndpoint(m_server_address, &policy);
  OLA_ASSERT_EQ(1u, connector.EndpointCount());

  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::DISCONNECTED, 0);

  // the timeout is 500ms, so we advance by 490 and set a 200ms timeout
  m_clock.AdvanceTime(0, 490000);
  m_ss->RunOnce(0, 200000);

  // should have one failure at this point
  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::DISCONNECTED, 1);

  // the next attempt should be in 5 seconds
  m_clock.AdvanceTime(4, 900000);
  m_ss->RunOnce(1, 0);

  // wait for the timeout
  m_clock.AdvanceTime(0, 490000);
  m_ss->RunOnce(0, 200000);

  ConfirmState(__LINE__, connector, m_server_address,
               AdvancedTCPConnector::DISCONNECTED, 2);

  // run once more to clean up
  m_ss->RunOnce(0, 10000);

  // clean up
  connector.RemoveEndpoint(m_server_address);
  OLA_ASSERT_EQ(0u, connector.EndpointCount());
}


/*
 * Test that we can destroy the Connector and everything will work.
 */
void AdvancedTCPConnectorTest::testEarlyDestruction() {
  // 5 per attempt, up to a max of 30
  LinearBackoffPolicy policy(TimeInterval(5, 0), TimeInterval(30, 0));

  {
    AdvancedTCPConnector connector(
        m_ss,
        m_tcp_socket_factory.get(),
        TimeInterval(0, CONNECT_TIMEOUT_IN_MS * 1000));

    connector.AddEndpoint(m_server_address, &policy);
    OLA_ASSERT_EQ(1u, connector.EndpointCount());
  }

  m_ss->RunOnce();
}


/**
 * Confirm the state & failed attempts matches what we expected
 */
void AdvancedTCPConnectorTest::ConfirmState(
    unsigned int line,
    AdvancedTCPConnector &connector,
    const IPV4SocketAddress &endpoint,
    AdvancedTCPConnector::ConnectionState expected_state,
    unsigned int expected_attempts) {
  std::stringstream str;
  str << "Line " << line;

  AdvancedTCPConnector::ConnectionState state;
  unsigned int failed_attempts;
  OLA_ASSERT_TRUE_MSG(
      connector.GetEndpointState(endpoint, &state, &failed_attempts)
      str.str());
  OLA_ASSERT_EQ_MSG(str.str(), expected_state, state);
  OLA_ASSERT_EQ_MSG(str.str(), expected_attempts, failed_attempts);
}



/**
 * Setup a TCP socket that accepts connections
 */
void AdvancedTCPConnectorTest::SetupListeningSocket(
    TCPAcceptingSocket *listening_socket) {
  OLA_ASSERT_TRUE_MSG(listening_socket->Listen(m_server_address),
                         "Check for another instance of olad running");
  // calling listen a second time should fail
  OLA_ASSERT_FALSE(listening_socket->Listen(m_server_address));
  OLA_INFO << "listening on " << m_server_address;
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(listening_socket));
}


/*
 * Accept a new TCP connection.
 */
void AdvancedTCPConnectorTest::AcceptedConnection(TCPSocket *new_socket) {
  OLA_ASSERT_NOT_NULL(new_socket);
  GenericSocketAddress address = new_socket->GetPeer();
  OLA_ASSERT_TRUE(address.Family() == AF_INET);
  OLA_INFO << "Connection from " << address;

  // terminate the ss when this connection is closed
  new_socket->SetOnClose(
    ola::NewSingleCallback(this, &AdvancedTCPConnectorTest::TerminateOnClose));
  m_ss->AddReadDescriptor(new_socket, true);
}

/*
 * Called when a connection completes or times out.
 */
void AdvancedTCPConnectorTest::OnConnect(TCPSocket *socket) {
  OLA_ASSERT_NOT_NULL(socket);

  GenericSocketAddress address = socket->GetPeer();
  OLA_ASSERT_TRUE(address.Family() == AF_INET);
  OLA_ASSERT_EQ(m_localhost, address.V4Addr().Host());

  m_connected_socket = socket;
  m_ss->Terminate();
}
