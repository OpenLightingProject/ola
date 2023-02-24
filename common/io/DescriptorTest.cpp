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
 * DescriptorTest.cpp
 * Test fixture for the Descriptor classes
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <string.h>
#include <string>

#ifdef _WIN32
#include <ola/win/CleanWinSock2.h>
#endif  // _WIN32

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServer.h"
#include "ola/testing/TestUtils.h"


using std::string;
using ola::io::ConnectedDescriptor;
using ola::io::LoopbackDescriptor;
using ola::io::PipeDescriptor;
#ifndef _WIN32
using ola::io::UnixSocket;
#endif  // !_WIN32
using ola::io::SelectServer;

static const unsigned char test_cstring[] = "Foo";
// used to set a timeout which aborts the tests
static const int ABORT_TIMEOUT_IN_MS = 1000;

class DescriptorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DescriptorTest);

  CPPUNIT_TEST(testLoopbackDescriptor);
  CPPUNIT_TEST(testPipeDescriptorClientClose);
  CPPUNIT_TEST(testPipeDescriptorServerClose);
#ifndef _WIN32
  CPPUNIT_TEST(testUnixSocketClientClose);
  CPPUNIT_TEST(testUnixSocketServerClose);
#endif  // !_WIN32
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void tearDown();
    void testLoopbackDescriptor();
    void testPipeDescriptorClientClose();
    void testPipeDescriptorServerClose();
#ifndef _WIN32
    void testUnixSocketClientClose();
    void testUnixSocketServerClose();
#endif  // !_WIN32

    // timing out indicates something went wrong
    void Timeout() {
      OLA_ASSERT_TRUE(false);
      m_timeout_closure = NULL;
    }

    // Socket data actions
    void ReceiveAndClose(ConnectedDescriptor *socket);
    void ReceiveAndTerminate(ConnectedDescriptor *socket);
    void Receive(ConnectedDescriptor *socket);
    void ReceiveAndSend(ConnectedDescriptor *socket);
    void ReceiveSendAndClose(ConnectedDescriptor *socket);

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

CPPUNIT_TEST_SUITE_REGISTRATION(DescriptorTest);


/*
 * Setup the select server
 */
void DescriptorTest::setUp() {
  m_ss = new SelectServer();
  m_timeout_closure = ola::NewSingleCallback(this, &DescriptorTest::Timeout);
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
void DescriptorTest::tearDown() {
  delete m_ss;

#ifdef _WIN32
  WSACleanup();
#endif  // _WIN32
}


/*
 * Test a loopback socket works correctly
 */
void DescriptorTest::testLoopbackDescriptor() {
  LoopbackDescriptor socket;
  OLA_ASSERT_TRUE(socket.Init());
  OLA_ASSERT_FALSE(socket.Init());
  socket.SetOnData(ola::NewCallback(this, &DescriptorTest::ReceiveAndTerminate,
                                   static_cast<ConnectedDescriptor*>(&socket)));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(&socket));

  ssize_t bytes_sent = socket.Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
  m_ss->Run();
  m_ss->RemoveReadDescriptor(&socket);
}


/*
 * Test a pipe socket works correctly.
 * The client sends some data and expects the same data to be returned. The
 * client then closes the connection.
 */
void DescriptorTest::testPipeDescriptorClientClose() {
  PipeDescriptor socket;
  OLA_ASSERT_TRUE(socket.Init());
  OLA_ASSERT_FALSE(socket.Init());
  SocketClientClose(&socket, socket.OppositeEnd());
}


/*
 * Test a pipe socket works correctly.
 * The client sends some data. The server echos the data and closes the
 * connection.
 */
void DescriptorTest::testPipeDescriptorServerClose() {
  PipeDescriptor socket;
  OLA_ASSERT_TRUE(socket.Init());
  OLA_ASSERT_FALSE(socket.Init());

  SocketServerClose(&socket, socket.OppositeEnd());
}

#ifndef _WIN32

/*
 * Test a unix socket works correctly.
 * The client sends some data and expects the same data to be returned. The
 * client then closes the connection.
 */
void DescriptorTest::testUnixSocketClientClose() {
  UnixSocket socket;
  OLA_ASSERT_TRUE(socket.Init());
  OLA_ASSERT_FALSE(socket.Init());
  SocketClientClose(&socket, socket.OppositeEnd());
}


/*
 * Test a unix socket works correctly.
 * The client sends some data. The server echos the data and closes the
 * connection.
 */
void DescriptorTest::testUnixSocketServerClose() {
  UnixSocket socket;
  OLA_ASSERT_TRUE(socket.Init());
  OLA_ASSERT_FALSE(socket.Init());
  SocketServerClose(&socket, socket.OppositeEnd());
}

#endif  // !_WIN32

/*
 * Receive some data and close the socket
 */
void DescriptorTest::ReceiveAndClose(ConnectedDescriptor *socket) {
  Receive(socket);
  m_ss->RemoveReadDescriptor(socket);
  socket->Close();
}


/*
 * Receive some data and terminate
 */
void DescriptorTest::ReceiveAndTerminate(ConnectedDescriptor *socket) {
  Receive(socket);
  m_ss->Terminate();
}


/*
 * Receive some data and check it's what we expected.
 */
void DescriptorTest::Receive(ConnectedDescriptor *socket) {
  // try to read more than what we sent to test non-blocking
  uint8_t buffer[sizeof(test_cstring) + 10];
  unsigned int data_read;

  OLA_ASSERT_FALSE(socket->Receive(buffer, sizeof(buffer), data_read));
  OLA_ASSERT_EQ(static_cast<unsigned int>(sizeof(test_cstring)),
                       data_read);
  OLA_ASSERT_FALSE(memcmp(test_cstring, buffer, data_read));
}


/*
 * Receive some data and send it back
 */
void DescriptorTest::ReceiveAndSend(ConnectedDescriptor *socket) {
  uint8_t buffer[sizeof(test_cstring) + 10];
  unsigned int data_read;
  OLA_ASSERT_EQ(0, socket->Receive(buffer, sizeof(buffer), data_read));
  OLA_ASSERT_EQ(static_cast<unsigned int>(sizeof(test_cstring)), data_read);
  ssize_t bytes_sent = socket->Send(buffer, data_read);
  OLA_ASSERT_EQ(static_cast<ssize_t>(sizeof(test_cstring)), bytes_sent);
}


/*
 * Receive some data, send the same data and close
 */
void DescriptorTest::ReceiveSendAndClose(ConnectedDescriptor *socket) {
  ReceiveAndSend(socket);
  m_ss->RemoveReadDescriptor(socket);
  socket->Close();
}


/**
 * Generic method to test client initiated close
 */
void DescriptorTest::SocketClientClose(ConnectedDescriptor *socket,
                                   ConnectedDescriptor *socket2) {
  OLA_ASSERT_TRUE(socket);
  socket->SetOnData(
      ola::NewCallback(this, &DescriptorTest::ReceiveAndClose,
                       static_cast<ConnectedDescriptor*>(socket)));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(socket));

  OLA_ASSERT_TRUE(socket2);
  socket2->SetOnData(
      ola::NewCallback(this, &DescriptorTest::ReceiveAndSend,
                       static_cast<ConnectedDescriptor*>(socket2)));
  socket2->SetOnClose(
      ola::NewSingleCallback(this, &DescriptorTest::TerminateOnClose));
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
void DescriptorTest::SocketServerClose(ConnectedDescriptor *socket,
                                       ConnectedDescriptor *socket2) {
  OLA_ASSERT_TRUE(socket);
  socket->SetOnData(ola::NewCallback(
        this, &DescriptorTest::Receive,
        static_cast<ConnectedDescriptor*>(socket)));
  socket->SetOnClose(
      ola::NewSingleCallback(this, &DescriptorTest::TerminateOnClose));
  OLA_ASSERT_TRUE(m_ss->AddReadDescriptor(socket));

  OLA_ASSERT_TRUE(socket2);
  socket2->SetOnData(ola::NewCallback(
        this, &DescriptorTest::ReceiveSendAndClose,
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
