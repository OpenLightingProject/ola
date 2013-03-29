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
using ola::network::StringToAddress;
using ola::network::TCPConnector;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using std::auto_ptr;
using std::string;

// used to set a timeout which aborts the tests
static const int CONNECT_TIMEOUT_IN_MS = 500;
static const int ABORT_TIMEOUT_IN_MS = 1000;
static const int SERVER_PORT = 9010;

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
          m_localhost(IPV4Address::Loopback()),
          m_server_address(m_localhost, SERVER_PORT) {
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
    IPV4SocketAddress m_server_address;
    ola::SingleUseCallback0<void> *m_timeout_closure;

    void AcceptedConnection(TCPSocket *socket);
    void OnConnect(int fd, int error);
    void OnConnectFailure(int fd, int error);
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPConnectorTest);


/*
 * Setup the select server
 */
void TCPConnectorTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  string localhost_str = "127.0.0.1";
  OLA_ASSERT_TRUE(IPV4Address::FromString(localhost_str, &m_localhost));

  m_ss = new SelectServer();
  m_timeout_closure = ola::NewSingleCallback(this, &TCPConnectorTest::Timeout);
  OLA_ASSERT_TRUE(m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS,
                                              m_timeout_closure));
}


/*
 * Cleanup the select server
 */
void TCPConnectorTest::tearDown() {
  delete m_ss;
}


/*
 * Test non-blocking TCP connects work correctly.
 */
void TCPConnectorTest::testNonBlockingConnect() {
  ola::network::TCPSocketFactory socket_factory(
      ola::NewCallback(this, &TCPConnectorTest::AcceptedConnection));
  TCPAcceptingSocket listening_socket(&socket_factory);
  CPPUNIT_ASSERT_MESSAGE("Check for another instance of olad running",
                         listening_socket.Listen(m_server_address));
  // calling listen a second time should fail
  OLA_ASSERT_FALSE(listening_socket.Listen(m_server_address));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&listening_socket));

  // now attempt a non-blocking connect
  // because we're connecting to the localhost this may run this callback
  // immediately.
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      m_server_address,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnect));
  OLA_ASSERT_TRUE(id);

  m_ss->Run();
  OLA_ASSERT_EQ(0u, connector.ConnectionsPending());
  m_ss->RemoveReadDescriptor(&listening_socket);
}


/*
 * Test a non-blocking TCP connect fails.
 */
void TCPConnectorTest::testNonBlockingConnectFailure() {
  // attempt a non-blocking connect, hopefully nothing is running on port 9010
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      m_server_address,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  OLA_ASSERT_TRUE(id);

  m_ss->Run();
  OLA_ASSERT_EQ(0u, connector.ConnectionsPending());
}


/*
 * Test a non-blocking TCP connect for an invalid port.
 */
void TCPConnectorTest::testNonBlockingConnectError() {
  IPV4Address bcast_address;
  OLA_ASSERT_TRUE(IPV4Address::FromString("255.255.255.255", &bcast_address));

  // attempt a non-blocking connect, hopefully nothing is running on port 9010
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      IPV4SocketAddress(bcast_address, 0),
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  OLA_ASSERT_FALSE(id);
  OLA_ASSERT_EQ(0u, connector.ConnectionsPending());
}


/*
 * Test that cancelling a connect works.
 */
void TCPConnectorTest::testNonBlockingCancel() {
  // attempt a non-blocking connect, hopefully nothing is running on port 9010
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      m_server_address,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  OLA_ASSERT_TRUE(id);
  OLA_ASSERT_EQ(1u, connector.ConnectionsPending());

  OLA_ASSERT_TRUE(connector.Cancel(id));
  OLA_ASSERT_EQ(0u, connector.ConnectionsPending());
}


/*
 * Test that we can destroy the Connector and everything will work.
 */
void TCPConnectorTest::testEarlyDestruction() {
  string m_localhost_str = "127.0.0.1";
  IPV4Address m_localhost;
  OLA_ASSERT_TRUE(IPV4Address::FromString(m_localhost_str, &m_localhost));

  // attempt a non-blocking connect, hopefully nothing is running on port 9010
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  {
    TCPConnector connector(m_ss);
    connector.Connect(
        m_server_address,
        connect_timeout,
        ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
    OLA_ASSERT_EQ(1u, connector.ConnectionsPending());
  }

  m_ss->RunOnce();
}


/*
 * Accept a new TCP connection.
 */
void TCPConnectorTest::AcceptedConnection(TCPSocket *new_socket) {
  OLA_ASSERT_NOT_NULL(new_socket);
  GenericSocketAddress address = new_socket->GetPeer();
  OLA_ASSERT_TRUE(address.Family() == AF_INET);
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
    std::stringstream str;
    str << "Failed to connect: " << strerror(error);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(str.str(), 0, error);
    m_ss->Terminate();
  } else {
    OLA_ASSERT_TRUE(fd >= 0);
    close(fd);
  }
}


/**
 * Called when a connection completes or times out.
 */
void TCPConnectorTest::OnConnectFailure(int fd, int error) {
  // The error could be one of many things, right now we just check it's non-0
  OLA_ASSERT_NE(0, error);
  OLA_ASSERT_EQ(-1, fd);
  m_ss->Terminate();
}
