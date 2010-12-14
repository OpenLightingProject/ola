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
#include "plugins/usbpro/DmxterWidget.h"
#include "plugins/usbpro/MockUsbWidget.h"


using ola::plugin::usbpro::DmxterWidget;
using ola::plugin::usbpro::UsbWidget;
using ola::rdm::RDMRequest;
using ola::rdm::UID;


class DmxterWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DmxterWidgetTest);
  CPPUNIT_TEST(testTod);
  CPPUNIT_TEST(testSendRDMRequest);
  CPPUNIT_TEST(testErrorCodes);
  CPPUNIT_TEST(testErrorConditions);
  CPPUNIT_TEST(testShutdown);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testTod();
    void testSendRDMRequest();
    void testErrorCodes();
    void testErrorConditions();
    void testShutdown();

  private:
    unsigned int m_tod_counter;
    void ValidateTod(const ola::rdm::UIDSet &uids);
    void ValidateResponse(ola::rdm::rdm_response_status status,
                          const ola::rdm::RDMResponse *response);
    void ValidateStatus(ola::rdm::rdm_response_status expected_status,
                        ola::rdm::rdm_response_status status,
                        const ola::rdm::RDMResponse *response);
    const RDMRequest *NewRequest(const UID &source,
                                 const UID &destination,
                                 const uint8_t *data,
                                 unsigned int length);

    ola::network::SelectServer m_ss;
    MockUsbWidget m_widget;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmxterWidgetTest);


void DmxterWidgetTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_tod_counter = 0;
}


/**
 * Check the TOD matches what we expect
 */
void DmxterWidgetTest::ValidateTod(const ola::rdm::UIDSet &uids) {
  UID uid1(0x707a, 0xffffff00);
  UID uid2(0x5252, 0x12345678);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, uids.Size());
  CPPUNIT_ASSERT(uids.Contains(uid1));
  CPPUNIT_ASSERT(uids.Contains(uid2));
  m_tod_counter++;
}


/**
 * Check the response matches what we expected.
 */
void DmxterWidgetTest::ValidateResponse(
    ola::rdm::rdm_response_status status,
    const ola::rdm::RDMResponse *response) {
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_COMPLETED_OK, status);
  CPPUNIT_ASSERT(response);
  uint8_t expected_data[] = {0x5a, 0x5a, 0x5a, 0x5a};
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4, response->ParamDataSize());
  CPPUNIT_ASSERT(0 == memcmp(expected_data, response->ParamData(),
                             response->ParamDataSize()));
  delete response;
}


/*
 * Check that we got an unknown UID status
 */
void DmxterWidgetTest::ValidateStatus(
    ola::rdm::rdm_response_status expected_status,
    ola::rdm::rdm_response_status status,
    const ola::rdm::RDMResponse *response) {
  CPPUNIT_ASSERT_EQUAL(expected_status, status);
  CPPUNIT_ASSERT(!response);
}


/**
 * Helper method to create new request objects
 */
const RDMRequest *DmxterWidgetTest::NewRequest(const UID &source,
                                               const UID &destination,
                                               const uint8_t *data,
                                               unsigned int length) {
  return new ola::rdm::RDMGetRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // message count
      10,  // sub device
      296,  // param id
      data,
      length);
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

  m_widget.Verify();
}


/**
 * Check that we send messages correctly.
 */
void DmxterWidgetTest::testSendRDMRequest() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  uint8_t RDM_BROADCAST_REQUEST_LABEL = 0x81;
  UID source(1, 2);
  UID destination(3, 4);
  UID bcast_destination(3, 0xffffffff);
  UID new_source(0x5253, 0x12345678);

  ola::plugin::usbpro::DmxterWidget dmxter(&m_ss,
                                           &m_widget,
                                           0x5253,
                                           0x12345678);

  const RDMRequest *request = NewRequest(source, destination, NULL, 0);

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
    0x00, 14,  // response code 'ok'
    0xcc,
    1, 28,  // sub code & length
    0x52, 0x53, 0x12, 0x34, 0x56, 0x78,   // dst uid
    0, 3, 0, 0, 0, 4,   // src uid
    0, 1, 0, 0, 0,  // transaction, port id, msg count & sub device
    0x21, 0x1, 0x28, 4,  // command, param id, param data length
    0x5a, 0x5a, 0x5a, 0x5a,  // param data
    0x04, 0x60  // checksum, filled in below
  };

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateResponse));

  // now check broadcast
  request = NewRequest(source, bcast_destination, NULL, 0);

  CPPUNIT_ASSERT(request->PackWithControllerParams(
        expected_packet + 1,
        &size,
        new_source,
        1,  // increment transaction #
        1));

  m_widget.AddExpectedCall(
      RDM_BROADCAST_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_BROADCAST_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(NULL),
      0);

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST));

  delete[] expected_packet;
  m_widget.Verify();
}


/**
 * Check that we handle invalid responses ok
 */
void DmxterWidgetTest::testErrorCodes() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  UID source(1, 2);
  UID destination(3, 4);
  UID new_source(0x5253, 0x12345678);

  ola::plugin::usbpro::DmxterWidget dmxter(&m_ss,
                                           &m_widget,
                                           0x5253,
                                           0x12345678);

  const RDMRequest *request = NewRequest(source, destination, NULL, 0);

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
    0x00, 1,  // checksum failure
  };

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_CHECKSUM_INCORRECT));
  m_widget.Verify();

  // update transaction # & checksum
  expected_packet[15]++;
  expected_packet[25] = 0xfa;
  return_packet[1] = 8; // packet too short
  request = NewRequest(source, destination, NULL, 0);
  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_INVALID_RESPONSE));
  m_widget.Verify();

  // update transaction # & checksum
  expected_packet[15]++;
  expected_packet[25] = 0xfb;
  return_packet[1] = 12; // transaction mismatch
  request = NewRequest(source, destination, NULL, 0);
  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TRANSACTION_MISMATCH));
  m_widget.Verify();

  // update transaction # & checksum
  expected_packet[15]++;
  expected_packet[25] = 0xfc;
  return_packet[1] = 41; // device mismatch
  request = NewRequest(source, destination, NULL, 0);
  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_DEVICE_MISMATCH));
  m_widget.Verify();

  // update transaction # & checksum
  expected_packet[15]++;
  expected_packet[25] = 0xfd;
  return_packet[1] = 42; // sub device mismatch
  request = NewRequest(source, destination, NULL, 0);
  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_SUB_DEVICE_MISMATCH));
  m_widget.Verify();

  delete[] expected_packet;
  m_widget.Verify();
}


/**
 * Check that we handle a timeout correctly
void DmxterWidgetTest::testTimeout() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  UID source(1, 2);
  UID destination(3, 4);
  UID new_source(0x5253, 0x12345678);

  ola::plugin::usbpro::DmxterWidget dmxter(&m_ss,
                                           &m_widget,
                                           0x5253,
                                           0x12345678);

  const RDMRequest *request = NewRequest(source, destination, NULL, 0);

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
    0x00, 17,  // response code 'timeout'
  };

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  request = NewRequest(source, destination, NULL, 0);
  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TIMEOUT));
  delete[] expected_packet;
  m_widget.Verify();
}
 */


/**
 * Check some of the error conditions
 */
void DmxterWidgetTest::testErrorConditions() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  UID source(1, 2);
  UID destination(3, 4);
  UID new_source(0x5253, 0x12345678);

  ola::plugin::usbpro::DmxterWidget dmxter(&m_ss,
                                           &m_widget,
                                           0x5253,
                                           0x12345678);

  const RDMRequest *request = NewRequest(source, destination, NULL, 0);

  unsigned int size = request->Size();
  uint8_t *expected_packet = new uint8_t[size + 1];
  expected_packet[0] = 0xcc;
  CPPUNIT_ASSERT(request->PackWithControllerParams(
        expected_packet + 1,
        &size,
        new_source,
        0,
        1));

  // to small to be valid
  uint8_t return_packet[] = {0x00};

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet),
      sizeof(return_packet));

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_INVALID_RESPONSE));

  // check mismatched version
  request = NewRequest(source, destination, NULL, 0);

  CPPUNIT_ASSERT(request->PackWithControllerParams(
        expected_packet + 1,
        &size,
        new_source,
        1,  // increment transaction #
        1));

  // non-0 version
  uint8_t return_packet2[] = {0x01, 0x11, 0xcc};

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(expected_packet),
      size + 1,
      RDM_REQUEST_LABEL,
      reinterpret_cast<uint8_t*>(return_packet2),
      sizeof(return_packet2));

  dmxter.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_INVALID_RESPONSE));

  delete[] expected_packet;
  m_widget.Verify();
}


/**
 * Check that the shutdown message works
 */
void DmxterWidgetTest::testShutdown() {
  uint8_t SHUTDOWN_LABEL = 0xf0;

  ola::plugin::usbpro::DmxterWidget dmxter(&m_ss,
                                           &m_widget,
                                           0x5253,
                                           0x12345678);
  CPPUNIT_ASSERT(!m_widget.IsClosed());

  // first try a bad message
  uint8_t data = 1;
  m_widget.SendUnsolicited(SHUTDOWN_LABEL, &data, 1);
  CPPUNIT_ASSERT(!m_widget.IsClosed());

  // now a valid one
  m_widget.SendUnsolicited(SHUTDOWN_LABEL, NULL, 0);
  CPPUNIT_ASSERT(m_widget.IsClosed());
  m_widget.Verify();
}
