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
 * BaseRobeWidgetTest.cpp
 * Test fixture for the BaseRobeWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <queue>

#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/BaseRobeWidget.h"
#include "plugins/usbpro/CommonWidgetTest.h"


using ola::DmxBuffer;
using std::auto_ptr;
using std::queue;


class BaseRobeWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(BaseRobeWidgetTest);
  CPPUNIT_TEST(testSend);
  CPPUNIT_TEST(testReceive);
  CPPUNIT_TEST(testRemove);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testSend();
    void testReceive();
    void testRemove();

  private:
    auto_ptr<ola::plugin::usbpro::DispatchingRobeWidget> m_widget;
    bool m_removed;

    typedef struct {
      uint8_t label;
      unsigned int size;
      const uint8_t *data;
    } expected_message;

    queue<expected_message> m_messages;

    void Terminate() { m_ss.Terminate(); }
    void AddExpectedMessage(uint8_t label,
                            unsigned int size,
                            const uint8_t *data);
    void ReceiveMessage(uint8_t label,
                        const uint8_t *data,
                        unsigned int size);
    void DeviceRemoved() {
      m_removed = true;
      m_ss.Terminate();
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(BaseRobeWidgetTest);


void BaseRobeWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  m_widget.reset(
      new ola::plugin::usbpro::DispatchingRobeWidget(
        &m_descriptor,
        ola::NewCallback(this, &BaseRobeWidgetTest::ReceiveMessage)));

  m_removed = false;

  m_ss.RegisterSingleTimeout(
      30,  // 30ms should be enough
      ola::NewSingleCallback(this, &BaseRobeWidgetTest::Terminate));
}


void BaseRobeWidgetTest::AddExpectedMessage(uint8_t label,
                                            unsigned int size,
                                            const uint8_t *data) {
  expected_message message = {
    label,
    size,
    data};
  m_messages.push(message);
}


/**
 * Called when a new message arrives
 */
void BaseRobeWidgetTest::ReceiveMessage(uint8_t label,
                                        const uint8_t *data,
                                        unsigned int size) {
  CPPUNIT_ASSERT(m_messages.size());
  expected_message message = m_messages.front();
  m_messages.pop();

  CPPUNIT_ASSERT_EQUAL(message.label, label);
  CPPUNIT_ASSERT_EQUAL(message.size, size);
  CPPUNIT_ASSERT(!memcmp(message.data, data, size));

  if (m_messages.empty())
    m_ss.Terminate();
}


/*
 * Test sending works
 */
void BaseRobeWidgetTest::testSend() {
  // simple empty frame
  uint8_t expected1[] = {0xa5, 0, 0, 0, 0xa5, 0x4a};
  m_endpoint->AddExpectedData(
      expected1,
      sizeof(expected1),
      ola::NewSingleCallback(this, &BaseRobeWidgetTest::Terminate));
  CPPUNIT_ASSERT(m_widget->SendMessage(0, NULL, 0));
  m_ss.Run();
  m_endpoint->Verify();

  // try a different label
  uint8_t expected2[] = {0xa5, 0x0a, 0, 0, 0xaf, 0x5e};
  m_endpoint->AddExpectedData(
      expected2,
      sizeof(expected2),
      ola::NewSingleCallback(this, &BaseRobeWidgetTest::Terminate));
  CPPUNIT_ASSERT(m_widget->SendMessage(10, NULL, 0));
  m_ss.Run();
  m_endpoint->Verify();

  // frame with data
  uint8_t expected3[] = {0xa5, 0x0b, 4, 0, 0xb4, 0xde, 0xad, 0xbe, 0xef, 0xa0};
  m_endpoint->AddExpectedData(
      expected3,
      sizeof(expected3),
      ola::NewSingleCallback(this, &BaseRobeWidgetTest::Terminate));
  uint32_t data = ola::network::HostToNetwork(0xdeadbeef);
  CPPUNIT_ASSERT(m_widget->SendMessage(11,
                                       reinterpret_cast<uint8_t*>(&data),
                                       sizeof(data)));
  // try to send an incorrect frame
  CPPUNIT_ASSERT(!m_widget->SendMessage(10, NULL, 4));
  m_ss.Run();
  m_endpoint->Verify();
}


/*
 * Test receiving works.
 */
void BaseRobeWidgetTest::testReceive() {
  uint8_t data[] = {
    0xa5, 0, 0, 0, 0xa5, 0x4a,
    0xa5, 0x0b, 4, 0, 0xb4, 0xde, 0xad, 0xbe, 0xef, 0xa0,
    0xaa, 0xbb,  // some random bytes
    0xa5, 0xff, 0xff, 0xff, 0xa2, 0x44,  // msg is too long
    0xa5, 0xa, 4, 0, 0xb3, 0xa5, 0xa5, 0xaa, 0xaa, 0x04,  // data contains 0xa5
    // invalid header checksum
    0xa5, 2, 4, 0, 0x00, 0xde, 0xad, 0xbe, 0xef, 0xaa,
    // invalid data checksum
    0xa5, 2, 4, 0, 0xab, 0xde, 0xad, 0xbe, 0xef, 0x00,
  };

  uint32_t data_chunk = ola::network::HostToNetwork(0xdeadbeef);
  AddExpectedMessage(0x0, 0, NULL);
  AddExpectedMessage(0x0b,
                     sizeof(data_chunk),
                     reinterpret_cast<uint8_t*>(&data_chunk));
  uint32_t data_chunk2 = ola::network::HostToNetwork(0xa5a5aaaa);
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
void BaseRobeWidgetTest::testRemove() {
  m_widget->GetDescriptor()->SetOnClose(
      ola::NewSingleCallback(this, &BaseRobeWidgetTest::DeviceRemoved));
  m_other_end->Close();
  m_ss.Run();

  CPPUNIT_ASSERT(m_removed);
}
