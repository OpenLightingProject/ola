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
 * DummyPortTest.cpp
 * Test class for the dummy responders
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <algorithm>
#include <string>
#include <vector>

#include "common/rdm/TestHelper.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/network/Interface.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/OpenLightingEnums.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/RDMControllerInterface.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/testing/TestUtils.h"
#include "plugins/dummy/DummyPort.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::network::HostToNetwork;
using ola::network::Interface;
using ola::rdm::RDMGetRequest;
using ola::rdm::RDMGetResponse;
using ola::rdm::RDMReply;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using ola::rdm::RDMSetRequest;
using ola::rdm::RDMSetResponse;
using ola::rdm::UID;
using ola::rdm::UIDSet;
using std::min;
using std::string;
using std::vector;

class MockDummyPort: public DummyPort {
 public:
  MockDummyPort()
      : DummyPort(NULL, DummyPort::Options(), 0) {
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
  CPPUNIT_TEST(testParamDescription);
  CPPUNIT_TEST(testOlaManufacturerPidCodeVersion);
  CPPUNIT_TEST(testSlotInfo);
  CPPUNIT_TEST(testListInterfaces);
  CPPUNIT_TEST_SUITE_END();

 public:
  DummyPortTest()
      : TestFixture(),
        m_expected_uid(0x7a70, 0xffffff00),
        m_network_expected_uid(0x7a70, 0xffffff05),
        m_test_source(1, 2) {
    ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  }

  void setUp() {
    m_expected_response = NULL;
    m_got_uids = false;
  }
  void HandleRDMResponse(RDMReply *reply);
  void SetExpectedResponse(ola::rdm::RDMStatusCode code,
                           const RDMResponse *response);
  void Verify() { OLA_ASSERT_FALSE(m_expected_response); }

  void testRDMDiscovery();
  void testUnknownPid();
  void testSupportedParams();
  void testDeviceInfo();
  void testSoftwareVersion();
  void testDmxAddress();
  void testIdentifyDevice();
  void testParamDescription();
  void testOlaManufacturerPidCodeVersion();
  void testSlotInfo();
  void testListInterfaces();

 private:
  UID m_expected_uid;
  UID m_network_expected_uid;
  UID m_test_source;
  MockDummyPort m_port;
  ola::rdm::RDMStatusCode m_expected_code;
  const RDMResponse *m_expected_response;
  bool m_got_uids;

  void VerifyUIDs(const UIDSet &uids);
  void checkSubDeviceOutOfRange(uint16_t pid);
  void checkSubDeviceOutOfRange(ola::rdm::rdm_pid pid) {
    checkSubDeviceOutOfRange(static_cast<uint16_t>(pid));
  }
  void checkSubDeviceOutOfRange(ola::rdm::rdm_ola_manufacturer_pid pid) {
    checkSubDeviceOutOfRange(static_cast<uint16_t>(pid));
  }
  void checkMalformedRequest(uint16_t pid,
                             ola::rdm::rdm_nack_reason expected_response =
                             ola::rdm::NR_FORMAT_ERROR);
  void checkMalformedRequest(ola::rdm::rdm_pid pid,
                             ola::rdm::rdm_nack_reason expected_response =
                             ola::rdm::NR_FORMAT_ERROR) {
    checkMalformedRequest(static_cast<uint16_t>(pid), expected_response);
  }
  void checkMalformedRequest(ola::rdm::rdm_ola_manufacturer_pid pid,
                             ola::rdm::rdm_nack_reason expected_response =
                             ola::rdm::NR_FORMAT_ERROR) {
    checkMalformedRequest(static_cast<uint16_t>(pid), expected_response);
  }
  void checkSetRequest(uint16_t pid);
  void checkSetRequest(ola::rdm::rdm_pid pid) {
    checkSetRequest(static_cast<uint16_t>(pid));
  }
  void checkSetRequest(ola::rdm::rdm_ola_manufacturer_pid pid) {
    checkSetRequest(static_cast<uint16_t>(pid));
  }
  void checkNoBroadcastResponse(uint16_t pid);
  void checkNoBroadcastResponse(ola::rdm::rdm_pid pid) {
    checkNoBroadcastResponse(static_cast<uint16_t>(pid));
  }
  void checkNoBroadcastResponse(ola::rdm::rdm_ola_manufacturer_pid pid) {
    checkNoBroadcastResponse(static_cast<uint16_t>(pid));
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(DummyPortTest);


void DummyPortTest::HandleRDMResponse(RDMReply *reply) {
  OLA_ASSERT_EQ(m_expected_code, reply->StatusCode());
  if (m_expected_response) {
    OLA_ASSERT(reply->Response());
    // Check the param data explicitly first, as it's most likely to be wrong
    // and will make developers lives easier
    OLA_ASSERT_DATA_EQUALS(m_expected_response->ParamData(),
                           m_expected_response->ParamDataSize(),
                           reply->Response()->ParamData(),
                           reply->Response()->ParamDataSize());
    OLA_ASSERT_TRUE(*m_expected_response == *reply->Response());
  } else {
    OLA_ASSERT_NULL(reply->Response());
  }
  delete m_expected_response;
  m_expected_response = NULL;
}

void DummyPortTest::SetExpectedResponse(ola::rdm::RDMStatusCode code,
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
 * Check that the supported params command works
 */
void DummyPortTest::testSupportedParams() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_SUPPORTED_PARAMETERS,  // param id
      NULL,  // data
      0);  // data length

  uint16_t supported_params[] = {
    ola::rdm::PID_PRODUCT_DETAIL_ID_LIST,
    ola::rdm::PID_DEVICE_MODEL_DESCRIPTION,
    ola::rdm::PID_MANUFACTURER_LABEL,
    ola::rdm::PID_DEVICE_LABEL,
    ola::rdm::PID_FACTORY_DEFAULTS,
    ola::rdm::PID_DMX_PERSONALITY,
    ola::rdm::PID_DMX_PERSONALITY_DESCRIPTION,
    ola::rdm::PID_SLOT_INFO,
    ola::rdm::PID_SLOT_DESCRIPTION,
    ola::rdm::PID_DEFAULT_SLOT_VALUE,
#ifdef HAVE_GETLOADAVG
    ola::rdm::PID_SENSOR_DEFINITION,
    ola::rdm::PID_SENSOR_VALUE,
    ola::rdm::PID_RECORD_SENSORS,
#endif  // HAVE_GETLOADAVG
    ola::rdm::PID_LAMP_STRIKES,
    ola::rdm::PID_REAL_TIME_CLOCK,
    ola::rdm::PID_LIST_INTERFACES,
    ola::rdm::PID_INTERFACE_LABEL,
    ola::rdm::PID_INTERFACE_HARDWARE_ADDRESS_TYPE1,
    ola::rdm::PID_IPV4_CURRENT_ADDRESS,
    ola::rdm::PID_IPV4_DEFAULT_ROUTE,
    ola::rdm::PID_DNS_NAME_SERVER,
    ola::rdm::PID_DNS_HOSTNAME,
    ola::rdm::PID_DNS_DOMAIN_NAME,
    ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION,
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
      0,  // sub device
      ola::rdm::PID_DEVICE_INFO,  // param id
      NULL,  // data
      0);  // data length

  ola::rdm::DeviceDescriptor device_descriptor;
  device_descriptor.protocol_version_high = 1;
  device_descriptor.protocol_version_low = 0;
  device_descriptor.device_model = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::OLA_DUMMY_DEVICE_MODEL));
  device_descriptor.product_category = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::PRODUCT_CATEGORY_OTHER));
  device_descriptor.software_version = HostToNetwork(static_cast<uint32_t>(3));
  device_descriptor.dmx_footprint =
    HostToNetwork(static_cast<uint16_t>(5));
  device_descriptor.current_personality = 2;
  device_descriptor.personality_count = 4;
  device_descriptor.dmx_start_address =
    HostToNetwork(static_cast<uint16_t>(1));
  device_descriptor.sub_device_count = 0;
#ifdef HAVE_GETLOADAVG
  device_descriptor.sensor_count = 3;
#else
  device_descriptor.sensor_count = 0;
#endif  // HAVE_GETLOADAVG

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
  UID broadcast_uid = UID::VendorcastAddress(OPEN_LIGHTING_ESTA_CODE);
  request = new RDMSetRequest(
      m_test_source,
      broadcast_uid,
      0,  // transaction #
      1,  // port id
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

  // check that broadcasting changes the identify
  new_mode = 0;
  UID broadcast_uid = UID::VendorcastAddress(OPEN_LIGHTING_ESTA_CODE);
  request = new RDMSetRequest(
      m_test_source,
      broadcast_uid,
      0,  // transaction #
      1,  // port id
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


/*
 * Check that the param description command works
 */
void DummyPortTest::testParamDescription() {
  uint16_t param_id = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION));
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_PARAMETER_DESCRIPTION,  // param id
      reinterpret_cast<uint8_t*>(&param_id),  // data
      sizeof(param_id));  // data length

  PACK(
  struct parameter_description_s {
    uint16_t pid;
    uint8_t pdl_size;
    uint8_t data_type;
    uint8_t command_class;
    uint8_t type;
    uint8_t unit;
    uint8_t prefix;
    uint32_t min_value;
    uint32_t default_value;
    uint32_t max_value;
    char description[ola::rdm::MAX_RDM_STRING_LENGTH];
  });

  struct parameter_description_s param_description;
  param_description.pid = HostToNetwork(
      static_cast<uint16_t>(ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION));
  param_description.pdl_size = HostToNetwork(
      static_cast<uint8_t>(ola::rdm::MAX_RDM_STRING_LENGTH));
  param_description.data_type = HostToNetwork(
      static_cast<uint8_t>(ola::rdm::DS_ASCII));
  param_description.command_class = HostToNetwork(
      static_cast<uint8_t>(ola::rdm::CC_GET));
  param_description.type = 0;
  param_description.unit = HostToNetwork(
      static_cast<uint8_t>(ola::rdm::UNITS_NONE));
  param_description.prefix = HostToNetwork(
      static_cast<uint8_t>(ola::rdm::PREFIX_NONE));
  param_description.min_value = 0;
  param_description.default_value = 0;
  param_description.max_value = 0;

  const string description("Code Version");
  size_t str_len = std::min(sizeof(param_description.description),
                            description.size());
  strncpy(param_description.description, description.c_str(), str_len);

  unsigned int param_data_length = (
      sizeof(param_description) - sizeof(param_description.description) +
      str_len);

  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&param_description), param_data_length);

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
      request,
      NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  // Alternative PID
  // This PID should be one the device won't respond to for the check to work
  uint16_t unknown_param_id = HostToNetwork(static_cast<uint16_t>(0xFFDF));
  request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_PARAMETER_DESCRIPTION,  // param id
      reinterpret_cast<uint8_t*>(&unknown_param_id),  // data
      sizeof(unknown_param_id));  // data length

  response = NackWithReason(request, ola::rdm::NR_DATA_OUT_OF_RANGE);
  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
      request,
      NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::PID_PARAMETER_DESCRIPTION);
  // We don't get the normal format error here, as we're expecting data anyway
  checkMalformedRequest(ola::rdm::PID_PARAMETER_DESCRIPTION,
                        ola::rdm::NR_DATA_OUT_OF_RANGE);
  checkSetRequest(ola::rdm::PID_PARAMETER_DESCRIPTION);
  checkNoBroadcastResponse(ola::rdm::PID_PARAMETER_DESCRIPTION);
}


/*
 * Check that the OLA manufacturer PID code version works
 */
void DummyPortTest::testOlaManufacturerPidCodeVersion() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION,  // param id
      NULL,  // data
      0);  // data length

  const string code_version = VERSION;
  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<const uint8_t*>(code_version.data()),
      code_version.size());

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
      request,
      NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION);
  checkMalformedRequest(ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION);
  checkSetRequest(ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION);
  checkNoBroadcastResponse(ola::rdm::OLA_MANUFACTURER_PID_CODE_VERSION);
}


/*
 * Check that the slot info command works
 */
void DummyPortTest::testSlotInfo() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_SLOT_INFO,  // param id
      NULL,  // data
      0);  // data length

  PACK(
  struct slot_info_struct {
    uint16_t offset;
    uint8_t type;
    uint16_t label;
  });

  PACK(
  struct slot_infos_s {
    slot_info_struct slot_info_s[5];
  });

  slot_infos_s slot_infos = {
      {
        {HostToNetwork(static_cast<uint16_t>(0)),
         static_cast<uint8_t>(ola::rdm::ST_PRIMARY),
         HostToNetwork(static_cast<uint16_t>(ola::rdm::SD_INTENSITY))},
        {HostToNetwork(static_cast<uint16_t>(1)),
         static_cast<uint8_t>(ola::rdm::ST_SEC_FINE),
         HostToNetwork(static_cast<uint16_t>(0))},
        {HostToNetwork(static_cast<uint16_t>(2)),
         static_cast<uint8_t>(ola::rdm::ST_PRIMARY),
         HostToNetwork(static_cast<uint16_t>(ola::rdm::SD_PAN))},
        {HostToNetwork(static_cast<uint16_t>(3)),
         static_cast<uint8_t>(ola::rdm::ST_PRIMARY),
         HostToNetwork(static_cast<uint16_t>(ola::rdm::SD_TILT))},
        {HostToNetwork(static_cast<uint16_t>(4)),
         static_cast<uint8_t>(ola::rdm::ST_PRIMARY),
         HostToNetwork(static_cast<uint16_t>(ola::rdm::SD_UNDEFINED))}
      }};

  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&slot_infos),
      sizeof(slot_infos));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
      request,
      NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::PID_SLOT_INFO);
  checkMalformedRequest(ola::rdm::PID_SLOT_INFO);
  checkSetRequest(ola::rdm::PID_SLOT_INFO);
  checkNoBroadcastResponse(ola::rdm::PID_SLOT_INFO);
}


/*
 * Check that the list interface command works
 */
void DummyPortTest::testListInterfaces() {
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_network_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      ola::rdm::PID_LIST_INTERFACES,  // param id
      NULL,  // data
      0);  // data length

  PACK(
  struct list_interface_struct {
    uint32_t interface_identifier;
    uint16_t interface_hardware_type;
  });

  PACK(
  struct list_interfaces_s {
    list_interface_struct list_interface_s[2];
  });

  list_interfaces_s list_interfaces = {
      {
        {HostToNetwork(static_cast<uint32_t>(1)),
         HostToNetwork(static_cast<uint16_t>(Interface::ARP_ETHERNET_TYPE))},
        {HostToNetwork(static_cast<uint32_t>(2)),
         HostToNetwork(static_cast<uint16_t>(Interface::ARP_ETHERNET_TYPE))}
      }};

  RDMResponse *response = GetResponseFromData(
      request,
      reinterpret_cast<uint8_t*>(&list_interfaces),
      sizeof(list_interfaces));

  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
      request,
      NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();

  checkSubDeviceOutOfRange(ola::rdm::PID_LIST_INTERFACES);
  checkMalformedRequest(ola::rdm::PID_LIST_INTERFACES);
  checkSetRequest(ola::rdm::PID_LIST_INTERFACES);
  checkNoBroadcastResponse(ola::rdm::PID_LIST_INTERFACES);
}


void DummyPortTest::VerifyUIDs(const UIDSet &uids) {
  UIDSet expected_uids;
  for (unsigned int i = 0; i < 6; i++) {
    UID uid(OPEN_LIGHTING_ESTA_CODE, 0xffffff00 + i);
    expected_uids.AddUID(uid);
  }
  OLA_ASSERT_EQ(expected_uids, uids);
  m_got_uids = true;
}


void DummyPortTest::checkSubDeviceOutOfRange(uint16_t pid) {
  // a request with a non-0 subdevice
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
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


void DummyPortTest::checkMalformedRequest(uint16_t pid,
      ola::rdm::rdm_nack_reason expected_response) {
  // a malformed request
  uint16_t bad_data = 0;
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
      0,  // sub device
      pid,
      reinterpret_cast<uint8_t*>(&bad_data),  // data
      sizeof(bad_data));  // data length

  RDMResponse *response = NackWithReason(
      request,
      expected_response);
  SetExpectedResponse(ola::rdm::RDM_COMPLETED_OK, response);
  m_port.SendRDMRequest(
      request,
      NewSingleCallback(this, &DummyPortTest::HandleRDMResponse));
  Verify();
}


void DummyPortTest::checkSetRequest(uint16_t pid) {
  // a set request
  RDMRequest *request = new RDMSetRequest(
      m_test_source,
      m_expected_uid,
      0,  // transaction #
      1,  // port id
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


void DummyPortTest::checkNoBroadcastResponse(uint16_t pid) {
  // a broadcast request
  UID broadcast_uid = UID::AllDevices();
  RDMRequest *request = new RDMGetRequest(
      m_test_source,
      broadcast_uid,
      0,  // transaction #
      1,  // port id
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

  broadcast_uid = UID::VendorcastAddress(OPEN_LIGHTING_ESTA_CODE);
  request = new RDMGetRequest(
      m_test_source,
      broadcast_uid,
      0,  // transaction #
      1,  // port id
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
}  // namespace dummy
}  // namespace plugin
}  // namespace ola
