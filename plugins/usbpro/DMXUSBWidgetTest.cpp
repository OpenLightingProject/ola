/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DMXUSBWidgetTest.cpp
 * Test fixture for the DMXUSBWidget class (based on DMX King Ultra DMX Pro
 * code by Simon Newton)
 * Copyright (C) 2019 Perry Naseck (DaAwesomeP)
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "plugins/usbpro/DMXUSBWidget.h"
#include "plugins/usbpro/CommonWidgetTest.h"


using std::auto_ptr;
using ola::DmxBuffer;


class DMXUSBWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(DMXUSBWidgetTest);
  CPPUNIT_TEST(testAllSendDMX);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void testAllSendDMX();

 private:
    auto_ptr<ola::plugin::usbpro::DMXUSBWidget> m_widget;

    void Terminate() { m_ss.Terminate(); }

    static const uint8_t START_DMX_LABEL = 100;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DMXUSBWidgetTest);


void DMXUSBWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  m_widget.reset(
      new ola::plugin::usbpro::DMXUSBWidget(&m_descriptor));
}


/**
 * Check that we can send DMX on the all ports
 */
void DMXUSBWidgetTest::testAllSendDMX() {
  // dmx data
  DmxBuffer buffer;
  buffer.SetFromString("0,1,2,3,4");

  // test on 47 universes
  for (int i = 0; i < 47; i++) {
    // expected message
    uint8_t dmx_frame_data[] = {ola::DMX512_START_CODE, 0, 1, 2, 3, 4};
    m_endpoint->AddExpectedUsbProMessage(
        START_DMX_LABEL + i,
        dmx_frame_data,
        sizeof(dmx_frame_data),
        ola::NewSingleCallback(this, &DMXUSBWidgetTest::Terminate));

    m_widget->SendDMXPort(i, buffer);
    m_ss.Run();
    m_endpoint->Verify();

    // now test an empty frame
    DmxBuffer buffer2;
    // just the start code
    uint8_t empty_frame_data[] = {ola::DMX512_START_CODE};
    m_endpoint->AddExpectedUsbProMessage(
        START_DMX_LABEL + i,
        empty_frame_data,
        sizeof(empty_frame_data),
        ola::NewSingleCallback(this, &DMXUSBWidgetTest::Terminate));
    m_widget->SendDMXPort(i, buffer2);
    m_ss.Run();
    m_endpoint->Verify();
  }
}
