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
 * DmxTriWidgetTest.cpp
 * Test fixture for the DmxTriWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <queue>
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/network/SelectServer.h"
#include "plugins/usbpro/MockUsbWidget.h"
#include "plugins/usbpro/DmxTriWidget.h"


using ola::plugin::usbpro::DmxTriWidget;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::string;
using std::vector;


class DmxTriWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DmxTriWidgetTest);
  CPPUNIT_TEST(testTod);
  CPPUNIT_TEST(testDmx);
  CPPUNIT_TEST(testSendRDM);
  CPPUNIT_TEST(testSendRDMErrors);
  CPPUNIT_TEST(testSendRDMBroadcast);
  CPPUNIT_TEST(testNack);
  CPPUNIT_TEST(testAckTimer);
  CPPUNIT_TEST(testAckOverflow);
  CPPUNIT_TEST(testQueuedMessages);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testTod();
    void testDmx();
    void testSendRDM();
    void testSendRDMErrors();
    void testSendRDMBroadcast();
    void testNack();
    void testAckTimer();
    void testAckOverflow();
    void testQueuedMessages();

  private:
    unsigned int m_tod_counter;
    bool m_expect_uids_in_tod;
    void PopulateTod(DmxTriWidget *widget);
    void ValidateTod(const ola::rdm::UIDSet &uids);
    void ValidateResponse(ola::rdm::rdm_response_code expected_code,
                          const RDMResponse *expected_response,
                          vector<string> expected_packets,
                          ola::rdm::rdm_response_code code,
                          const RDMResponse *response,
                          const vector<string> &packets);
    void ValidateStatus(ola::rdm::rdm_response_code expected_code,
                        vector<string> expected_packets,
                        ola::rdm::rdm_response_code code,
                        const RDMResponse *response,
                        const vector<string> &packets);
    const RDMRequest *NewRequest(const UID &source,
                                 const UID &destination,
                                 const uint8_t *data,
                                 unsigned int length);
    const RDMRequest *NewQueuedMessageRequest(const UID &source,
                                              const UID &destination,
                                              uint8_t code);

    ola::network::SelectServer m_ss;
    MockUsbWidget m_widget;

    static const uint8_t EXTENDED_LABEL = 0x58;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmxTriWidgetTest);


void DmxTriWidgetTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_tod_counter = 0;
  m_expect_uids_in_tod = false;
}


/**
 * Check the TOD matches what we expect
 */
void DmxTriWidgetTest::ValidateTod(const ola::rdm::UIDSet &uids) {
  if (m_expect_uids_in_tod) {
    UID uid1(0x707a, 0xffffff00);
    UID uid2(0x5252, 0x12345678);
    CPPUNIT_ASSERT_EQUAL((unsigned int) 2, uids.Size());
    CPPUNIT_ASSERT(uids.Contains(uid1));
    CPPUNIT_ASSERT(uids.Contains(uid2));
  } else {
    CPPUNIT_ASSERT_EQUAL((unsigned int) 0, uids.Size());
  }
  m_tod_counter++;
  m_ss.Terminate();
}


/**
 * Check the response matches what we expected.
 */
void DmxTriWidgetTest::ValidateResponse(
    ola::rdm::rdm_response_code expected_code,
    const RDMResponse *expected_response,
    vector<string> expected_packets,
    ola::rdm::rdm_response_code code,
    const RDMResponse *response,
    const vector<string> &packets) {
  CPPUNIT_ASSERT_EQUAL(expected_code, code);
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT(*expected_response == *response);
  delete response;

  // the TRIs can't return the actual packets
  CPPUNIT_ASSERT_EQUAL(expected_packets.size(), packets.size());
}


/*
 * Check that we got an unknown UID code
 */
void DmxTriWidgetTest::ValidateStatus(
    ola::rdm::rdm_response_code expected_code,
    vector<string> expected_packets,
    ola::rdm::rdm_response_code code,
    const RDMResponse *response,
    const vector<string> &packets) {
  CPPUNIT_ASSERT_EQUAL(expected_code, code);
  CPPUNIT_ASSERT(!response);

  // the TRIs can't return the actual packets
  CPPUNIT_ASSERT_EQUAL(expected_packets.size(), packets.size());
}


/**
 * Helper method to create new request objects
 */
const RDMRequest *DmxTriWidgetTest::NewRequest(const UID &source,
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
 * Helper method to create a new queued request object
 */
const RDMRequest *DmxTriWidgetTest::NewQueuedMessageRequest(
    const UID &source,
    const UID &destination,
    uint8_t code) {
  return new ola::rdm::RDMGetRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // message count
      10,  // sub device
      ola::rdm::PID_QUEUED_MESSAGE,
      &code,
      sizeof(code));
}


/**
 * Run the sequence of commands to populate the TOD
 */
void DmxTriWidgetTest::PopulateTod(DmxTriWidget *widget) {
  uint8_t expected_discovery = 0x33;
  uint8_t discovery_ack[] = {0x33, 0x00};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      &expected_discovery,
      sizeof(expected_discovery),
      EXTENDED_LABEL,
      discovery_ack,
      sizeof(discovery_ack));

  uint8_t expected_stat = 0x34;
  uint8_t stat_ack[] = {0x34, 0x00, 0x02, 0x00};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      &expected_stat,
      sizeof(expected_stat),
      EXTENDED_LABEL,
      stat_ack,
      sizeof(stat_ack));

  uint8_t expected_fetch_uid1[] = {0x35, 0x02};
  uint8_t expected_fetch_response1[] = {
    0x35, 0x00,
    0x70, 0x7a, 0xff, 0xff, 0xff, 0x00};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_fetch_uid1,
      sizeof(expected_fetch_uid1),
      EXTENDED_LABEL,
      expected_fetch_response1,
      sizeof(expected_fetch_response1));

  uint8_t expected_fetch_uid2[] = {0x35, 0x01};
  uint8_t expected_fetch_response2[] = {
    0x35, 0x00,
    0x52, 0x52, 0x12, 0x34, 0x56, 0x78};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_fetch_uid2,
      sizeof(expected_fetch_uid2),
      EXTENDED_LABEL,
      expected_fetch_response2,
      sizeof(expected_fetch_response2));

  widget->SetUIDListCallback(
      ola::NewCallback(this, &DmxTriWidgetTest::ValidateTod));

  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, m_tod_counter);
  m_expect_uids_in_tod = true;
  widget->RunRDMDiscovery();
  m_ss.Run();
}


/**
 * Check that the discovery sequence works correctly.
 */
void DmxTriWidgetTest::testTod() {
  DmxTriWidget dmxtri(&m_ss, &m_widget);

  PopulateTod(&dmxtri);

  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, m_tod_counter);
  dmxtri.SendUIDUpdate();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, m_tod_counter);
  m_widget.Verify();

  // check that where there are no devices, things work
  // this also tests multiple stat commands
  uint8_t expected_discovery = 0x33;
  uint8_t discovery_ack[] = {0x33, 0x00};

  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      &expected_discovery,
      sizeof(expected_discovery),
      EXTENDED_LABEL,
      discovery_ack,
      sizeof(discovery_ack));

  uint8_t expected_stat = 0x34;
  uint8_t stat_in_progress_ack[] = {0x34, 0x00, 0x00, 0x01};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      &expected_stat,
      sizeof(expected_stat),
      EXTENDED_LABEL,
      stat_in_progress_ack,
      sizeof(stat_in_progress_ack));

  uint8_t stat_nil_devices_ack[] = {0x34, 0x00, 0x00, 0x00};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      &expected_stat,
      sizeof(expected_stat),
      EXTENDED_LABEL,
      stat_nil_devices_ack,
      sizeof(stat_nil_devices_ack));

  m_expect_uids_in_tod = false;
  dmxtri.RunRDMDiscovery();
  m_ss.Run();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 3, m_tod_counter);
  m_widget.Verify();

  // check that an error behaves like we expect
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      &expected_discovery,
      sizeof(expected_discovery),
      EXTENDED_LABEL,
      discovery_ack,
      sizeof(discovery_ack));

  uint8_t stat_error_ack[] = {0x34, 0x1b, 0x00, 0x00};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      &expected_stat,
      sizeof(expected_stat),
      EXTENDED_LABEL,
      stat_error_ack,
      sizeof(stat_error_ack));

  m_expect_uids_in_tod = false;
  dmxtri.RunRDMDiscovery();
  m_ss.Run();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4, m_tod_counter);
  m_widget.Verify();
}


/**
 * Check we can send DMX correctly
 */
void DmxTriWidgetTest::testDmx() {
  uint8_t DMX_LABEL = 0x06;
  ola::plugin::usbpro::DmxTriWidget dmxtri(&m_ss, &m_widget);
  ola::DmxBuffer buffer;
  buffer.SetFromString("0,18,147,204,255");

  uint8_t expected_dmx[] = {0x00, 0x00, 0x12, 0x93, 0xcc, 0xff};
  m_widget.AddExpectedCall(
      DMX_LABEL,
      expected_dmx,
      sizeof(expected_dmx));

  dmxtri.SendDMX(buffer);
  m_widget.Verify();
}


/**
 * Check that we send messages correctly.
 */
void DmxTriWidgetTest::testSendRDM() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);
  ola::plugin::usbpro::DmxTriWidget dmxtri(&m_ss, &m_widget);
  uint8_t param_data[] = {0xa1, 0xb2};

  const RDMRequest *request = NewRequest(
      source,
      destination,
      param_data,
      sizeof(param_data));

  // first of all confirm we can't send to a UID not in the TOD
  vector<string> packets;
  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_UNKNOWN_UID,
                             packets));

  // now populate the TOD
  PopulateTod(&dmxtri);

  request = NewRequest(
      source,
      destination,
      param_data,
      sizeof(param_data));

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28, 0xa1,
                                    0xb2};
  uint8_t command_response[] = {0x38, 0x00, 0x5a, 0xa5, 0x5a, 0xa5};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      command_response,
      sizeof(command_response));

  uint8_t return_data[] = {0x5a, 0xa5, 0x5a, 0xa5};

  ola::rdm::RDMGetResponse response(
      destination,
      source,
      0,  // transaction #
      1,  // port id
      0,  // message count
      10,  // sub device
      296,  // param id
      return_data,
      sizeof(return_data));  // data length

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateResponse,
                             ola::rdm::RDM_COMPLETED_OK,
                             static_cast<const RDMResponse*>(&response),
                             packets));
  m_widget.Verify();

  // confirm a queued message shows up in the counter
  request = NewRequest(
      source,
      destination,
      param_data,
      sizeof(param_data));

  uint8_t queued_command_response[] = {0x38, 0x11, 0x5a, 0xa5, 0x5a, 0xa5};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      queued_command_response,
      sizeof(queued_command_response));

  ola::rdm::RDMGetResponse response2(
      destination,
      source,
      0,  // transaction #
      ola::rdm::RDM_ACK,
      1,  // message count
      10,  // sub device
      296,  // param id
      return_data,
      sizeof(return_data));  // data length

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(
        this,
        &DmxTriWidgetTest::ValidateResponse,
        ola::rdm::RDM_COMPLETED_OK,
        static_cast<const RDMResponse*>(&response2),
        packets));
  m_widget.Verify();
}


/**
 * Check that various errors are handled correctly
 */
void DmxTriWidgetTest::testSendRDMErrors() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);
  ola::plugin::usbpro::DmxTriWidget dmxtri(&m_ss, &m_widget);

  // populate the TOD
  PopulateTod(&dmxtri);
  vector<string> packets;

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28};

  // confirm transaction mis-match works
  const RDMRequest *request = NewRequest(source, destination, NULL, 0);

  uint8_t transaction_mismatch_response[] = {0x38, 0x13};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      transaction_mismatch_response,
      sizeof(transaction_mismatch_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TRANSACTION_MISMATCH,
                             packets));
  m_widget.Verify();

  // confirm wrong sub device works
  request = NewRequest(source, destination, NULL, 0);

  uint8_t wrong_subdevice_response[] = {0x38, 0x14};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      wrong_subdevice_response,
      sizeof(wrong_subdevice_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_SUB_DEVICE_MISMATCH,
                             packets));
  m_widget.Verify();

  // confirm a invalid reponse behaves
  request = NewRequest(source, destination, NULL, 0);

  uint8_t invalid_response[] = {0x38, 0x15};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      invalid_response,
      sizeof(invalid_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_INVALID_RESPONSE,
                             packets));
  m_widget.Verify();

  // confirm a checksum mismatch fails
  request = NewRequest(source, destination, NULL, 0);

  uint8_t bad_checksum_response[] = {0x38, 0x16};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      bad_checksum_response,
      sizeof(bad_checksum_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_CHECKSUM_INCORRECT,
                             packets));
  m_widget.Verify();

  // confirm a timeout behaves
  request = NewRequest(source, destination, NULL, 0);

  uint8_t timeout_response[] = {0x38, 0x18};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      timeout_response,
      sizeof(timeout_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TIMEOUT,
                             packets));
  m_widget.Verify();

  // confirm a wrong device works
  request = NewRequest(source, destination, NULL, 0);

  uint8_t wrong_device_response[] = {0x38, 0x1a};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      wrong_device_response,
      sizeof(wrong_device_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_SRC_UID_MISMATCH,
                             packets));
  m_widget.Verify();
}


/*
 * Check that broadcast / vendorcast works
 */
void DmxTriWidgetTest::testSendRDMBroadcast() {
  UID source(1, 2);
  UID vendor_cast_destination(0x707a, 0xffffffff);
  UID bcast_destination(0xffff, 0xffffffff);
  ola::plugin::usbpro::DmxTriWidget dmxtri(&m_ss, &m_widget);

  vector<string> packets;
  PopulateTod(&dmxtri);

  const RDMRequest *request = NewRequest(
      source,
      vendor_cast_destination,
      NULL,
      0);

  uint8_t expected_set_filter[] = {0x3d, 0x70, 0x7a};
  uint8_t set_filter_response[] = {0x3d, 0x00};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_set_filter,
      sizeof(expected_set_filter),
      EXTENDED_LABEL,
      set_filter_response,
      sizeof(set_filter_response));

  uint8_t expected_rdm_command[] = {0x38, 0x00, 0x00, 0x0a, 0x01, 0x28};
  uint8_t command_response[] = {0x38, 0x00};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      command_response,
      sizeof(command_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST,
                             packets));
  m_widget.Verify();

  // check broad cast
  request = NewRequest(source, bcast_destination, NULL, 0);

  uint8_t expected_bcast_set_filter[] = {0x3d, 0xff, 0xff};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_bcast_set_filter,
      sizeof(expected_bcast_set_filter),
      EXTENDED_LABEL,
      set_filter_response,
      sizeof(set_filter_response));

  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      command_response,
      sizeof(command_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST,
                             packets));
  m_widget.Verify();

  // check that we don't call set filter if it's the same uid
  request = NewRequest(source, bcast_destination, NULL, 0);

  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      command_response,
      sizeof(command_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST,
                             packets));
  m_widget.Verify();

  // check that we fail correctly if set filter fails
  request = NewRequest(source, vendor_cast_destination, NULL, 0);

  uint8_t failed_set_filter_response[] = {0x3d, 0x02};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_set_filter,
      sizeof(expected_set_filter),
      EXTENDED_LABEL,
      failed_set_filter_response,
      sizeof(failed_set_filter_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_FAILED_TO_SEND,
                             packets));
  m_widget.Verify();
}


/*
 * Check that nacks work as expected.
 */
void DmxTriWidgetTest::testNack() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);
  ola::plugin::usbpro::DmxTriWidget dmxtri(&m_ss, &m_widget);

  vector<string> packets;
  PopulateTod(&dmxtri);

  const RDMRequest *request = NewRequest(source, destination, NULL, 0);

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28};
  uint8_t nack_pid_response[] = {0x38, 0x20};  // unknown pid
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      nack_pid_response,
      sizeof(nack_pid_response));

  RDMResponse *response = ola::rdm::NackWithReason(
      request,
      ola::rdm::NR_UNKNOWN_PID);

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateResponse,
                             ola::rdm::RDM_COMPLETED_OK,
                             static_cast<const RDMResponse*>(response),
                             packets));
  m_widget.Verify();
  delete response;

  // try a proxy buffer full
  request = NewRequest(source, destination, NULL, 0);

  uint8_t nack_proxy_response[] = {0x38, 0x2a};  // bad proxy
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      nack_proxy_response,
      sizeof(nack_proxy_response));

  response = ola::rdm::NackWithReason(
      request,
      ola::rdm::NR_PROXY_BUFFER_FULL);

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateResponse,
                             ola::rdm::RDM_COMPLETED_OK,
                             static_cast<const RDMResponse*>(response),
                             packets));
  m_widget.Verify();
  delete response;
}


/*
 * Check that ack timer works as expected.
 */
void DmxTriWidgetTest::testAckTimer() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);
  ola::plugin::usbpro::DmxTriWidget dmxtri(&m_ss, &m_widget);

  vector<string> packets;
  PopulateTod(&dmxtri);

  const RDMRequest *request = NewRequest(source, destination, NULL, 0);

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28};
  uint8_t ack_timer_response[] = {0x38, 0x10, 0x00, 0x10};  // ack timer, 1.6s
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      ack_timer_response,
      sizeof(ack_timer_response));

  uint8_t return_data[] = {0x00, 0x10};

  ola::rdm::RDMGetResponse response(
      destination,
      source,
      0,  // transaction #
      ola::rdm::RDM_ACK_TIMER,
      0,  // message count
      10,  // sub device
      296,  // param id
      return_data,
      sizeof(return_data));  // data length

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateResponse,
                             ola::rdm::RDM_COMPLETED_OK,
                             static_cast<const RDMResponse*>(&response),
                             packets));
  m_widget.Verify();
}


/*
 * Check that ack overflow works as expected.
 */
void DmxTriWidgetTest::testAckOverflow() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);
  ola::plugin::usbpro::DmxTriWidget dmxtri(&m_ss, &m_widget);

  vector<string> packets;
  PopulateTod(&dmxtri);

  const RDMRequest *request = NewRequest(source, destination, NULL, 0);

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28};
  uint8_t ack_overflow_response[] = {0x38, 0x12, 0x12, 0x34};  // ack overflow
  uint8_t ack_response[] = {0x38, 0x0, 0x56, 0x78};  // ack overflow
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      ack_overflow_response,
      sizeof(ack_overflow_response));

  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      ack_response,
      sizeof(ack_response));

  uint8_t return_data[] = {0x12, 0x34, 0x56, 0x78};

  ola::rdm::RDMGetResponse response(
      destination,
      source,
      0,  // transaction #
      ola::rdm::RDM_ACK,
      0,  // message count
      10,  // sub device
      296,  // param id
      return_data,
      sizeof(return_data));  // data length

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateResponse,
                             ola::rdm::RDM_COMPLETED_OK,
                             static_cast<const RDMResponse*>(&response),
                             packets));
  m_widget.Verify();
}


/*
 * Check that queued messages work.
 */
void DmxTriWidgetTest::testQueuedMessages() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);
  ola::plugin::usbpro::DmxTriWidget dmxtri(&m_ss, &m_widget);

  vector<string> packets;
  PopulateTod(&dmxtri);

  // first try a response which is too short
  const RDMRequest *request = NewQueuedMessageRequest(source, destination, 1);
  uint8_t expected_rdm_command[] = {0x3a, 0x02, 0x01};
  uint8_t small_response[] = {0x3a, 0x04};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      small_response,
      sizeof(small_response));

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateStatus,
                             ola::rdm::RDM_INVALID_RESPONSE,
                             packets));
  m_widget.Verify();

  // now try a proper response
  request = NewQueuedMessageRequest(source, destination, 1);
  uint8_t queued_response[] = {0x3a, 0x00, 0x00, 0x60, 0x52};
  m_widget.AddExpectedCall(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      queued_response,
      sizeof(queued_response));

  uint8_t return_data = 0x52;
  ola::rdm::RDMGetResponse response(
      destination,
      source,
      0,  // transaction #
      ola::rdm::RDM_ACK,
      0,  // message count
      10,  // sub device
      0x0060,  // param id
      &return_data,
      sizeof(return_data));  // data length

  dmxtri.SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxTriWidgetTest::ValidateResponse,
                             ola::rdm::RDM_COMPLETED_OK,
                             static_cast<const RDMResponse*>(&response),
                             packets));
  m_widget.Verify();
}
