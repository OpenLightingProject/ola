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

#include <stdint.h>
#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include <lla/Closure.h>
#include <lla/network/SelectServer.h>
#include <lla/network/Socket.h>

using namespace lla::network;
using std::string;

static const char test_cstring[] = "Foo";
static const string test_string = test_cstring;
// used to set a timeout which aborts the tests
static const int ABORT_TIMEOUT_IN_MS = 1000;

class SocketTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SocketTest);
  CPPUNIT_TEST(testLoopbackSocket);
  CPPUNIT_TEST(testPipeSocketClientClose);
  CPPUNIT_TEST(testPipeSocketServerClose);
  CPPUNIT_TEST(testTcpSocketClientClose);
  CPPUNIT_TEST(testTcpSocketServerClose);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testLoopbackSocket();
    void testPipeSocketClientClose();
    void testPipeSocketServerClose();
    void testTcpSocketClientClose();
    void testTcpSocketServerClose();

    // timing out indicates something went wrong
    int Timeout() { CPPUNIT_ASSERT(false); m_timeout_closure = NULL; }

    // Socket data actions
    int ReceiveAndClose(ReceivingSocket *socket);
    int ReceiveAndTerminate(ReceivingSocket *socket);
    int Receive(ReceivingSocket *socket);
    int ReceiveAndSend(ConnectedSocket *socket);
    int ReceiveSendAndClose(ConnectedSocket *socket);
    int AcceptAndSend(TcpAcceptingSocket *socket, SocketManager *manger);
    int AcceptSendAndClose(TcpAcceptingSocket *socket);

  private:
    SelectServer *m_ss;
    AcceptingSocket *m_accepting_socket;
    lla::SingleUseClosure *m_timeout_closure;
};

CPPUNIT_TEST_SUITE_REGISTRATION(SocketTest);


/*
 * Stops the select server when a socket is closed.
 */
class TerminatingSocketManager: public SocketManager {
  public:
    TerminatingSocketManager(SelectServer *ss): m_ss(ss) {}
    void SocketClosed(Socket *socket) { m_ss->Terminate(); }
  private:
    SelectServer *m_ss;
};


/*
 * Setup the select server
 */
void SocketTest::setUp() {
  m_ss = new SelectServer();
  m_timeout_closure = lla::NewSingleClosure(this, &SocketTest::Timeout);
  CPPUNIT_ASSERT(m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS,
                                             m_timeout_closure));
}


/*
 * Cleanup the select server
 */
void SocketTest::tearDown() {
  delete m_ss;
  if (m_timeout_closure) {
    delete m_timeout_closure;
    m_timeout_closure = NULL;
  }
}


/*
 * Test a loopback socket works correctly
 */
void SocketTest::testLoopbackSocket() {
  LoopbackSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          &socket,
          lla::NewClosure(this, &SocketTest::ReceiveAndTerminate,
                          (ReceivingSocket *) &socket)
      )
  );

  ssize_t bytes_sent = socket.Send((uint8_t*) test_string.data(),
                                   test_string.length());
  CPPUNIT_ASSERT_EQUAL((ssize_t) test_string.length(), bytes_sent);
  m_ss->Run();
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

  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          &socket,
          lla::NewClosure(this, &SocketTest::ReceiveAndClose,
                          (ReceivingSocket *) &socket)
      )
  );

  PipeSocket *other_end = socket.OppositeEnd();
  CPPUNIT_ASSERT(other_end);
  TerminatingSocketManager manager(m_ss);
  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          other_end,
          lla::NewClosure(this, &SocketTest::ReceiveAndSend,
                          (ConnectedSocket *) other_end),
          &manager
      )
  );

  size_t bytes_sent = socket.Send((uint8_t*) test_string.c_str(),
                                  test_string.length());
  CPPUNIT_ASSERT_EQUAL(test_string.length(), bytes_sent);
  m_ss->Run();

  // delete other end?
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

  TerminatingSocketManager manager(m_ss);
  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          &socket,
          lla::NewClosure(this, &SocketTest::Receive,
                          (ReceivingSocket *) &socket),
          &manager)
  );

  PipeSocket *other_end = socket.OppositeEnd();
  CPPUNIT_ASSERT(other_end);
  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          other_end,
          lla::NewClosure(this, &SocketTest::ReceiveSendAndClose,
                          (ConnectedSocket *) other_end))
  );

  size_t bytes_sent = socket.Send((uint8_t*) test_string.c_str(),
                                  test_string.length());
  CPPUNIT_ASSERT_EQUAL(test_string.length(), bytes_sent);
  m_ss->Run();
  delete other_end;
}


/*
 * Test TCP sockets work correctly.
 * The client connects and the server sends some data. The client checks the
 * data matches and then closes the connection.
 */
void SocketTest::testTcpSocketClientClose() {
  string ip_address = "127.0.0.1";
  unsigned short server_port = 9010;
  TcpAcceptingSocket socket(ip_address, server_port);
  CPPUNIT_ASSERT(socket.Listen());
  CPPUNIT_ASSERT(!socket.Listen());

  TerminatingSocketManager manager(m_ss);
  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          &socket,
          lla::NewClosure(this, &SocketTest::AcceptAndSend, &socket,
                          (SocketManager*) &manager)
      )
  );

  TcpSocket client_socket(ip_address, server_port);
  CPPUNIT_ASSERT(client_socket.Connect());
  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          &client_socket,
          lla::NewClosure(this, &SocketTest::ReceiveAndClose,
                          (ReceivingSocket*) &client_socket)
      )
  );
  m_ss->Run();
}


/*
 * Test TCP sockets work correctly.
 * The client connects and the server then sends some data and closes the
 * connection.
 */
void SocketTest::testTcpSocketServerClose() {
  string ip_address = "127.0.0.1";
  unsigned short server_port = 9010;
  TcpAcceptingSocket socket(ip_address, server_port);
  CPPUNIT_ASSERT(socket.Listen());
  CPPUNIT_ASSERT(!socket.Listen());

  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          &socket,
          lla::NewClosure(this, &SocketTest::AcceptSendAndClose, &socket)
      )
  );

  // The client socket checks the response and terminates on close
  TcpSocket client_socket(ip_address, server_port);
  CPPUNIT_ASSERT(client_socket.Connect());
  TerminatingSocketManager manager(m_ss);
  CPPUNIT_ASSERT(
      m_ss->AddSocket(
          &client_socket,
          lla::NewClosure(this, &SocketTest::Receive,
                          (ReceivingSocket*) &client_socket),
          &manager
      )
  );
  m_ss->Run();
}


/*
 * Receive some data and close the socket
 */
int SocketTest::ReceiveAndClose(ReceivingSocket *socket) {
  int ret = Receive(socket);
  m_ss->RemoveSocket(socket);
  socket->Close();
  return ret;
}


/*
 * Receive some data and terminate
 */
int SocketTest::ReceiveAndTerminate(ReceivingSocket *socket) {
  int ret = Receive(socket);
  m_ss->Terminate();
  return ret;
}


/*
 * Receive some data and check it's what we expected.
 */
int SocketTest::Receive(ReceivingSocket *socket) {
  // try to read more than what we sent to test non-blocking
  uint8_t buffer[sizeof(test_cstring) + 10];
  string result;
  unsigned int data_read;

  CPPUNIT_ASSERT(!socket->Receive(buffer, sizeof(buffer), data_read));
  CPPUNIT_ASSERT_EQUAL((unsigned int) test_string.length(), data_read);
  buffer[data_read] = 0x00;
  result.assign((char*) buffer);
  CPPUNIT_ASSERT_EQUAL(test_string, result);
}


/*
 * Receive some data and send it back
 */
int SocketTest::ReceiveAndSend(ConnectedSocket *socket) {
  uint8_t buffer[sizeof(test_cstring) + 10];
  unsigned int data_read;
  int ret = socket->Receive(buffer, sizeof(buffer), data_read);
  CPPUNIT_ASSERT_EQUAL((unsigned int) test_string.length(), data_read);
  ssize_t bytes_sent = socket->Send(buffer, data_read);
  CPPUNIT_ASSERT_EQUAL((ssize_t) test_string.length(), bytes_sent);
}


/*
 * Receive some data, send the same data and close
 */
int SocketTest::ReceiveSendAndClose(ConnectedSocket *socket) {
  int ret = ReceiveAndSend(socket);
  m_ss->RemoveSocket(socket);
  socket->Close();
  return ret;
}


/*
 * Accept a new connection and send some test data
 */
int SocketTest::AcceptAndSend(TcpAcceptingSocket *socket,
                              SocketManager *manager) {
  ConnectedSocket *new_socket = socket->Accept();
  CPPUNIT_ASSERT(new_socket);
  ssize_t bytes_sent = new_socket->Send((uint8_t*) test_string.data(),
      test_string.length());
  CPPUNIT_ASSERT_EQUAL((ssize_t) test_string.length(), bytes_sent);
  m_ss->AddSocket(new_socket, NULL, manager, true);
}


/*
 * Accept a new connect, send some data and close
 */
int SocketTest::AcceptSendAndClose(TcpAcceptingSocket *socket) {
  ConnectedSocket *new_socket = socket->Accept();
  CPPUNIT_ASSERT(new_socket);
  ssize_t bytes_sent = new_socket->Send((uint8_t*) test_string.data(),
      test_string.length());
  CPPUNIT_ASSERT_EQUAL((ssize_t) test_string.length(), bytes_sent);
  new_socket->Close();
  delete new_socket;
}
