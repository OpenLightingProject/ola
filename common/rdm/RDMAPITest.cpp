/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * RDMAPITest.cpp
 * Test fixture for the RDM API class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <deque>
#include <string>
#include <vector>

#include "ola/StringUtils.h"
#include "ola/base/Macro.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/RDMAPIImplInterface.h"
#include "ola/testing/TestUtils.h"



using ola::NewSingleCallback;
using ola::network::HostToNetwork;
using ola::rdm::RDMAPI;
using ola::rdm::ResponseStatus;
using ola::rdm::ResponseStatus;
using ola::rdm::UID;
using std::deque;
using std::string;
using std::vector;


class ExpectedResult {
 public:
    unsigned int universe;
    const UID uid;
    uint16_t sub_device;
    uint16_t pid;
    const string return_data;
    const uint8_t *data;
    unsigned int data_length;

    ExpectedResult(unsigned int universe,
                   const UID &uid,
                   uint16_t sub_device,
                   uint16_t pid,
                   const string &return_data,
                   const uint8_t *data_arg = NULL,
                   unsigned int data_length = 0):
      universe(universe),
      uid(uid),
      sub_device(sub_device),
      pid(pid),
      return_data(return_data),
      data(NULL),
      data_length(data_length) {
    if (data_arg) {
      uint8_t *d = new uint8_t[data_length];
      memcpy(d, data_arg, data_length);
      data = d;
    }
  }

  ~ExpectedResult() {
    if (data)
      delete[] data;
  }
};


class MockRDMAPIImpl: public ola::rdm::RDMAPIImplInterface {
 public:
    bool RDMGet(rdm_callback *callback,
                unsigned int universe,
                const UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data = NULL,
                unsigned int data_length = 0) {
      OLA_ASSERT_TRUE(m_get_expected.size());
      const ExpectedResult *result = m_get_expected.front();
      OLA_ASSERT_EQ(result->universe, universe);
      OLA_ASSERT_EQ(result->uid, uid);
      OLA_ASSERT_EQ(result->sub_device, sub_device);
      OLA_ASSERT_EQ(result->pid, pid);

      (void) data;
      (void) data_length;

      ResponseStatus status;
      status.response_code = uid.IsBroadcast() ? ola::rdm::RDM_WAS_BROADCAST:
        ola::rdm::RDM_COMPLETED_OK;
      status.response_type = ola::rdm::RDM_ACK;
      status.message_count = 0;
      callback->Run(status, result->return_data);

      delete result;
      m_get_expected.pop_front();
      return true;
    }

    bool RDMGet(rdm_pid_callback *callback,
                unsigned int universe,
                const UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data = NULL,
                unsigned int data_length = 0) {
      OLA_ASSERT_TRUE(m_get_expected.size());
      const ExpectedResult *result = m_get_expected.front();
      OLA_ASSERT_EQ(result->universe, universe);
      OLA_ASSERT_EQ(result->uid, uid);
      OLA_ASSERT_EQ(result->sub_device, sub_device);
      OLA_ASSERT_EQ(result->pid, pid);

      (void) data;
      (void) data_length;

      ResponseStatus status;
      status.response_code = uid.IsBroadcast() ? ola::rdm::RDM_WAS_BROADCAST:
        ola::rdm::RDM_COMPLETED_OK;
      status.response_type = ola::rdm::RDM_ACK;
      status.message_count = 0;
      callback->Run(status, pid, result->return_data);

      delete result;
      m_get_expected.pop_front();
      return true;
    }

    bool RDMSet(rdm_callback *callback,
                unsigned int universe,
                const UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data = NULL,
                unsigned int data_length = 0) {
      OLA_ASSERT_TRUE(m_set_expected.size());
      const ExpectedResult *result = m_set_expected.front();
      OLA_ASSERT_EQ(result->universe, universe);
      OLA_ASSERT_EQ(result->uid, uid);
      OLA_ASSERT_EQ(result->sub_device, sub_device);
      OLA_ASSERT_EQ(result->pid, pid);

      if (result->data) {
        OLA_ASSERT_EQ(result->data_length, data_length);
        OLA_ASSERT_EQ(0, memcmp(result->data, data, data_length));
      }

      ResponseStatus status;
      status.response_code = uid.IsBroadcast() ? ola::rdm::RDM_WAS_BROADCAST:
        ola::rdm::RDM_COMPLETED_OK;
      status.response_type = ola::rdm::RDM_ACK;
      status.message_count = 0;
      callback->Run(status, result->return_data);

      delete result;
      m_set_expected.pop_front();
      return true;
    }

    void AddExpectedGet(
                        // const ResponseStatus &status,
                        const string &return_data,
                        unsigned int universe,
                        const UID &uid,
                        uint16_t sub_device,
                        uint16_t pid,
                        const uint8_t *data = NULL,
                        unsigned int data_length = 0) {
      ExpectedResult *expected_result = new ExpectedResult(universe,
                                                           uid,
                                                           sub_device,
                                                           pid,
                                                           return_data,
                                                           data,
                                                           data_length);
      m_get_expected.push_back(expected_result);
    }

    void AddExpectedSet(unsigned int universe,
                        const UID &uid,
                        uint16_t sub_device,
                        uint16_t pid,
                        const uint8_t *data = NULL,
                        unsigned int data_length = 0) {
      string s;
      ExpectedResult *expected_result = new ExpectedResult(universe,
                                                           uid,
                                                           sub_device,
                                                           pid,
                                                           s,
                                                           data,
                                                           data_length);
      m_set_expected.push_back(expected_result);
    }

    void Verify() {
      OLA_ASSERT_FALSE(m_get_expected.size());
      OLA_ASSERT_FALSE(m_set_expected.size());
    }

 private:
    deque<ExpectedResult*> m_get_expected;
    deque<ExpectedResult*> m_set_expected;
};


class RDMAPITest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMAPITest);
  CPPUNIT_TEST(testProxyCommands);
  CPPUNIT_TEST(testNetworkCommands);
  CPPUNIT_TEST(testRDMInformation);
  CPPUNIT_TEST(testProductInformation);
  CPPUNIT_TEST(testDmxSetup);
  CPPUNIT_TEST_SUITE_END();

 public:
    RDMAPITest():
      m_api(&m_impl),
      m_uid(1, 2),
      m_bcast_uid(UID::AllDevices()),
      m_group_uid(UID::VendorcastAddress(52)),
      m_test_uid1(4, 5),
      m_test_uid2(7, 9) {
    }
    void testProxyCommands();
    void testNetworkCommands();
    void testRDMInformation();
    void testProductInformation();
    void testDmxSetup();
    void setUp() {}
    void tearDown();

 private:
    static const unsigned int UNIVERSE = 1;
    static const char BROADCAST_ERROR[];
    static const char DEVICE_RANGE_ERROR[];
    static const char DEVICE_RANGE_BCAST_ERROR[];
    static const char TEST_DESCRIPTION[];
    MockRDMAPIImpl m_impl;
    RDMAPI m_api;
    UID m_uid;
    UID m_bcast_uid;
    UID m_group_uid;
    UID m_test_uid1;
    UID m_test_uid2;

    // check that a RDM call failed because we tried to send to the bcast UID
    void CheckForBroadcastError(string *error) {
      OLA_ASSERT_EQ(string(BROADCAST_ERROR), *error);
      error->clear();
    }

    // check that a RDM call failed because we tried to send to all devices
    void CheckForDeviceRangeError(string *error) {
      OLA_ASSERT_EQ(string(DEVICE_RANGE_ERROR), *error);
      error->clear();
    }

    void CheckForDeviceRangeBcastError(string *error) {
      OLA_ASSERT_EQ(string(DEVICE_RANGE_BCAST_ERROR), *error);
      error->clear();
    }

    // check that a RDM command was successful
    void CheckResponseStatus(const ResponseStatus &status) {
      OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK, status.response_code);
    }

    // check that a RDM command was successful and broadcast
    void CheckWasBroadcast(const ResponseStatus &status) {
      OLA_ASSERT_EQ(ola::rdm::RDM_WAS_BROADCAST, status.response_code);
    }

    void CheckProxiedDeviceCount(const ResponseStatus &status,
                                 uint16_t count,
                                 bool changed) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(static_cast<uint16_t>(2), count);
      OLA_ASSERT_EQ(false, changed);
    }

    void CheckProxiedDevices(const ResponseStatus &status,
                             const vector<UID> &devices) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(static_cast<size_t>(2), devices.size());
      OLA_ASSERT_EQ(m_test_uid1, devices[0]);
      OLA_ASSERT_EQ(m_test_uid2, devices[1]);
    }

    void CheckCommsStatus(const ResponseStatus &status,
                          uint16_t short_message,
                          uint16_t length_mismatch,
                          uint16_t checksum_fail) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(static_cast<uint16_t>(14), short_message);
      OLA_ASSERT_EQ(static_cast<uint16_t>(187), length_mismatch);
      OLA_ASSERT_EQ(static_cast<uint16_t>(92), checksum_fail);
    }

    void CheckLabel(const ResponseStatus &status,
                    const string &description) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(string(TEST_DESCRIPTION), description);
    }

    void CheckSupportedParams(const ResponseStatus &status,
                              const vector<uint16_t> &params) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(static_cast<size_t>(3), params.size());
      // params are sorted
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x00aa), params[0]);
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x1234), params[1]);
      OLA_ASSERT_EQ(static_cast<uint16_t>(0xabcd), params[2]);
    }

    void CheckParameterDescription(
        const ResponseStatus &status,
        const ola::rdm::ParameterDescriptor &description) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x1234), description.pid);
      OLA_ASSERT_EQ(static_cast<uint8_t>(10), description.pdl_size);
      OLA_ASSERT_EQ(static_cast<uint8_t>(ola::rdm::DS_UNSIGNED_DWORD),
                    description.data_type);
      OLA_ASSERT_EQ(static_cast<uint8_t>(ola::rdm::CC_GET),
                    description.command_class);
      OLA_ASSERT_EQ(static_cast<uint8_t>(ola::rdm::UNITS_METERS),
                    description.unit);
      OLA_ASSERT_EQ(static_cast<uint8_t>(ola::rdm::PREFIX_KILO),
                    description.prefix);
      OLA_ASSERT_EQ(static_cast<uint32_t>(0), description.min_value);
      OLA_ASSERT_EQ(static_cast<uint32_t>(200000),
                    description.max_value);
      OLA_ASSERT_EQ(static_cast<uint32_t>(1000),
                    description.default_value);
      OLA_ASSERT_EQ(string(TEST_DESCRIPTION).size(),
                    description.description.size());
      OLA_ASSERT_EQ(string(TEST_DESCRIPTION), description.description);
    }

    void CheckMalformedParameterDescription(
        const ResponseStatus &status,
        const ola::rdm::ParameterDescriptor &description) {
      OLA_ASSERT_EQ(ola::rdm::RDM_COMPLETED_OK,
                           status.response_code);
      (void) description;
    }

    void CheckDeviceInfo(const ResponseStatus &status,
                         const ola::rdm::DeviceDescriptor &descriptor) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(static_cast<uint8_t>(1),
                           descriptor.protocol_version_high);
      OLA_ASSERT_EQ(static_cast<uint8_t>(0),
                           descriptor.protocol_version_low);
      OLA_ASSERT_EQ(static_cast<uint16_t>(2),
                           descriptor.device_model);
      OLA_ASSERT_EQ(static_cast<uint16_t>(3),
                           descriptor.product_category);
      OLA_ASSERT_EQ(static_cast<uint32_t>(0x12345678),
                           descriptor.software_version);
      OLA_ASSERT_EQ(static_cast<uint16_t>(400),
                           descriptor.dmx_footprint);
      OLA_ASSERT_EQ(static_cast<uint8_t>(1),
                           descriptor.current_personality);
      OLA_ASSERT_EQ(static_cast<uint8_t>(2),
                           descriptor.personality_count);
      OLA_ASSERT_EQ(static_cast<uint16_t>(12),
                           descriptor.dmx_start_address);
      OLA_ASSERT_EQ(static_cast<uint16_t>(10),
                           descriptor.sub_device_count);
      OLA_ASSERT_EQ(static_cast<uint8_t>(4),
                           descriptor.sensor_count);
    }

    void CheckProductDetailList(const ResponseStatus &status,
                                const vector<uint16_t> &params) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(static_cast<size_t>(3), params.size());
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x5678), params[0]);
      OLA_ASSERT_EQ(static_cast<uint16_t>(0xfedc), params[1]);
      OLA_ASSERT_EQ(static_cast<uint16_t>(0xaa00), params[2]);
    }

    void CheckDMXStartAddress(const ResponseStatus &status,
                              uint16_t start_address) {
      CheckResponseStatus(status);
      OLA_ASSERT_EQ(static_cast<uint16_t>(44), start_address);
    }
};

const char RDMAPITest::BROADCAST_ERROR[] = "Cannot send to broadcast address";
const char RDMAPITest::DEVICE_RANGE_ERROR[] = "Sub device must be <= 0x0200";
const char RDMAPITest::DEVICE_RANGE_BCAST_ERROR[] =
    "Sub device must be <= 0x0200 or 0xffff";
const char RDMAPITest::TEST_DESCRIPTION[] = "This is a description";

void RDMAPITest::tearDown() {
  m_impl.Verify();
}

CPPUNIT_TEST_SUITE_REGISTRATION(RDMAPITest);


/*
 * Test the proxied commands work.
 */
void RDMAPITest::testProxyCommands() {
  string error;
  // get proxied device count
  OLA_ASSERT_FALSE(m_api.GetProxiedDeviceCount(
      UNIVERSE,
      m_bcast_uid,
      NewSingleCallback(this, &RDMAPITest::CheckProxiedDeviceCount),
      &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetProxiedDeviceCount(
      UNIVERSE,
      m_group_uid,
      NewSingleCallback(this, &RDMAPITest::CheckProxiedDeviceCount),
      &error));
  CheckForBroadcastError(&error);

  struct {
    uint16_t count;
    uint8_t changed;
  } count_response;
  count_response.count = HostToNetwork(static_cast<uint16_t>(2));
  count_response.changed = 0;
  string s(reinterpret_cast<char*>(&count_response), sizeof(count_response));
  m_impl.AddExpectedGet(s,
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_PROXIED_DEVICE_COUNT);
  OLA_ASSERT_TRUE(m_api.GetProxiedDeviceCount(
      UNIVERSE,
      m_uid,
      NewSingleCallback(this, &RDMAPITest::CheckProxiedDeviceCount),
      &error));

  // get proxied devices
  OLA_ASSERT_FALSE(m_api.GetProxiedDevices(
      UNIVERSE,
      m_bcast_uid,
      NewSingleCallback(this, &RDMAPITest::CheckProxiedDevices),
      &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetProxiedDevices(
      UNIVERSE,
      m_group_uid,
      NewSingleCallback(this, &RDMAPITest::CheckProxiedDevices),
      &error));
  CheckForBroadcastError(&error);

  struct {
    uint8_t uid1[UID::UID_SIZE];
    uint8_t uid2[UID::UID_SIZE];
  } uids_response;
  m_test_uid1.Pack(uids_response.uid1, UID::UID_SIZE);
  m_test_uid2.Pack(uids_response.uid2, UID::UID_SIZE);
  string s2(reinterpret_cast<char*>(&uids_response), sizeof(uids_response));
  m_impl.AddExpectedGet(s2,
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_PROXIED_DEVICES);
  OLA_ASSERT_TRUE(m_api.GetProxiedDevices(
      UNIVERSE,
      m_uid,
      NewSingleCallback(this, &RDMAPITest::CheckProxiedDevices),
      &error));
}


/*
 * test that network commands work
 */
void RDMAPITest::testNetworkCommands() {
  string error;
  // get comms status
  OLA_ASSERT_FALSE(m_api.GetCommStatus(
      UNIVERSE,
      m_bcast_uid,
      NewSingleCallback(this, &RDMAPITest::CheckCommsStatus),
      &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetCommStatus(
      UNIVERSE,
      m_group_uid,
      NewSingleCallback(this, &RDMAPITest::CheckCommsStatus),
      &error));
  CheckForBroadcastError(&error);

  struct {
    uint16_t short_message;
    uint16_t length_mismatch;
    uint16_t checksum_fail;
  } comms_response;
  comms_response.short_message = HostToNetwork(static_cast<uint16_t>(14));
  comms_response.length_mismatch = HostToNetwork(static_cast<uint16_t>(187));
  comms_response.checksum_fail = HostToNetwork(static_cast<uint16_t>(92));
  string s(reinterpret_cast<char*>(&comms_response), sizeof(comms_response));
  m_impl.AddExpectedGet(s,
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_COMMS_STATUS);
  OLA_ASSERT_TRUE(m_api.GetCommStatus(
      UNIVERSE,
      m_uid,
      NewSingleCallback(this, &RDMAPITest::CheckCommsStatus),
      &error));

  // clear comms status
  m_impl.AddExpectedSet(UNIVERSE,
                        m_bcast_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_COMMS_STATUS);
  OLA_ASSERT_TRUE(m_api.ClearCommStatus(
      UNIVERSE,
      m_bcast_uid,
      NewSingleCallback(this, &RDMAPITest::CheckWasBroadcast),
      &error));

  m_impl.AddExpectedSet(UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_COMMS_STATUS);
  OLA_ASSERT_TRUE(m_api.ClearCommStatus(
      UNIVERSE,
      m_uid,
      NewSingleCallback(this, &RDMAPITest::CheckResponseStatus),
      &error));

  // TODO(simon): test status message here

  // status id description
  uint16_t status_id = 12;
  OLA_ASSERT_FALSE(m_api.GetStatusIdDescription(
      UNIVERSE,
      m_bcast_uid,
      status_id,
      NewSingleCallback(this, &RDMAPITest::CheckLabel),
      &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetStatusIdDescription(
      UNIVERSE,
      m_group_uid,
      status_id,
      NewSingleCallback(this, &RDMAPITest::CheckLabel),
      &error));
  CheckForBroadcastError(&error);

  m_impl.AddExpectedGet(string(TEST_DESCRIPTION),
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_STATUS_ID_DESCRIPTION);
  OLA_ASSERT_TRUE(m_api.GetStatusIdDescription(
      UNIVERSE,
      m_uid,
      status_id,
      NewSingleCallback(this, &RDMAPITest::CheckLabel),
      &error));

  // clear status id
  uint16_t sub_device = 3;
  m_impl.AddExpectedSet(UNIVERSE,
                        m_bcast_uid,
                        sub_device,
                        ola::rdm::PID_CLEAR_STATUS_ID);
  OLA_ASSERT_TRUE(m_api.ClearStatusId(
      UNIVERSE,
      m_bcast_uid,
      sub_device,
      NewSingleCallback(this, &RDMAPITest::CheckWasBroadcast),
      &error));
  m_impl.AddExpectedSet(UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_CLEAR_STATUS_ID);
  OLA_ASSERT_TRUE(m_api.ClearStatusId(
      UNIVERSE,
      m_uid,
      ola::rdm::ROOT_RDM_DEVICE,
      NewSingleCallback(this, &RDMAPITest::CheckResponseStatus),
      &error));

  // TODO(simon): add sub device reporting threshold here
}


/*
 * Test RDM Information commands work correctly
 */
void RDMAPITest::testRDMInformation() {
  string error;
  // supported params
  struct {
    uint16_t param1;
    uint16_t param2;
    uint16_t param3;
  } pid_list;
  pid_list.param1 = HostToNetwork(static_cast<uint16_t>(0x1234));
  pid_list.param2 = HostToNetwork(static_cast<uint16_t>(0xabcd));
  pid_list.param3 = HostToNetwork(static_cast<uint16_t>(0x00aa));
  uint16_t sub_device = 1;
  OLA_ASSERT_FALSE(m_api.GetSupportedParameters(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckSupportedParams),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetSupportedParameters(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckSupportedParams),
    &error));
  CheckForBroadcastError(&error);
  string s(reinterpret_cast<char*>(&pid_list), sizeof(pid_list));
  m_impl.AddExpectedGet(s,
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_SUPPORTED_PARAMETERS);
  OLA_ASSERT_TRUE(m_api.GetSupportedParameters(
    UNIVERSE,
    m_uid,
    ola::rdm::ROOT_RDM_DEVICE,
    NewSingleCallback(this, &RDMAPITest::CheckSupportedParams),
    &error));

  // parameter description
  uint16_t pid = 16;
  OLA_ASSERT_FALSE(m_api.GetParameterDescription(
    UNIVERSE,
    m_bcast_uid,
    pid,
    NewSingleCallback(this, &RDMAPITest::CheckMalformedParameterDescription),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetParameterDescription(
    UNIVERSE,
    m_group_uid,
    pid,
    NewSingleCallback(this, &RDMAPITest::CheckMalformedParameterDescription),
    &error));
  CheckForBroadcastError(&error);

  s = "";
  m_impl.AddExpectedGet(s,
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_PARAMETER_DESCRIPTION);
  OLA_ASSERT_TRUE(m_api.GetParameterDescription(
    UNIVERSE,
    m_uid,
    pid,
    NewSingleCallback(this, &RDMAPITest::CheckMalformedParameterDescription),
    &error));

  PACK(
  struct param_info_s {
    uint16_t pid;
    uint8_t pdl_size;
    uint8_t data_type;
    uint8_t command_class;
    uint8_t type;
    uint8_t unit;
    uint8_t prefix;
    uint32_t min_value;
    uint32_t max_value;
    uint32_t default_value;
    char label[32];
  });
  STATIC_ASSERT(sizeof(param_info_s) == 52);
  struct param_info_s param_info;
  param_info.pid = HostToNetwork(static_cast<uint16_t>(0x1234));
  param_info.pdl_size = 10;
  param_info.data_type = ola::rdm::DS_UNSIGNED_DWORD;
  param_info.command_class = ola::rdm::CC_GET;
  param_info.type = 0;
  param_info.unit = ola::rdm::UNITS_METERS;
  param_info.prefix = ola::rdm::PREFIX_KILO;
  param_info.min_value = HostToNetwork(static_cast<uint32_t>(0));
  param_info.max_value = HostToNetwork(static_cast<uint32_t>(200000));
  param_info.default_value = HostToNetwork(static_cast<uint32_t>(1000));
  strncpy(param_info.label, TEST_DESCRIPTION, sizeof(param_info.label));

  string s2(reinterpret_cast<char*>(&param_info),
            sizeof(param_info) - sizeof(param_info.label) +
            strlen(TEST_DESCRIPTION));
  m_impl.AddExpectedGet(s2,
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_PARAMETER_DESCRIPTION);
  OLA_ASSERT_TRUE(m_api.GetParameterDescription(
    UNIVERSE,
    m_uid,
    ola::rdm::ROOT_RDM_DEVICE,
    NewSingleCallback(this, &RDMAPITest::CheckParameterDescription),
    &error));
}


/*
 * Check that the product information commands work correctly
 */
void RDMAPITest::testProductInformation() {
  string error;
  uint16_t sub_device = 1;

  // device info
  PACK(
  struct device_info_s {
    uint8_t version_high;
    uint8_t version_low;
    uint16_t model;
    uint16_t product_category;
    uint32_t software_version;
    uint16_t dmx_footprint;
    uint8_t current_personality;
    uint8_t personality_count;
    uint16_t dmx_start_address;
    uint16_t sub_device_count;
    uint8_t sensor_count;
  });
  STATIC_ASSERT(sizeof(device_info_s) == 19);
  struct device_info_s device_info;
  device_info.version_high = 1;
  device_info.version_low = 0;
  device_info.model = HostToNetwork(static_cast<uint16_t>(2));
  device_info.product_category = HostToNetwork(static_cast<uint16_t>(3));
  device_info.software_version = HostToNetwork(
    static_cast<uint32_t>(0x12345678));
  device_info.dmx_footprint = HostToNetwork(static_cast<uint16_t>(400));
  device_info.current_personality = 1;
  device_info.personality_count = 2;
  device_info.dmx_start_address = HostToNetwork(static_cast<uint16_t>(12));
  device_info.sub_device_count = HostToNetwork(static_cast<uint16_t>(10));
  device_info.sensor_count = 4;

  OLA_ASSERT_FALSE(m_api.GetDeviceInfo(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckDeviceInfo),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetDeviceInfo(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckDeviceInfo),
    &error));
  CheckForBroadcastError(&error);
  string s(reinterpret_cast<char*>(&device_info), sizeof(device_info));
  m_impl.AddExpectedGet(s,
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_DEVICE_INFO);
  OLA_ASSERT_TRUE(m_api.GetDeviceInfo(
    UNIVERSE,
    m_uid,
    ola::rdm::ROOT_RDM_DEVICE,
    NewSingleCallback(this, &RDMAPITest::CheckDeviceInfo),
    &error));

  // product detail id list
  struct {
    uint16_t detail1;
    uint16_t detail2;
    uint16_t detail3;
  } detail_list;
  detail_list.detail1 = HostToNetwork(static_cast<uint16_t>(0x5678));
  detail_list.detail2 = HostToNetwork(static_cast<uint16_t>(0xfedc));
  detail_list.detail3 = HostToNetwork(static_cast<uint16_t>(0xaa00));
  OLA_ASSERT_FALSE(m_api.GetProductDetailIdList(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckProductDetailList),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetProductDetailIdList(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckProductDetailList),
    &error));
  CheckForBroadcastError(&error);
  string s2(reinterpret_cast<char*>(&detail_list), sizeof(detail_list));
  m_impl.AddExpectedGet(s2,
                        UNIVERSE,
                        m_uid,
                        ola::rdm::ROOT_RDM_DEVICE,
                        ola::rdm::PID_PRODUCT_DETAIL_ID_LIST);
  OLA_ASSERT_TRUE(m_api.GetProductDetailIdList(
    UNIVERSE,
    m_uid,
    ola::rdm::ROOT_RDM_DEVICE,
    NewSingleCallback(this, &RDMAPITest::CheckProductDetailList),
    &error));

  // device model description
  OLA_ASSERT_FALSE(m_api.GetDeviceModelDescription(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetDeviceModelDescription(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);

  m_impl.AddExpectedGet(string(TEST_DESCRIPTION),
                        UNIVERSE,
                        m_uid,
                        sub_device,
                        ola::rdm::PID_DEVICE_MODEL_DESCRIPTION);
  OLA_ASSERT_TRUE(m_api.GetDeviceModelDescription(
    UNIVERSE,
    m_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));

  // manufacturer label
  OLA_ASSERT_FALSE(m_api.GetManufacturerLabel(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetManufacturerLabel(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);

  m_impl.AddExpectedGet(string(TEST_DESCRIPTION),
                        UNIVERSE,
                        m_uid,
                        sub_device,
                        ola::rdm::PID_MANUFACTURER_LABEL);
  OLA_ASSERT_TRUE(m_api.GetManufacturerLabel(
    UNIVERSE,
    m_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));

  // get device label
  OLA_ASSERT_FALSE(m_api.GetDeviceLabel(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetDeviceLabel(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);

  m_impl.AddExpectedGet(string(TEST_DESCRIPTION),
                        UNIVERSE,
                        m_uid,
                        sub_device,
                        ola::rdm::PID_DEVICE_LABEL);
  OLA_ASSERT_TRUE(m_api.GetDeviceLabel(
    UNIVERSE,
    m_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));

  // set device label
  s = TEST_DESCRIPTION;
  m_impl.AddExpectedSet(UNIVERSE,
                        m_uid,
                        sub_device,
                        ola::rdm::PID_DEVICE_LABEL,
                        reinterpret_cast<const uint8_t*>(s.data()),
                        s.size());
  OLA_ASSERT_TRUE(m_api.SetDeviceLabel(
    UNIVERSE,
    m_uid,
    sub_device,
    s,
    NewSingleCallback(this, &RDMAPITest::CheckResponseStatus),
    &error));
  // check we can bcast
  m_impl.AddExpectedSet(UNIVERSE,
                        m_bcast_uid,
                        ola::rdm::ALL_RDM_SUBDEVICES,
                        ola::rdm::PID_DEVICE_LABEL,
                        reinterpret_cast<const uint8_t*>(s.data()),
                        s.size());
  OLA_ASSERT_TRUE(m_api.SetDeviceLabel(
    UNIVERSE,
    m_bcast_uid,
    ola::rdm::ALL_RDM_SUBDEVICES,
    s,
    NewSingleCallback(this, &RDMAPITest::CheckWasBroadcast),
    &error));
  m_impl.AddExpectedSet(UNIVERSE,
                        m_group_uid,
                        ola::rdm::ALL_RDM_SUBDEVICES,
                        ola::rdm::PID_DEVICE_LABEL,
                        reinterpret_cast<const uint8_t*>(s.data()),
                        s.size());
  OLA_ASSERT_TRUE(m_api.SetDeviceLabel(
    UNIVERSE,
    m_group_uid,
    ola::rdm::ALL_RDM_SUBDEVICES,
    s,
    NewSingleCallback(this, &RDMAPITest::CheckWasBroadcast),
    &error));
  // check out of range sub devices fail
  OLA_ASSERT_FALSE(m_api.SetDeviceLabel(
    UNIVERSE,
    m_group_uid,
    0x0201,
    s,
    NewSingleCallback(this, &RDMAPITest::CheckResponseStatus),
    &error));
  CheckForDeviceRangeBcastError(&error);

  // software version label
  OLA_ASSERT_FALSE(m_api.GetSoftwareVersionLabel(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetSoftwareVersionLabel(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);

  m_impl.AddExpectedGet(string(TEST_DESCRIPTION),
                        UNIVERSE,
                        m_uid,
                        sub_device,
                        ola::rdm::PID_SOFTWARE_VERSION_LABEL);
  OLA_ASSERT_TRUE(m_api.GetSoftwareVersionLabel(
    UNIVERSE,
    m_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));

  // Boot software label
  OLA_ASSERT_FALSE(m_api.GetBootSoftwareVersionLabel(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetBootSoftwareVersionLabel(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
  CheckForBroadcastError(&error);

  m_impl.AddExpectedGet(string(TEST_DESCRIPTION),
                        UNIVERSE,
                        m_uid,
                        sub_device,
                        ola::rdm::PID_BOOT_SOFTWARE_VERSION_LABEL);
  OLA_ASSERT_TRUE(m_api.GetBootSoftwareVersionLabel(
    UNIVERSE,
    m_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckLabel),
    &error));
}

/*
 * Check that DMX commands work
 */
void RDMAPITest::testDmxSetup() {
  string error;
  uint16_t sub_device = 1;

  // Check get start address
  OLA_ASSERT_FALSE(m_api.GetDMXAddress(
    UNIVERSE,
    m_bcast_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckDMXStartAddress),
    &error));
  CheckForBroadcastError(&error);
  OLA_ASSERT_FALSE(m_api.GetDMXAddress(
    UNIVERSE,
    m_group_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckDMXStartAddress),
    &error));
  CheckForBroadcastError(&error);

  uint16_t start_address = HostToNetwork(static_cast<uint16_t>(44));
  string s(reinterpret_cast<char*>(&start_address), sizeof(start_address));
  m_impl.AddExpectedGet(s,
                        UNIVERSE,
                        m_uid,
                        sub_device,
                        ola::rdm::PID_DMX_START_ADDRESS);
  OLA_ASSERT_TRUE(m_api.GetDMXAddress(
    UNIVERSE,
    m_uid,
    sub_device,
    NewSingleCallback(this, &RDMAPITest::CheckDMXStartAddress),
    &error));

  // Check set start address
  start_address = 64;
  uint16_t address_data = HostToNetwork(start_address);
  m_impl.AddExpectedSet(UNIVERSE,
                        m_uid,
                        sub_device,
                        ola::rdm::PID_DMX_START_ADDRESS,
                        reinterpret_cast<const uint8_t*>(&address_data),
                        sizeof(address_data));
  OLA_ASSERT_TRUE(m_api.SetDMXAddress(
    UNIVERSE,
    m_uid,
    sub_device,
    start_address,
    NewSingleCallback(this, &RDMAPITest::CheckResponseStatus),
    &error));
  // check bcasts work
  m_impl.AddExpectedSet(UNIVERSE,
                        m_bcast_uid,
                        ola::rdm::ALL_RDM_SUBDEVICES,
                        ola::rdm::PID_DMX_START_ADDRESS,
                        reinterpret_cast<const uint8_t*>(&address_data),
                        sizeof(address_data));
  OLA_ASSERT_TRUE(m_api.SetDMXAddress(
    UNIVERSE,
    m_bcast_uid,
    ola::rdm::ALL_RDM_SUBDEVICES,
    start_address,
    NewSingleCallback(this, &RDMAPITest::CheckWasBroadcast),
    &error));
  m_impl.AddExpectedSet(UNIVERSE,
                        m_group_uid,
                        0x0200,
                        ola::rdm::PID_DMX_START_ADDRESS,
                        reinterpret_cast<const uint8_t*>(&address_data),
                        sizeof(address_data));
  OLA_ASSERT_TRUE(m_api.SetDMXAddress(
    UNIVERSE,
    m_group_uid,
    0x0200,
    start_address,
    NewSingleCallback(this, &RDMAPITest::CheckWasBroadcast),
    &error));
  OLA_ASSERT_FALSE(m_api.SetDMXAddress(
    UNIVERSE,
    m_group_uid,
    0x0201,
    start_address,
    NewSingleCallback(this, &RDMAPITest::CheckWasBroadcast),
    &error));
  CheckForDeviceRangeBcastError(&error);
}
