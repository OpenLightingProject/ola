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
 * DmxTriWidgetTest.cpp
 * Test fixture for the DmxTriWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>
#include <vector>

#include "common/rdm/TestHelper.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/testing/TestUtils.h"
#include "plugins/usbpro/CommonWidgetTest.h"
#include "plugins/usbpro/DmxTriWidget.h"


using ola::DmxBuffer;
using ola::NewSingleCallback;
using ola::plugin::usbpro::DmxTriWidget;
using ola::rdm::NewDiscoveryUniqueBranchRequest;
using ola::rdm::RDMDiscoveryRequest;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;
using std::string;
using std::vector;


class DmxTriWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(DmxTriWidgetTest);
  CPPUNIT_TEST(testTod);
  CPPUNIT_TEST(testTodFailure);
  CPPUNIT_TEST(testLockedTod);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST(testSendRDM);
  CPPUNIT_TEST(testSendRDMErrors);
  CPPUNIT_TEST(testSendRDMBroadcast);
  CPPUNIT_TEST(testRawDiscovery);
  CPPUNIT_TEST(testNack);
  CPPUNIT_TEST(testAckTimer);
  CPPUNIT_TEST(testAckOverflow);
  CPPUNIT_TEST(testQueuedMessages);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();

    void testTod();
    void testTodFailure();
    void testLockedTod();
    void testSendDMX();
    void testSendRDM();
    void testSendRDMErrors();
    void testSendRDMBroadcast();
    void testRawDiscovery();
    void testNack();
    void testAckTimer();
    void testAckOverflow();
    void testQueuedMessages();

 private:
    auto_ptr<ola::plugin::usbpro::DmxTriWidget> m_widget;
    unsigned int m_tod_counter;
    bool m_expect_uids_in_tod;

    void AckSingleTX();
    void AckSingleTxAndTerminate();
    void AckSingleTxAndExpectData();
    void PopulateTod();
    void ValidateTod(const ola::rdm::UIDSet &uids);
    void ValidateResponse(ola::rdm::RDMStatusCode expected_code,
                          const RDMResponse *expected_response,
                          RDMReply *reply);
    void ValidateStatus(ola::rdm::RDMStatusCode expected_code,
                        RDMReply *reply);
    RDMRequest *NewRequest(const UID &source,
                           const UID &destination,
                           const uint8_t *data,
                           unsigned int length);
    RDMRequest *NewQueuedMessageRequest(const UID &source,
                                        const UID &destination,
                                        uint8_t code);

    static const uint8_t EXTENDED_LABEL = 0x58;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmxTriWidgetTest);


void DmxTriWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  m_widget.reset(
      new ola::plugin::usbpro::DmxTriWidget(&m_ss, &m_descriptor));
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
    OLA_ASSERT_EQ((unsigned int) 2, uids.Size());
    OLA_ASSERT(uids.Contains(uid1));
    OLA_ASSERT(uids.Contains(uid2));
  } else {
    OLA_ASSERT_EQ((unsigned int) 0, uids.Size());
  }
  m_tod_counter++;
  m_ss.Terminate();
}


/**
 * Check the response matches what we expected.
 */
void DmxTriWidgetTest::ValidateResponse(
    ola::rdm::RDMStatusCode expected_code,
    const RDMResponse *expected_response,
    RDMReply *reply) {
  OLA_ASSERT_EQ(expected_code, reply->StatusCode());
  OLA_ASSERT(reply->Response());
  OLA_ASSERT_TRUE(*expected_response == *reply->Response());

  // the TRIs can't return the actual packets
  OLA_ASSERT_EMPTY(reply->Frames());
  m_ss.Terminate();
}


/*
 * Check that we received the correct status code.
 */
void DmxTriWidgetTest::ValidateStatus(
    ola::rdm::RDMStatusCode expected_code,
    RDMReply *reply) {
  OLA_ASSERT_EQ(expected_code, reply->StatusCode());
  OLA_ASSERT_NULL(reply->Response());

  // the TRIs can't return the actual packets
  OLA_ASSERT_EMPTY(reply->Frames());
  m_ss.Terminate();
}


/**
 * Helper method to create new request objects
 */
RDMRequest *DmxTriWidgetTest::NewRequest(const UID &source,
                                         const UID &destination,
                                         const uint8_t *data,
                                         unsigned int length) {
  return new ola::rdm::RDMGetRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      10,  // sub device
      296,  // param id
      data,
      length);
}


/**
 * Helper method to create a new queued request object
 */
RDMRequest *DmxTriWidgetTest::NewQueuedMessageRequest(
    const UID &source,
    const UID &destination,
    uint8_t code) {
  return new ola::rdm::RDMGetRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      10,  // sub device
      ola::rdm::PID_QUEUED_MESSAGE,
      &code,
      sizeof(code));
}

void DmxTriWidgetTest::AckSingleTX() {
  uint8_t ack_response[] = {0x21, 0};
  m_endpoint->SendUnsolicitedUsbProData(
      EXTENDED_LABEL,
      ack_response,
      sizeof(ack_response));
}


void DmxTriWidgetTest::AckSingleTxAndTerminate() {
  AckSingleTX();
  m_ss.Terminate();
}


void DmxTriWidgetTest::AckSingleTxAndExpectData() {
  AckSingleTX();
  uint8_t expected_dmx_command[] = {0x21, 0x00, 0x00, 3, 2, 3, 45};
  m_endpoint->AddExpectedUsbProMessage(
      EXTENDED_LABEL,
      expected_dmx_command,
      sizeof(expected_dmx_command),
      NewSingleCallback(this, &DmxTriWidgetTest::AckSingleTxAndTerminate));
}


/**
 * Run the sequence of commands to populate the TOD
 */
void DmxTriWidgetTest::PopulateTod() {
  uint8_t expected_discovery = 0x33;
  uint8_t discovery_ack[] = {0x33, 0x00};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      &expected_discovery,
      sizeof(expected_discovery),
      EXTENDED_LABEL,
      discovery_ack,
      sizeof(discovery_ack));

  uint8_t expected_stat = 0x34;
  uint8_t stat_ack[] = {0x34, 0x00, 0x02, 0x00};
  m_endpoint->AddExpectedUsbProDataAndReturn(
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
  m_endpoint->AddExpectedUsbProDataAndReturn(
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
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_fetch_uid2,
      sizeof(expected_fetch_uid2),
      EXTENDED_LABEL,
      expected_fetch_response2,
      sizeof(expected_fetch_response2));

  OLA_ASSERT_EQ((unsigned int) 0, m_tod_counter);
  m_expect_uids_in_tod = true;
  m_widget->RunFullDiscovery(
      NewSingleCallback(this, &DmxTriWidgetTest::ValidateTod));
  m_ss.Run();
  m_endpoint->Verify();
}


/**
 * Check that the discovery sequence works correctly.
 */
void DmxTriWidgetTest::testTod() {
  PopulateTod();
  OLA_ASSERT_EQ((unsigned int) 1, m_tod_counter);
  m_endpoint->Verify();

  // check that where there are no devices, things work
  // this also tests multiple stat commands
  uint8_t expected_discovery = 0x33;
  uint8_t discovery_ack[] = {0x33, 0x00};

  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      &expected_discovery,
      sizeof(expected_discovery),
      EXTENDED_LABEL,
      discovery_ack,
      sizeof(discovery_ack));

  uint8_t expected_stat = 0x34;
  uint8_t stat_in_progress_ack[] = {0x34, 0x00, 0x00, 0x01};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      &expected_stat,
      sizeof(expected_stat),
      EXTENDED_LABEL,
      stat_in_progress_ack,
      sizeof(stat_in_progress_ack));

  uint8_t stat_nil_devices_ack[] = {0x34, 0x00, 0x00, 0x00};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      &expected_stat,
      sizeof(expected_stat),
      EXTENDED_LABEL,
      stat_nil_devices_ack,
      sizeof(stat_nil_devices_ack));

  m_expect_uids_in_tod = false;
  m_widget->RunFullDiscovery(
      NewSingleCallback(this, &DmxTriWidgetTest::ValidateTod));
  m_ss.Run();
  OLA_ASSERT_EQ((unsigned int) 2, m_tod_counter);
  m_endpoint->Verify();

  // check that an error behaves like we expect
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      &expected_discovery,
      sizeof(expected_discovery),
      EXTENDED_LABEL,
      discovery_ack,
      sizeof(discovery_ack));

  uint8_t stat_error_ack[] = {0x34, 0x1b, 0x00, 0x00};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      &expected_stat,
      sizeof(expected_stat),
      EXTENDED_LABEL,
      stat_error_ack,
      sizeof(stat_error_ack));

  m_expect_uids_in_tod = false;
  m_widget->RunFullDiscovery(
      NewSingleCallback(this, &DmxTriWidgetTest::ValidateTod));
  m_ss.Run();
  OLA_ASSERT_EQ((unsigned int) 3, m_tod_counter);
  m_endpoint->Verify();
}

/**
 * Check what happens if a DiscoAuto command fails.
 */
void DmxTriWidgetTest::testTodFailure() {
  PopulateTod();
  OLA_ASSERT_EQ((unsigned int) 1, m_tod_counter);
  m_endpoint->Verify();

  m_descriptor.Close();
  m_other_end->Close();
  // Failures cause the last Tod to be returned.
  m_expect_uids_in_tod = true;
  m_widget->RunFullDiscovery(
      NewSingleCallback(this, &DmxTriWidgetTest::ValidateTod));
  OLA_ASSERT_EQ((unsigned int) 2, m_tod_counter);
  m_endpoint->Verify();
}


/**
 * Check that the discovery works when the widget doesn't support RDM
 */
void DmxTriWidgetTest::testLockedTod() {
  uint8_t expected_discovery = 0x33;
  uint8_t discovery_ack[] = {0x33, 0x02};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      &expected_discovery,
      sizeof(expected_discovery),
      EXTENDED_LABEL,
      discovery_ack,
      sizeof(discovery_ack));

  m_expect_uids_in_tod = false;
  m_widget->RunFullDiscovery(
      NewSingleCallback(this, &DmxTriWidgetTest::ValidateTod));
  m_ss.Run();
  m_endpoint->Verify();
}


/**
 * Check that we send DMX correctly.
 */
void DmxTriWidgetTest::testSendDMX() {
  DmxBuffer data, data2, data3;
  data.SetFromString("1,2,3,45");
  data2.SetFromString("2,2,3,45");
  data3.SetFromString("3,2,3,45");

  m_widget->SendDMX(data);
  m_widget->SendDMX(data2);
  m_widget->SendDMX(data3);

  uint8_t expected_dmx_command1[] = {0x21, 0x0, 0x00, 1, 2, 3, 45};
  m_endpoint->AddExpectedUsbProMessage(
      EXTENDED_LABEL,
      expected_dmx_command1,
      sizeof(expected_dmx_command1),
      NewSingleCallback(this, &DmxTriWidgetTest::AckSingleTxAndExpectData));

  m_ss.Run();
  // The ss may terminate before the widget has a chance to read from the
  // descriptor. Run the ss once more to catch this case.
  m_ss.RunOnce(ola::TimeInterval(1, 0));
  m_endpoint->Verify();
}


/**
 * Check that we send RDM messages correctly.
 */
void DmxTriWidgetTest::testSendRDM() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);
  uint8_t param_data[] = {0xa1, 0xb2};

  RDMRequest *request = NewRequest(
      source,
      destination,
      param_data,
      sizeof(param_data));

  // first of all confirm we can't send to a UID not in the TOD
  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_UNKNOWN_UID));

  // now populate the TOD
  PopulateTod();

  request = NewRequest(
      source,
      destination,
      param_data,
      sizeof(param_data));

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28, 0xa1,
                                    0xb2};
  uint8_t command_response[] = {0x38, 0x00, 0x5a, 0xa5, 0x5a, 0xa5};
  m_endpoint->AddExpectedUsbProDataAndReturn(
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

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateResponse,
                        ola::rdm::RDM_COMPLETED_OK,
                        static_cast<const RDMResponse*>(&response)));
  m_ss.Run();
  m_endpoint->Verify();

  // confirm a queued message shows up in the counter
  request = NewRequest(
      source,
      destination,
      param_data,
      sizeof(param_data));

  uint8_t queued_command_response[] = {0x38, 0x11, 0x5a, 0xa5, 0x5a, 0xa5};
  m_endpoint->AddExpectedUsbProDataAndReturn(
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

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(
        this,
        &DmxTriWidgetTest::ValidateResponse,
        ola::rdm::RDM_COMPLETED_OK,
        static_cast<const RDMResponse*>(&response2)));
  m_ss.Run();
  m_endpoint->Verify();
}

/**
 * Check that various errors are handled correctly
 */
void DmxTriWidgetTest::testSendRDMErrors() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);

  // populate the TOD
  PopulateTod();

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28};

  // confirm transaction mismatch works
  RDMRequest *request = NewRequest(source, destination, NULL, 0);

  uint8_t transaction_mismatch_response[] = {0x38, 0x13};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      transaction_mismatch_response,
      sizeof(transaction_mismatch_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_TRANSACTION_MISMATCH));
  m_ss.Run();
  m_endpoint->Verify();

  // confirm wrong sub device works
  request = NewRequest(source, destination, NULL, 0);

  uint8_t wrong_subdevice_response[] = {0x38, 0x14};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      wrong_subdevice_response,
      sizeof(wrong_subdevice_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_SUB_DEVICE_MISMATCH));
  m_ss.Run();
  m_endpoint->Verify();

  // confirm a invalid response behaves
  request = NewRequest(source, destination, NULL, 0);

  uint8_t invalid_response[] = {0x38, 0x15};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      invalid_response,
      sizeof(invalid_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_INVALID_RESPONSE));
  m_ss.Run();
  m_endpoint->Verify();

  // confirm a checksum mismatch fails
  request = NewRequest(source, destination, NULL, 0);

  uint8_t bad_checksum_response[] = {0x38, 0x16};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      bad_checksum_response,
      sizeof(bad_checksum_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_CHECKSUM_INCORRECT));
  m_ss.Run();
  m_endpoint->Verify();

  // confirm a timeout behaves
  request = NewRequest(source, destination, NULL, 0);

  uint8_t timeout_response[] = {0x38, 0x18};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      timeout_response,
      sizeof(timeout_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_TIMEOUT));
  m_ss.Run();
  m_endpoint->Verify();

  // confirm a wrong device works
  request = NewRequest(source, destination, NULL, 0);

  uint8_t wrong_device_response[] = {0x38, 0x1a};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      wrong_device_response,
      sizeof(wrong_device_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_SRC_UID_MISMATCH));
  m_ss.Run();
  m_endpoint->Verify();
}

/*
 * Check that broadcast / vendorcast works
 */
void DmxTriWidgetTest::testSendRDMBroadcast() {
  UID source(1, 2);
  UID vendor_cast_destination(0x707a, 0xffffffff);
  UID bcast_destination(0xffff, 0xffffffff);

  PopulateTod();

  RDMRequest *request = NewRequest(
      source,
      vendor_cast_destination,
      NULL,
      0);

  uint8_t expected_set_filter[] = {0x3d, 0x70, 0x7a};
  uint8_t set_filter_response[] = {0x3d, 0x00};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_set_filter,
      sizeof(expected_set_filter),
      EXTENDED_LABEL,
      set_filter_response,
      sizeof(set_filter_response));

  uint8_t expected_rdm_command[] = {0x38, 0x00, 0x00, 0x0a, 0x01, 0x28};
  uint8_t command_response[] = {0x38, 0x00};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      command_response,
      sizeof(command_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_WAS_BROADCAST));
  m_ss.Run();
  m_endpoint->Verify();

  // check broad cast
  request = NewRequest(source, bcast_destination, NULL, 0);

  uint8_t expected_bcast_set_filter[] = {0x3d, 0xff, 0xff};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_bcast_set_filter,
      sizeof(expected_bcast_set_filter),
      EXTENDED_LABEL,
      set_filter_response,
      sizeof(set_filter_response));

  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      command_response,
      sizeof(command_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_WAS_BROADCAST));
  m_ss.Run();
  m_endpoint->Verify();

  // check that we don't call set filter if it's the same uid
  request = NewRequest(source, bcast_destination, NULL, 0);

  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      command_response,
      sizeof(command_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_WAS_BROADCAST));
  m_ss.Run();
  m_endpoint->Verify();

  // check that we fail correctly if set filter fails
  request = NewRequest(source, vendor_cast_destination, NULL, 0);

  uint8_t failed_set_filter_response[] = {0x3d, 0x02};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_set_filter,
      sizeof(expected_set_filter),
      EXTENDED_LABEL,
      failed_set_filter_response,
      sizeof(failed_set_filter_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_FAILED_TO_SEND));
  m_ss.Run();
  m_endpoint->Verify();
}


/*
 * Check that raw discovery commands work.
 */
void DmxTriWidgetTest::testRawDiscovery() {
  UID source(1, 2);
  UID lower(0, 0);
  UID upper(0xffff, 0xfffffff);

  PopulateTod();

  RDMDiscoveryRequest *dub_request = NewDiscoveryUniqueBranchRequest(
      source, lower, upper, 1);

  // Verify we can't send raw commands with the non-raw widget.
  m_widget->SendRDMRequest(
      dub_request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_PLUGIN_DISCOVERY_NOT_SUPPORTED));
  m_endpoint->Verify();
}


/*
 * Check that nacks work as expected.
 */
void DmxTriWidgetTest::testNack() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);

  PopulateTod();

  RDMRequest *request = NewRequest(source, destination, NULL, 0);

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28};
  uint8_t nack_pid_response[] = {0x38, 0x20};  // unknown pid
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      nack_pid_response,
      sizeof(nack_pid_response));

  RDMResponse *response = ola::rdm::NackWithReason(
      request,
      ola::rdm::NR_UNKNOWN_PID);

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateResponse,
                        ola::rdm::RDM_COMPLETED_OK,
                        static_cast<const RDMResponse*>(response)));
  m_ss.Run();
  m_endpoint->Verify();
  delete response;

  // try a proxy buffer full
  request = NewRequest(source, destination, NULL, 0);

  uint8_t nack_proxy_response[] = {0x38, 0x2a};  // bad proxy
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      nack_proxy_response,
      sizeof(nack_proxy_response));

  response = ola::rdm::NackWithReason(
      request,
      ola::rdm::NR_PROXY_BUFFER_FULL);

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateResponse,
                        ola::rdm::RDM_COMPLETED_OK,
                        static_cast<const RDMResponse*>(response)));
  m_ss.Run();
  m_endpoint->Verify();
  delete response;
}


/*
 * Check that ack timer works as expected.
 */
void DmxTriWidgetTest::testAckTimer() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);

  PopulateTod();

  RDMRequest *request = NewRequest(source, destination, NULL, 0);

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28};
  uint8_t ack_timer_response[] = {0x38, 0x10, 0x00, 0x10};  // ack timer, 1.6s
  m_endpoint->AddExpectedUsbProDataAndReturn(
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

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateResponse,
                        ola::rdm::RDM_COMPLETED_OK,
                        static_cast<const RDMResponse*>(&response)));
  m_ss.Run();
  m_endpoint->Verify();
}


/*
 * Check that ack overflow works as expected.
 */
void DmxTriWidgetTest::testAckOverflow() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);

  PopulateTod();

  RDMRequest *request = NewRequest(source, destination, NULL, 0);

  uint8_t expected_rdm_command[] = {0x38, 0x02, 0x00, 0x0a, 0x01, 0x28};
  uint8_t ack_overflow_response[] = {0x38, 0x12, 0x12, 0x34};  // ack overflow
  uint8_t ack_response[] = {0x38, 0x0, 0x56, 0x78};  // ack overflow
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      ack_overflow_response,
      sizeof(ack_overflow_response));

  m_endpoint->AddExpectedUsbProDataAndReturn(
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

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateResponse,
                        ola::rdm::RDM_COMPLETED_OK,
                        static_cast<const RDMResponse*>(&response)));
  m_ss.Run();
  m_endpoint->Verify();
}

/*
 * Check that queued messages work.
 */
void DmxTriWidgetTest::testQueuedMessages() {
  UID source(1, 2);
  UID destination(0x707a, 0xffffff00);
  PopulateTod();

  // first try a response which is too short
  RDMRequest *request = NewQueuedMessageRequest(source, destination, 1);
  uint8_t expected_rdm_command[] = {0x3a, 0x02, 0x01};
  uint8_t small_response[] = {0x3a, 0x04};
  m_endpoint->AddExpectedUsbProDataAndReturn(
      EXTENDED_LABEL,
      expected_rdm_command,
      sizeof(expected_rdm_command),
      EXTENDED_LABEL,
      small_response,
      sizeof(small_response));

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateStatus,
                        ola::rdm::RDM_INVALID_RESPONSE));
  m_ss.Run();
  m_endpoint->Verify();

  // now try a proper response
  request = NewQueuedMessageRequest(source, destination, 1);
  uint8_t queued_response[] = {0x3a, 0x00, 0x00, 0x60, 0x52};
  m_endpoint->AddExpectedUsbProDataAndReturn(
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

  m_widget->SendRDMRequest(
      request,
      NewSingleCallback(this,
                        &DmxTriWidgetTest::ValidateResponse,
                        ola::rdm::RDM_COMPLETED_OK,
                        static_cast<const RDMResponse*>(&response)));
  m_ss.Run();
  m_endpoint->Verify();
}
