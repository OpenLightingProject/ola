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
 * DescriptorTest.cpp
 * Test fixture for the Descriptor classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <string.h>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServer.h"

using std::string;
using ola::io::ConnectedDescriptor;
using ola::io::LoopbackDescriptor;
using ola::io::PipeDescriptor;
using ola::io::UnixSocket;
using ola::io::SelectServer;

static const unsigned char test_cstring[] = "Foo";
// used to set a timeout which aborts the tests
static const int ABORT_TIMEOUT_IN_MS = 1000;

class DescriptorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DescriptorTest);

  CPPUNIT_TEST(testLoopbackDescriptor);
  CPPUNIT_TEST(testPipeDescriptorClientClose);
  CPPUNIT_TEST(testPipeDescriptorServerClose);
  CPPUNIT_TEST(testUnixSocketClientClose);
  CPPUNIT_TEST(testUnixSocketServerClose);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();
    void testLoopbackDescriptor();
    void testPipeDescriptorClientClose();
    void testPipeDescriptorServerClose();
    void testUnixSocketClientClose();
    void testUnixSocketServerClose();

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
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_ss = new SelectServer();
  m_timeout_closure = ola::NewSingleCallback(this, &DescriptorTest::Timeout);
  CPPUNIT_ASSERT(m_ss->RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS,
                                             m_timeout_closure));
}


/*
 * Cleanup the select server
 */
void DescriptorTest::tearDown() {
  delete m_ss;
}


/*
 * Test a loopback socket works correctly
 */
void DescriptorTest::testLoopbackDescriptor() {
  LoopbackDescriptor socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  socket.SetOnData(ola::NewCallback(this, &DescriptorTest::ReceiveAndTerminate,
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
void DescriptorTest::testPipeDescriptorClientClose() {
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
void DescriptorTest::testPipeDescriptorServerClose() {
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
void DescriptorTest::testUnixSocketClientClose() {
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
void DescriptorTest::testUnixSocketServerClose() {
  UnixSocket socket;
  CPPUNIT_ASSERT(socket.Init());
  CPPUNIT_ASSERT(!socket.Init());
  SocketServerClose(&socket, socket.OppositeEnd());
}


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

  CPPUNIT_ASSERT(!socket->Receive(buffer, sizeof(buffer), data_read));
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(sizeof(test_cstring)),
                       data_read);
  CPPUNIT_ASSERT(!memcmp(test_cstring, buffer, data_read));
}


/*
 * Receive some data and send it back
 */
void DescriptorTest::ReceiveAndSend(ConnectedDescriptor *socket) {
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
  CPPUNIT_ASSERT(socket);
  socket->SetOnData(
      ola::NewCallback(this, &DescriptorTest::ReceiveAndClose,
                       static_cast<ConnectedDescriptor*>(socket)));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(socket));

  CPPUNIT_ASSERT(socket2);
  socket2->SetOnData(
      ola::NewCallback(this, &DescriptorTest::ReceiveAndSend,
                       static_cast<ConnectedDescriptor*>(socket2)));
  socket2->SetOnClose(
      ola::NewSingleCallback(this,
                             &DescriptorTest::TerminateOnClose));
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
void DescriptorTest::SocketServerClose(ConnectedDescriptor *socket,
                                   ConnectedDescriptor *socket2) {
  CPPUNIT_ASSERT(socket);
  socket->SetOnData(ola::NewCallback(
        this, &DescriptorTest::Receive,
        static_cast<ConnectedDescriptor*>(socket)));
  socket->SetOnClose(
      ola::NewSingleCallback(this, &DescriptorTest::TerminateOnClose));
  CPPUNIT_ASSERT(m_ss->AddReadDescriptor(socket));

  CPPUNIT_ASSERT(socket2);
  socket2->SetOnData(ola::NewCallback(
        this, &DescriptorTest::ReceiveSendAndClose,
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
