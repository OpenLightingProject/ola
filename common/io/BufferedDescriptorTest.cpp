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
 * BufferedDescriptorTest.cpp
 * Test fixture for the BufferedDescriptor classes
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <string.h>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/io/BufferedWriteDescriptor.h"

using std::string;
using ola::io::BufferedLoopbackDescriptor;
using ola::io::ConnectedDescriptor;
using ola::io::SelectServer;

static const unsigned char test_cstring[] = "Foo Bar Baz";
// used to set a timeout which aborts the tests
static const int ABORT_TIMEOUT_IN_MS = 1000;

class BufferedDescriptorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BufferedDescriptorTest);
  CPPUNIT_TEST(testBufferedLoopbackDescriptor);
  CPPUNIT_TEST(testBufferedLoopbackDescriptorDoubleWrite);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void testBufferedLoopbackDescriptor();
    void testBufferedLoopbackDescriptorDoubleWrite();

    // timing out indicates something went wrong
    void Timeout() {
      CPPUNIT_ASSERT(false);
      m_timeout_closure = NULL;
    }

    // Socket data actions
    void ReceiveAndTerminate(ConnectedDescriptor *socket);
    void Receive(ConnectedDescriptor *socket);

    // Socket close actions
    void TerminateOnClose() {
      m_ss.Terminate();
    }

  private:
    SelectServer m_ss;
    ola::SingleUseCallback0<void> *m_timeout_closure;
};

CPPUNIT_TEST_SUITE_REGISTRATION(BufferedDescriptorTest);


/*
 * Setup the select server
 */
void BufferedDescriptorTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_timeout_closure = ola::NewSingleCallback(this, &BufferedDescriptorTest::Timeout);
  CPPUNIT_ASSERT(m_ss.RegisterSingleTimeout(ABORT_TIMEOUT_IN_MS,
                                            m_timeout_closure));
}


/*
 * Test a BufferedDescriptor using a Loopback Descriptor works.
 */
void BufferedDescriptorTest::testBufferedLoopbackDescriptor() {
  BufferedLoopbackDescriptor descriptor(&m_ss);
  descriptor.SendV(NULL, 0);
  CPPUNIT_ASSERT(descriptor.Init());
  CPPUNIT_ASSERT(!descriptor.Init());
  descriptor.SetOnData(
      ola::NewCallback(this,
                       &BufferedDescriptorTest::ReceiveAndTerminate,
                       static_cast<ConnectedDescriptor*>(&descriptor)));
  CPPUNIT_ASSERT(m_ss.AddReadDescriptor(&descriptor));

  ssize_t bytes_buffered = descriptor.Send(
      static_cast<const uint8_t*>(test_cstring),
      sizeof(test_cstring));
  CPPUNIT_ASSERT(!descriptor.Empty());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(sizeof(test_cstring)),
                       descriptor.Size());
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(test_cstring)),
                       bytes_buffered);
  m_ss.Run();

  // confirm all data has been sent
  CPPUNIT_ASSERT(descriptor.Empty());
  CPPUNIT_ASSERT_EQUAL(0u, descriptor.Size());

  // Disassociate, this is optional but it tests the code path
  descriptor.AssociateSelectServer(NULL);
}


/*
 * Test a BufferedDescriptor using a Loopback Descriptor works if we write data
 * multiple times.
 */
void BufferedDescriptorTest::testBufferedLoopbackDescriptorDoubleWrite() {
  BufferedLoopbackDescriptor descriptor;
  // this time use the AssociateSelectServer method
  descriptor.AssociateSelectServer(&m_ss);
  CPPUNIT_ASSERT(descriptor.Init());
  CPPUNIT_ASSERT(!descriptor.Init());
  descriptor.SetOnData(
      ola::NewCallback(this,
                       &BufferedDescriptorTest::ReceiveAndTerminate,
                       static_cast<ConnectedDescriptor*>(&descriptor)));
  CPPUNIT_ASSERT(m_ss.AddReadDescriptor(&descriptor));

  unsigned int first_chunk = sizeof(test_cstring) / 2;

  ssize_t bytes_buffered = descriptor.Send(
      static_cast<const uint8_t*>(test_cstring),
      first_chunk);
  CPPUNIT_ASSERT(!descriptor.Empty());
  CPPUNIT_ASSERT_EQUAL(first_chunk, descriptor.Size());
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(first_chunk), bytes_buffered);

  // send the other chunk
  bytes_buffered = descriptor.Send(
      static_cast<const uint8_t*>(test_cstring + first_chunk),
      sizeof(test_cstring) - first_chunk);
  CPPUNIT_ASSERT(!descriptor.Empty());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(sizeof(test_cstring)),
                       descriptor.Size());
  CPPUNIT_ASSERT_EQUAL(
      static_cast<ssize_t>(sizeof(test_cstring) - first_chunk),
      bytes_buffered);

  m_ss.Run();

  // confirm all data has been sent
  CPPUNIT_ASSERT(descriptor.Empty());
  CPPUNIT_ASSERT_EQUAL(0u, descriptor.Size());

  // Disassociate, this is optional but it tests the code path
  descriptor.AssociateSelectServer(NULL);
}


/*
 * Receive some data and terminate
 */
void BufferedDescriptorTest::ReceiveAndTerminate(ConnectedDescriptor *socket) {
  Receive(socket);
  m_ss.Terminate();
}


/*
 * Receive some data and check it's what we expected.
 */
void BufferedDescriptorTest::Receive(ConnectedDescriptor *socket) {
  // try to read more than what we sent to test non-blocking
  uint8_t buffer[sizeof(test_cstring) + 10];
  unsigned int data_read;

  CPPUNIT_ASSERT(!socket->Receive(buffer, sizeof(buffer), data_read));
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(sizeof(test_cstring)),
                       data_read);
  CPPUNIT_ASSERT(!memcmp(test_cstring, buffer, data_read));
}
