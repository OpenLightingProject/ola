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
 * UsbProWidgetTest.cpp
 * Test fixture for the DmxterWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <queue>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/network/SelectServer.h"
#include "plugins/usbpro/MockUsbWidget.h"
#include "plugins/usbpro/UsbProWidget.h"


using ola::plugin::usbpro::UsbProWidget;
using ola::plugin::usbpro::UsbWidget;
using ola::plugin::usbpro::usb_pro_parameters;

class UsbProWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UsbProWidgetTest);
  CPPUNIT_TEST(testParams);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST(testReceiveDMX);
  CPPUNIT_TEST(testChangeMode);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testParams();
    void testSendDMX();
    void testReceiveDMX();
    void testChangeMode();

  private:
    void ValidateParams(bool status, const usb_pro_parameters &params);
    void ValidateDMX(const ola::plugin::usbpro::UsbProWidget *widget,
                     const ola::DmxBuffer *buffer);

    ola::network::SelectServer m_ss;
    MockUsbWidget m_widget;
    bool m_got_dmx;
};

CPPUNIT_TEST_SUITE_REGISTRATION(UsbProWidgetTest);


void UsbProWidgetTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_got_dmx = false;
}


/**
 * Check the params are ok
 */
void UsbProWidgetTest::ValidateParams(
    bool status,
    const usb_pro_parameters &params) {
  CPPUNIT_ASSERT(status);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, params.firmware);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, params.firmware_high);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, params.break_time);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 14, params.mab_time);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 40, params.rate);
}


/**
 * Check the DMX data is what we expected
 */
void UsbProWidgetTest::ValidateDMX(
    const ola::plugin::usbpro::UsbProWidget *widget,
    const ola::DmxBuffer *expected_buffer) {
  const ola::DmxBuffer &buffer = widget->FetchDMX();
  CPPUNIT_ASSERT(*expected_buffer == buffer);
  m_got_dmx = true;
}


/**
 * Check that discovery works for a device that just implements the serial #
 */
void UsbProWidgetTest::testParams() {
  uint8_t GET_PARAM_LABEL = 3;
  uint8_t SET_PARAM_LABEL = 4;
  ola::plugin::usbpro::UsbProWidget widget(&m_ss, &m_widget);
  uint8_t expected_get_param_packet[] = {0, 0};
  uint8_t param_packet[] = {
    0, 1, 10, 14, 40
  };

  m_widget.AddExpectedCall(
      GET_PARAM_LABEL,
      expected_get_param_packet,
      sizeof(expected_get_param_packet),
      GET_PARAM_LABEL,
      param_packet,
      sizeof(param_packet));

  widget.GetParameters(
      ola::NewSingleCallback(this, &UsbProWidgetTest::ValidateParams));

  m_widget.Verify();

  uint8_t expected_set_param_packet[] = {
    0, 0, 9, 63, 20};

  m_widget.AddExpectedCall(
      SET_PARAM_LABEL,
      expected_set_param_packet,
      sizeof(expected_set_param_packet));
  CPPUNIT_ASSERT(widget.SetParameters(9, 63, 20));

  m_widget.Verify();

  // TODO(simon): add a test for a get request that times out
}


/**
 * Check that sending DMX works.
 */
void UsbProWidgetTest::testSendDMX() {
  uint8_t SEND_DMX_LABEL = 6;
  ola::plugin::usbpro::UsbProWidget widget(&m_ss, &m_widget);
  uint8_t dmx_packet[] = {
    0,
    1, 10, 14, 40, 35, 27, 61, 89, 255, 100,
    43, 183, 99, 42, 55, 0, 230, 10, 0, 128, 143,
    67, 52, 70, 94, 23,
  };

  ola::DmxBuffer buffer;
  buffer.SetFromString(
    "1,10,14,40,35,27,61,89,255,100,43,183,99,42,55,0,230,10,0,128,143,"
    "67,52,70,94,23");

  m_widget.AddExpectedCall(
      SEND_DMX_LABEL,
      dmx_packet,
      sizeof(dmx_packet));

  widget.SendDMX(buffer);
  m_widget.Verify();
}


/**
 * Check that recieving DMX works.
 */
void UsbProWidgetTest::testReceiveDMX() {
  uint8_t RECEIVE_DMX_LABEL = 5;
  uint8_t CHANGE_OF_STATE_LABEL = 9;
  ola::plugin::usbpro::UsbProWidget widget(&m_ss, &m_widget);

  ola::DmxBuffer buffer;
  buffer.SetFromString("1,10,14,40");
  widget.SetDMXCallback(ola::NewCallback(
      this,
      &UsbProWidgetTest::ValidateDMX,
      const_cast<const UsbProWidget*>(&widget),
      const_cast<const ola::DmxBuffer*>(&buffer)));
  uint8_t dmx_packet[] = {
    0, 0,  // no error
    1, 10, 14, 40
  };

  m_widget.SendUnsolicited(
      RECEIVE_DMX_LABEL,
      dmx_packet,
      sizeof(dmx_packet));
  CPPUNIT_ASSERT(m_got_dmx);

  // now try one with the error bit set
  dmx_packet[0] = 1;
  m_got_dmx = false;
  m_widget.SendUnsolicited(
      RECEIVE_DMX_LABEL,
      dmx_packet,
      sizeof(dmx_packet));
  CPPUNIT_ASSERT(!m_got_dmx);

  // now try a non-0 start code
  dmx_packet[0] = 0;
  dmx_packet[1] = 0x0a;
  m_got_dmx = false;
  m_widget.SendUnsolicited(
      RECEIVE_DMX_LABEL,
      dmx_packet,
      sizeof(dmx_packet));
  CPPUNIT_ASSERT(!m_got_dmx);

  // now do a change of state packet
  buffer.SetFromString("1,10,22,93,144");
  uint8_t change_of_state_packet[] = {
    0, 0x38, 0, 0, 0, 0,
    22, 93, 144, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0
  };

  m_widget.SendUnsolicited(
      CHANGE_OF_STATE_LABEL,
      change_of_state_packet,
      sizeof(change_of_state_packet));
  CPPUNIT_ASSERT(m_got_dmx);
}


/**
 * Check that changing mode works.
 */
void UsbProWidgetTest::testChangeMode() {
  uint8_t CHANGE_MODE_LABEL = 8;
  ola::plugin::usbpro::UsbProWidget widget(&m_ss, &m_widget);

  uint8_t expected_mode_packet = 0;
  m_widget.AddExpectedCall(
      CHANGE_MODE_LABEL,
      &expected_mode_packet,
      sizeof(expected_mode_packet));

  widget.ChangeToReceiveMode(false);
  m_widget.Verify();

  expected_mode_packet = 1;
  m_widget.AddExpectedCall(
      CHANGE_MODE_LABEL,
      &expected_mode_packet,
      sizeof(expected_mode_packet));

  widget.ChangeToReceiveMode(true);
  m_widget.Verify();
}
