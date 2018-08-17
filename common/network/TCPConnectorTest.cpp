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
 * TCPConnectorTest.cpp
 * Test fixture for the TCPConnector class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <string.h>
#include <string>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SocketAddress.h"
#include "ola/network/Socket.h"
#include "ola/network/TCPConnector.h"
#include "ola/network/TCPSocketFactory.h"
#include "ola/testing/TestUtils.h"


using ola::TimeInterval;
using ola::io::ConnectedDescriptor;
using ola::network::GenericSocketAddress;
using ola::network::IPV4Address;
using ola::network::IPV4SocketAddress;
using ola::io::SelectServer;
using ola::network::TCPConnector;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using std::auto_ptr;
using std::string;

// used to set a timeout which aborts the tests
static const int CONNECT_TIMEOUT_IN_MS = 500;
static const int ABORT_TIMEOUT_IN_MS = 1000;

class TCPConnectorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TCPConnectorTest);

  CPPUNIT_TEST(testNonBlockingConnect);
  CPPUNIT_TEST(testNonBlockingConnectFailure);
  CPPUNIT_TEST(testNonBlockingConnectError);
  CPPUNIT_TEST(testNonBlockingCancel);
  CPPUNIT_TEST(testEarlyDestruction);
  CPPUNIT_TEST_SUITE_END();

 public:
    TCPConnectorTest()
        : CppUnit::TestFixture(),
          m_localhost(IPV4Address::Loopback()) {
    }

    void setUp();
    void tearDown();
    void testNonBlockingConnect();
    void testNonBlockingConnectFailure();
    void testNonBlockingConnectError();
    void testNonBlockingCancel();
    void testEarlyDestruction();

    // timing out indicates something went wrong
    void Timeout() {
      m_timeout_closure = NULL;
      OLA_FAIL("timeout");
    }

    // Socket close actions
    void TerminateOnClose() {
      m_ss->Terminate();
    }

 private:
    SelectServer *m_ss;
    IPV4Address m_localhost;
    ola::SingleUseCallback0<void> *m_timeout_closure;
    unsigned int m_successful_calls;
    unsigned int m_failure_calls;

    void AcceptedConnection(TCPSocket *socket);
    void OnConnect(int fd, int error);
    void OnConnectFailure(int fd, int error);
    uint16_t ReservePort();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPConnectorTest);


/*
 * Setup the select server
 */
void TCPConnectorTest::setUp() {
  m_ss = new SelectServer();
  m_timeout_closure = ola::NewSingleCallback(this, &TCPConnectorTest::Timeout);
  m_successful_calls = 0;
  m_failure_calls = 0;
  OLA_ASSERT_TRUE(m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS,
                                              m_timeout_closure));

#if _WIN32
  WSADATA wsa_data;
  int result = WSAStartup(MAKEWORD(2, 0), &wsa_data);
  OLA_ASSERT_EQ(result, 0);
#endif  // _WIN32
}


/*
 * Cleanup the select server
 */
void TCPConnectorTest::tearDown() {
  delete m_ss;

#ifdef _WIN32
  WSACleanup();
#endif  // _WIN32
}


/*
 * Test non-blocking TCP connects work correctly.
 */
void TCPConnectorTest::testNonBlockingConnect() {
  ola::network::TCPSocketFactory socket_factory(
      ola::NewCallback(this, &TCPConnectorTest::AcceptedConnection));


  TCPAcceptingSocket listening_socket(&socket_factory);
  IPV4SocketAddress listen_address(m_localhost, 0);
  OLA_ASSERT_TRUE_MSG(listening_socket.Listen(listen_address),
                      "Failed to listen");
  GenericSocketAddress addr = listening_socket.GetLocalAddress();
  OLA_ASSERT_TRUE(addr.IsValid());

  // calling listen a second time should fail
  OLA_ASSERT_FALSE(listening_socket.Listen(listen_address));

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&listening_socket));

  // Attempt a non-blocking connect
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      addr.V4Addr(),
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnect));

  if (id) {
    OLA_ASSERT_EQ(1u, connector.ConnectionsPending());
    m_ss->Run();
    OLA_ASSERT_EQ(0u, connector.ConnectionsPending());
  }

  OLA_ASSERT_EQ(1u, m_successful_calls);
  OLA_ASSERT_EQ(0u, m_failure_calls);
  m_ss->RemoveReadDescriptor(&listening_socket);
}


/*
 * Test a non-blocking TCP connect fails.
 */
void TCPConnectorTest::testNonBlockingConnectFailure() {
  uint16_t port = ReservePort();
  OLA_ASSERT_NE(0, port);
  IPV4SocketAddress target(m_localhost, port);

  // attempt a non-blocking connect, hopefully nothing is running on port 9010
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      target,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  // On platforms where connect() doesn't return EINPROGRESS, it's hard to
  // actually test this without knowing a non-local address.
  if (id) {
    m_ss->Run();
    OLA_ASSERT_EQ(0u, connector.ConnectionsPending());
  }
  OLA_ASSERT_EQ(0u, m_successful_calls);
  OLA_ASSERT_EQ(1u, m_failure_calls);
}


/*
 * Test a non-blocking TCP connect for an invalid port.
 */
void TCPConnectorTest::testNonBlockingConnectError() {
  IPV4Address bcast_address;
  OLA_ASSERT_TRUE(IPV4Address::FromString("255.255.255.255", &bcast_address));

  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      IPV4SocketAddress(bcast_address, 0),
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  OLA_ASSERT_FALSE(id);
  OLA_ASSERT_EQ(0u, connector.ConnectionsPending());
  OLA_ASSERT_EQ(0u, m_successful_calls);
  OLA_ASSERT_EQ(1u, m_failure_calls);
}


/*
 * Test that cancelling a connect works.
 */
void TCPConnectorTest::testNonBlockingCancel() {
  uint16_t port = ReservePort();
  OLA_ASSERT_NE(0, port);
  IPV4SocketAddress target(m_localhost, port);

  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      target,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  // On platforms where connect() doesn't return EINPROGRESS, it's hard to
  // actually test this without knowing a non-local address.
  if (id) {
    OLA_ASSERT_EQ(1u, connector.ConnectionsPending());
    OLA_ASSERT_TRUE(connector.Cancel(id));
    OLA_ASSERT_EQ(0u, connector.ConnectionsPending());
  }
  OLA_ASSERT_EQ(0u, m_successful_calls);
  OLA_ASSERT_EQ(1u, m_failure_calls);
}


/*
 * Test that we can destroy the Connector and everything will work.
 */
void TCPConnectorTest::testEarlyDestruction() {
  uint16_t port = ReservePort();
  OLA_ASSERT_NE(0, port);
  IPV4SocketAddress target(m_localhost, port);

  // attempt a non-blocking connect.
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  {
    TCPConnector connector(m_ss);
    TCPConnector::TCPConnectionID id = connector.Connect(
        target,
        connect_timeout,
        ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
    if (id != 0) {
      // The callback hasn't run yet.
      OLA_ASSERT_EQ(1u, connector.ConnectionsPending());
      m_ss->RunOnce(TimeInterval(1, 0));
    }
    OLA_ASSERT_EQ(0u, m_successful_calls);
    OLA_ASSERT_EQ(1u, m_failure_calls);
  }
}


/*
 * Accept a new TCP connection.
 */
void TCPConnectorTest::AcceptedConnection(TCPSocket *new_socket) {
  OLA_ASSERT_NOT_NULL(new_socket);
  GenericSocketAddress address = new_socket->GetPeerAddress();
  OLA_ASSERT_EQ(address.Family(), static_cast<uint16_t>(AF_INET));
  OLA_INFO << "Connection from " << address;

  // terminate the ss when this connection is closed
  new_socket->SetOnClose(
      ola::NewSingleCallback(this, &TCPConnectorTest::TerminateOnClose));
  m_ss->AddReadDescriptor(new_socket, true);
}


/**
 * Called when a connection completes or times out.
 */
void TCPConnectorTest::OnConnect(int fd, int error) {
  if (error) {
    std::ostringstream str;
    str << "Failed to connect: " << strerror(error);
    OLA_ASSERT_EQ_MSG(0, error, str.str());
    m_ss->Terminate();
  } else {
    OLA_ASSERT_TRUE(fd >= 0);
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif  // _WIN32
  }
  m_successful_calls++;
}


/**
 * Called when a connection completes or times out.
 */
void TCPConnectorTest::OnConnectFailure(int fd, int error) {
  // The error could be one of many things, right now we just check it's non-0
  OLA_ASSERT_NE(0, error);
  OLA_ASSERT_EQ(-1, fd);
  m_ss->Terminate();
  m_failure_calls++;
}


/**
 * For certain tests we need to ensure there isn't something listening on a TCP
 * port. The best way I've come up with doing this is to bind to port 0, then
 * close the socket. REUSE_ADDR means that the port shouldn't be allocated
 * again for a while.
 */
uint16_t TCPConnectorTest::ReservePort() {
  TCPAcceptingSocket listening_socket(NULL);
  IPV4SocketAddress listen_address(m_localhost, 0);
  OLA_ASSERT_TRUE_MSG(listening_socket.Listen(listen_address),
                      "Failed to listen");
  GenericSocketAddress addr = listening_socket.GetLocalAddress();
  OLA_ASSERT_TRUE(addr.IsValid());
  return addr.V4Addr().Port();
}
