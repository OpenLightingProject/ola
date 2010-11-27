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
 * DmxterWidgetTest.cpp
 * Test fixture for the DmxterWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <queue>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/SelectServer.h"
#include "plugins/usbpro/UsbWidget.h"
#include "plugins/usbpro/DmxterWidget.h"


using ola::network::ConnectedSocket;
using ola::network::PipeSocket;
using ola::plugin::usbpro::DmxterWidget;
using ola::plugin::usbpro::UsbWidget;
using ola::rdm::RDMRequest;
using ola::rdm::UID;


/**
 * The MockUsbWidget, used to verify calls.
 */
class MockUsbWidget: public ola::plugin::usbpro::UsbWidgetInterface {
  public:
    MockUsbWidget():
      m_callback(NULL) {}
    ~MockUsbWidget() {
      if (m_callback)
        delete m_callback;
    }

    void SetMessageHandler(
        ola::Callback3<void, uint8_t, const uint8_t*,
                       unsigned int> *callback) {
      if (m_callback)
        delete m_callback;
      m_callback = callback;
    }

    // this doesn't do anything
    void SetOnRemove(ola::SingleUseCallback0<void> *on_close) {
      delete on_close;
    }

    bool SendMessage(uint8_t label,
                     const uint8_t *data,
                     unsigned int length) const;

    void AddExpectedCall(uint8_t expected_label,
                         const uint8_t *expected_data,
                         unsigned int expected_length,
                         uint8_t return_label,
                         const uint8_t *return_data,
                         unsigned int return_length);

    void Verify() {
      CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), m_expected_calls.size());
    }

  private:
    ola::Callback3<void, uint8_t, const uint8_t*, unsigned int> *m_callback;

    typedef struct {
      uint8_t label;
      unsigned int length;
      const uint8_t *data;
    } command;

    typedef struct {
      command expected_command;
      command return_command;
    } expected_call;

    mutable std::queue<expected_call> m_expected_calls;
};


class DmxterWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DmxterWidgetTest);
  CPPUNIT_TEST(testTod);
  CPPUNIT_TEST(testSendRequest);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testTod();
    void testSendRequest();

  private:
    unsigned int m_tod_counter;
    void ValidateTod(const ola::rdm::UIDSet &uids);
    void ValidateResponse(ola::rdm::rdm_request_status status,
                          const ola::rdm::RDMResponse *response);

    ola::network::SelectServer m_ss;
    MockUsbWidget m_widget;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmxterWidgetTest);



bool MockUsbWidget::SendMessage(uint8_t label,
                                const uint8_t *data,
                                unsigned int length) const {
  CPPUNIT_ASSERT(m_expected_calls.size());
  expected_call call = m_expected_calls.front();
  m_expected_calls.pop();
  CPPUNIT_ASSERT_EQUAL(call.expected_command.label, label);
  CPPUNIT_ASSERT_EQUAL(call.expected_command.length, length);
  CPPUNIT_ASSERT(!memcmp(call.expected_command.data, data, length));

  m_callback->Run(call.return_command.label,
                  call.return_command.data,
                  call.return_command.length);
  return true;
}


void MockUsbWidget::AddExpectedCall(uint8_t expected_label,
                                    const uint8_t *expected_data,
                                    unsigned int expected_length,
                                    uint8_t return_label,
                                    const uint8_t *return_data,
                                    unsigned int return_length) {
  expected_call call;
  call.expected_command.label = expected_label;
  call.expected_command.data = expected_data;
  call.expected_command.length = expected_length;
  call.return_command.label = return_label;
  call.return_command.data = return_data;
  call.return_command.length = return_length;
  m_expected_calls.push(call);
}


void DmxterWidgetTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_tod_counter = 0;
}


/**
 * Check the TOD matches what we expect
 */
void DmxterWidgetTest::ValidateTod(const ola::rdm::UIDSet &uids) {
  ola::rdm::UID uid1(0x707a, 0xffffff00);
  ola::rdm::UID uid2(0x5252, 0x12345678);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, uids.Size());
  CPPUNIT_ASSERT(uids.Contains(uid1));
  CPPUNIT_ASSERT(uids.Contains(uid2));
  m_tod_counter++;
}


/**
 * Check the response matches what we expected.
 */
void DmxterWidgetTest::ValidateResponse(
    ola::rdm::rdm_request_status status,
    const ola::rdm::RDMResponse *response) {

  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_INVALID_RESPONSE, status);
  delete response;
}


/**
 * Check that discovery works for a device that just implements the serial #
 */
void DmxterWidgetTest::testTod() {
  uint8_t TOD_LABEL = 0x82;
  ola::plugin::usbpro::DmxterWidget dmxter(&m_ss,
                                           &m_widget,
                                           0,
                                           0);
  uint8_t return_packet[] = {
    0x70, 0x7a, 0xff, 0xff, 0xff, 0x00,
    0x52, 0x52, 0x12, 0x34, 0x56, 0x78,
  };

  m_widget.AddExpectedCall(
      TOD_LABEL,
      NULL,
      0,
      TOD_LABEL,
      reinterpret_cast<uint8_t*>(&return_packet),
      sizeof(return_packet));

  dmxter.SetUIDListCallback(
      ola::NewCallback(this, &DmxterWidgetTest::ValidateTod));

  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, m_tod_counter);
  dmxter.SendTodRequest();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, m_tod_counter);
  dmxter.SendUIDUpdate();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, m_tod_counter);
}


/**
 * Check that we send messages correctly.
 */
void DmxterWidgetTest::testSendRequest() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  UID source(1, 2);
  UID destination(3, 4);
  UID new_source(0x5253, 0x12345678);

  ola::plugin::usbpro::DmxterWidget dmxter(&m_ss,
                                           &m_widget,
                                           0x5253,
                                           0x12345678);

  RDMRequest *request = new ola::rdm::RDMGetRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // message count
      10,  // sub device
      296,  // param id
      NULL,  // data
      0);  // data length

  unsigned int size = request->Size();
  uint8_t *expected_packet = new uint8_t[size + 1];
  expected_packet[0] = 0xcc;
  CPPUNIT_ASSERT(request->PackWithControllerParams(
        expected_packet + 1,
        &size,
        new_source,
        0,
        1));

  uint8_t return_packet[] = {
    0xcc, 0x70, 0x7a, 0xff, 0xff, 0xff, 0x00,
    0x52, 0x52, 0x12, 0x34, 0x56, 0x78,
  };

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  dmxter.SendRequest(
      request,
      ola::NewSingleCallback(this, &DmxterWidgetTest::ValidateResponse));

  delete[] expected_packet;

  // now check broadcast
}
