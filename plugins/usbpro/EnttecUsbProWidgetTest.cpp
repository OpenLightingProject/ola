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
 * EnttecUsbProWidgetTest.cpp
 * Test fixture for the EnttecUsbProWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "plugins/usbpro/EnttecUsbProWidget.h"
#include "plugins/usbpro/CommonWidgetTest.h"


using std::auto_ptr;
using ola::plugin::usbpro::usb_pro_parameters;


class EnttecUsbProWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(EnttecUsbProWidgetTest);
  CPPUNIT_TEST(testParams);
  CPPUNIT_TEST(testReceiveDMX);
  CPPUNIT_TEST(testChangeMode);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testParams();
    void testReceiveDMX();
    void testChangeMode();

  private:
    auto_ptr<ola::plugin::usbpro::EnttecUsbProWidget> m_widget;

    void Terminate() { m_ss.Terminate(); }
    void ValidateParams(bool status, const usb_pro_parameters &params);
    void ValidateDMX(const ola::DmxBuffer *expected_buffer);

    bool m_got_dmx;

    static const uint8_t CHANGE_MODE_LABEL = 8;
    static const uint8_t CHANGE_OF_STATE_LABEL = 9;
    static const uint8_t GET_PARAM_LABEL = 3;
    static const uint8_t RECEIVE_DMX_LABEL = 5;
    static const uint8_t SET_PARAM_LABEL = 4;
    static const unsigned int FOOTER_SIZE = 1;
    static const unsigned int HEADER_SIZE = 4;
};

CPPUNIT_TEST_SUITE_REGISTRATION(EnttecUsbProWidgetTest);


void EnttecUsbProWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  m_widget.reset(
      new ola::plugin::usbpro::EnttecUsbProWidget(&m_ss, &m_descriptor));

  m_got_dmx = false;
}


/**
 * Check the params are ok
 */
void EnttecUsbProWidgetTest::ValidateParams(
    bool status,
    const usb_pro_parameters &params) {
  CPPUNIT_ASSERT(status);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, params.firmware);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 1, params.firmware_high);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 10, params.break_time);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 14, params.mab_time);
  CPPUNIT_ASSERT_EQUAL((uint8_t) 40, params.rate);
  m_ss.Terminate();
}


/**
 * Check the DMX data is what we expected
 */
void EnttecUsbProWidgetTest::ValidateDMX(
    const ola::DmxBuffer *expected_buffer) {
  const ola::DmxBuffer &buffer = m_widget->FetchDMX();
  CPPUNIT_ASSERT(*expected_buffer == buffer);
  m_got_dmx = true;
  m_ss.Terminate();
}


/**
 * Check that discovery works for a device that just implements the serial #
 */
void EnttecUsbProWidgetTest::testParams() {
  uint8_t get_param_request_data[] = {0, 0};
  uint8_t get_param_response_data[] = {0, 1, 10, 14, 40};

  m_endpoint->AddExpectedUsbProDataAndReturn(
      GET_PARAM_LABEL,
      get_param_request_data,
      sizeof(get_param_request_data),
      GET_PARAM_LABEL,
      get_param_response_data,
      sizeof(get_param_response_data));

  m_widget->GetParameters(
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::ValidateParams));

  m_ss.Run();
  m_endpoint->Verify();

  // now try a set params request
  uint8_t set_param_request_data[] = {0, 0, 9, 63, 20};
  m_endpoint->AddExpectedUsbProMessage(
      SET_PARAM_LABEL,
      set_param_request_data,
      sizeof(set_param_request_data),
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));

  CPPUNIT_ASSERT(m_widget->SetParameters(9, 63, 20));

  m_ss.Run();
  m_endpoint->Verify();
}


/**
 * Check that recieving DMX works.
 */
void EnttecUsbProWidgetTest::testReceiveDMX() {
  ola::DmxBuffer buffer;
  buffer.SetFromString("1,10,14,40");
  m_widget->SetDMXCallback(ola::NewCallback(
      this,
      &EnttecUsbProWidgetTest::ValidateDMX,
      const_cast<const ola::DmxBuffer*>(&buffer)));
  uint8_t dmx_data[] = {
    0, 0,  // no error
    1, 10, 14, 40
  };

  m_endpoint->SendUnsolicitedUsbProData(
      RECEIVE_DMX_LABEL,
      dmx_data,
      sizeof(dmx_data));
  m_ss.Run();
  m_endpoint->Verify();
  CPPUNIT_ASSERT(m_got_dmx);

  // now try one with the error bit set
  dmx_data[0] = 1;
  m_got_dmx = false;
  m_endpoint->SendUnsolicitedUsbProData(
      RECEIVE_DMX_LABEL,
      dmx_data,
      sizeof(dmx_data));
  // because this doesn't trigger the callback we have no way to terminate the
  // select server, so we use a timeout, which is nasty, but fails closed
  m_ss.RegisterSingleTimeout(
      100,  // shold be more than enough time
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));
  m_ss.Run();
  m_endpoint->Verify();
  CPPUNIT_ASSERT(!m_got_dmx);

  // now try a non-0 start code
  dmx_data[0] = 0;
  dmx_data[1] = 0x0a;
  m_got_dmx = false;
  m_endpoint->SendUnsolicitedUsbProData(
      RECEIVE_DMX_LABEL,
      dmx_data,
      sizeof(dmx_data));
  // use the timeout trick again
  m_ss.RegisterSingleTimeout(
      100,
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));
  m_ss.Run();
  m_endpoint->Verify();
  CPPUNIT_ASSERT(!m_got_dmx);

  // now do a change of state packet
  buffer.SetFromString("1,10,22,93,144");
  uint8_t change_of_state_data[] = {
    0, 0x38, 0, 0, 0, 0,
    22, 93, 144, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0
  };

  m_endpoint->SendUnsolicitedUsbProData(
      CHANGE_OF_STATE_LABEL,
      change_of_state_data,
      sizeof(change_of_state_data));
  m_ss.Run();
  m_endpoint->Verify();
  CPPUNIT_ASSERT(m_got_dmx);
}


/*
 * Check that changing mode works.
 */
void EnttecUsbProWidgetTest::testChangeMode() {
  // first we test 'send always' mode
  uint8_t change_mode_data[] = {0};
  m_endpoint->AddExpectedUsbProMessage(
      CHANGE_MODE_LABEL,
      change_mode_data,
      sizeof(change_mode_data),
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));

  m_widget->ChangeToReceiveMode(false);

  m_ss.Run();
  m_endpoint->Verify();

  // now try 'send data on change' mode
  change_mode_data[0] = 1;
  m_endpoint->AddExpectedUsbProMessage(
      CHANGE_MODE_LABEL,
      change_mode_data,
      sizeof(change_mode_data),
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));

  m_widget->ChangeToReceiveMode(true);
  m_ss.Run();
  m_endpoint->Verify();
}
