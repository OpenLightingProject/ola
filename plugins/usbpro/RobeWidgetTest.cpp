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
 * RobeWidgetTest.cpp
 * Test fixture for the DmxterWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <queue>
#include <string>
#include <vector>

#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/MockEndpoint.h"


using ola::DmxBuffer;
using ola::plugin::usbpro::RobeWidget;
using std::auto_ptr;
using std::string;


class RobeWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RobeWidgetTest);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();

    void testSendDMX();

  private:
    ola::network::SelectServer m_ss;
    ola::network::PipeDescriptor m_descriptor;
    auto_ptr<ola::network::PipeDescriptor> m_other_end;
    auto_ptr<MockEndpoint> m_endpoint;
    auto_ptr<ola::plugin::usbpro::RobeWidget> m_widget;

    void Terminate() {
      m_ss.Terminate();
    }

    uint8_t *BuildRobeMessage(uint8_t label,
                              const uint8_t *data,
                              unsigned int data_size,
                              unsigned int *total_size);

    static const unsigned int FOOTER_SIZE = 1;
    static const unsigned int HEADER_SIZE = 5;
    static const uint8_t DMX_FRAME_LABEL = 0x06;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RobeWidgetTest);


void RobeWidgetTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_descriptor.Init();
  m_other_end.reset(m_descriptor.OppositeEnd());
  m_endpoint.reset(new MockEndpoint(m_other_end.get()));
  m_ss.AddReadDescriptor(&m_descriptor);
  m_ss.AddReadDescriptor(m_other_end.get());

  m_widget.reset(new ola::plugin::usbpro::RobeWidget(&m_descriptor));
}


void RobeWidgetTest::tearDown() {
  m_endpoint->Verify();
  m_ss.RemoveReadDescriptor(&m_descriptor);
  m_ss.RemoveReadDescriptor(m_other_end.get());
}


/**
 * Pack data into a Robe style frame.
 * @param label the message label
 * @param data the message data
 * @param data_size the data size
 * @param total_size, pointer which is updated with the message size.
 */
uint8_t *RobeWidgetTest::BuildRobeMessage(uint8_t label,
                                          const uint8_t *data,
                                          unsigned int data_size,
                                          unsigned int *total_size) {
  uint8_t *frame = new uint8_t[data_size + HEADER_SIZE + FOOTER_SIZE];
  frame[0] = 0xa5;  // som
  frame[1] = label;
  frame[2] = data_size & 0xff;  // len
  frame[3] = (data_size + 1) >> 8;  // len hi

  // header crc
  uint8_t crc = 0;
  for (unsigned int i = 0; i < 4; i++)
    crc += frame[i];
  frame[4] = crc;

  // data & final crc
  crc += frame[4];
  memcpy(frame + HEADER_SIZE, data, data_size);
  for (unsigned int i = 0; i < data_size; i++)
    crc += data[i];
  frame[data_size + HEADER_SIZE] = crc;
  *total_size = data_size + HEADER_SIZE + FOOTER_SIZE;
  return frame;
}


/*
 * Check that we can send DMX
 */
void RobeWidgetTest::testSendDMX() {
  // dmx data
  DmxBuffer buffer;
  buffer.SetFromString("0,1,2,3,4");

  // expected message
  uint8_t dmx_frame_data[] = {0, 1, 2, 3, 4, 0, 0, 0, 0};
  unsigned int size;
  uint8_t *expected_message = BuildRobeMessage(DMX_FRAME_LABEL,
                                               dmx_frame_data,
                                               sizeof(dmx_frame_data),
                                               &size);
  // add the expected data, run and verify.
  m_endpoint->AddExpectedData(
      expected_message,
      size,
      ola::NewSingleCallback(this, &RobeWidgetTest::Terminate));
  m_widget->SendDMX(buffer);
  m_ss.Run();
  m_endpoint->Verify();

  // now test an empty frame
  DmxBuffer buffer2;
  uint8_t empty_frame_data[] = {0, 0, 0, 0};  // null frames still have 4 bytes
  uint8_t *expected_message2 = BuildRobeMessage(DMX_FRAME_LABEL,
                                                empty_frame_data,
                                                sizeof(empty_frame_data),
                                                &size);

  // add the expected data, run and verify.
  m_endpoint->AddExpectedData(
      expected_message2,
      size,
      ola::NewSingleCallback(this, &RobeWidgetTest::Terminate));
  m_widget->SendDMX(buffer2);
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_message;
  delete[] expected_message2;
}

