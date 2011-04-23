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
#include <string>
#include <vector>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "plugins/usbpro/ArduinoWidget.h"
#include "plugins/usbpro/MockUsbWidget.h"


using ola::plugin::usbpro::ArduinoWidget;
using ola::rdm::GetResponseFromData;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::UID;
using std::string;
using std::vector;


class ArduinoWidgetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ArduinoWidgetTest);
  CPPUNIT_TEST(testDiscovery);
  CPPUNIT_TEST(testSendRDMRequest);
  CPPUNIT_TEST(testErrorCodes);
  CPPUNIT_TEST(testErrorConditions);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();

    void testDiscovery();
    void testSendRDMRequest();
    void testErrorCodes();
    void testErrorConditions();

  private:
    MockUsbWidget m_widget;
    unsigned int m_tod_counter;
    uint8_t m_transaction_number;
    ola::plugin::usbpro::ArduinoWidget *m_arduino;

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

    uint8_t *PackRequest(const RDMRequest *request, unsigned int *size);
    uint8_t *PackResponse(const RDMResponse *response, unsigned int *size);

    static const uint16_t ESTA_ID = 0x7890;
    static const uint32_t SERIAL_NUMBER = 0x01020304;
    static const uint8_t RDM_REQUEST_LABEL = 0x52;
    static const UID SOURCE;
    static const UID DESTINATION;
    static const UID BCAST_DESTINATION;
};

const UID ArduinoWidgetTest::SOURCE(1, 2);
const UID ArduinoWidgetTest::DESTINATION(ESTA_ID, SERIAL_NUMBER);
const UID ArduinoWidgetTest::BCAST_DESTINATION(ESTA_ID, 0xffffffff);



CPPUNIT_TEST_SUITE_REGISTRATION(ArduinoWidgetTest);


void ArduinoWidgetTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_tod_counter = 0;
  m_transaction_number = 0;

  m_arduino = new ola::plugin::usbpro::ArduinoWidget(&m_widget,
                                                     ESTA_ID,
                                                     SERIAL_NUMBER);
}


void ArduinoWidgetTest::tearDown() {
  delete m_arduino;
}


/**
 * Check the TOD matches what we expect
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
  uint8_t expected_data[] = {0x5a, 0x5a, 0x5a, 0x5a};
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4, response->ParamDataSize());
  CPPUNIT_ASSERT(0 == memcmp(expected_data, response->ParamData(),
                             response->ParamDataSize()));

  CPPUNIT_ASSERT_EQUAL((size_t) 1, packets.size());
  ola::rdm::rdm_response_code raw_code;
  ola::rdm::RDMResponse *raw_response =
    ola::rdm::RDMResponse::InflateFromData(packets[0], &raw_code);
  CPPUNIT_ASSERT(*raw_response == *response);
  delete raw_response;
  delete response;
}


/*
 * Check that this request was broadcast
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
    CPPUNIT_ASSERT(expected_packets[i] == packets[i]);
  }
}


/*
 * Helper method to create new request objects
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
 * Pack a RDM request into a buffer
 */
uint8_t *ArduinoWidgetTest::PackRequest(const RDMRequest *request,
                                        unsigned int *size) {
  unsigned int request_size = request->Size();
  uint8_t *expected_packet = new uint8_t[request_size + 1];
  expected_packet[0] = ola::rdm::RDMCommand::START_CODE;
  CPPUNIT_ASSERT(request->Pack(expected_packet + 1, &request_size));
  *size = request_size + 1;
  return expected_packet;
}


/**
 * Pack a RDM Response into a buffer
 */
uint8_t *ArduinoWidgetTest::PackResponse(const RDMResponse *response,
                                         unsigned int *size) {
  unsigned int response_size = response->Size();
  uint8_t *response_packet = new uint8_t[response_size + 2];
  response_packet[0] = 0x00;
  response_packet[1] = ola::rdm::RDMCommand::START_CODE;
  CPPUNIT_ASSERT(response->Pack(response_packet + 2, &response_size));
  *size = response_size + 2;
  return response_packet;
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
  m_widget.Verify();
}


/**
 * Check that we send messages correctly.
 */
void ArduinoWidgetTest::testSendRDMRequest() {
  vector<string> packets;
  const RDMRequest *request = NewRequest(DESTINATION);
  unsigned int size;
  uint8_t *expected_packet = PackRequest(request, &size);

  uint8_t param_data[] = {0x5a, 0x5a, 0x5a, 0x5a};
  const RDMResponse *response = GetResponseFromData(request,
                                                    param_data,
                                                    sizeof(param_data));
  unsigned int response_size;
  uint8_t *response_packet = PackResponse(response, &response_size);

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      expected_packet,
      size,
      RDM_REQUEST_LABEL,
      response_packet,
      response_size);

  m_arduino->SendRDMRequest(
      request,
      ola::NewSingleCallback(this, &ArduinoWidgetTest::ValidateResponse));

  m_widget.Verify();

  // now check broadcast
  request = NewRequest(BCAST_DESTINATION);
  uint8_t *expected_bcast_packet = PackRequest(request, &size);

  // must mach ArduinoWidgetImpl::RESPONSE_WAS_BROADCAST
  uint8_t response_was_broadcast = 1;

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      expected_bcast_packet,
      size,
      RDM_REQUEST_LABEL,
      &response_was_broadcast,
      sizeof(response_was_broadcast));

  m_arduino->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_WAS_BROADCAST,
                             packets));

  delete[] expected_packet;
  delete[] expected_bcast_packet;
  delete[] response_packet;
  delete response;
  m_widget.Verify();
}


/*
 * Check that we handle invalid responses ok
 */
void ArduinoWidgetTest::testErrorCodes() {
  vector<string> packets;

  const RDMRequest *request = NewRequest(DESTINATION);
  unsigned int size;
  uint8_t *expected_packet = PackRequest(request, &size);

  const RDMResponse *response = GetResponseFromData(request);
  unsigned int response_size;
  uint8_t *response_packet = PackResponse(response, &response_size);

  // first verify a checksum error is detected
  // twiddle the final bit so that the checksum fails
  response_packet[response_size - 1] += 1;
  packets.push_back(string(reinterpret_cast<char*>(response_packet + 2),
                           response_size - 2));

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      expected_packet,
      size,
      RDM_REQUEST_LABEL,
      response_packet,
      response_size);

  m_arduino->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_CHECKSUM_INCORRECT,
                             packets));
  m_widget.Verify();
  packets.clear();
  delete[] response_packet;
  delete response;
  delete[] expected_packet;

  // now try a packet which is too short
  request = NewRequest(DESTINATION);
  expected_packet = PackRequest(request, &size);

  response = GetResponseFromData(request);
  response_packet = PackResponse(response, &response_size);

  // only return the first 10 bytes of the response
  packets.push_back(string(reinterpret_cast<char*>(response_packet + 2), 8));

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      expected_packet,
      size,
      RDM_REQUEST_LABEL,
      response_packet,
      10);

  m_arduino->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_PACKET_TOO_SHORT,
                             packets));

  m_widget.Verify();
  packets.clear();
  delete[] response_packet;
  delete response;
  delete[] expected_packet;

  // now test a transaction # mismatch
  request = NewRequest(DESTINATION);
  expected_packet = PackRequest(request, &size);

  response = GetResponseFromData(request);
  response_packet = PackResponse(response, &response_size);
  response_packet[16] += 1;
  response_packet[response_size - 1] += 1;
  packets.push_back(string(reinterpret_cast<char*>(response_packet + 2),
                           response_size - 2));

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      expected_packet,
      size,
      RDM_REQUEST_LABEL,
      response_packet,
      response_size);

  m_arduino->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_TRANSACTION_MISMATCH,
                             packets));

  m_widget.Verify();
  packets.clear();
  delete[] response_packet;
  delete response;
  delete[] expected_packet;

  // sub device mismatch
  request = NewRequest(DESTINATION);
  expected_packet = PackRequest(request, &size);

  response = GetResponseFromData(request);
  response_packet = PackResponse(response, &response_size);
  response_packet[20] += 1;
  response_packet[response_size - 1] += 1;
  packets.push_back(string(reinterpret_cast<char*>(response_packet + 2),
                           response_size - 2));

  m_widget.AddExpectedCall(
      RDM_REQUEST_LABEL,
      expected_packet,
      size,
      RDM_REQUEST_LABEL,
      response_packet,
      response_size);

  m_arduino->SendRDMRequest(
      request,
      ola::NewSingleCallback(this,
                             &ArduinoWidgetTest::ValidateStatus,
                             ola::rdm::RDM_SUB_DEVICE_MISMATCH,
                             packets));

  m_widget.Verify();
  packets.clear();
  delete[] response_packet;
  delete response;
  delete[] expected_packet;
}


/*
 * Check some of the error conditions
 */
void ArduinoWidgetTest::testErrorConditions() {
  vector<string> packets;

  uint8_t ERROR_CODES[] = {2, 3, 4, 5};
  for (unsigned int i = 0; i < sizeof(ERROR_CODES); ++i) {
    const RDMRequest *request = NewRequest(DESTINATION);
    unsigned int size;
    uint8_t *expected_packet = PackRequest(request, &size);

    m_widget.AddExpectedCall(
        RDM_REQUEST_LABEL,
        expected_packet,
        size,
        RDM_REQUEST_LABEL,
        ERROR_CODES + i,
        sizeof(ERROR_CODES[0]));

    m_arduino->SendRDMRequest(
        request,
        ola::NewSingleCallback(this,
                               &ArduinoWidgetTest::ValidateStatus,
                               ola::rdm::RDM_FAILED_TO_SEND,
                               packets));
    delete[] expected_packet;
  }
  m_widget.Verify();
}
