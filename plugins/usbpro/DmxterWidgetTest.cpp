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
 * DmxterWidgetTest.cpp
 * Test fixture for the DmxterWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>
#include <vector>

#include "common/rdm/TestHelper.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMPacket.h"
#include "plugins/usbpro/DmxterWidget.h"
#include "plugins/usbpro/CommonWidgetTest.h"
#include "ola/testing/TestUtils.h"

using ola::plugin::usbpro::DmxterWidget;
using ola::rdm::GetResponseFromData;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMFrame;
using ola::rdm::RDMFrames;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;
using std::string;
using std::vector;

class DmxterWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(DmxterWidgetTest);
  CPPUNIT_TEST(testTod);
  CPPUNIT_TEST(testSendRDMRequest);
  CPPUNIT_TEST(testSendRDMMute);
  CPPUNIT_TEST(testSendRDMDUB);
  CPPUNIT_TEST(testErrorCodes);
  CPPUNIT_TEST(testErrorConditions);
  CPPUNIT_TEST(testShutdown);
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();

  void testTod();
  void testSendRDMRequest();
  void testSendRDMMute();
  void testSendRDMDUB();
  void testErrorCodes();
  void testErrorConditions();
  void testShutdown();

 private:
  auto_ptr<ola::plugin::usbpro::DmxterWidget> m_widget;
  unsigned int m_tod_counter;

  void Terminate() {
    m_ss.Terminate();
  }

  void CloseAndTerminate() {
    m_ss.RemoveReadDescriptor(&m_descriptor);
    m_ss.RemoveReadDescriptor(m_other_end.get());
    m_descriptor.Close();
    m_other_end->Close();
    m_ss.Terminate();
  }
  void ValidateTod(const ola::rdm::UIDSet &uids);
  void ValidateResponse(RDMReply *reply);
  void ValidateStatus(ola::rdm::RDMStatusCode expected_code,
                      RDMFrames expected_frames,
                      RDMReply *reply);
  RDMRequest *NewRequest(const UID &source,
                         const UID &destination,
                         const uint8_t *data,
                         unsigned int length);

  static const uint8_t TEST_RDM_DATA[];
};


const uint8_t DmxterWidgetTest::TEST_RDM_DATA[] = {0x5a, 0x5a, 0x5a, 0x5a};

CPPUNIT_TEST_SUITE_REGISTRATION(DmxterWidgetTest);


void DmxterWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  m_widget.reset(
      new ola::plugin::usbpro::DmxterWidget(&m_descriptor,
                                            0x4744,
                                            0x12345678));
  m_tod_counter = 0;
}


/**
 * Check the TOD matches what we expect
 */
void DmxterWidgetTest::ValidateTod(const ola::rdm::UIDSet &uids) {
  UID uid1(0x707a, 0xffffff00);
  UID uid2(0x5252, 0x12345678);
  OLA_ASSERT_EQ((unsigned int) 2, uids.Size());
  OLA_ASSERT(uids.Contains(uid1));
  OLA_ASSERT(uids.Contains(uid2));
  m_tod_counter++;
  m_ss.Terminate();
}


/**
 * Check the response matches what we expected.
 */
void DmxterWidgetTest::ValidateResponse(RDMReply *reply) {
  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, reply->StatusCode());
  OLA_ASSERT_NOT_NULL(reply->Response());

  const RDMResponse *response = reply->Response();

  uint8_t expected_data[] = {0x5a, 0x5a, 0x5a, 0x5a};
  OLA_ASSERT_DATA_EQUALS(expected_data, arraysize(expected_data),
                         response->ParamData(), response->ParamDataSize());

  const RDMFrames &frames = reply->Frames();
  OLA_ASSERT_EQ((size_t) 1, frames.size());

  ola::rdm::RDMStatusCode raw_code;
  auto_ptr<ola::rdm::RDMResponse> raw_response(
      ola::rdm::RDMResponse::InflateFromData(
          frames[0].data.substr(1), &raw_code));
  OLA_ASSERT_TRUE(*raw_response == *response);
  m_ss.Terminate();
}


/*
 * Check that we got an unknown UID code
 */
void DmxterWidgetTest::ValidateStatus(
    ola::rdm::RDMStatusCode expected_code,
    RDMFrames expected_frames,
    RDMReply *reply) {
  OLA_ASSERT_EQ(expected_code, reply->StatusCode());
  OLA_ASSERT_NULL(reply->Response());

  const RDMFrames &frames = reply->Frames();
  OLA_ASSERT_EQ(expected_frames.size(), frames.size());
  for (unsigned int i = 0; i < frames.size(); i++) {
    OLA_ASSERT_DATA_EQUALS(expected_frames[i].data.data(),
                           expected_frames[i].data.size(),
                           frames[i].data.data(),
                           frames[i].data.size());
    OLA_ASSERT_TRUE(expected_frames[i] == frames[i]);
  }
  m_ss.Terminate();
}


/**
 * Helper method to create new request objects
 */
RDMRequest *DmxterWidgetTest::NewRequest(const UID &source,
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
 * Check that discovery works for a device that just implements the serial #
 */
void DmxterWidgetTest::testTod() {
  uint8_t FULL_DISCOVERY_LABEL = 0x84;
  uint8_t INCREMENTAL_DISCOVERY_LABEL = 0x85;
  uint8_t TOD_LABEL = 0x82;
  uint8_t return_packet[] = {
    0x70, 0x7a, 0xff, 0xff, 0xff, 0x00,
    0x52, 0x52, 0x12, 0x34, 0x56, 0x78,
  };

  m_endpoint->AddExpectedUsbProDataAndReturn(
      FULL_DISCOVERY_LABEL,
      NULL,
      0,
      TOD_LABEL,
      return_packet,
      sizeof(return_packet));

  OLA_ASSERT_EQ((unsigned int) 0, m_tod_counter);
  m_widget->RunFullDiscovery(
      ola::NewSingleCallback(this, &DmxterWidgetTest::ValidateTod));
  m_ss.Run();
  m_endpoint->Verify();
  OLA_ASSERT_EQ((unsigned int) 1, m_tod_counter);

  m_endpoint->AddExpectedUsbProDataAndReturn(
      INCREMENTAL_DISCOVERY_LABEL,
      NULL,
      0,
      TOD_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->RunIncrementalDiscovery(
      ola::NewSingleCallback(this, &DmxterWidgetTest::ValidateTod));

  m_ss.Run();
  m_endpoint->Verify();
  OLA_ASSERT_EQ((unsigned int) 2, m_tod_counter);
}


/**
 * Check that we send messages correctly.
 */
void DmxterWidgetTest::testSendRDMRequest() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  uint8_t RDM_BROADCAST_REQUEST_LABEL = 0x81;
  UID source(0x4744, 0x12345678);
  UID destination(3, 4);
  UID bcast_destination(3, 0xffffffff);

  RDMRequest *request = NewRequest(source, destination, NULL, 0);

  unsigned int size = RDMCommandSerializer::RequiredSize(*request);
  uint8_t *expected_packet = new uint8_t[size + 1];
  expected_packet[0] = 0xcc;
  OLA_ASSERT(RDMCommandSerializer::Pack(*request, expected_packet + 1, &size));

  uint8_t return_packet[] = {
    0x00, 14,  // response code 'ok'
    0xcc,
    1, 28,  // sub code & length
    0x47, 0x44, 0x12, 0x34, 0x56, 0x78,   // dst uid
    0, 3, 0, 0, 0, 4,   // src uid
    0, 1, 0, 0, 10,  // transaction, port id, msg count & sub device
    0x21, 0x1, 0x28, 4,  // command, param id, param data length
    0x5a, 0x5a, 0x5a, 0x5a,  // param data
    0x04, 0x50  // checksum
  };

  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this, &DmxterWidgetTest::ValidateResponse));
  m_ss.Run();
  m_endpoint->Verify();

  // now check broadcast
  request = NewRequest(source, bcast_destination, NULL, 0);
  request->SetTransactionNumber(1);
  OLA_ASSERT(RDMCommandSerializer::Pack(*request, expected_packet + 1, &size));

  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_BROADCAST_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_BROADCAST_REQUEST_LABEL,
      static_cast<uint8_t*>(NULL),
      0);

  RDMFrames frames;
  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST,
                             frames));

  delete[] expected_packet;
  m_ss.Run();
  m_endpoint->Verify();
}


/**
 * Check that RDM Mute requests work
 */
void DmxterWidgetTest::testSendRDMMute() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  const UID source(0x4744, 0x12345678);
  const UID destination(3, 4);

  // request
  RDMRequest *rdm_request = new ola::rdm::RDMDiscoveryRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_DISC_MUTE,  // param id
      NULL,
      0);

  unsigned int request_size = RDMCommandSerializer::RequiredSize(*rdm_request);
  uint8_t *expected_request_frame = new uint8_t[request_size + 1];
  expected_request_frame[0] = 0xcc;
  OLA_ASSERT(RDMCommandSerializer::Pack(*rdm_request,
                                        expected_request_frame + 1,
                                        &request_size));

  // response
  // to keep things simple here we return the TEST_RDM_DATA.
  auto_ptr<const RDMResponse> response(
    GetResponseFromData(rdm_request, TEST_RDM_DATA, sizeof(TEST_RDM_DATA)));

  unsigned int response_size = RDMCommandSerializer::RequiredSize(
      *response.get());
  uint8_t *response_frame = new uint8_t[response_size + 3];
  response_frame[0] = 0;  // version
  response_frame[1] = 14;  // status ok
  response_frame[2] = ola::rdm::START_CODE;
  memset(&response_frame[3], 0, response_size);
  OLA_ASSERT(RDMCommandSerializer::Pack(*response, &response_frame[3],
                                        &response_size));
  response_size += 3;

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_request_frame,
      request_size + 1,
      RDM_REQUEST_LABEL,
      response_frame,
      response_size);

  m_widget->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this, &DmxterWidgetTest::ValidateResponse));
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_request_frame;
  delete[] response_frame;
}


/**
 * Check that we send RDM discovery messages correctly.
 */
void DmxterWidgetTest::testSendRDMDUB() {
  uint8_t RDM_DUB_LABEL = 0x83;
  const UID source(0x4744, 0x12345678);
  const UID destination = UID::AllDevices();

  static const uint8_t REQUEST_DATA[] = {
    0x7a, 0x70, 0, 0, 0, 0,
    0x7a, 0x70, 0xff, 0xff, 0xff, 0xff
  };

  // request
  RDMRequest *rdm_request = new ola::rdm::RDMDiscoveryRequest(
      source,
      destination,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_DISC_UNIQUE_BRANCH,  // param id
      REQUEST_DATA,
      sizeof(REQUEST_DATA));

  unsigned int request_size = RDMCommandSerializer::RequiredSize(*rdm_request);
  uint8_t *expected_request_frame = new uint8_t[request_size + 1];
  expected_request_frame[0] = 0xcc;
  OLA_ASSERT(RDMCommandSerializer::Pack(*rdm_request,
                                        expected_request_frame + 1,
                                        &request_size));

  // a 4 byte response means a timeout
  static const uint8_t TIMEOUT_RESPONSE[] = {0, 17};

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_DUB_LABEL,
      expected_request_frame,
      request_size + 1,
      RDM_DUB_LABEL,
      TIMEOUT_RESPONSE,
      sizeof(TIMEOUT_RESPONSE));

  RDMFrames frames;
  m_widget->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TIMEOUT,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_request_frame;

  // now try a dub response that returns something
  rdm_request = new ola::rdm::RDMDiscoveryRequest(
      source,
      destination,
      1,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_DISC_UNIQUE_BRANCH,  // param id
      REQUEST_DATA,
      sizeof(REQUEST_DATA));

  request_size = RDMCommandSerializer::RequiredSize(*rdm_request);
  expected_request_frame = new uint8_t[request_size + 1];
  expected_request_frame[0] = 0xcc;
  OLA_ASSERT(RDMCommandSerializer::Pack(*rdm_request,
                                        expected_request_frame + 1,
                                        &request_size));

  // something that looks like a DUB response
  static const uint8_t FAKE_RESPONSE[] = {0x00, 19, 0xfe, 0xfe, 0xaa, 0xaa};

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_DUB_LABEL,
      expected_request_frame,
      request_size + 1,
      RDM_DUB_LABEL,
      FAKE_RESPONSE,
      sizeof(FAKE_RESPONSE));

  frames.push_back(RDMFrame(&FAKE_RESPONSE[2], arraysize(FAKE_RESPONSE) - 2));
  m_widget->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_DUB_RESPONSE,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_request_frame;
}


/**
 * Check that we handle invalid responses ok
 */
void DmxterWidgetTest::testErrorCodes() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  UID source(0x4744, 0x12345678);
  UID destination(3, 4);

  RDMFrames frames;

  RDMRequest *request = NewRequest(source, destination, NULL, 0);

  unsigned int size = RDMCommandSerializer::RequiredSize(*request);
  uint8_t *expected_packet = new uint8_t[size + 1];
  expected_packet[0] = 0xcc;
  OLA_ASSERT(RDMCommandSerializer::Pack(*request, expected_packet + 1, &size));

  uint8_t return_packet[] = {
    0x00, 1,  // checksum failure
  };

  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_CHECKSUM_INCORRECT,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  return_packet[1] = 8;  // packet too short
  // Update TN & Checksum
  expected_packet[15]++;
  expected_packet[25]++;
  request = NewRequest(source, destination, NULL, 0);
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_PACKET_TOO_SHORT,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  // update transaction # & checksum
  expected_packet[15]++;
  expected_packet[25] = 0xe1;
  return_packet[1] = 12;  // transaction mismatch
  request = NewRequest(source, destination, NULL, 0);
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TRANSACTION_MISMATCH,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  // update transaction # & checksum
  expected_packet[15]++;
  expected_packet[25] = 0xe2;
  return_packet[1] = 17;  // timeout
  request = NewRequest(source, destination, NULL, 0);
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TIMEOUT,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  // update transaction # & checksum
  expected_packet[15]++;
  expected_packet[25] = 0xe3;
  return_packet[1] = 41;  // device mismatch
  request = NewRequest(source, destination, NULL, 0);
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_SRC_UID_MISMATCH,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  // update transaction # & checksum
  expected_packet[15]++;
  expected_packet[25] = 0xe4;
  return_packet[1] = 42;  // sub device mismatch
  request = NewRequest(source, destination, NULL, 0);
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_SUB_DEVICE_MISMATCH,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_packet;
}


/*
 * Check some of the error conditions
 */
void DmxterWidgetTest::testErrorConditions() {
  uint8_t RDM_REQUEST_LABEL = 0x80;
  UID source(0x4744, 0x12345678);
  UID destination(3, 4);
  RDMFrames frames;

  RDMRequest *request = NewRequest(source, destination, NULL, 0);

  unsigned int size = RDMCommandSerializer::RequiredSize(*request);
  uint8_t *expected_packet = new uint8_t[size + 1];
  expected_packet[0] = 0xcc;
  OLA_ASSERT(RDMCommandSerializer::Pack(*request, expected_packet + 1, &size));

  // to small to be valid
  uint8_t return_packet[] = {0x00};

  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet,
      sizeof(return_packet));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_INVALID_RESPONSE,
                             frames));

  m_ss.Run();
  m_endpoint->Verify();

  // check mismatched version
  request = NewRequest(source, destination, NULL, 0);
  request->SetTransactionNumber(1);

  OLA_ASSERT(RDMCommandSerializer::Pack(*request, expected_packet + 1, &size));

  // non-0 version
  uint8_t return_packet2[] = {0x01, 0x11, 0xcc};

  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_REQUEST_LABEL,
      expected_packet,
      size + 1,
      RDM_REQUEST_LABEL,
      return_packet2,
      sizeof(return_packet2));

  m_widget->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &DmxterWidgetTest::ValidateStatus,
                             ola::rdm::RDM_INVALID_RESPONSE,
                             frames));

  delete[] expected_packet;
  m_ss.Run();
  m_endpoint->Verify();
}


/*
 * Check that the shutdown message works
 */
void DmxterWidgetTest::testShutdown() {
  uint8_t SHUTDOWN_LABEL = 0xf0;

  m_descriptor.SetOnClose(
      ola::NewSingleCallback(this, &DmxterWidgetTest::Terminate));
  OLA_ASSERT(m_descriptor.ValidReadDescriptor());
  OLA_ASSERT(m_descriptor.ValidWriteDescriptor());

  // first try a bad message
  uint8_t data = 1;
  m_endpoint->SendUnsolicitedUsbProData(SHUTDOWN_LABEL, &data, 1);
  // an invalid message doesn't generate a callback so we need to set a timer
  // here.
  m_ss.RegisterSingleTimeout(
      30,  // 30ms should be enough
      ola::NewSingleCallback(this, &DmxterWidgetTest::Terminate));
  m_ss.Run();
  m_endpoint->Verify();
  OLA_ASSERT(m_descriptor.ValidReadDescriptor());
  OLA_ASSERT(m_descriptor.ValidWriteDescriptor());

  m_descriptor.SetOnClose(
      ola::NewSingleCallback(this, &DmxterWidgetTest::CloseAndTerminate));

  // now send a valid shutdown message
  m_endpoint->SendUnsolicitedUsbProData(SHUTDOWN_LABEL, NULL, 0);
  m_ss.Run();
  m_endpoint->Verify();
  OLA_ASSERT_FALSE(m_descriptor.ValidReadDescriptor());
  OLA_ASSERT_FALSE(m_descriptor.ValidWriteDescriptor());
}
