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
 * SocketTest.cpp
 * Test fixture for the Socket classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <string.h>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"

using std::string;
using ola::network::ConnectedDescriptor;
using ola::network::IPV4Address;
using ola::network::LoopbackDescriptor;
using ola::network::PipeDescriptor;
using ola::network::SelectServer;
using ola::network::StringToAddress;
using ola::network::UnixSocket;
using ola::network::TcpAcceptingSocket;
using ola::network::TcpSocket;
using ola::network::UdpSocket;

static const unsigned char test_cstring[] = "Foo";
// used to set a timeout which aborts the tests
static const int ABORT_TIMEOUT_IN_MS = 1000;

class SocketTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SocketTest);

  CPPUNIT_TEST(testLoopbackDescriptor);
  CPPUNIT_TEST(testPipeDescriptorClientClose);
  CPPUNIT_TEST(testPipeDescriptorServerClose);
  CPPUNIT_TEST(testUnixSocketClientClose);
  CPPUNIT_TEST(testUnixSocketServerClose);
  CPPUNIT_TEST(testTcpSocketClientClose);
  CPPUNIT_TEST(testTcpSocketServerClose);
  CPPUNIT_TEST(testUdpSocket);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testLoopbackDescriptor();
    void testPipeDescriptorClientClose();
    void testPipeDescriptorServerClose();
    void testUnixSocketClientClose();
    void testUnixSocketServerClose();
    void testTcpSocketClientClose();
    void testTcpSocketServerClose();
    void testUdpSocket();

    // timing out indicates something went wrong
    void Timeout() {
      CPPUNIT_ASSERT(false);
      m_timeout_closure = NULL;
    }

    // Socket data actions
    void ReceiveAndClose(ConnectedDescriptor *socket);
    void ReceiveAndTerminate(ConnectedDescriptor *socket);
    void Receive(ConnectedDescriptor *socket);
    void ReceiveAndSend(ConnectedDescriptor *socket);
    void ReceiveSendAndClose(ConnectedDescriptor *socket);
    void NewConnectionSend(TcpSocket *socket);
    void NewConnectionSendAndClose(TcpSocket *socket);
    void UdpReceiveAndTerminate(UdpSocket *socket);
    void UdpReceiveAndSend(UdpSocket *socket);

    // Socket close actions
    void TerminateOnClose() {
      m_ss->Terminate();
    }

  private:
    SelectServer *m_ss;
    TcpAcceptingSocket *m_accepting_socket;
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
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_ss = new SelectServer();
  m_timeout_closure = ola::NewSingleCallback(this, &SocketTest::Timeout);
  CPPUNIT_ASSERT(m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS,
                                             m_timeout_closure));
}


/*
 * Cleanup the select server
 */
void SocketTest::tearDown() {
  delete m_ss;
}


/*
 * Test a loopback socket works correctly
 */
void SocketTest::testLoopbackDescriptor() {
  LoopbackDescriptor socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  socket.SetOnData(ola::NewCallback(this, &SocketTest::ReceiveAndTerminate,
                                   static_cast<ConnectedDescriptor*>(&socket)));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&socket));

  ssize_t bytes_sent = socket.Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveReadDescriptor(&socket);
}


/*
 * Test a pipe socket works correctly.
 * The client sends some data and expects the same data to be returned. The
 * client then closes the connection.
 */
void SocketTest::testPipeDescriptorClientClose() {
  PipeDescriptor socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  SocketClientClose(&socket, socket.OppositeEnd());
}


/*
 * Test a pipe socket works correctly.
 * The client sends some data. The server echos the data and closes the
 * connection.
 */
void SocketTest::testPipeDescriptorServerClose() {
  PipeDescriptor socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());

  SocketServerClose(&socket, socket.OppositeEnd());
}


/*
 * Test a unix socket works correctly.
 * The client sends some data and expects the same data to be returned. The
 * client then closes the connection.
 */
void SocketTest::testUnixSocketClientClose() {
  UnixSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  SocketClientClose(&socket, socket.OppositeEnd());
}


/*
 * Test a unix socket works correctly.
 * The client sends some data. The server echos the data and closes the
 * connection.
 */
void SocketTest::testUnixSocketServerClose() {
  UnixSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  SocketServerClose(&socket, socket.OppositeEnd());
}


/*
 * Test TCP sockets work correctly.
 * The client connects and the server sends some data. The client checks the
 * data matches and then closes the connection.
 */
void SocketTest::testTcpSocketClientClose() {
  string ip_address = "127.0.0.1";
  uint16_t server_port = 9010;
  TcpAcceptingSocket socket;
  CPPUNIT_ASSERT_MESSAGE(
      "Check for another instance of olad running",
      socket.Listen(ip_address, server_port));
  CPPUNIT_ASSERT(!socket.Listen(ip_address, server_port));

  socket.SetOnAccept(ola::NewCallback(this, &SocketTest::NewConnectionSend));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&socket));

  TcpSocket *client_socket = TcpSocket::Connect(ip_address, server_port);
  CPPUNIT_ASSERT(client_socket);
  client_socket->SetOnData(ola::NewCallback(
        this, &SocketTest::ReceiveAndClose,
        static_cast<ConnectedDescriptor*>(client_socket)));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(client_socket));
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
void SocketTest::testTcpSocketServerClose() {
  string ip_address = "127.0.0.1";
  uint16_t server_port = 9010;
  TcpAcceptingSocket socket;
  CPPUNIT_ASSERT_MESSAGE(
      "Check for another instance of olad running",
      socket.Listen(ip_address, server_port));
  CPPUNIT_ASSERT(!socket.Listen(ip_address, server_port));

  socket.SetOnAccept(
      ola::NewCallback(this, &SocketTest::NewConnectionSendAndClose));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&socket));

  // The client socket checks the response and terminates on close
  TcpSocket *client_socket = TcpSocket::Connect(ip_address, server_port);
  CPPUNIT_ASSERT(client_socket);

  client_socket->SetOnData(ola::NewCallback(
        this, &SocketTest::Receive,
        static_cast<ConnectedDescriptor*>(client_socket)));
  client_socket->SetOnClose(
      ola::NewSingleCallback(this, &SocketTest::TerminateOnClose));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(client_socket));

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
void SocketTest::testUdpSocket() {
  IPV4Address ip_address;
  CPPUNIT_ASSERT(IPV4Address::FromString("127.0.0.1", &ip_address));
  uint16_t server_port = 9010;
  UdpSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  CPPUNIT_ASSERT(socket.Bind(server_port));
  CPPUNIT_ASSERT(!socket.Bind(server_port));

  socket.SetOnData(
      ola::NewCallback(this, &SocketTest::UdpReceiveAndSend, &socket));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&socket));

  UdpSocket client_socket;
  CPPUNIT_ASSERT(client_socket.Init());
  CPPUNIT_ASSERT(!client_socket.Init());

  client_socket.SetOnData(
      ola::NewCallback(
        this, &SocketTest::UdpReceiveAndTerminate,
        static_cast<UdpSocket*>(&client_socket)));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(&client_socket));

  ssize_t bytes_sent = client_socket.SendTo(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring),
      ip_address,
      server_port);
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
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

  CPPUNIT_ASSERT(!socket->Receive(buffer, sizeof(buffer), data_read));
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(sizeof(test_cstring)),
                       data_read);
  CPPUNIT_ASSERT(!memcmp(test_cstring, buffer, data_read));
}


/*
 * Receive some data and send it back
 */
void SocketTest::ReceiveAndSend(ConnectedDescriptor *socket) {
  uint8_t buffer[sizeof(test_cstring) + 10];
  unsigned int data_read;
  socket->Receive(buffer, sizeof(buffer), data_read);
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(sizeof(test_cstring)),
                       data_read);
  ssize_t bytes_sent = socket->Send(buffer, data_read);
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
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
void SocketTest::NewConnectionSend(TcpSocket *new_socket) {
  CPPUNIT_ASSERT(new_socket);
  IPV4Address address;
  uint16_t port;
  CPPUNIT_ASSERT(new_socket->GetPeer(&address, &port));
  OLA_INFO << "Connection from " << address << ":" << port;
  ssize_t bytes_sent = new_socket->Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  new_socket->SetOnClose(ola::NewSingleCallback(this,
                                               &SocketTest::TerminateOnClose));
  m_ss->AddReadDescriptor(new_socket, true);
}


/*
 * Accept a new connect, send some data and close
 */
void SocketTest::NewConnectionSendAndClose(TcpSocket *new_socket) {
  CPPUNIT_ASSERT(new_socket);
  IPV4Address address;
  uint16_t port;
  CPPUNIT_ASSERT(new_socket->GetPeer(&address, &port));
  OLA_INFO << "Connection from " << address << ":" << port;
  ssize_t bytes_sent = new_socket->Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  new_socket->Close();
  delete new_socket;
}


/*
 * Receive some data and check it.
 */
void SocketTest::UdpReceiveAndTerminate(UdpSocket *socket) {
  IPV4Address expected_address, src_address;
  CPPUNIT_ASSERT(IPV4Address::FromString("127.0.0.1", &expected_address));

  uint16_t src_port;
  uint8_t buffer[sizeof(test_cstring) + 10];
  ssize_t data_read = sizeof(buffer);
  socket->RecvFrom(buffer, &data_read, src_address, src_port);
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), data_read);
  CPPUNIT_ASSERT(expected_address == src_address);
  m_ss->Terminate();
}


/*
 * Receive some data and echo it back.
 */
void SocketTest::UdpReceiveAndSend(UdpSocket *socket) {
  IPV4Address expected_address;
  CPPUNIT_ASSERT(IPV4Address::FromString("127.0.0.1", &expected_address));

  IPV4Address src_address;
  uint16_t src_port;
  uint8_t buffer[sizeof(test_cstring) + 10];
  ssize_t data_read = sizeof(buffer);
  socket->RecvFrom(buffer, &data_read, src_address, src_port);
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), data_read);
  CPPUNIT_ASSERT(expected_address == src_address);

  ssize_t data_sent = socket->SendTo(
      buffer,
      data_read,
      src_address,
      src_port);
  CPPUNIT_ASSERT_EQUAL(data_read, data_sent);
}


/**
 * Generic method to test client initiated close
 */
void SocketTest::SocketClientClose(ConnectedDescriptor *socket,
                                   ConnectedDescriptor *socket2) {
  CPPUNIT_ASSERT(socket);
  socket->SetOnData(
      ola::NewCallback(this, &SocketTest::ReceiveAndClose,
                       static_cast<ConnectedDescriptor*>(socket)));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(socket));

  CPPUNIT_ASSERT(socket2);
  socket2->SetOnData(
      ola::NewCallback(this, &SocketTest::ReceiveAndSend,
                       static_cast<ConnectedDescriptor*>(socket2)));
  socket2->SetOnClose(ola::NewSingleCallback(this,
                                             &SocketTest::TerminateOnClose));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(socket2));

  ssize_t bytes_sent = socket->Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
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
  CPPUNIT_ASSERT(socket);
  socket->SetOnData(ola::NewCallback(
        this, &SocketTest::Receive,
        static_cast<ConnectedDescriptor*>(socket)));
  socket->SetOnClose(
      ola::NewSingleCallback(this, &SocketTest::TerminateOnClose));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(socket));

  CPPUNIT_ASSERT(socket2);
  socket2->SetOnData(ola::NewCallback(
        this, &SocketTest::ReceiveSendAndClose,
        static_cast<ConnectedDescriptor*>(socket2)));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(socket2));

  ssize_t bytes_sent = socket->Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveReadDescriptor(socket);
  m_ss->RemoveReadDescriptor(socket2);
  delete socket2;
}
