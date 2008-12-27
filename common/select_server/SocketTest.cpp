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

#include <lla/select_server/SelectServer.h>
#include <lla/select_server/Socket.h>

using namespace lla::select_server;
using namespace std;

static const char test_cstring[] = "Foo";
static const string test_string = test_cstring;
// used to set a timeout which aborts the tests
static const int ABORT_TIMEOUT_IN_MS = 1000;

class SocketTest: public CppUnit::TestFixture,
                  public TimeoutListener,
                  public SocketListener {
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
    int SocketReady(ConnectedSocket *socket);
    int Timeout();

  private:
    SelectServer *m_ss;
    ListeningSocket *m_listening_socket;
    bool m_terminate_on_recv;
    bool m_close_on_recv;
};


/*
 * An EchoSocketListener just echos back what it receives.
 */
class EchoSocketListener: public SocketListener {
  public:
    EchoSocketListener(SelectServer *ss=NULL, bool close_on_recv=false):
      m_close_on_recv(close_on_recv),
      m_ss(ss) {}
    int SocketReady(ConnectedSocket *socket);
  private:
    bool m_close_on_recv;
    SelectServer *m_ss;
};


/*
 * Handles the cleanup of a socket when it's closed
 */
class EchoSocketManager: public SocketManager {
  public:
    EchoSocketManager(SelectServer *ss, bool delete_on_close=true):
      m_ss(ss),
      m_delete_on_close(delete_on_close) {}
    void SocketClosed(Socket *socket);
  private:
    SelectServer *m_ss;
    bool m_delete_on_close;
};



/*
 * An EchoAcceptSocketListener to echo a string to new connections.
 */
class EchoAcceptSocketListener: public AcceptSocketListener {
  public:
    EchoAcceptSocketListener(SelectServer *ss,
                             EchoSocketManager *manager,
                             bool close_on_send=false):
      m_ss(ss),
      m_manager(manager),
      m_close_on_send(close_on_send) {}
    int NewConnection(ConnectedSocket *socket);
  private:
    SelectServer *m_ss;
    EchoSocketManager *m_manager;
    bool m_close_on_send;
};



CPPUNIT_TEST_SUITE_REGISTRATION(SocketTest);


int EchoSocketListener::SocketReady(ConnectedSocket *socket) {
  uint8_t buffer[sizeof(test_cstring) + 10];
  unsigned int data_read;
  int ret = socket->Receive(buffer, sizeof(buffer), data_read);
  CPPUNIT_ASSERT_EQUAL((unsigned int) test_string.length(), data_read);
  ssize_t bytes_sent = socket->Send(buffer, data_read);
  CPPUNIT_ASSERT_EQUAL((ssize_t) test_string.length(), bytes_sent);
  if (m_close_on_recv) {
    m_ss->RemoveSocket(socket);
    socket->Close();
  }
}


int EchoAcceptSocketListener::NewConnection(ConnectedSocket *socket) {
  ssize_t bytes_sent = socket->Send((uint8_t*) test_string.c_str(), test_string.length());
  CPPUNIT_ASSERT_EQUAL((ssize_t) test_string.length(), bytes_sent);
  if (m_close_on_send) {
    socket->Close();
    delete socket;
  }
  else
    m_ss->AddSocket(socket, m_manager);
}


void EchoSocketManager::SocketClosed(Socket *socket) {
  socket->Close();
  m_ss->Terminate();
  if (m_delete_on_close)
    delete socket;
}


void SocketTest::setUp() {
  m_ss = new SelectServer();
  int ret = m_ss->RegisterTimeout(ABORT_TIMEOUT_IN_MS, this, false);
  CPPUNIT_ASSERT(!ret);
  m_close_on_recv = true;
  m_terminate_on_recv = false;
}


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
  socket.SetListener(this);
  CPPUNIT_ASSERT(!m_ss->AddSocket(&socket));

  ssize_t bytes_sent = socket.Send((uint8_t*) test_string.c_str(), test_string.length());
  CPPUNIT_ASSERT_EQUAL((ssize_t) test_string.length(), bytes_sent);
  m_terminate_on_recv = true;
  m_ss->Run();
}


/*
 * Test a pipe socket works correctly.
 * The client sends some data and expects the same data to be returns. The
 * client then closes the connection.
 */
void SocketTest::testPipeSocketClientClose() {
  PipeSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  socket.SetListener(this);

  PipeSocket *other_end = socket.OppositeEnd();
  CPPUNIT_ASSERT(other_end);
  EchoSocketListener echo_listener;
  other_end->SetListener(&echo_listener);

  EchoSocketManager manager(m_ss);
  CPPUNIT_ASSERT(!m_ss->AddSocket(&socket));
  CPPUNIT_ASSERT(!m_ss->AddSocket(other_end, &manager));

  size_t bytes_sent = socket.Send((uint8_t*) test_string.c_str(), test_string.length());
  CPPUNIT_ASSERT_EQUAL(test_string.length(), bytes_sent);
  m_ss->Run();
}


/*
 * Test a pipe socket works correctly.
 * The client sends some data and expects the same data to be returned. The
 * client then closes the connection.
 */
void SocketTest::testPipeSocketServerClose() {
  PipeSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  socket.SetListener(this);

  PipeSocket *other_end = socket.OppositeEnd();
  CPPUNIT_ASSERT(other_end);
  EchoSocketListener echo_listener(m_ss, true);
  other_end->SetListener(&echo_listener);

  EchoSocketManager manager(m_ss, false);
  CPPUNIT_ASSERT(!m_ss->AddSocket(&socket, &manager));
  CPPUNIT_ASSERT(!m_ss->AddSocket(other_end));

  size_t bytes_sent = socket.Send((uint8_t*) test_string.c_str(), test_string.length());
  CPPUNIT_ASSERT_EQUAL(test_string.length(), bytes_sent);
  m_close_on_recv = false;
  m_ss->Run();
  delete other_end;
}


/*
 * Test TCP sockets work correctly.
 * The client connects and the server then sends some data and closes the
 * connection.
 */
void SocketTest::testTcpSocketClientClose() {
  string ip_address = "127.0.0.1";
  unsigned short server_port = 9010;
  TcpListeningSocket socket(ip_address, server_port);
  CPPUNIT_ASSERT(socket.Listen());
  CPPUNIT_ASSERT(!socket.Listen());

  EchoSocketManager manager(m_ss);
  EchoAcceptSocketListener accept_listener(m_ss, &manager);
  socket.SetListener(&accept_listener);

  TcpSocket client_socket;
  CPPUNIT_ASSERT(client_socket.Connect(ip_address, server_port));
  client_socket.SetListener(this);

  CPPUNIT_ASSERT(!m_ss->AddSocket(&socket));
  CPPUNIT_ASSERT(!m_ss->AddSocket(&client_socket));
  m_ss->Run();
}


/*
 * Test TCP sockets work correctly.
 * The client connects and the server sends some data. The client checks the data
 * matches and then closes the connection.
 */
void SocketTest::testTcpSocketServerClose() {
  string ip_address = "127.0.0.1";
  unsigned short server_port = 9010;
  TcpListeningSocket socket(ip_address, server_port);
  CPPUNIT_ASSERT(socket.Listen());
  CPPUNIT_ASSERT(!socket.Listen());

  EchoSocketManager manager(m_ss, false);
  EchoAcceptSocketListener accept_listener(m_ss, &manager, true);
  socket.SetListener(&accept_listener);

  TcpSocket client_socket;
  CPPUNIT_ASSERT(client_socket.Connect(ip_address, server_port));
  client_socket.SetListener(this);

  CPPUNIT_ASSERT(!m_ss->AddSocket(&socket));
  CPPUNIT_ASSERT(!m_ss->AddSocket(&client_socket, &manager));
  m_close_on_recv = false;
  m_ss->Run();
}


int SocketTest::SocketReady(ConnectedSocket *socket) {
  // try to read more than what we sent to test non-blocking
  uint8_t buffer[sizeof(test_cstring) + 10];
  string result;
  unsigned int data_read;

  CPPUNIT_ASSERT(!socket->Receive(buffer, sizeof(buffer), data_read));
  CPPUNIT_ASSERT_EQUAL((unsigned int) test_string.length() , data_read);
  buffer[data_read] = 0x00;
  result.assign((char*) buffer);
  CPPUNIT_ASSERT_EQUAL(test_string, result);
  if (m_close_on_recv) {
    m_ss->RemoveSocket(socket);
    socket->Close();
  }
  if (m_terminate_on_recv)
    m_ss->Terminate();
}


int SocketTest::Timeout() {
  CPPUNIT_ASSERT(false);
}
