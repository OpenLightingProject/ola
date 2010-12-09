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
 * UsbWidgetTest.cpp
 * Test fixture for the UsbWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <queue>

#include "ola/Callback.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/usbpro/UsbWidget.h"


using ola::plugin::usbpro::UsbWidget;
using ola::network::ConnectedSocket;
using ola::network::PipeSocket;
using std::string;
using std::queue;


class UsbWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UsbWidgetTest);
  CPPUNIT_TEST(testSend);
  CPPUNIT_TEST(testReceive);
  CPPUNIT_TEST(testRemove);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();

    void testSend();
    void testReceive();
    void testRemove();

  private:
    ola::network::SelectServer m_ss;
    PipeSocket m_socket;
    PipeSocket *m_other_end;
    UsbWidget *m_widget;
    string m_expected;
    bool m_removed;

    typedef struct {
      uint8_t label;
      unsigned int size;
      const uint8_t *data;
    } expected_message;

    queue<expected_message> m_messages;

    void AddExpected(const string &data);
    void AddExpectedMessage(uint8_t label,
                            unsigned int size,
                            const uint8_t *data);
    void Receive();
    void ReceiveMessage(uint8_t label,
                        const uint8_t *data,
                        unsigned int size);
    void Timeout() { m_ss.Terminate(); }
    void DeviceRemoved() {
      m_removed = true;
      m_ss.Terminate();
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(UsbWidgetTest);


void UsbWidgetTest::setUp() {
  m_socket.Init();
  m_other_end = m_socket.OppositeEnd();

  m_ss.AddSocket(&m_socket);
  m_ss.AddSocket(m_other_end, true);
  m_widget = new UsbWidget(&m_socket);
  m_removed = false;

  m_ss.RegisterSingleTimeout(
      30,  // 30ms should be enough
      ola::NewSingleCallback(this, &UsbWidgetTest::Timeout));
}


void UsbWidgetTest::tearDown() {
  delete m_widget;
}


void UsbWidgetTest::AddExpected(const string &data) {
  m_expected.append(data.data(), data.size());
}


void UsbWidgetTest::AddExpectedMessage(uint8_t label,
                                       unsigned int size,
                                       const uint8_t *data) {
  expected_message message = {
    label,
    size,
    data};
  m_messages.push(message);
}


/*
 * Ceceive some data and check it's what we expected.
 */
void UsbWidgetTest::Receive() {
  uint8_t buffer[100];
  unsigned int data_read;

  CPPUNIT_ASSERT(!m_other_end->Receive(buffer, sizeof(buffer), data_read));

  string recieved(reinterpret_cast<char*>(buffer), data_read);
  CPPUNIT_ASSERT(0 == m_expected.compare(0, data_read, recieved));
  m_expected.erase(0, data_read);

  if (m_expected.size() == 0)
    m_ss.Terminate();
}


/**
 * Called when a new message arrives
 */
void UsbWidgetTest::ReceiveMessage(uint8_t label,
                                   const uint8_t *data,
                                   unsigned int size) {
  CPPUNIT_ASSERT(m_messages.size());
  expected_message message = m_messages.front();
  m_messages.pop();

  CPPUNIT_ASSERT_EQUAL(message.label, label);
  CPPUNIT_ASSERT_EQUAL(message.size, size);
  CPPUNIT_ASSERT(!memcmp(message.data, data, size));

  if (!m_messages.size())
    m_ss.Terminate();
}


/**
 * Test sending works
 */
void UsbWidgetTest::testSend() {
  m_other_end->SetOnData(ola::NewCallback(this, &UsbWidgetTest::Receive));

  uint8_t expected[] = {
    0x7e, 0, 0, 0, 0xe7,
    0x7e, 0x0a, 0, 0, 0xe7,
    0x7e, 0x0b, 4, 0, 0xde, 0xad, 0xbe, 0xef, 0xe7,
  };
  AddExpected(string(reinterpret_cast<char*>(expected), sizeof(expected)));
  CPPUNIT_ASSERT(m_widget->SendMessage(0, NULL, 0));
  CPPUNIT_ASSERT(m_widget->SendMessage(10, NULL, 0));

  uint32_t data = ola::network::HostToNetwork(0xdeadbeef);
  CPPUNIT_ASSERT(m_widget->SendMessage(11,
                                      reinterpret_cast<uint8_t*>(&data),
                                      sizeof(data)));

  CPPUNIT_ASSERT(!m_widget->SendMessage(10, NULL, 4));
  m_ss.Run();

  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), m_expected.size());
}


/**
 * Test receiving works.
 */
void UsbWidgetTest::testReceive() {
  m_widget->SetMessageHandler(
      ola::NewCallback(this, &UsbWidgetTest::ReceiveMessage));

  uint8_t data[] = {
    0x7e, 0, 0, 0, 0xe7,
    0x7e, 0x0b, 4, 0, 0xde, 0xad, 0xbe, 0xef, 0xe7,
    0xaa, 0xbb,  // some random bytes
    0x7e, 0xff, 0xff, 0xff, 0xe7,  // msg is too long
    0x7e, 0xa, 4, 0, 0xe7, 0xe7, 0x7e, 0xe7, 0xe7,  // data contains 0xe7
    0x7e, 2, 4, 0, 0xde, 0xad, 0xbe, 0xef, 0xaa,  // missing EOM
  };

  uint32_t data_chunk = ola::network::HostToNetwork(0xdeadbeef);
  AddExpectedMessage(0x0, 0, NULL);
  AddExpectedMessage(0x0b,
                     sizeof(data_chunk),
                     reinterpret_cast<uint8_t*>(&data_chunk));
  uint32_t data_chunk2 = ola::network::HostToNetwork(0xe7e77ee7);
  AddExpectedMessage(0x0a,
                     sizeof(data_chunk2),
                     reinterpret_cast<uint8_t*>(&data_chunk2));

  ssize_t bytes_sent = m_other_end->Send(data, sizeof(data));
  CPPUNIT_ASSERT_EQUAL(static_cast<ssize_t>(sizeof(data)), bytes_sent);
  m_ss.Run();


  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), m_messages.size());
}


/**
 * Test on remove works.
 */
void UsbWidgetTest::testRemove() {
  m_widget->SetOnRemove(
      ola::NewSingleCallback(this, &UsbWidgetTest::DeviceRemoved));
  m_other_end->Close();
  m_ss.Run();

  CPPUNIT_ASSERT(m_removed);
}
