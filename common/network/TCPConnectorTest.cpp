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
#include "ola/network/Socket.h"
#include "ola/network/TCPConnector.h"

using ola::TimeInterval;
using ola::io::ConnectedDescriptor;
using ola::network::IPV4Address;
using ola::io::SelectServer;
using ola::network::StringToAddress;
using ola::network::TCPConnector;
using ola::network::TcpAcceptingSocket;
using ola::network::TcpSocket;
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
      CPPUNIT_ASSERT(false);
    }

    // Socket close actions
    void TerminateOnClose() {
      m_ss->Terminate();
    }

  private:
    SelectServer *m_ss;
    IPV4Address m_localhost;
    ola::SingleUseCallback0<void> *m_timeout_closure;

    void AcceptedConnection(TcpSocket *socket);
    void OnConnect(TcpSocket *socket, int error);
    void OnConnectFailure(TcpSocket *socket, int error);
};

CPPUNIT_TEST_SUITE_REGISTRATION(TCPConnectorTest);


/*
 * Setup the select server
 */
void TCPConnectorTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  string localhost_str = "127.0.0.1";
  CPPUNIT_ASSERT(IPV4Address::FromString(localhost_str, &m_localhost));

  m_ss = new SelectServer();
  m_timeout_closure = ola::NewSingleCallback(this, &TCPConnectorTest::Timeout);
  CPPUNIT_ASSERT(m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS,
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
  uint16_t server_port = 9010;

  TcpAcceptingSocket listening_socket;
  listening_socket.SetOnAccept(
      ola::NewCallback(this, &TCPConnectorTest::AcceptedConnection));
  CPPUNIT_ASSERT_MESSAGE(
      "Check for another instance of olad running",
      listening_socket.Listen(m_localhost, server_port));
  // calling listen a second time should fail
  CPPUNIT_ASSERT(!listening_socket.Listen(m_localhost, server_port));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&listening_socket));

  // now attempt a non-blocking connect
  // because we're connecting to the localhost this may run this callback
  // immediately.
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      m_localhost,
      server_port,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnect));
  CPPUNIT_ASSERT(id);

  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL(0u, connector.ConnectionsPending());
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
      m_localhost,
      9010,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  CPPUNIT_ASSERT(id);

  m_ss->Run();
  CPPUNIT_ASSERT_EQUAL(0u, connector.ConnectionsPending());
}


/*
 * Test a non-blocking TCP connect for an invalid port.
 */
void TCPConnectorTest::testNonBlockingConnectError() {
  IPV4Address bcast_address;
  CPPUNIT_ASSERT(IPV4Address::FromString("255.255.255.255", &bcast_address));

  // attempt a non-blocking connect, hopefully nothing is running on port 9010
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      bcast_address,
      0,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  CPPUNIT_ASSERT(!id);
  CPPUNIT_ASSERT_EQUAL(0u, connector.ConnectionsPending());
}


/*
 * Test that cancelling a connect works.
 */
void TCPConnectorTest::testNonBlockingCancel() {
  // attempt a non-blocking connect, hopefully nothing is running on port 9010
  TCPConnector connector(m_ss);
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  TCPConnector::TCPConnectionID id = connector.Connect(
      m_localhost,
      9010,
      connect_timeout,
      ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
  CPPUNIT_ASSERT(id);
  CPPUNIT_ASSERT_EQUAL(1u, connector.ConnectionsPending());

  CPPUNIT_ASSERT(connector.Cancel(id));
  CPPUNIT_ASSERT_EQUAL(0u, connector.ConnectionsPending());
}


/*
 * Test that we can destroy the Connector and everything will work.
 */
void TCPConnectorTest::testEarlyDestruction() {
  string m_localhost_str = "127.0.0.1";
  uint16_t server_port = 9010;
  IPV4Address m_localhost;
  CPPUNIT_ASSERT(IPV4Address::FromString(m_localhost_str, &m_localhost));

  // attempt a non-blocking connect, hopefully nothing is running on port 9010
  TimeInterval connect_timeout(0, CONNECT_TIMEOUT_IN_MS * 1000);
  {
    TCPConnector connector(m_ss);
    connector.Connect(
        m_localhost,
        server_port,
        connect_timeout,
        ola::NewSingleCallback(this, &TCPConnectorTest::OnConnectFailure));
    CPPUNIT_ASSERT_EQUAL(1u, connector.ConnectionsPending());
  }

  m_ss->RunOnce();
}


/*
 * Accept a new TCP connection.
 */
void TCPConnectorTest::AcceptedConnection(TcpSocket *new_socket) {
  CPPUNIT_ASSERT(new_socket);
  IPV4Address address;
  uint16_t port;
  CPPUNIT_ASSERT(new_socket->GetPeer(&address, &port));
  OLA_INFO << "Connection from " << address << ":" << port;

  // terminate the ss when this connection is closed
  new_socket->SetOnClose(
      ola::NewSingleCallback(this, &TCPConnectorTest::TerminateOnClose));
  m_ss->AddReadDescriptor(new_socket, true);
}


/**
 * Called when a connection completes or times out.
 */
void TCPConnectorTest::OnConnect(TcpSocket *socket, int error) {
  if (error) {
    std::stringstream str;
    str << "Failed to connect: " << strerror(error);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(str.str(), 0, error);
    m_ss->Terminate();
  } else {
    CPPUNIT_ASSERT(socket);
    socket->Close();
    delete socket;
  }
}


/**
 * Called when a connection completes or times out.
 */
void TCPConnectorTest::OnConnectFailure(TcpSocket *socket, int error) {
  // The error could be one of many things, right now we just check it's non-0
  CPPUNIT_ASSERT(error != 0);
  CPPUNIT_ASSERT(!socket);
  m_ss->Terminate();
}
