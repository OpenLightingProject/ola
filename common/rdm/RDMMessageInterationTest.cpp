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
 * RDMMessageInterationTest.cpp
 * Test fixture for the StringBuilder classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include <ola/Logging.h>
#include <ola/messaging/Message.h>
#include <ola/messaging/MessagePrinter.h>
#include <ola/rdm/MessageDeserializer.h>
#include <ola/rdm/MessageSerializer.h>
#include <ola/rdm/PidStore.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/StringMessageBuilder.h>
#include <memory>
#include <string>
#include <vector>
#include "common/rdm/PidStoreLoader.h"


using ola::messaging::Descriptor;
using ola::messaging::Message;
using ola::rdm::PidDescriptor;
using ola::rdm::PidStore;
using ola::rdm::StringMessageBuilder;
using std::auto_ptr;
using std::string;
using std::vector;


class RDMMessageInterationTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMMessageInterationTest);
  CPPUNIT_TEST(testProxiedDevices);
  CPPUNIT_TEST(testDeviceInfoRequest);
  CPPUNIT_TEST(testParameterDescription);
  CPPUNIT_TEST(testDeviceModelDescription);
  CPPUNIT_TEST_SUITE_END();

  public:
    RDMMessageInterationTest();
    ~RDMMessageInterationTest();

    void testProxiedDevices();
    void testDeviceInfoRequest();
    void testDeviceModelDescription();
    void testParameterDescription();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
      CPPUNIT_ASSERT(m_store);
      m_esta_store = m_store->EstaStore();
      CPPUNIT_ASSERT(m_esta_store);
    }

  private:
    const ola::rdm::RootPidStore *m_store;
    const ola::rdm::PidStore *m_esta_store;
    ola::rdm::StringMessageBuilder m_builder;
    ola::messaging::MessagePrinter m_printer;
    ola::rdm::MessageSerializer m_serializer;
    ola::rdm::MessageDeserializer m_deserializer;
};


CPPUNIT_TEST_SUITE_REGISTRATION(RDMMessageInterationTest);

RDMMessageInterationTest::RDMMessageInterationTest() {
  ola::rdm::PidStoreLoader loader;
  m_store = loader.LoadFromFile("./testdata/test_pids.proto");
}


RDMMessageInterationTest::~RDMMessageInterationTest() {
  delete m_store;
}


/**
 * test PROXIED_DEVICES
 */
void RDMMessageInterationTest::testProxiedDevices() {
  const PidDescriptor *device_info_pid =
    m_esta_store->LookupPID(ola::rdm::PID_PROXIED_DEVICES);
  CPPUNIT_ASSERT(device_info_pid);
  const Descriptor *descriptor = device_info_pid->GetResponse();
  CPPUNIT_ASSERT(descriptor);

  vector<string> inputs;
  inputs.push_back("31344");  // manufacturer ID
  inputs.push_back("1");  // device id
  inputs.push_back("31344");  // manufacturer ID
  inputs.push_back("2");  // device id
  inputs.push_back("21324");  // manufacturer ID
  inputs.push_back("1");  // device id

  const Message *message = m_builder.GetMessage(inputs, descriptor);
  CPPUNIT_ASSERT(message);

  unsigned int data_length;
  const uint8_t *data = m_serializer.SerializeMessage(message, &data_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(18), data_length);

  const Message *inflated_message = m_deserializer.InflateMessage(
      descriptor,
      data,
      data_length);
  CPPUNIT_ASSERT(inflated_message);

  const string input = m_printer.AsString(message);
  const string output = m_printer.AsString(inflated_message);
  CPPUNIT_ASSERT_EQUAL(input, output);

  const string expected = (
      "uids {\n  manufacturer_id: 31344\n  device_id: 1\n}\n"
      "uids {\n  manufacturer_id: 31344\n  device_id: 2\n}\n"
      "uids {\n  manufacturer_id: 21324\n  device_id: 1\n}\n");
  CPPUNIT_ASSERT_EQUAL(expected, output);
}


/**
 * test Device Info Request
 */
void RDMMessageInterationTest::testDeviceInfoRequest() {
  const PidDescriptor *device_info_pid =
    m_esta_store->LookupPID(ola::rdm::PID_DEVICE_INFO);
  CPPUNIT_ASSERT(device_info_pid);
  const Descriptor *descriptor = device_info_pid->GetResponse();
  CPPUNIT_ASSERT(descriptor);

  vector<string> inputs;
  inputs.push_back("1");  // major
  inputs.push_back("0");  // minor
  inputs.push_back("300");  // device model
  inputs.push_back("400");  // product category
  inputs.push_back("40000");  // software version
  inputs.push_back("512");  // dmx footprint
  inputs.push_back("1");  // current personality
  inputs.push_back("5");  // personality count
  inputs.push_back("1");  // dmx start address
  inputs.push_back("0");  // sub device count
  inputs.push_back("6");  // sensor count

  const Message *message = m_builder.GetMessage(inputs, descriptor);
  CPPUNIT_ASSERT(message);

  unsigned int data_length;
  const uint8_t *data = m_serializer.SerializeMessage(message, &data_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(19), data_length);

  const Message *inflated_message = m_deserializer.InflateMessage(
      descriptor,
      data,
      data_length);
  CPPUNIT_ASSERT(inflated_message);

  const string input = m_printer.AsString(message);
  const string output = m_printer.AsString(inflated_message);
  CPPUNIT_ASSERT_EQUAL(input, output);

  const string expected = (
      "protocol_major: 1\nprotocol_minor: 0\ndevice_model: 300\n"
      "product_category: 400\nsoftware_version: 40000\n"
      "dmx_footprint: 512\ncurrent_personality: 1\npersonality_count: 5\n"
      "dmx_start_address: 1\nsub_device_count: 0\nsensor_count: 6\n");
  CPPUNIT_ASSERT_EQUAL(expected, output);
}


/**
 * test device model description works
 */
void RDMMessageInterationTest::testDeviceModelDescription() {
  const PidDescriptor *device_model_pid =
    m_esta_store->LookupPID(ola::rdm::PID_DEVICE_MODEL_DESCRIPTION);
  CPPUNIT_ASSERT(device_model_pid);
  const Descriptor *descriptor = device_model_pid->GetResponse();
  CPPUNIT_ASSERT(descriptor);

  vector<string> inputs;
  inputs.push_back("wigglelight 2000");  // description

  const Message *message = m_builder.GetMessage(inputs, descriptor);
  CPPUNIT_ASSERT(message);

  unsigned int data_length;
  const uint8_t *data = m_serializer.SerializeMessage(message, &data_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(16), data_length);

  const Message *inflated_message = m_deserializer.InflateMessage(
      descriptor,
      data,
      data_length);
  CPPUNIT_ASSERT(inflated_message);

  const string input = m_printer.AsString(message);
  const string output = m_printer.AsString(inflated_message);
  CPPUNIT_ASSERT_EQUAL(input, output);

  const string expected = "description: wigglelight 2000\n";
  CPPUNIT_ASSERT_EQUAL(expected, output);
}


/**
 * test parameter description works
 */
void RDMMessageInterationTest::testParameterDescription() {
  const PidDescriptor *param_description_pid =
    m_esta_store->LookupPID(ola::rdm::PID_PARAMETER_DESCRIPTION);
  CPPUNIT_ASSERT(param_description_pid);
  const Descriptor *descriptor = param_description_pid->GetResponse();
  CPPUNIT_ASSERT(descriptor);

  vector<string> inputs;
  inputs.push_back("8000");  // pid
  inputs.push_back("2");  // pdl size
  inputs.push_back("6");  // data type
  inputs.push_back("3");  // command class
  inputs.push_back("0");  // type (unused)
  inputs.push_back("1");  // unit
  inputs.push_back("0");  // prefix
  inputs.push_back("0");  // min valid value
  inputs.push_back("400");  // max valid value
  inputs.push_back("0");  // default value
  inputs.push_back("room temp");  // description

  const Message *message = m_builder.GetMessage(inputs, descriptor);
  CPPUNIT_ASSERT(message);

  unsigned int data_length;
  const uint8_t *data = m_serializer.SerializeMessage(message, &data_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(29), data_length);

  const Message *inflated_message = m_deserializer.InflateMessage(
      descriptor,
      data,
      data_length);
  CPPUNIT_ASSERT(inflated_message);

  const string input = m_printer.AsString(message);
  const string output = m_printer.AsString(inflated_message);
  CPPUNIT_ASSERT_EQUAL(input, output);

  const string expected = (
      "pid: 8000\npdl_size: 2\ndata_type: 6\ncommand_class: 3\n"
      "type: 0\nunit: 1\nprefix: 0\nmin_value: 0\nmax_value: 400\n"
      "default_value: 0\ndescription: room temp\n");
  CPPUNIT_ASSERT_EQUAL(expected, output);
}
