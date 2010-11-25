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
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"

using std::string;
using ola::network::AcceptingSocket;
using ola::network::ConnectedSocket;
using ola::network::LoopbackSocket;
using ola::network::PipeSocket;
using ola::network::SelectServer;
using ola::network::TcpAcceptingSocket;
using ola::network::TcpSocket;
using ola::network::StringToAddress;
using ola::network::UdpSocket;

static const char test_cstring[] = "Foo";
// used to set a timeout which aborts the tests
static const int ABORT_TIMEOUT_IN_MS = 1000;

class SocketTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SocketTest);
  CPPUNIT_TEST(testLoopbackSocket);
  CPPUNIT_TEST(testPipeSocketClientClose);
  CPPUNIT_TEST(testPipeSocketServerClose);
  CPPUNIT_TEST(testTcpSocketClientClose);
  CPPUNIT_TEST(testTcpSocketServerClose);
  CPPUNIT_TEST(testUdpSocket);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testLoopbackSocket();
    void testPipeSocketClientClose();
    void testPipeSocketServerClose();
    void testTcpSocketClientClose();
    void testTcpSocketServerClose();
    void testUdpSocket();

    // timing out indicates something went wrong
    void Timeout() {
      CPPUNIT_ASSERT(false);
      m_timeout_closure = NULL;
    }

    // Socket data actions
    void ReceiveAndClose(ConnectedSocket *socket);
    void ReceiveAndTerminate(ConnectedSocket *socket);
    void Receive(ConnectedSocket *socket);
    void ReceiveAndSend(ConnectedSocket *socket);
    void ReceiveSendAndClose(ConnectedSocket *socket);
    void AcceptAndSend(TcpAcceptingSocket *socket);
    void AcceptSendAndClose(TcpAcceptingSocket *socket);
    void UdpReceiveAndTerminate(UdpSocket *socket);
    void UdpReceiveAndSend(UdpSocket *socket);

    // Socket close actions
    void TerminateOnClose() {
      m_ss->Terminate();
    }

  private:
    SelectServer *m_ss;
    AcceptingSocket *m_accepting_socket;
    ola::SingleUseCallback0<void> *m_timeout_closure;
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketTest);


/*
 * Setup the select server
 */
void SocketTest::setUp() {
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
void SocketTest::testLoopbackSocket() {
  LoopbackSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  socket.SetOnData(ola::NewCallback(this, &SocketTest::ReceiveAndTerminate,
                                   static_cast<ConnectedSocket*>(&socket)));
  CPPUNIT_ASSERT(m_ss->AddSocket(&socket));

  ssize_t bytes_sent = socket.Send(
      reinterpret_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveSocket(&socket);
}


/*
 * Test a pipe socket works correctly.
 * The client sends some data and expects the same data to be returned. The
 * client then closes the connection.
 */
void SocketTest::testPipeSocketClientClose() {
  PipeSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());

  socket.SetOnData(
      ola::NewCallback(this, &SocketTest::ReceiveAndClose,
                      static_cast<ConnectedSocket*>(&socket)));
  CPPUNIT_ASSERT(m_ss->AddSocket(&socket));

  PipeSocket *other_end = socket.OppositeEnd();
  CPPUNIT_ASSERT(other_end);
  other_end->SetOnData(
      ola::NewCallback(this, &SocketTest::ReceiveAndSend,
                      static_cast<ConnectedSocket*>(other_end)));
  other_end->SetOnClose(ola::NewSingleCallback(this,
                                              &SocketTest::TerminateOnClose));
  CPPUNIT_ASSERT(m_ss->AddSocket(other_end));

  ssize_t bytes_sent = socket.Send(
      reinterpret_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveSocket(&socket);
  m_ss->RemoveSocket(other_end);
  delete other_end;
}


/*
 * Test a pipe socket works correctly.
 * The client sends some data. The server echos the data and closes the
 * connection.
 */
void SocketTest::testPipeSocketServerClose() {
  PipeSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());

  socket.SetOnData(ola::NewCallback(
        this, &SocketTest::Receive,
        static_cast<ConnectedSocket*>(&socket)));
  socket.SetOnClose(ola::NewSingleCallback(this, &SocketTest::TerminateOnClose));
  CPPUNIT_ASSERT(m_ss->AddSocket(&socket));

  PipeSocket *other_end = socket.OppositeEnd();
  CPPUNIT_ASSERT(other_end);
  other_end->SetOnData(ola::NewCallback(
        this, &SocketTest::ReceiveSendAndClose,
        static_cast<ConnectedSocket*>(other_end)));
  CPPUNIT_ASSERT(m_ss->AddSocket(other_end));

  ssize_t bytes_sent = socket.Send(
      reinterpret_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveSocket(&socket);
  m_ss->RemoveSocket(other_end);
  delete other_end;
}


/*
 * Test TCP sockets work correctly.
 * The client connects and the server sends some data. The client checks the
 * data matches and then closes the connection.
 */
void SocketTest::testTcpSocketClientClose() {
  string ip_address = "127.0.0.1";
  uint16_t server_port = 9010;
  TcpAcceptingSocket socket(ip_address, server_port);
  CPPUNIT_ASSERT(socket.Listen());
  CPPUNIT_ASSERT(!socket.Listen());

  socket.SetOnData(ola::NewCallback(this, &SocketTest::AcceptAndSend, &socket));
  CPPUNIT_ASSERT(m_ss->AddSocket(&socket));

  TcpSocket *client_socket = TcpSocket::Connect(ip_address, server_port);
  CPPUNIT_ASSERT(client_socket);
  client_socket->SetOnData(ola::NewCallback(
        this, &SocketTest::ReceiveAndClose,
        static_cast<ConnectedSocket*>(client_socket)));
  CPPUNIT_ASSERT(m_ss->AddSocket(client_socket));
  m_ss->Run();
  m_ss->RemoveSocket(&socket);
  m_ss->RemoveSocket(client_socket);
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
  TcpAcceptingSocket socket(ip_address, server_port);
  CPPUNIT_ASSERT(socket.Listen());
  CPPUNIT_ASSERT(!socket.Listen());

  socket.SetOnData(
      ola::NewCallback(this, &SocketTest::AcceptSendAndClose, &socket));
  CPPUNIT_ASSERT(m_ss->AddSocket(&socket));

  // The client socket checks the response and terminates on close
  TcpSocket *client_socket = TcpSocket::Connect(ip_address, server_port);
  CPPUNIT_ASSERT(client_socket);

  client_socket->SetOnData(ola::NewCallback(
        this, &SocketTest::Receive,
        static_cast<ConnectedSocket*>(client_socket)));
  client_socket->SetOnClose(
      ola::NewSingleCallback(this, &SocketTest::TerminateOnClose));
  CPPUNIT_ASSERT(m_ss->AddSocket(client_socket));

  m_ss->Run();
  m_ss->RemoveSocket(&socket);
  m_ss->RemoveSocket(client_socket);
  delete client_socket;
}


/*
 * Test UDP sockets work correctly.
 * The client connects and the server sends some data. The client checks the
 * data matches and then closes the connection.
 */
void SocketTest::testUdpSocket() {
  string ip_address = "127.0.0.1";
  uint16_t server_port = 9010;
  UdpSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  CPPUNIT_ASSERT(socket.Bind(server_port));
  CPPUNIT_ASSERT(!socket.Bind(server_port));

  socket.SetOnData(
      ola::NewCallback(this, &SocketTest::UdpReceiveAndSend, &socket));
  CPPUNIT_ASSERT(m_ss->AddSocket(&socket));

  UdpSocket client_socket;
  CPPUNIT_ASSERT(client_socket.Init());
  CPPUNIT_ASSERT(!client_socket.Init());

  client_socket.SetOnData(
      ola::NewCallback(
        this, &SocketTest::UdpReceiveAndTerminate,
        static_cast<UdpSocket*>(&client_socket)));
  CPPUNIT_ASSERT(m_ss->AddSocket(&client_socket));

  ssize_t bytes_sent = client_socket.SendTo(
      reinterpret_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring),
      ip_address,
      server_port);
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveSocket(&socket);
  m_ss->RemoveSocket(&client_socket);
}



/*
 * Receive some data and close the socket
 */
void SocketTest::ReceiveAndClose(ConnectedSocket *socket) {
  Receive(socket);
  m_ss->RemoveSocket(socket);
  socket->Close();
}


/*
 * Receive some data and terminate
 */
void SocketTest::ReceiveAndTerminate(ConnectedSocket *socket) {
  Receive(socket);
  m_ss->Terminate();
}


/*
 * Receive some data and check it's what we expected.
 */
void SocketTest::Receive(ConnectedSocket *socket) {
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
void SocketTest::ReceiveAndSend(ConnectedSocket *socket) {
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
void SocketTest::ReceiveSendAndClose(ConnectedSocket *socket) {
  ReceiveAndSend(socket);
  m_ss->RemoveSocket(socket);
  socket->Close();
}


/*
 * Accept a new connection and send some test data
 */
void SocketTest::AcceptAndSend(TcpAcceptingSocket *socket) {
  ConnectedSocket *new_socket = socket->Accept();
  CPPUNIT_ASSERT(new_socket);
  ssize_t bytes_sent = new_socket->Send(
      reinterpret_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  new_socket->SetOnClose(ola::NewSingleCallback(this,
                                               &SocketTest::TerminateOnClose));
  m_ss->AddSocket(new_socket, true);
}


/*
 * Accept a new connect, send some data and close
 */
void SocketTest::AcceptSendAndClose(TcpAcceptingSocket *socket) {
  ConnectedSocket *new_socket = socket->Accept();
  CPPUNIT_ASSERT(new_socket);
  ssize_t bytes_sent = new_socket->Send(
      reinterpret_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  new_socket->Close();
  delete new_socket;
}


/*
 * Receive some data and check it.
 */
void SocketTest::UdpReceiveAndTerminate(UdpSocket *socket) {
  struct in_addr expected_address;
  CPPUNIT_ASSERT(StringToAddress("127.0.0.1", expected_address));

  struct sockaddr_in src;
  socklen_t src_size = sizeof(src);
  uint8_t buffer[sizeof(test_cstring) + 10];
  ssize_t data_read = sizeof(buffer);
  socket->RecvFrom(buffer, &data_read, src, src_size);
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), data_read);
  CPPUNIT_ASSERT(expected_address.s_addr == src.sin_addr.s_addr);
  m_ss->Terminate();
}


/*
 * Receive some data and echo it back.
 */
void SocketTest::UdpReceiveAndSend(UdpSocket *socket) {
  struct in_addr expected_address;
  CPPUNIT_ASSERT(StringToAddress("127.0.0.1", expected_address));

  struct sockaddr_in src;
  socklen_t src_size = sizeof(src);
  uint8_t buffer[sizeof(test_cstring) + 10];
  ssize_t data_read = sizeof(buffer);
  socket->RecvFrom(buffer, &data_read, src, src_size);
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)), data_read);
  CPPUNIT_ASSERT(expected_address.s_addr == src.sin_addr.s_addr);

  ssize_t data_sent = socket->SendTo(buffer, data_read, src);
  CPPUNIT_ASSERT_EQUAL(data_read, data_sent);
}
