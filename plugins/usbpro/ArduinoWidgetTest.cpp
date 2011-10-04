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
 * ArduinoWidgetTest.cpp
 * Test fixture for the ArduinoWidget class
 * Copyright (C) 2011 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>
#include <vector>

#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/network/Socket.h"
#include "ola/network/SelectServer.h"
#include "plugins/usbpro/ArduinoWidget.h"
#include "plugins/usbpro/MockEndpoint.h"


using ola::DmxBuffer;
using ola::plugin::usbpro::ArduinoWidget;
using ola::rdm::GetResponseFromData;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::auto_ptr;
using std::string;
using std::vector;


class ArduinoWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ArduinoWidgetTest);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST(testDiscovery);
  CPPUNIT_TEST(testSendRDMRequest);
  CPPUNIT_TEST(testErrorCodes);
  CPPUNIT_TEST(testErrorConditions);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();

    void testSendDMX();
    void testDiscovery();
    void testSendRDMRequest();
    void testErrorCodes();
    void testErrorConditions();

  private:
    ola::network::SelectServer m_ss;
    ola::network::PipeDescriptor m_descriptor;
    auto_ptr<ola::network::PipeDescriptor> m_other_end;
    auto_ptr<MockEndpoint> m_endpoint;
    auto_ptr<ola::plugin::usbpro::ArduinoWidget> m_arduino;
    unsigned int m_tod_counter;
    uint8_t m_transaction_number;

    void Terminate() {
      m_ss.Terminate();
    }
    void ValidateTod(const ola::rdm::UIDSet &uids);
    void ValidateResponse(ola::rdm::rdm_response_code code,
                          const ola::rdm::RDMResponse *response,
                          const vector<string> &packets);
    void ValidateStatus(ola::rdm::rdm_response_code expected_code,
                        vector<string> expected_packets,
                        ola::rdm::rdm_response_code code,
                        const ola::rdm::RDMResponse *response,
                        const vector<string> &packets);
    const RDMRequest *NewRequest(const UID &DESTINATION,
                                 const uint8_t *data = NULL,
                                 unsigned int length = 0);

    uint8_t *BuildUsbProMessage(uint8_t label,
                          const uint8_t *data,
                          unsigned int data_size,
                          unsigned int *total_size);
    uint8_t *PackRDMRequest(const RDMRequest *request, unsigned int *size);
    uint8_t *PackRDMResponse(const RDMResponse *response, unsigned int *size);
    uint8_t *PackRDMError(uint8_t error_code, unsigned int *size);

    static const uint16_t ESTA_ID = 0x7890;
    static const uint32_t SERIAL_NUMBER = 0x01020304;
    static const uint8_t RDM_REQUEST_LABEL = 0x52;
    static const uint8_t DMX_FRAME_LABEL = 0x06;
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
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_descriptor.Init();
  m_other_end.reset(m_descriptor.OppositeEnd());
  m_endpoint.reset(new MockEndpoint(m_other_end.get()));
  m_ss.AddReadDescriptor(&m_descriptor);
  m_ss.AddReadDescriptor(m_other_end.get());

  m_tod_counter = 0;
  m_transaction_number = 0;

  m_arduino.reset(new ola::plugin::usbpro::ArduinoWidget(
      &m_descriptor,
      ESTA_ID,
      SERIAL_NUMBER));
}


/**
 * Clean up.
 */
void ArduinoWidgetTest::tearDown() {
  m_endpoint->Verify();
  m_ss.RemoveReadDescriptor(&m_descriptor);
  m_ss.RemoveReadDescriptor(m_other_end.get());
}


/**
 * Called when the widget returns a new TOD. Check this matches what we expect.
 */
void ArduinoWidgetTest::ValidateTod(const ola::rdm::UIDSet &uids) {
  UID uid1(ESTA_ID, SERIAL_NUMBER);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, uids.Size());
  CPPUNIT_ASSERT(uids.Contains(uid1));
  m_tod_counter++;
}


/*
 * Check the response matches what we expected.
 */
void ArduinoWidgetTest::ValidateResponse(
    ola::rdm::rdm_response_code code,
    const ola::rdm::RDMResponse *response,
    const vector<string> &packets) {
  CPPUNIT_ASSERT_EQUAL(ola::rdm::RDM_COMPLETED_OK, code);
  CPPUNIT_ASSERT(response);
  CPPUNIT_ASSERT_EQUAL(
      sizeof(TEST_RDM_DATA),
      static_cast<long unsigned int>(response->ParamDataSize()));
  CPPUNIT_ASSERT(0 == memcmp(TEST_RDM_DATA, response->ParamData(),
                             response->ParamDataSize()));

  CPPUNIT_ASSERT_EQUAL((size_t) 1, packets.size());
  ola::rdm::rdm_response_code raw_code;
  auto_ptr<ola::rdm::RDMResponse> raw_response(
    ola::rdm::RDMResponse::InflateFromData(packets[0], &raw_code));
  CPPUNIT_ASSERT(*(raw_response.get()) == *response);
  delete response;
  m_ss.Terminate();
}


/*
 * Check that this request returned the expected status code
 * @param expected_code the expected widget status code
 * @param expected_code a list of expected packets
 * @param code the actual status code returns
 * @param response the RDMResponse object, or NULL
 * @param packets the actual packets involved
 */
void ArduinoWidgetTest::ValidateStatus(
    ola::rdm::rdm_response_code expected_code,
    vector<string> expected_packets,
    ola::rdm::rdm_response_code code,
    const ola::rdm::RDMResponse *response,
    const vector<string> &packets) {

  CPPUNIT_ASSERT_EQUAL(expected_code, code);
  CPPUNIT_ASSERT(!response);

  CPPUNIT_ASSERT_EQUAL(expected_packets.size(), packets.size());
  for (unsigned int i = 0; i < packets.size(); i++) {
    if (expected_packets[i].size() != packets[i].size())
      OLA_INFO << expected_packets[i].size() << " != " << packets[i].size();
    CPPUNIT_ASSERT_EQUAL(expected_packets[i].size(), packets[i].size());

    if (expected_packets[i] != packets[i]) {
      for (unsigned int j = 0; j < packets[i].size(); j++) {
        OLA_INFO << std::hex << (int) packets[i][j] << " - " << (int)
          expected_packets[i][j];
      }
    }
    CPPUNIT_ASSERT(expected_packets[i] == packets[i]);
  }
  m_ss.Terminate();
}


/*
 * Helper method to create new GetRDMRequest objects.
 * @param destination the destination UID
 * @param data the RDM Request data
 * @param length the size of the RDM data.
 */
const RDMRequest *ArduinoWidgetTest::NewRequest(const UID &destination,
                                                const uint8_t *data,
                                                unsigned int length) {
  return new ola::rdm::RDMGetRequest(
      SOURCE,
      destination,
      m_transaction_number++,  // transaction #
      1,  // port id
      0,  // message count
      10,  // sub device
      296,  // param id
      data,
      length);
}


/**
 * Pack data into a Usb Pro style frame.
 * @param label the message label
 * @param data the message data
 * @param data_size the data size
 * @param total_size, pointer which is updated with the message size.
 */
uint8_t *ArduinoWidgetTest::BuildUsbProMessage(uint8_t label,
                                               const uint8_t *data,
                                               unsigned int data_size,
                                               unsigned int *total_size) {
  uint8_t *frame = new uint8_t[data_size + HEADER_SIZE + FOOTER_SIZE];
  frame[0] = 0x7e;  // som
  frame[1] = label;
  frame[2] = data_size & 0xff;  // len
  frame[3] = (data_size + 1) >> 8;  // len hi
  memcpy(frame + 4, data, data_size);
  frame[data_size + HEADER_SIZE] = 0xe7;
  *total_size = data_size + HEADER_SIZE + FOOTER_SIZE;
  return frame;
}


/**
 * Pack a RDM request into a buffer
 */
uint8_t *ArduinoWidgetTest::PackRDMRequest(const RDMRequest *request,
                                           unsigned int *size) {
  unsigned int request_size = request->Size();
  uint8_t rdm_data[request_size + 1];
  rdm_data[0] = ola::rdm::RDMCommand::START_CODE;
  CPPUNIT_ASSERT(request->Pack(
        rdm_data + 1,
        &request_size));
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
  unsigned int response_size = response->Size();
  uint8_t rdm_data[response_size + 2];
  rdm_data[0] = ola::rdm::RDM_COMPLETED_OK;
  rdm_data[1] = ola::rdm::RDMCommand::START_CODE;
  CPPUNIT_ASSERT(response->Pack(
        rdm_data + 2,
        &response_size));
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
 * Check that we can send DMX
 */
void ArduinoWidgetTest::testSendDMX() {
  // dmx data
  DmxBuffer buffer;
  buffer.SetFromString("0,1,2,3,4");

  // expected message
  uint8_t dmx_frame_data[] = {DMX512_START_CODE, 0, 1, 2, 3, 4};
  unsigned int size;
  uint8_t *expected_message = BuildUsbProMessage(DMX_FRAME_LABEL,
                                                 dmx_frame_data,
                                                 sizeof(dmx_frame_data),
                                                 &size);

  // add the expected data, run and verify.
  m_endpoint->AddExpectedData(
      expected_message,
      size,
      ola::NewSingleCallback(this, &ArduinoWidgetTest::Terminate));
  m_arduino->SendDMX(buffer);
  m_ss.Run();
  m_endpoint->Verify();

  // now test an empty frame
  DmxBuffer buffer2;
  uint8_t empty_frame_data[] = {DMX512_START_CODE};  // just the start code
  uint8_t *expected_message2 = BuildUsbProMessage(DMX_FRAME_LABEL,
                                                  empty_frame_data,
                                                  sizeof(empty_frame_data),
                                                  &size);

  // add the expected data, run and verify.
  m_endpoint->AddExpectedData(
      expected_message2,
      size,
      ola::NewSingleCallback(this, &ArduinoWidgetTest::Terminate));
  m_arduino->SendDMX(buffer2);
  m_ss.Run();
  m_endpoint->Verify();

  delete[] expected_message;
  delete[] expected_message2;
}


/**
 * Check that discovery works.
 */
void ArduinoWidgetTest::testDiscovery() {
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, m_tod_counter);
  m_arduino->RunFullDiscovery(
      ola::NewSingleCallback(this, &ArduinoWidgetTest::ValidateTod));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, m_tod_counter);

  m_arduino->RunIncrementalDiscovery(
      ola::NewSingleCallback(this, &ArduinoWidgetTest::ValidateTod));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, m_tod_counter);
}


/**
 * Check that we send RDM messages correctly.
 */
void ArduinoWidgetTest::testSendRDMRequest() {
  // request
  const RDMRequest *rdm_request = NewRequest(DESTINATION);
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

  vector<string> packets;
  m_arduino->SendRDMRequest(
      rdm_request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST,
                             packets));
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
  vector<string> packets;
  // request
  const RDMRequest *rdm_request = NewRequest(DESTINATION);
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
  packets.push_back(string(reinterpret_cast<char*>(response_frame + 6),
        response->Size()));

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
                             packets));
  m_ss.Run();
  m_endpoint->Verify();
  packets.clear();
  delete[] expected_request_frame;
  delete[] response_frame;

  // now try a packet which is too short
  rdm_request = NewRequest(DESTINATION);
  expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  //  we truncate the response here
  response.reset(GetResponseFromData(rdm_request));
  response_size = response->Size();
  uint8_t rdm_data[response_size + 2];
  rdm_data[0] = ola::rdm::RDM_COMPLETED_OK;
  rdm_data[1] = ola::rdm::RDMCommand::START_CODE;
  CPPUNIT_ASSERT(response->Pack(
        rdm_data + 2,
        &response_size));
  response_frame = BuildUsbProMessage(
      RDM_REQUEST_LABEL,
      rdm_data,
      10,  // only send 10 bytes of the response
      &response_size);

  // only return the first 10 bytes of the rdm response
  packets.push_back(string(reinterpret_cast<char*>(response_frame + 6), 8));

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
                             packets));
  m_ss.Run();
  m_endpoint->Verify();
  packets.clear();
  delete[] expected_request_frame;
  delete[] response_frame;

  // now test a transaction # mismatch
  rdm_request = NewRequest(DESTINATION);
  expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  response.reset(GetResponseFromData(rdm_request));
  response_size = response->Size();
  CPPUNIT_ASSERT(response->Pack(
        rdm_data + 2,
        &response_size));
  // twiddle the transaction number
  rdm_data[16] += 1;
  // 'correct' the checksum
  rdm_data[response_size + 2 - 1] += 1;
  packets.push_back(string(reinterpret_cast<char*>(rdm_data + 2),
                           response_size));

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
                             packets));
  m_ss.Run();
  m_endpoint->Verify();
  packets.clear();

  delete[] expected_request_frame;
  delete[] response_frame;

  // sub device mismatch
  rdm_request = NewRequest(DESTINATION);
  expected_request_frame = PackRDMRequest(
      rdm_request,
      &expected_request_frame_size);

  response.reset(GetResponseFromData(rdm_request));
  response_size = response->Size();
  // change the sub device
  rdm_data[20] += 1;
  // 'correct' the checksum
  rdm_data[response_size + 2 - 1] += 1;
  packets.push_back(string(reinterpret_cast<char*>(rdm_data + 2),
                           response_size));

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
                             packets));

  m_ss.Run();
  m_endpoint->Verify();
  packets.clear();
  delete[] expected_request_frame;
  delete[] response_frame;
}


/*
 * Check some of the error conditions
 */
void ArduinoWidgetTest::testErrorConditions() {
  vector<string> packets;

  uint8_t ERROR_CODES[] = {2, 3, 4, 5};
  // test each of the error codes.
  for (unsigned int i = 0; i < sizeof(ERROR_CODES); ++i) {
    // request
    const RDMRequest *request = NewRequest(DESTINATION);
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
                               packets));
    m_ss.Run();
    m_endpoint->Verify();
    delete[] expected_request_frame;
  }
}
