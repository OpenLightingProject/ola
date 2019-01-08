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
 * SocketTest.cpp
 * Test fixture for the Socket classes
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <string.h>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/io/IOQueue.h"
#include "ola/io/SelectServer.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/Socket.h"
#include "ola/network/TCPSocketFactory.h"
#include "ola/testing/TestUtils.h"


using ola::io::ConnectedDescriptor;
using ola::io::IOQueue;
using ola::io::SelectServer;
using ola::network::IPV4Address;
using ola::network::GenericSocketAddress;
using ola::network::IPV4SocketAddress;
using ola::network::TCPAcceptingSocket;
using ola::network::TCPSocket;
using ola::network::UDPSocket;
using std::string;

static const unsigned char test_cstring[] = "Foo";
// used to set a timeout which aborts the tests
static const int ABORT_TIMEOUT_IN_MS = 1000;

class SocketTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SocketTest);
  CPPUNIT_TEST(testTCPSocketClientClose);
  CPPUNIT_TEST(testTCPSocketServerClose);
  CPPUNIT_TEST(testUDPSocket);
  CPPUNIT_TEST(testIOQueueUDPSend);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void tearDown();
    void testTCPSocketClientClose();
    void testTCPSocketServerClose();
    void testUDPSocket();
    void testIOQueueUDPSend();

    // timing out indicates something went wrong
    void Timeout() {
      OLA_FAIL("timeout");
      m_timeout_closure = NULL;
    }

    // Socket data actions
    void ReceiveAndClose(ConnectedDescriptor *socket);
    void ReceiveAndTerminate(ConnectedDescriptor *socket);
    void Receive(ConnectedDescriptor *socket);
    void ReceiveAndSend(ConnectedDescriptor *socket);
    void ReceiveSendAndClose(ConnectedDescriptor *socket);
    void NewConnectionSend(TCPSocket *socket);
    void NewConnectionSendAndClose(TCPSocket *socket);
    void UDPReceiveAndTerminate(UDPSocket *socket);
    void UDPReceiveAndSend(UDPSocket *socket);

    // Socket close actions
    void TerminateOnClose() {
      m_ss->Terminate();
    }

 private:
    SelectServer *m_ss;
    ola::SingleUseCallback0<void> *m_timeout_closure;

    void SocketClientClose(ConnectedDescriptor *socket,
                           ConnectedDescriptor *socket2);
    void SocketServerClose(ConnectedDescriptor *socket,
                           ConnectedDescriptor *socket2);
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketTest);


/*
 * Setup the select server
 */
void SocketTest::setUp() {
  m_ss = new SelectServer();
  m_timeout_closure = ola::NewSingleCallback(this, &SocketTest::Timeout);
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
void SocketTest::tearDown() {
  delete m_ss;

#ifdef _WIN32
  WSACleanup();
#endif  // _WIN32
}


/*
 * Test TCP sockets work correctly.
 * The client connects and the server sends some data. The client checks the
 * data matches and then closes the connection.
 */
void SocketTest::testTCPSocketClientClose() {
  IPV4SocketAddress socket_address(IPV4Address::Loopback(), 0);
  ola::network::TCPSocketFactory socket_factory(
      ola::NewCallback(this, &SocketTest::NewConnectionSend));
  TCPAcceptingSocket socket(&socket_factory);
  OLA_ASSERT_TRUE_MSG(socket.Listen(socket_address),
                      "Check for another instance of olad running");
  OLA_ASSERT_FALSE(socket.Listen(socket_address));

  GenericSocketAddress local_address = socket.GetLocalAddress();
  OLA_ASSERT_EQ(static_cast<uint16_t>(AF_INET), local_address.Family());

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&socket));

  TCPSocket *client_socket = TCPSocket::Connect(local_address);
  OLA_ASSERT_NOT_NULL(client_socket);
  client_socket->SetOnData(ola::NewCallback(
        this, &SocketTest::ReceiveAndClose,
        static_cast<ConnectedDescriptor*>(client_socket)));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(client_socket));
  m_ss->Run();
  m_ss->RemoveReadDescriptor(&socket);
  m_ss->RemoveReadDescriptor(client_socket);
  delete client_socket;
}


/*
 * Test TCP sockets work correctly.
 * The client connects and the server then sends some data and closes the
 * connection.
 */
void SocketTest::testTCPSocketServerClose() {
  IPV4SocketAddress socket_address(IPV4Address::Loopback(), 0);
  ola::network::TCPSocketFactory socket_factory(
      ola::NewCallback(this, &SocketTest::NewConnectionSendAndClose));
  TCPAcceptingSocket socket(&socket_factory);
  OLA_ASSERT_TRUE_MSG(socket.Listen(socket_address),
                      "Check for another instance of olad running");

  OLA_ASSERT_FALSE(socket.Listen(socket_address));

  GenericSocketAddress local_address = socket.GetLocalAddress();
  OLA_ASSERT_EQ(static_cast<uint16_t>(AF_INET), local_address.Family());

  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&socket));

  // The client socket checks the response and terminates on close
  TCPSocket *client_socket = TCPSocket::Connect(local_address);
  OLA_ASSERT_NOT_NULL(client_socket);

  client_socket->SetOnData(ola::NewCallback(
        this, &SocketTest::Receive,
        static_cast<ConnectedDescriptor*>(client_socket)));
  client_socket->SetOnClose(
      ola::NewSingleCallback(this, &SocketTest::TerminateOnClose));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(client_socket));

  m_ss->Run();
  m_ss->RemoveReadDescriptor(&socket);
  m_ss->RemoveReadDescriptor(client_socket);
  delete client_socket;
}


/*
 * Test UDP sockets work correctly.
 * The client connects and the server sends some data. The client checks the
 * data matches and then closes the connection.
 */
void SocketTest::testUDPSocket() {
  IPV4SocketAddress socket_address(IPV4Address::Loopback(), 0);
  UDPSocket socket;
  OLA_ASSERT_TRUE(socket.Init());
  OLA_ASSERT_FALSE(socket.Init());
  OLA_ASSERT_TRUE(socket.Bind(socket_address));
  OLA_ASSERT_FALSE(socket.Bind(socket_address));

  IPV4SocketAddress local_address;
  OLA_ASSERT_TRUE(socket.GetSocketAddress(&local_address));
  OLA_ASSERT_EQ(static_cast<uint16_t>(AF_INET), local_address.Family());

  socket.SetOnData(
      ola::NewCallback(this, &SocketTest::UDPReceiveAndSend, &socket));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&socket));

  UDPSocket client_socket;
  OLA_ASSERT_TRUE(client_socket.Init());
  OLA_ASSERT_FALSE(client_socket.Init());

  client_socket.SetOnData(
      ola::NewCallback(
        this, &SocketTest::UDPReceiveAndTerminate,
        static_cast<UDPSocket*>(&client_socket)));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&client_socket));

  ssize_t bytes_sent = client_socket.SendTo(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring),
      local_address);
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveReadDescriptor(&socket);
  m_ss->RemoveReadDescriptor(&client_socket);
}


/*
 * Test UDP sockets with an IOQueue work correctly.
 * The client connects and the server sends some data. The client checks the
 * data matches and then closes the connection.
 */
void SocketTest::testIOQueueUDPSend() {
  IPV4SocketAddress socket_address(IPV4Address::Loopback(), 9010);
  UDPSocket socket;
  OLA_ASSERT_TRUE(socket.Init());
  OLA_ASSERT_FALSE(socket.Init());
  OLA_ASSERT_TRUE(socket.Bind(socket_address));
  OLA_ASSERT_FALSE(socket.Bind(socket_address));

  socket.SetOnData(
      ola::NewCallback(this, &SocketTest::UDPReceiveAndSend, &socket));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&socket));

  UDPSocket client_socket;
  OLA_ASSERT_TRUE(client_socket.Init());
  OLA_ASSERT_FALSE(client_socket.Init());

  client_socket.SetOnData(
      ola::NewCallback(
        this, &SocketTest::UDPReceiveAndTerminate,
        static_cast<UDPSocket*>(&client_socket)));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&client_socket));

  IOQueue output;
  output.Write(static_cast<const uint8_t*>(test_cstring), sizeof(test_cstring));
  ssize_t bytes_sent = client_socket.SendTo(&output, socket_address);
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveReadDescriptor(&socket);
  m_ss->RemoveReadDescriptor(&client_socket);
}


/*
 * Receive some data and close the socket
 */
void SocketTest::ReceiveAndClose(ConnectedDescriptor *socket) {
  Receive(socket);
  m_ss->RemoveReadDescriptor(socket);
  socket->Close();
}


/*
 * Receive some data and terminate
 */
void SocketTest::ReceiveAndTerminate(ConnectedDescriptor *socket) {
  Receive(socket);
  m_ss->Terminate();
}


/*
 * Receive some data and check it's what we expected.
 */
void SocketTest::Receive(ConnectedDescriptor *socket) {
  // try to read more than what we sent to test non-blocking
  uint8_t buffer[sizeof(test_cstring) + 10];
  unsigned int data_read;

  OLA_ASSERT_FALSE(socket->Receive(buffer, sizeof(buffer), data_read));
  OLA_ASSERT_EQ(static_cast<unsigned int>(sizeof(test_cstring)),
                       data_read);
  OLA_ASSERT_EQ(0, memcmp(test_cstring, buffer, data_read));
}


/*
 * Receive some data and send it back
 */
void SocketTest::ReceiveAndSend(ConnectedDescriptor *socket) {
  uint8_t buffer[sizeof(test_cstring) + 10];
  unsigned int data_read;
  socket->Receive(buffer, sizeof(buffer), data_read);
  OLA_ASSERT_EQ(static_cast<unsigned int>(sizeof(test_cstring)),
                       data_read);
  ssize_t bytes_sent = socket->Send(buffer, data_read);
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
}


/*
 * Receive some data, send the same data and close
 */
void SocketTest::ReceiveSendAndClose(ConnectedDescriptor *socket) {
  ReceiveAndSend(socket);
  m_ss->RemoveReadDescriptor(socket);
  socket->Close();
}


/*
 * Accept a new connection and send some test data
 */
void SocketTest::NewConnectionSend(TCPSocket *new_socket) {
  OLA_ASSERT_TRUE(new_socket);
  GenericSocketAddress address = new_socket->GetPeerAddress();
  OLA_ASSERT_EQ(address.Family(), static_cast<uint16_t>(AF_INET));
  OLA_INFO << "Connection from " << address;
  ssize_t bytes_sent = new_socket->Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  new_socket->SetOnClose(ola::NewSingleCallback(this,
                                               &SocketTest::TerminateOnClose));
  m_ss->AddReadDescriptor(new_socket, true);
}


/*
 * Accept a new connect, send some data and close
 */
void SocketTest::NewConnectionSendAndClose(TCPSocket *new_socket) {
  OLA_ASSERT_NOT_NULL(new_socket);
  GenericSocketAddress address = new_socket->GetPeerAddress();
  OLA_ASSERT_EQ(address.Family(), static_cast<uint16_t>(AF_INET));
  OLA_INFO << "Connection from " << address;
  ssize_t bytes_sent = new_socket->Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  new_socket->Close();
  delete new_socket;
}


/*
 * Receive some data and check it.
 */
void SocketTest::UDPReceiveAndTerminate(UDPSocket *socket) {
  IPV4Address expected_address;
  OLA_ASSERT_TRUE(IPV4Address::FromString("127.0.0.1", &expected_address));

  IPV4SocketAddress source;

  uint8_t buffer[sizeof(test_cstring) + 10];
  ssize_t data_read = sizeof(buffer);
  socket->RecvFrom(buffer, &data_read, &source);
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), data_read);
  OLA_ASSERT_EQ(expected_address, source.Host());
  m_ss->Terminate();
}


/*
 * Receive some data and echo it back.
 */
void SocketTest::UDPReceiveAndSend(UDPSocket *socket) {
  IPV4Address expected_address;
  OLA_ASSERT_TRUE(IPV4Address::FromString("127.0.0.1", &expected_address));
  IPV4SocketAddress source;

  uint8_t buffer[sizeof(test_cstring) + 10];
  ssize_t data_read = sizeof(buffer);
  socket->RecvFrom(buffer, &data_read, &source);
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), data_read);
  OLA_ASSERT_EQ(expected_address, source.Host());

  ssize_t data_sent = socket->SendTo(buffer, data_read, source);
  OLA_ASSERT_EQ(data_read, data_sent);
}


/**
 * Generic method to test client initiated close
 */
void SocketTest::SocketClientClose(ConnectedDescriptor *socket,
                                   ConnectedDescriptor *socket2) {
  OLA_ASSERT_NOT_NULL(socket);
  socket->SetOnData(
      ola::NewCallback(this, &SocketTest::ReceiveAndClose,
                       static_cast<ConnectedDescriptor*>(socket)));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(socket));

  OLA_ASSERT_NOT_NULL(socket2);
  socket2->SetOnData(
      ola::NewCallback(this, &SocketTest::ReceiveAndSend,
                       static_cast<ConnectedDescriptor*>(socket2)));
  socket2->SetOnClose(ola::NewSingleCallback(this,
                                             &SocketTest::TerminateOnClose));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(socket2));

  ssize_t bytes_sent = socket->Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveReadDescriptor(socket);
  m_ss->RemoveReadDescriptor(socket2);
  delete socket2;
}


/**
 * Generic method to test server initiated close
 */
void SocketTest::SocketServerClose(ConnectedDescriptor *socket,
                                   ConnectedDescriptor *socket2) {
  OLA_ASSERT_NOT_NULL(socket);
  socket->SetOnData(ola::NewCallback(
        this, &SocketTest::Receive,
        static_cast<ConnectedDescriptor*>(socket)));
  socket->SetOnClose(
      ola::NewSingleCallback(this, &SocketTest::TerminateOnClose));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(socket));

  OLA_ASSERT_TRUE(socket2);
  socket2->SetOnData(ola::NewCallback(
        this, &SocketTest::ReceiveSendAndClose,
        static_cast<ConnectedDescriptor*>(socket2)));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(socket2));

  ssize_t bytes_sent = socket->Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveReadDescriptor(socket);
  m_ss->RemoveReadDescriptor(socket2);
  delete socket2;
}
