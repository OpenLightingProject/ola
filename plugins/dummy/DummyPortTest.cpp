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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DummyPortTest.cpp
 * Test fixture for the E131PDU class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include "plugins/dummy/DummyPort.h"  //  NOLINT, this has to be first
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>
#include <vector>

#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/testing/TestUtils.h"


namespace ola {
namespace plugin {
namespace dummy {

using ola::network::HostToNetwork;
using ola::rdm::RDMGetRequest;
using ola::rdm::RDMGetResponse;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::RDMSetResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;

class MockDummyPort: public DummyPort {
  public:
    MockDummyPort():
      DummyPort(NULL, 0, 10, 0) {
    }
};


class DummyPortTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DummyPortTest);
  CPPUNIT_TEST(testRDMDiscovery);
  CPPUNIT_TEST(testUnknownPid);
  CPPUNIT_TEST(testSupportedParams);
  CPPUNIT_TEST(testDeviceInfo);
  CPPUNIT_TEST(testSoftwareVersion);
  CPPUNIT_TEST(testDmxAddress);
  CPPUNIT_TEST(testIdentifyDevice);
  CPPUNIT_TEST_SUITE_END();

  public:
    DummyPortTest():
      TestFixture(),
      m_expected_uid(0x7a70, 0xffffff00),
      m_test_source(1, 2) {
        ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

    void setUp() {
      m_expected_response = NULL;
      m_got_uids = false;
    }
    void HandleRDMResponse(ola::rdm::rdm_response_code code,
                           const RDMResponse *response,
                           const vector<string> &packets);
    void SetExpectedResponse(ola::rdm::rdm_response_code code,
                             const RDMResponse *response);
    void Verify() { OLA_ASSERT_FALSE(m_expected_response); }

    void testRDMDiscovery();
    void testUnknownPid();
    void testSupportedParams();
    void testDeviceInfo();
    void testSoftwareVersion();
    void testDmxAddress();
    void testIdentifyDevice();


  private:
    UID m_expected_uid;
    UID m_test_source;
    MockDummyPort m_port;
    ola::rdm::rdm_response_code m_expected_code;
    const RDMResponse *m_expected_response;
    bool m_got_uids;

    void VerifyUIDs(const UIDSet &uids);
    void checkSubDeviceOutOfRange(ola::rdm::rdm_pid pid);
    void checkMalformedRequest(ola::rdm::rdm_pid pid);
    void checkSetRequest(ola::rdm::rdm_pid pid);
    void checkNoBroadcastResponse(ola::rdm::rdm_pid pid);
};

CPPUNIT_TEST_SUITE_REGISTRATION(DummyPortTest);


void DummyPortTest::HandleRDMResponse(ola::rdm::rdm_response_code code,
                                      const ola::rdm::RDMResponse *response,
                                      const vector<string>&) {
  OLA_ASSERT_EQ(m_expected_code, code);
  if (m_expected_response)
    OLA_ASSERT(*m_expected_response == *response);
  else
    OLA_ASSERT_EQ(m_expected_response, response);
  delete response;
  delete m_expected_response;
  m_expected_response = NULL;
}

void DummyPortTest::SetExpectedResponse(ola::rdm::rdm_response_code code,
                                        const RDMResponse *response) {
  m_expected_code = code;
  m_expected_response = response;
}


/*
 * Check that RDM discovery works
 */
void DummyPortTest::testRDMDiscovery() {
  m_port.RunFullDiscovery(NewSingleCallback(this, &DummyPortTest::VerifyUIDs));
  OLA_ASSERT(m_got_uids);
}


/*
 * Check that unknown pids fail
 */
void DummyPortTest::testUnknownPid() {
  UID source(1, 2);

  RDMRequest *request = new RDMGetRequest(
      source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_COMMS_STATUS,  // param id
      NULL,  // data
      0);  // data length

  RDMResponse *response = NackWithReason(
      request,
      ola::rdm::NR_UNKNOWN_PID);

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();
}


/*
 * Check that the supported params command work
 */
void DummyPortTest::testSupportedParams() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_SUPPORTED_PARAMETERS,  // param id
      NULL,  // data
      0);  // data length

  uint16_t supported_params[] = {
    ola::rdm::PID_DEVICE_LABEL,
    ola::rdm::PID_FACTORY_DEFAULTS,
    ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    ola::rdm::PID_DMX_PERSONALITY,
    ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    ola::rdm::PID_MANUFACTURER_LABEL,
    ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    ola::rdm::PID_LAMP_STRIKES,
    ola::rdm::PID_REAL_TIME_CLOCK
  };

  for (unsigned int i = 0; i < sizeof(supported_params) / 2; i++)
    supported_params[i] = HostToNetwork(supported_params[i]);

  RDMResponse *response = GetResponseFromData(
    request,
    reinterpret_cast<uint8_t*>(&supported_params),
    sizeof(supported_params));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::PID_SUPPORTED_PARAMETERS);
  checkMalformedRequest(ola::rdm::PID_SUPPORTED_PARAMETERS);
  checkSetRequest(ola::rdm::PID_SUPPORTED_PARAMETERS);
  checkNoBroadcastResponse(ola::rdm::PID_SUPPORTED_PARAMETERS);
}


/*
 * Check that the device info command works
 */
void DummyPortTest::testDeviceInfo() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_DEVICE_INFO,  // param id
      NULL,  // data
      0);  // data length

  ola::rdm::DeviceDescriptor device_descriptor;
  device_descriptor.protocol_version_high = 1;
  device_descriptor.protocol_version_low = 0;
  device_descriptor.device_model = HostToNetwork(static_cast<uint16_t>(1));
  device_descriptor.product_category = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::PRODUCT_CATEGORY_OTHER));
  device_descriptor.software_version = HostToNetwork(static_cast<uint32_t>(1));
  device_descriptor.dmx_footprint =
    HostToNetwork(static_cast<uint16_t>(5));
  device_descriptor.current_personality = 2;
  device_descriptor.personaility_count = 4;
  device_descriptor.dmx_start_address =
    HostToNetwork(static_cast<uint16_t>(1));
  device_descriptor.sub_device_count = 0;
  device_descriptor.sensor_count = 0;

  RDMResponse *response = GetResponseFromData(
    request,
    reinterpret_cast<uint8_t*>(&device_descriptor),
    sizeof(device_descriptor));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::PID_DEVICE_INFO);
  checkMalformedRequest(ola::rdm::PID_DEVICE_INFO);
  checkSetRequest(ola::rdm::PID_DEVICE_INFO);
  checkNoBroadcastResponse(ola::rdm::PID_DEVICE_INFO);
}


/*
 * Check that the software version command works
 */
void DummyPortTest::testSoftwareVersion() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_SOFTWARE_VERSION_LABEL,  // param id
      NULL,  // data
      0);  // data length

  const string version = "Dummy Software Version";
  RDMResponse *response = GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(version.data()),
    version.size());

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::PID_SOFTWARE_VERSION_LABEL);
  checkMalformedRequest(ola::rdm::PID_SOFTWARE_VERSION_LABEL);
  checkSetRequest(ola::rdm::PID_SOFTWARE_VERSION_LABEL);
  checkNoBroadcastResponse(ola::rdm::PID_SOFTWARE_VERSION_LABEL);
}


/*
 * Check that the dmx address command works
 */
void DummyPortTest::testDmxAddress() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_DMX_START_ADDRESS,  // param id
      NULL,  // data
      0);  // data length

  uint16_t dmx_address = HostToNetwork(static_cast<uint16_t>(1));
  RDMResponse *response = GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&dmx_address),
    sizeof(dmx_address));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // now attempt to set it
  dmx_address = HostToNetwork(static_cast<uint16_t>(99));
  request = new RDMSetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      2,  // message count
      0,  // sub device
      ola::rdm::PID_DMX_START_ADDRESS,  // param id
      reinterpret_cast<const uint8_t*>(&dmx_address),
      sizeof(dmx_address));

  response = GetResponseFromData(request, NULL, 0);
  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // now check it updated
  request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_DMX_START_ADDRESS,  // param id
      NULL,  // data
      0);  // data length

  dmx_address = HostToNetwork(static_cast<uint16_t>(99));
  response = GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&dmx_address),
    sizeof(dmx_address));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // check that broadcasting changes the address
  dmx_address = HostToNetwork(static_cast<uint16_t>(48));
  UID broadcast_uid = UID::AllManufactureDevices(OPEN_LIGHTING_ESTA_CODE);
  request = new RDMSetRequest(
      m_test_source,
      broadcast_uid,
      0,  // transaction #
      1,  // port id
      3,  // message count
      0,  // sub device
      ola::rdm::PID_DMX_START_ADDRESS,  // param id
      reinterpret_cast<const uint8_t*>(&dmx_address),
      sizeof(dmx_address));

  // no response expected
  SetExpectedResponse(ola::rdm::RDM_WAS_BROADCAST, NULL);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // now check it updated
  request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_DMX_START_ADDRESS,  // param id
      NULL,  // data
      0);  // data length

  dmx_address = HostToNetwork(static_cast<uint16_t>(48));
  response = GetResponseFromData(
    request,
    reinterpret_cast<const uint8_t*>(&dmx_address),
    sizeof(dmx_address));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::PID_DMX_START_ADDRESS);
  checkMalformedRequest(ola::rdm::PID_DMX_START_ADDRESS);
  checkNoBroadcastResponse(ola::rdm::PID_DMX_START_ADDRESS);
}


/*
 * Check that the identify mode works
 */
void DummyPortTest::testIdentifyDevice() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_IDENTIFY_DEVICE,  // param id
      NULL,  // data
      0);  // data length

  uint8_t mode = 0;
  RDMResponse *response = GetResponseFromData(
    request,
    &mode,
    sizeof(mode));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // now attempt to set it
  uint8_t new_mode = 1;
  request = new RDMSetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      2,  // message count
      0,  // sub device
      ola::rdm::PID_IDENTIFY_DEVICE,  // param id
      &new_mode,
      sizeof(new_mode));

  response = GetResponseFromData(request, NULL, 0);
  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // now check it updated
  request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_IDENTIFY_DEVICE,  // param id
      NULL,  // data
      0);  // data length

  mode = 1;
  response = GetResponseFromData(
    request,
    &mode,
    sizeof(mode));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // check that broadcasting changes the address
  new_mode = 0;
  UID broadcast_uid = UID::AllManufactureDevices(OPEN_LIGHTING_ESTA_CODE);
  request = new RDMSetRequest(
      m_test_source,
      broadcast_uid,
      0,  // transaction #
      1,  // port id
      3,  // message count
      0,  // sub device
      ola::rdm::PID_IDENTIFY_DEVICE,  // param id
      &new_mode,
      sizeof(new_mode));

  // no response expected
  SetExpectedResponse(ola::rdm::RDM_WAS_BROADCAST, NULL);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // now check it updated
  request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      ola::rdm::PID_IDENTIFY_DEVICE,  // param id
      NULL,  // data
      0);  // data length

  mode = 0;
  response = GetResponseFromData(
    request,
    &mode,
    sizeof(mode));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::PID_IDENTIFY_DEVICE);
  checkMalformedRequest(ola::rdm::PID_IDENTIFY_DEVICE);
  checkNoBroadcastResponse(ola::rdm::PID_IDENTIFY_DEVICE);
}



void DummyPortTest::VerifyUIDs(const UIDSet &uids) {
  UIDSet expected_uids;
  for (unsigned int i = 0; i < 10; i++) {
    UID uid(OPEN_LIGHTING_ESTA_CODE, DummyPort::kStartAddress + i);
    expected_uids.AddUID(uid);
  }
  OLA_ASSERT_EQ(expected_uids, uids);
  m_got_uids = true;
}


void DummyPortTest::checkSubDeviceOutOfRange(ola::rdm::rdm_pid pid) {
  // a request with a non-0 subdevice
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      1,  // sub device
      pid,
      NULL,  // data
      0);  // data length

  RDMResponse *response = NackWithReason(
      request,
      ola::rdm::NR_SUB_DEVICE_OUT_OF_RANGE);
  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();
}


void DummyPortTest::checkMalformedRequest(ola::rdm::rdm_pid pid) {
  // a malformed request
  uint16_t bad_data = 0;
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      pid,
      reinterpret_cast<uint8_t*>(&bad_data),  // data
      sizeof(bad_data));  // data length

  RDMResponse *response = NackWithReason(
      request,
      ola::rdm::NR_FORMAT_ERROR);
  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();
}


void DummyPortTest::checkSetRequest(ola::rdm::rdm_pid pid) {
  // a set request
  RDMRequest *request = new RDMSetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      pid,
      NULL,  // data
      0);  // data length

  RDMResponse *response = NackWithReason(
      request,
      ola::rdm::NR_UNSUPPORTED_COMMAND_CLASS);
  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();
}


void DummyPortTest::checkNoBroadcastResponse(ola::rdm::rdm_pid pid) {
  // a broadcast request
  UID broadcast_uid = UID::AllDevices();
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      broadcast_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      pid,  // param id
      NULL,  // data
      0);  // data length

  // we don't expect any response to this
  SetExpectedResponse(ola::rdm::RDM_WAS_BROADCAST, NULL);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  broadcast_uid = UID::AllManufactureDevices(OPEN_LIGHTING_ESTA_CODE);
  request = new RDMGetRequest(
      m_test_source,
      broadcast_uid,
      0,  // transaction #
      1,  // port id
      0,  // message count
      0,  // sub device
      pid,  // param id
      NULL,  // data
      0);  // data length

  // we don't expect any response to this
  SetExpectedResponse(ola::rdm::RDM_WAS_BROADCAST, NULL);
  m_port.SendRDMRequest(
        request,
        NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();
}
}  // dummy
}  // ola
}  // plugin
