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
 * EnttecUsbProWidgetTest.cpp
 * Test fixture for the EnttecUsbProWidget class
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
#include "ola/base/Array.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"
#include "plugins/usbpro/CommonWidgetTest.h"
#include "plugins/usbpro/EnttecUsbProWidget.h"
#include "plugins/usbpro/EnttecUsbProWidgetImpl.h"


using ola::plugin::usbpro::EnttecPort;
using ola::plugin::usbpro::EnttecUsbProWidget;
using ola::rdm::GetResponseFromData;
using ola::rdm::RDMCommandSerializer;
using ola::rdm::RDMDiscoveryRequest;
using ola::rdm::RDMFrame;
using ola::rdm::RDMFrames;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;

using std::auto_ptr;
using std::string;
using std::vector;
using ola::plugin::usbpro::usb_pro_parameters;


class EnttecUsbProWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(EnttecUsbProWidgetTest);
  CPPUNIT_TEST(testParams);
  CPPUNIT_TEST(testReceiveDMX);
  CPPUNIT_TEST(testChangeMode);
  CPPUNIT_TEST(testSendRDMRequest);
  CPPUNIT_TEST(testSendRDMMute);
  CPPUNIT_TEST(testSendRDMDUB);
  CPPUNIT_TEST(testMuteDevice);
  CPPUNIT_TEST(testUnMuteAll);
  CPPUNIT_TEST(testBranch);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();

    void testParams();
    void testReceiveDMX();
    void testChangeMode();
    void testSendRDMRequest();
    void testSendRDMMute();
    void testSendRDMDUB();
    void testMuteDevice();
    void testUnMuteAll();
    void testBranch();

 private:
    auto_ptr<EnttecUsbProWidget> m_widget;
    uint8_t m_transaction_number;
    ola::rdm::RDMStatusCode m_received_code;

    RDMRequest *NewRequest(const UID &destination,
                           const uint8_t *data = NULL,
                           unsigned int length = 0);
    uint8_t *PackRDMRequest(const RDMRequest *request, unsigned int *size);
    uint8_t *PackRDMResponse(const RDMResponse *response, unsigned int *size);
    void ValidateResponse(RDMReply *reply);
    void ValidateStatus(ola::rdm::RDMStatusCode expected_code,
                        RDMFrames expected_frames,
                        RDMReply *reply);
    void ValidateMuteStatus(bool expected,
                            bool actual);
    void ValidateBranchStatus(const uint8_t *expected_data,
                              unsigned int length,
                              const uint8_t *actual_data,
                              unsigned int actual_length);
    void SendResponseAndTimeout(const uint8_t *response_data,
                                unsigned int length);

    void Terminate() { m_ss.Terminate(); }
    void ValidateParams(bool status, const usb_pro_parameters &params);
    void ValidateDMX(EnttecPort *port,
                     const ola::DmxBuffer *expected_buffer);

    bool m_got_dmx;

    static const UID BCAST_DESTINATION;
    static const UID DESTINATION;
    static const UID SOURCE;
    static const uint16_t ESTA_ID = 0x00a1;
    static const uint32_t SERIAL_NUMBER = 0x01020304;
    static const uint8_t CHANGE_MODE_LABEL = 8;
    static const uint8_t CHANGE_OF_STATE_LABEL = 9;
    static const uint8_t GET_PARAM_LABEL = 3;
    static const uint8_t RDM_DISCOVERY_PACKET = 11;
    static const uint8_t RDM_PACKET = 7;
    static const uint8_t RDM_TIMEOUT_PACKET = 12;
    static const uint8_t RECEIVE_DMX_LABEL = 5;
    static const uint8_t SET_PARAM_LABEL = 4;
    static const uint8_t TEST_RDM_DATA[];
    static const unsigned int FOOTER_SIZE = 1;
    static const unsigned int HEADER_SIZE = 4;
};

const UID EnttecUsbProWidgetTest::DESTINATION(ESTA_ID, SERIAL_NUMBER);
const UID EnttecUsbProWidgetTest::BCAST_DESTINATION(ESTA_ID, 0xffffffff);
const UID EnttecUsbProWidgetTest::SOURCE(EnttecUsbProWidget::ENTTEC_ESTA_ID,
                                         1);
const uint8_t EnttecUsbProWidgetTest::TEST_RDM_DATA[] =
  { 0x5a, 0x5a, 0x5a, 0x5a};

CPPUNIT_TEST_SUITE_REGISTRATION(EnttecUsbProWidgetTest);


void EnttecUsbProWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  EnttecUsbProWidget::EnttecUsbProWidgetOptions options(
      EnttecUsbProWidget::ENTTEC_ESTA_ID, 1);
  options.enable_rdm = true;
  m_widget.reset(new EnttecUsbProWidget(&m_ss, &m_descriptor, options));

  m_transaction_number = 0;
  m_got_dmx = false;
}


/*
 * Helper method to create new GetRDMRequest objects.
 * @param destination the destination UID
 * @param data the RDM Request data
 * @param length the size of the RDM data.
 */
RDMRequest *EnttecUsbProWidgetTest::NewRequest(const UID &destination,
                                               const uint8_t *data,
                                               unsigned int length) {
  return new ola::rdm::RDMGetRequest(
      SOURCE,
      destination,
      m_transaction_number++,  // transaction #
      1,  // port id
      10,  // sub device
      296,  // param id
      data,
      length);
}


/**
 * Pack a RDM request into a buffer
 */
uint8_t *EnttecUsbProWidgetTest::PackRDMRequest(const RDMRequest *request,
                                                unsigned int *size) {
  unsigned int request_size = RDMCommandSerializer::RequiredSize(*request);
  uint8_t *rdm_data = new uint8_t[request_size + 1];
  rdm_data[0] = ola::rdm::RDMCommand::START_CODE;
  memset(&rdm_data[1], 0, request_size);
  OLA_ASSERT(RDMCommandSerializer::Pack(*request, &rdm_data[1], &request_size));
  *size = request_size + 1;
  return rdm_data;
}


/**
 * Pack a RDM Response into a buffer
 */
uint8_t *EnttecUsbProWidgetTest::PackRDMResponse(const RDMResponse *response,
                                                 unsigned int *size) {
  unsigned int response_size = RDMCommandSerializer::RequiredSize(*response);
  uint8_t *rdm_data = new uint8_t[response_size + 2];
  rdm_data[0] = 0;  // status ok
  rdm_data[1] = ola::rdm::RDMCommand::START_CODE;
  memset(&rdm_data[2], 0, response_size);
  OLA_ASSERT(RDMCommandSerializer::Pack(*response, &rdm_data[2],
                                        &response_size));
  *size = response_size + 2;
  return rdm_data;
}


/*
 * Check the response matches what we expected.
 */
void EnttecUsbProWidgetTest::ValidateResponse(RDMReply *reply) {
  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, reply->StatusCode());
  OLA_ASSERT(reply->Response());
  OLA_ASSERT_DATA_EQUALS(TEST_RDM_DATA, arraysize(TEST_RDM_DATA),
                         reply->Response()->ParamData(),
                         reply->Response()->ParamDataSize());

  const RDMFrames &frames = reply->Frames();
  OLA_ASSERT_EQ((size_t) 1, frames.size());
  ola::rdm::RDMStatusCode raw_code;
  auto_ptr<ola::rdm::RDMResponse> raw_response(
    ola::rdm::RDMResponse::InflateFromData(frames[0].data.data() + 1,
                                           frames[0].data.size() - 1,
                                           &raw_code));
  OLA_ASSERT_TRUE(*raw_response.get() == *reply->Response());
  m_ss.Terminate();
}


/*
 * Check that this request returned the expected status code
 */
void EnttecUsbProWidgetTest::ValidateStatus(
    ola::rdm::RDMStatusCode expected_code,
    RDMFrames expected_frames,
    RDMReply *reply) {
  OLA_ASSERT_EQ(expected_code, reply->StatusCode());
  OLA_ASSERT_FALSE(reply->Response());

  const RDMFrames &frames = reply->Frames();
  OLA_ASSERT_EQ(expected_frames.size(), frames.size());
  for (unsigned int i = 0; i < frames.size(); i++) {
    OLA_ASSERT_DATA_EQUALS(expected_frames[i].data.data(),
                           expected_frames[i].data.size(),
                           frames[i].data.data(),
                           frames[i].data.size());
    OLA_ASSERT_TRUE(expected_frames[i] == frames[i]);
  }
  m_received_code = expected_code;
  m_ss.Terminate();
}


/**
 * Validate that a mute response matches what we expect
 */
void EnttecUsbProWidgetTest::ValidateMuteStatus(bool expected,
                                                bool actual) {
  OLA_ASSERT_EQ(expected, actual);
  m_ss.Terminate();
}



/**
 * Validate that a branch request returns what we expect.
 */
void EnttecUsbProWidgetTest::ValidateBranchStatus(const uint8_t *expected_data,
                                                  unsigned int length,
                                                  const uint8_t *actual_data,
                                                  unsigned int actual_length) {
  OLA_ASSERT_EQ(length, actual_length);
  OLA_ASSERT_FALSE(memcmp(expected_data, actual_data, length));
  m_ss.Terminate();
}


/**
 * Send a RDM response message, followed by a RDM timeout message
 */
void EnttecUsbProWidgetTest::SendResponseAndTimeout(
    const uint8_t *response_data,
    unsigned int length) {
  m_endpoint->SendUnsolicitedUsbProData(
    RECEIVE_DMX_LABEL,
    response_data,
    length);
  m_endpoint->SendUnsolicitedUsbProData(
    RDM_TIMEOUT_PACKET,
    NULL,
    0);
}


/**
 * Check the params are ok
 */
void EnttecUsbProWidgetTest::ValidateParams(
    bool status,
    const usb_pro_parameters &params) {
  OLA_ASSERT(status);
  OLA_ASSERT_EQ((uint8_t) 0, params.firmware);
  OLA_ASSERT_EQ((uint8_t) 1, params.firmware_high);
  OLA_ASSERT_EQ((uint8_t) 10, params.break_time);
  OLA_ASSERT_EQ((uint8_t) 14, params.mab_time);
  OLA_ASSERT_EQ((uint8_t) 40, params.rate);
  m_ss.Terminate();
}


/**
 * Check the DMX data is what we expected
 */
void EnttecUsbProWidgetTest::ValidateDMX(
    EnttecPort *port,
    const ola::DmxBuffer *expected_buffer) {
  const ola::DmxBuffer &buffer = port->FetchDMX();
  // Dereference the pointer so we can use our existing test
  ola::DmxBuffer expected = *expected_buffer;
  OLA_ASSERT_DMX_EQUALS(expected, buffer);
  m_got_dmx = true;
  m_ss.Terminate();
}


/**
 * Check that discovery works for a device that just implements the serial #
 */
void EnttecUsbProWidgetTest::testParams() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);
  OLA_ASSERT_NULL(m_widget->GetPort(1));

  uint8_t get_param_request_data[] = {0, 0};
  uint8_t get_param_response_data[] = {0, 1, 10, 14, 40};

  m_endpoint->AddExpectedUsbProDataAndReturn(
      GET_PARAM_LABEL,
      get_param_request_data,
      sizeof(get_param_request_data),
      GET_PARAM_LABEL,
      get_param_response_data,
      sizeof(get_param_response_data));

  port->GetParameters(
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

  OLA_ASSERT(port->SetParameters(9, 63, 20));

  m_ss.Run();
  m_endpoint->Verify();
}


/**
 * Check that receiving DMX works.
 */
void EnttecUsbProWidgetTest::testReceiveDMX() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);

  ola::DmxBuffer buffer;
  buffer.SetFromString("1,10,14,40");
  port->SetDMXCallback(ola::NewCallback(
      this,
      &EnttecUsbProWidgetTest::ValidateDMX,
      port,
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
  OLA_ASSERT(m_got_dmx);

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
      100,  // should be more than enough time
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));
  m_ss.Run();
  m_endpoint->Verify();
  OLA_ASSERT_FALSE(m_got_dmx);

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
  OLA_ASSERT_FALSE(m_got_dmx);

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
  OLA_ASSERT(m_got_dmx);
}


/*
 * Check that changing mode works.
 */
void EnttecUsbProWidgetTest::testChangeMode() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);

  // first we test 'send always' mode
  uint8_t change_mode_data[] = {0};
  m_endpoint->AddExpectedUsbProMessage(
      CHANGE_MODE_LABEL,
      change_mode_data,
      sizeof(change_mode_data),
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));

  port->ChangeToReceiveMode(false);

  m_ss.Run();
  m_endpoint->Verify();

  // now try 'send data on change' mode
  change_mode_data[0] = 1;
  m_endpoint->AddExpectedUsbProMessage(
      CHANGE_MODE_LABEL,
      change_mode_data,
      sizeof(change_mode_data),
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));

  port->ChangeToReceiveMode(true);
  m_ss.Run();
  m_endpoint->Verify();
}


/**
 * Check that we send RDM messages correctly.
 */
void EnttecUsbProWidgetTest::testSendRDMRequest() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);

  // request
  RDMRequest *rdm_request = NewRequest(DESTINATION);
  unsigned int expected_request_frame_size;
  uint8_t *expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  // response
  auto_ptr<const RDMResponse> response(
    GetResponseFromData(rdm_request, TEST_RDM_DATA, sizeof(TEST_RDM_DATA)));
  unsigned int response_size;
  uint8_t *response_frame = PackRDMResponse(response.get(), &response_size);

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      RECEIVE_DMX_LABEL,
      response_frame,
      response_size);

  port->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::ValidateResponse));
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_request_frame;
  delete[] response_frame;

  // now check broadcast messages
  // request
  rdm_request = NewRequest(BCAST_DESTINATION);
  uint8_t *expected_bcast_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_PACKET,
      expected_bcast_request_frame,
      expected_request_frame_size,
      RDM_TIMEOUT_PACKET,
      NULL,
      0);

  RDMFrames frames;
  m_received_code = ola::rdm::RDM_COMPLETED_OK;
  port->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &EnttecUsbProWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST,
                             frames));
  m_ss.Run();
  OLA_ASSERT_EQ(ola::rdm::RDM_WAS_BROADCAST, m_received_code);
  m_endpoint->Verify();

  // cleanup time
  delete[] expected_bcast_request_frame;
}


/**
 * Check that RDM Mute requests work
 */
void EnttecUsbProWidgetTest::testSendRDMMute() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);

  // request
  RDMRequest *rdm_request = new ola::rdm::RDMDiscoveryRequest(
      SOURCE,
      DESTINATION,
      m_transaction_number++,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_DISC_MUTE,  // param id
      NULL,
      0);
  unsigned int expected_request_frame_size;
  uint8_t *expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  // response
  // to keep things simple here we return the TEST_RDM_DATA.
  auto_ptr<const RDMResponse> response(
    GetResponseFromData(rdm_request, TEST_RDM_DATA, sizeof(TEST_RDM_DATA)));
  unsigned int response_size;
  uint8_t *response_frame = PackRDMResponse(response.get(), &response_size);

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      RECEIVE_DMX_LABEL,
      response_frame,
      response_size);

  port->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::ValidateResponse));
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_request_frame;
  delete[] response_frame;
}


/**
 * Check that we send RDM discovery messages correctly.
 */
void EnttecUsbProWidgetTest::testSendRDMDUB() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);

  static const uint8_t REQUEST_DATA[] = {
    0x7a, 0x70, 0, 0, 0, 0,
    0x7a, 0x70, 0xff, 0xff, 0xff, 0xff
  };

  // request
  RDMRequest *rdm_request = new ola::rdm::RDMDiscoveryRequest(
      SOURCE,
      DESTINATION,
      m_transaction_number++,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_DISC_UNIQUE_BRANCH,  // param id
      REQUEST_DATA,
      sizeof(REQUEST_DATA));
  unsigned int expected_request_frame_size;
  uint8_t *expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  // a 4 byte response means a timeout
  static const uint8_t EMPTY_RESPONSE[] = {0, 0, 0, 0};

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_DISCOVERY_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      RDM_TIMEOUT_PACKET,
      EMPTY_RESPONSE,
      sizeof(EMPTY_RESPONSE));

  RDMFrames frames;
  port->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &EnttecUsbProWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TIMEOUT,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_request_frame;

  // now try a dub response that returns something
  rdm_request = new ola::rdm::RDMDiscoveryRequest(
      SOURCE,
      DESTINATION,
      m_transaction_number++,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_DISC_UNIQUE_BRANCH,  // param id
      REQUEST_DATA,
      sizeof(REQUEST_DATA));
  expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  // something that looks like a DUB response
  static const uint8_t FAKE_RESPONSE[] = {0x00, 0xfe, 0xfe, 0xaa, 0xaa};

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_DISCOVERY_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      RECEIVE_DMX_LABEL,
      FAKE_RESPONSE,
      sizeof(FAKE_RESPONSE));

  frames.push_back(RDMFrame(FAKE_RESPONSE + 1, arraysize(FAKE_RESPONSE) - 1));
  port->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &EnttecUsbProWidgetTest::ValidateStatus,
                             ola::rdm::RDM_DUB_RESPONSE,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_request_frame;
}


/**
 * Test mute device
 */
void EnttecUsbProWidgetTest::testMuteDevice() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);

  // first test when a device doesn't respond
  auto_ptr<RDMRequest> mute_request(
      ola::rdm::NewMuteRequest(SOURCE,
                               DESTINATION,
                               m_transaction_number++));
  unsigned int expected_request_frame_size;
  uint8_t *expected_request_frame = PackRDMRequest(
      mute_request.get(),
      &expected_request_frame_size);

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      RDM_TIMEOUT_PACKET,
      NULL,
      0);

  port->m_impl->MuteDevice(
      DESTINATION,
      ola::NewSingleCallback(this,
                             &EnttecUsbProWidgetTest::ValidateMuteStatus,
                             false));
  m_ss.Run();
  m_endpoint->Verify();
  delete[] expected_request_frame;

  // now try an actual mute response
  auto_ptr<RDMRequest> mute_request2(
      ola::rdm::NewMuteRequest(SOURCE,
                               DESTINATION,
                               m_transaction_number++));
  expected_request_frame = PackRDMRequest(
      mute_request2.get(),
      &expected_request_frame_size);

  // We can really return anything
  // TODO(simon): make this better
  uint8_t mute_response_frame[] = {
      0,
      ola::rdm::RDMCommand::START_CODE,
      0};

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      RECEIVE_DMX_LABEL,
      mute_response_frame,
      sizeof(mute_response_frame));

  port->m_impl->MuteDevice(
      DESTINATION,
      ola::NewSingleCallback(this,
                             &EnttecUsbProWidgetTest::ValidateMuteStatus,
                             true));
  m_ss.Run();
  m_endpoint->Verify();
  delete[] expected_request_frame;
}


/**
 * Test the unmute all request works
 */
void EnttecUsbProWidgetTest::testUnMuteAll() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);

  auto_ptr<RDMRequest> unmute_request(
      ola::rdm::NewUnMuteRequest(SOURCE,
                                 UID::AllDevices(),
                                 m_transaction_number++));
  unsigned int expected_request_frame_size;
  uint8_t *expected_request_frame = PackRDMRequest(
      unmute_request.get(),
      &expected_request_frame_size);

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      RDM_TIMEOUT_PACKET,
      NULL,
      0);

  port->m_impl->UnMuteAll(
      ola::NewSingleCallback(this, &EnttecUsbProWidgetTest::Terminate));
  m_ss.Run();
  m_endpoint->Verify();
  delete[] expected_request_frame;
}


/**
 * Test the DUB request works
 */
void EnttecUsbProWidgetTest::testBranch() {
  EnttecPort *port = m_widget->GetPort(0);
  OLA_ASSERT_NOT_NULL(port);

  // first test when no devices respond
  auto_ptr<RDMRequest> discovery_request(
      ola::rdm::NewDiscoveryUniqueBranchRequest(
          SOURCE,
          UID(0, 0),
          UID::AllDevices(),
          m_transaction_number++));
  unsigned int expected_request_frame_size;
  uint8_t *expected_request_frame = PackRDMRequest(
      discovery_request.get(),
      &expected_request_frame_size);

  // add the expected response, send and verify
  m_endpoint->AddExpectedUsbProDataAndReturn(
      RDM_DISCOVERY_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      RDM_TIMEOUT_PACKET,
      NULL,
      0);

  port->m_impl->Branch(
      UID(0, 0),
      UID::AllDevices(),
      ola::NewSingleCallback(this,
                             &EnttecUsbProWidgetTest::ValidateBranchStatus,
                             static_cast<const uint8_t*>(NULL),
                             static_cast<unsigned int>(0)));
  m_ss.Run();
  m_endpoint->Verify();
  delete[] expected_request_frame;

  // now try an actual response, the data doesn't actually have to be valid
  // because it's just passed straight to the callback.
  auto_ptr<RDMRequest> discovery_request2(
      ola::rdm::NewDiscoveryUniqueBranchRequest(
          SOURCE,
          UID(0, 0),
          UID::AllDevices(),
          m_transaction_number++));
  expected_request_frame = PackRDMRequest(
      discovery_request2.get(),
      &expected_request_frame_size);

  // the response, can be anything really, only the first byte counts
  uint8_t response_frame2[] = {0, 1, 2, 3, 4};

  m_endpoint->AddExpectedUsbProMessage(
      RDM_DISCOVERY_PACKET,
      expected_request_frame,
      expected_request_frame_size,
      ola::NewSingleCallback(
        this,
        &EnttecUsbProWidgetTest::SendResponseAndTimeout,
        static_cast<const uint8_t*>(response_frame2),
        static_cast<unsigned int>(sizeof(response_frame2))));

  port->m_impl->Branch(
      UID(0, 0),
      UID::AllDevices(),
      ola::NewSingleCallback(
        this,
        &EnttecUsbProWidgetTest::ValidateBranchStatus,
        static_cast<const uint8_t*>(&response_frame2[1]),
        static_cast<unsigned int>(sizeof(response_frame2) - 1)));
  m_ss.Run();
  m_endpoint->Verify();
  delete[] expected_request_frame;
}
