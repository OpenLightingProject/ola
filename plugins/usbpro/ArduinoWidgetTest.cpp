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
 * ArduinoWidgetTest.cpp
 * Test fixture for the ArduinoWidget class
 * Copyright (C) 2011 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>
#include <vector>

#include "common/rdm/TestHelper.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/rdm/RDMPacket.h"
#include "plugins/usbpro/ArduinoWidget.h"
#include "plugins/usbpro/CommonWidgetTest.h"
#include "ola/testing/TestUtils.h"

using ola::io::ByteString;
using ola::plugin::usbpro::ArduinoWidget;
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

class ArduinoWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(ArduinoWidgetTest);
  CPPUNIT_TEST(testDiscovery);
  CPPUNIT_TEST(testSendRDMRequest);
  CPPUNIT_TEST(testErrorCodes);
  CPPUNIT_TEST(testErrorConditions);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();

    void testDiscovery();
    void testSendRDMRequest();
    void testErrorCodes();
    void testErrorConditions();

 private:
    auto_ptr<ola::plugin::usbpro::ArduinoWidget> m_arduino;
    unsigned int m_tod_counter;
    uint8_t m_transaction_number;

    void ValidateTod(const ola::rdm::UIDSet &uids);
    void ValidateResponse(RDMReply *reply);
    void ValidateStatus(ola::rdm::RDMStatusCode expected_code,
                        RDMFrames expected_frames,
                        RDMReply *reply);
    RDMRequest *NewRequest(const UID &destination,
                           const uint8_t *data = NULL,
                           unsigned int length = 0);

    uint8_t *PackRDMRequest(const RDMRequest *request, unsigned int *size);
    uint8_t *PackRDMResponse(const RDMResponse *response, unsigned int *size);
    uint8_t *PackRDMError(uint8_t error_code, unsigned int *size);

    static const uint16_t ESTA_ID = 0x7890;
    static const uint32_t SERIAL_NUMBER = 0x01020304;
    static const uint8_t RDM_REQUEST_LABEL = 0x52;
    static const uint8_t BROADCAST_STATUS_CODE = 1;
    static const unsigned int FOOTER_SIZE = 1;
    static const unsigned int HEADER_SIZE = 4;
    static const UID SOURCE;
    static const UID DESTINATION;
    static const UID BCAST_DESTINATION;
    static const uint8_t TEST_RDM_DATA[];
};

const UID ArduinoWidgetTest::SOURCE(1, 2);
const UID ArduinoWidgetTest::DESTINATION(ESTA_ID, SERIAL_NUMBER);
const UID ArduinoWidgetTest::BCAST_DESTINATION(ESTA_ID, 0xffffffff);
const uint8_t ArduinoWidgetTest::TEST_RDM_DATA[] = {0x5a, 0x5a, 0x5a, 0x5a};



CPPUNIT_TEST_SUITE_REGISTRATION(ArduinoWidgetTest);


/**
 * Setup the descriptors, ss and the MockEndpoint
 */
void ArduinoWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  m_tod_counter = 0;
  m_transaction_number = 0;

  m_arduino.reset(new ola::plugin::usbpro::ArduinoWidget(
      &m_descriptor,
      ESTA_ID,
      SERIAL_NUMBER));
}


/**
 * Called when the widget returns a new TOD. Check this matches what we expect.
 */
void ArduinoWidgetTest::ValidateTod(const ola::rdm::UIDSet &uids) {
  UID uid1(ESTA_ID, SERIAL_NUMBER);
  OLA_ASSERT_EQ((unsigned int) 1, uids.Size());
  OLA_ASSERT(uids.Contains(uid1));
  m_tod_counter++;
}


/*
 * Check the response matches what we expected.
 */
void ArduinoWidgetTest::ValidateResponse(RDMReply *reply) {
  OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, reply->StatusCode());
  OLA_ASSERT(reply->Response());

  const ola::rdm::RDMResponse *response = reply->Response();
  OLA_ASSERT_DATA_EQUALS(TEST_RDM_DATA,
                         static_cast<unsigned int>(sizeof(TEST_RDM_DATA)),
                         response->ParamData(),
                         response->ParamDataSize());

  OLA_ASSERT_EQ((size_t) 1, reply->Frames().size());
  const ola::rdm::RDMFrame frame = reply->Frames()[0];

  ola::rdm::RDMStatusCode raw_code;
  auto_ptr<ola::rdm::RDMResponse> raw_response(
    ola::rdm::RDMResponse::InflateFromData(frame.data.data() + 1,
      frame.data.size() - 1, &raw_code));
  OLA_ASSERT_TRUE(*raw_response.get() == *response);
  m_ss.Terminate();
}


/*
 * Check that this request returned the expected status code & frames.
 */
void ArduinoWidgetTest::ValidateStatus(ola::rdm::RDMStatusCode expected_code,
                                       RDMFrames expected_frames,
                                       RDMReply *reply) {
  OLA_ASSERT_EQ(expected_code, reply->StatusCode());
  OLA_ASSERT_NULL(reply->Response());

  const RDMFrames &frames = reply->Frames();

  OLA_ASSERT_EQ(expected_frames.size(), frames.size());
  for (unsigned int i = 0; i < frames.size(); i++) {
    const ByteString &expected = expected_frames[i].data;
    const ByteString &actual = frames[i].data;
    OLA_ASSERT_DATA_EQUALS(expected.data(), expected.size(), actual.data(),
                           actual.size());
  }
  m_ss.Terminate();
}


/*
 * Helper method to create new GetRDMRequest objects.
 * @param destination the destination UID
 * @param data the RDM Request data
 * @param length the size of the RDM data.
 */
RDMRequest *ArduinoWidgetTest::NewRequest(const UID &destination,
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
uint8_t *ArduinoWidgetTest::PackRDMRequest(const RDMRequest *request,
                                           unsigned int *size) {
  unsigned int request_size = RDMCommandSerializer::RequiredSize(*request);
  uint8_t rdm_data[request_size + 1];
  rdm_data[0] = ola::rdm::START_CODE;
  OLA_ASSERT(RDMCommandSerializer::Pack(*request, rdm_data + 1, &request_size));
  uint8_t *frame = BuildUsbProMessage(RDM_REQUEST_LABEL,
                                      rdm_data,
                                      request_size + 1,
                                      size);
  return frame;
}


/**
 * Pack a RDM Response into a buffer
 */
uint8_t *ArduinoWidgetTest::PackRDMResponse(const RDMResponse *response,
                                            unsigned int *size) {
  unsigned int response_size = RDMCommandSerializer::RequiredSize(*response);
  uint8_t rdm_data[response_size + 2];
  rdm_data[0] = ola::rdm::RDM_COMPLETED_OK;
  rdm_data[1] = ola::rdm::START_CODE;
  OLA_ASSERT(
      RDMCommandSerializer::Pack(*response, rdm_data + 2, &response_size));
  uint8_t *frame = BuildUsbProMessage(RDM_REQUEST_LABEL,
                                      rdm_data,
                                      response_size + 2,
                                      size);
  return frame;
}


/**
 * Pack a RDM response which just contains an error code
 * @param error_code the error status code
 * @param total_size, pointer which is updated with the message size.
 */
uint8_t *ArduinoWidgetTest::PackRDMError(uint8_t error_code,
                                         unsigned int *size) {
  uint8_t *frame = BuildUsbProMessage(RDM_REQUEST_LABEL,
                                      &error_code,
                                      sizeof(error_code),
                                      size);
  return frame;
}


/**
 * Check that discovery works.
 */
void ArduinoWidgetTest::testDiscovery() {
  OLA_ASSERT_EQ((unsigned int) 0, m_tod_counter);
  m_arduino->RunFullDiscovery(
      ola::NewSingleCallback(this, &ArduinoWidgetTest::ValidateTod));
  OLA_ASSERT_EQ((unsigned int) 1, m_tod_counter);

  m_arduino->RunIncrementalDiscovery(
      ola::NewSingleCallback(this, &ArduinoWidgetTest::ValidateTod));
  OLA_ASSERT_EQ((unsigned int) 2, m_tod_counter);
}


/**
 * Check that we send RDM messages correctly.
 */
void ArduinoWidgetTest::testSendRDMRequest() {
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
  m_endpoint->AddExpectedDataAndReturn(
      expected_request_frame,
      expected_request_frame_size,
      response_frame,
      response_size);

  m_arduino->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this, &ArduinoWidgetTest::ValidateResponse));
  m_ss.Run();
  m_endpoint->Verify();

  // now check broadcast messages
  // request
  rdm_request = NewRequest(BCAST_DESTINATION);
  uint8_t *expected_bcast_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  // must match ArduinoWidgetImpl::RESPONSE_WAS_BROADCAST
  uint8_t *broadcast_response_frame = PackRDMError(
      BROADCAST_STATUS_CODE,
      &response_size);

  // add the expected response, send and verify
  m_endpoint->AddExpectedDataAndReturn(
      expected_bcast_request_frame,
      expected_request_frame_size,
      broadcast_response_frame,
      response_size);

  RDMFrames frames;
  m_arduino->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();

  // cleanup time
  delete[] expected_request_frame;
  delete[] expected_bcast_request_frame;
  delete[] response_frame;
  delete[] broadcast_response_frame;
}


/*
 * Check that we handle invalid responses ok
 */
void ArduinoWidgetTest::testErrorCodes() {
  RDMFrames frames;
  // request
  RDMRequest *rdm_request = NewRequest(DESTINATION);
  unsigned int expected_request_frame_size;
  uint8_t *expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  // response
  auto_ptr<const RDMResponse> response(GetResponseFromData(rdm_request));
  unsigned int response_size;
  uint8_t *response_frame = PackRDMResponse(response.get(), &response_size);

  // verify a checksum error is detected
  // twiddle the penultimate bit so that the checksum fails
  response_frame[response_size - 2] += 1;
  frames.push_back(RDMFrame(response_frame + 5,
                            RDMCommandSerializer::RequiredSize(*response) + 1));

  // add the expected response, send and verify
  m_endpoint->AddExpectedDataAndReturn(
      expected_request_frame,
      expected_request_frame_size,
      response_frame,
      response_size);

  m_arduino->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_CHECKSUM_INCORRECT,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();
  frames.clear();
  delete[] expected_request_frame;
  delete[] response_frame;

  // now try a packet which is too short
  rdm_request = NewRequest(DESTINATION);
  expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  //  we truncate the response here
  response.reset(GetResponseFromData(rdm_request));
  response_size = RDMCommandSerializer::RequiredSize(*response);
  uint8_t rdm_data[response_size + 2];
  rdm_data[0] = ola::rdm::RDM_COMPLETED_OK;
  rdm_data[1] = ola::rdm::START_CODE;
  OLA_ASSERT(
      RDMCommandSerializer::Pack(*response, rdm_data + 2, &response_size));
  response_frame = BuildUsbProMessage(
      RDM_REQUEST_LABEL,
      rdm_data,
      10,  // only send 10 bytes of the response
      &response_size);

  // only return the first 10 bytes of the rdm response
  frames.push_back(RDMFrame(response_frame + 5, 9));

  m_endpoint->AddExpectedDataAndReturn(
      expected_request_frame,
      expected_request_frame_size,
      response_frame,
      response_size);

  m_arduino->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_PACKET_TOO_SHORT,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();
  frames.clear();
  delete[] expected_request_frame;
  delete[] response_frame;

  // now test a transaction # mismatch
  rdm_request = NewRequest(DESTINATION);
  expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  response.reset(GetResponseFromData(rdm_request));
  response_size = RDMCommandSerializer::RequiredSize(*response);
  OLA_ASSERT(
      RDMCommandSerializer::Pack(*response, rdm_data + 2, &response_size));
  // twiddle the transaction number
  rdm_data[16] += 1;
  // 'correct' the checksum
  rdm_data[response_size + 2 - 1] += 1;
  frames.push_back(RDMFrame(rdm_data + 1, response_size + 1));

  response_frame = BuildUsbProMessage(
      RDM_REQUEST_LABEL,
      rdm_data,
      response_size + 2,
      &response_size);

  m_endpoint->AddExpectedDataAndReturn(
      expected_request_frame,
      expected_request_frame_size,
      response_frame,
      response_size);

  m_arduino->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TRANSACTION_MISMATCH,
                             frames));
  m_ss.Run();
  m_endpoint->Verify();
  frames.clear();

  delete[] expected_request_frame;
  delete[] response_frame;

  // sub device mismatch
  rdm_request = NewRequest(DESTINATION);
  expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  response.reset(GetResponseFromData(rdm_request));
  response_size = RDMCommandSerializer::RequiredSize(*response);
  // change the sub device
  rdm_data[20] += 1;
  // 'correct' the checksum
  rdm_data[response_size + 2 - 1] += 1;
  frames.push_back(RDMFrame(rdm_data + 1, response_size + 1));

  response_frame = BuildUsbProMessage(
      RDM_REQUEST_LABEL,
      rdm_data,
      response_size + 2,
      &response_size);

  m_endpoint->AddExpectedDataAndReturn(
      expected_request_frame,
      expected_request_frame_size,
      response_frame,
      response_size);

  m_arduino->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_SUB_DEVICE_MISMATCH,
                             frames));

  m_ss.Run();
  m_endpoint->Verify();
  frames.clear();
  delete[] expected_request_frame;
  delete[] response_frame;
}


/*
 * Check some of the error conditions
 */
void ArduinoWidgetTest::testErrorConditions() {
  RDMFrames frames;

  uint8_t ERROR_CODES[] = {2, 3, 4, 5};
  // test each of the error codes.
  for (unsigned int i = 0; i < sizeof(ERROR_CODES); ++i) {
    // request
    RDMRequest *request = NewRequest(DESTINATION);
    unsigned int expected_request_frame_size;
    uint8_t *expected_request_frame = PackRDMRequest(
        request,
        &expected_request_frame_size);

    // expected response
    unsigned int response_size;
    uint8_t *response_frame = PackRDMError(
        ERROR_CODES[i],
        &response_size);

    m_endpoint->AddExpectedDataAndReturn(
        expected_request_frame,
        expected_request_frame_size,
        response_frame,
        response_size);

    m_arduino->SendRDMRequest(
        request,
        ola::NewSingleCallback(this,
                               &ArduinoWidgetTest::ValidateStatus,
                               ola::rdm::RDM_FAILED_TO_SEND,
                               frames));
    m_ss.Run();
    m_endpoint->Verify();
    delete[] expected_request_frame;
    delete[] response_frame;
  }
}
