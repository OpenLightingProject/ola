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
 * BaseUsbProWidgetTest.cpp
 * Test fixture for the BaseUsbProWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <queue>

#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/usbpro/BaseUsbProWidget.h"
#include "plugins/usbpro/MockEndpoint.h"


using ola::DmxBuffer;
using std::auto_ptr;
using std::queue;


class BaseUsbProWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BaseUsbProWidgetTest);
  CPPUNIT_TEST(testSend);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST(testReceive);
  CPPUNIT_TEST(testRemove);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();

    void testSend();
    void testSendDMX();
    void testReceive();
    void testRemove();

  private:
    ola::network::SelectServer m_ss;
    ola::network::PipeDescriptor m_descriptor;
    auto_ptr<ola::network::PipeDescriptor> m_other_end;
    auto_ptr<MockEndpoint> m_endpoint;
    auto_ptr<ola::plugin::usbpro::DispatchingUsbProWidget> m_widget;
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
    uint8_t *BuildUsbProMessage(uint8_t label,
                                const uint8_t *data,
                                unsigned int data_size,
                                unsigned int *total_size);
    void DeviceRemoved() {
      m_removed = true;
      m_ss.Terminate();
    }

    static const uint8_t DMX_FRAME_LABEL = 0x06;
    static const unsigned int FOOTER_SIZE = 1;
    static const unsigned int HEADER_SIZE = 4;
};


CPPUNIT_TEST_SUITE_REGISTRATION(BaseUsbProWidgetTest);


void BaseUsbProWidgetTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_descriptor.Init();
  m_other_end.reset(m_descriptor.OppositeEnd());
  m_endpoint.reset(new MockEndpoint(m_other_end.get()));
  m_ss.AddReadDescriptor(&m_descriptor);
  m_ss.AddReadDescriptor(m_other_end.get());

  m_widget.reset(
      new ola::plugin::usbpro::DispatchingUsbProWidget(
        &m_descriptor,
        ola::NewCallback(this, &BaseUsbProWidgetTest::ReceiveMessage)));

  m_removed = false;

  m_ss.RegisterSingleTimeout(
      30,  // 30ms should be enough
      ola::NewSingleCallback(this, &BaseUsbProWidgetTest::Terminate));
}


void BaseUsbProWidgetTest::tearDown() {
  m_endpoint->Verify();
  m_ss.RemoveReadDescriptor(&m_descriptor);
  m_ss.RemoveReadDescriptor(m_other_end.get());
}


void BaseUsbProWidgetTest::AddExpectedMessage(uint8_t label,
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
void BaseUsbProWidgetTest::ReceiveMessage(uint8_t label,
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


/**
 * Pack data into a Usb Pro style frame.
 * @param label the message label
 * @param data the message data
 * @param data_size the data size
 * @param total_size, pointer which is updated with the message size.
 */
uint8_t *BaseUsbProWidgetTest::BuildUsbProMessage(uint8_t label,
                                                  const uint8_t *data,
                                                  unsigned int data_size,
                                                  unsigned int *total_size) {
  uint8_t *frame = new uint8_t[data_size + HEADER_SIZE + FOOTER_SIZE];
  frame[0] = 0x7e;  // som
  frame[1] = label;
  frame[2] = data_size & 0xff;  // len
  frame[3] = (data_size + 1) >> 8;  // len hi
  memcpy(frame + 4, data, data_size);
  frame[data_size + HEADER_SIZE] = 0xe7;
  *total_size = data_size + HEADER_SIZE + FOOTER_SIZE;
  return frame;
}


/*
 * Test sending works
 */
void BaseUsbProWidgetTest::testSend() {
  // simple empty frame
  uint8_t expected1[] = {0x7e, 0, 0, 0, 0xe7};
  m_endpoint->AddExpectedData(
      expected1,
      sizeof(expected1),
      ola::NewSingleCallback(this, &BaseUsbProWidgetTest::Terminate));
  CPPUNIT_ASSERT(m_widget->SendMessage(0, NULL, 0));
  m_ss.Run();
  m_endpoint->Verify();

  // try a different label
  uint8_t expected2[] = {0x7e, 0x0a, 0, 0, 0xe7};
  m_endpoint->AddExpectedData(
      expected2,
      sizeof(expected2),
      ola::NewSingleCallback(this, &BaseUsbProWidgetTest::Terminate));
  CPPUNIT_ASSERT(m_widget->SendMessage(10, NULL, 0));
  m_ss.Run();
  m_endpoint->Verify();

  // frame with data
  uint8_t expected3[] = {0x7e, 0x0b, 4, 0, 0xde, 0xad, 0xbe, 0xef, 0xe7};
  m_endpoint->AddExpectedData(
      expected3,
      sizeof(expected3),
      ola::NewSingleCallback(this, &BaseUsbProWidgetTest::Terminate));
  uint32_t data = ola::network::HostToNetwork(0xdeadbeef);
  CPPUNIT_ASSERT(m_widget->SendMessage(11,
                                       reinterpret_cast<uint8_t*>(&data),
                                       sizeof(data)));
  // try to send an incorrect frame
  CPPUNIT_ASSERT(!m_widget->SendMessage(10, NULL, 4));
  m_ss.Run();
  m_endpoint->Verify();
}


/**
 * Check that we can send DMX
 */
void BaseUsbProWidgetTest::testSendDMX() {
  // dmx data
  DmxBuffer buffer;
  buffer.SetFromString("0,1,2,3,4");

  // expected message
  uint8_t dmx_frame_data[] = {DMX512_START_CODE, 0, 1, 2, 3, 4};
  unsigned int size;
  uint8_t *expected_message = BuildUsbProMessage(DMX_FRAME_LABEL,
                                                 dmx_frame_data,
                                                 sizeof(dmx_frame_data),
                                                 &size);

  // add the expected data, run and verify.
  m_endpoint->AddExpectedData(
      expected_message,
      size,
      ola::NewSingleCallback(this, &BaseUsbProWidgetTest::Terminate));
  m_widget->SendDMX(buffer);
  m_ss.Run();
  m_endpoint->Verify();

  // now test an empty frame
  DmxBuffer buffer2;
  uint8_t empty_frame_data[] = {DMX512_START_CODE};  // just the start code
  uint8_t *expected_message2 = BuildUsbProMessage(DMX_FRAME_LABEL,
                                                  empty_frame_data,
                                                  sizeof(empty_frame_data),
                                                  &size);

  // add the expected data, run and verify.
  m_endpoint->AddExpectedData(
      expected_message2,
      size,
      ola::NewSingleCallback(this, &BaseUsbProWidgetTest::Terminate));
  m_widget->SendDMX(buffer2);
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_message;
  delete[] expected_message2;
}


/*
 * Test receiving works.
 */
void BaseUsbProWidgetTest::testReceive() {
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
void BaseUsbProWidgetTest::testRemove() {
  m_widget->SetOnRemove(
      ola::NewSingleCallback(this, &BaseUsbProWidgetTest::DeviceRemoved));
  m_other_end->Close();
  m_ss.Run();

  CPPUNIT_ASSERT(m_removed);
}
