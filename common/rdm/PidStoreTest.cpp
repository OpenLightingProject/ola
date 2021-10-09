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
 * PidStoreTest.cpp
 * Test fixture for the PidStore & Pid classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "common/rdm/PidStoreLoader.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/messaging/Descriptor.h"
#include "ola/messaging/SchemaPrinter.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/testing/TestUtils.h"


using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::rdm::PidDescriptor;
using ola::rdm::PidStore;
using ola::rdm::PidStoreLoader;
using ola::rdm::RootPidStore;
using std::auto_ptr;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;


class PidStoreTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PidStoreTest);
  CPPUNIT_TEST(testPidDescriptor);
  CPPUNIT_TEST(testPidStore);
  CPPUNIT_TEST(testPidStoreLoad);
  CPPUNIT_TEST(testPidStoreFileLoad);
  CPPUNIT_TEST(testPidStoreDirectoryLoad);
  CPPUNIT_TEST(testPidStoreLoadMissingFile);
  CPPUNIT_TEST(testPidStoreLoadDuplicateManufacturer);
  CPPUNIT_TEST(testPidStoreLoadDuplicateValue);
  CPPUNIT_TEST(testPidStoreLoadDuplicateName);
  CPPUNIT_TEST(testPidStoreLoadInvalidEstaPid);
  CPPUNIT_TEST(testInconsistentData);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testPidDescriptor();
  void testPidStore();
  void testPidStoreLoad();
  void testPidStoreFileLoad();
  void testPidStoreDirectoryLoad();
  void testPidStoreLoadMissingFile();
  void testPidStoreLoadDuplicateManufacturer();
  void testPidStoreLoadDuplicateValue();
  void testPidStoreLoadDuplicateName();
  void testPidStoreLoadInvalidEstaPid();
  void testInconsistentData();

 private:
  string GetTestDataFile(const string &filename) {
    string path = TEST_SRC_DIR;
    path.append("/common/rdm/testdata/");
    path.append(filename);
    return path;
  }
};


CPPUNIT_TEST_SUITE_REGISTRATION(PidStoreTest);


/*
 * Test that the PidDescriptor works.
 */
void PidStoreTest::testPidDescriptor() {
  // just use empty fields for now
  vector<const class FieldDescriptor*> fields;
  const Descriptor *get_request_descriptor = new Descriptor("GET Request",
                                                            fields);
  const Descriptor *get_response_descriptor = new Descriptor("GET Response",
                                                             fields);
  const Descriptor *set_request_descriptor = new Descriptor("SET Request",
                                                            fields);
  const Descriptor *set_response_descriptor = new Descriptor("SET Response",
                                                             fields);

  PidDescriptor pid("foo",
                    10,
                    get_request_descriptor,
                    get_response_descriptor,
                    set_request_descriptor,
                    set_response_descriptor,
                    PidDescriptor::NON_BROADCAST_SUB_DEVICE,
                    PidDescriptor::ANY_SUB_DEVICE);

  // basic checks
  OLA_ASSERT_EQ(string("foo"), pid.Name());
  OLA_ASSERT_EQ(static_cast<uint16_t>(10), pid.Value());
  OLA_ASSERT_EQ(get_request_descriptor, pid.GetRequest());
  OLA_ASSERT_EQ(get_response_descriptor, pid.GetResponse());
  OLA_ASSERT_EQ(set_request_descriptor, pid.SetRequest());
  OLA_ASSERT_EQ(set_response_descriptor, pid.SetResponse());

  // check sub device constraints
  OLA_ASSERT_TRUE(pid.IsGetValid(0));
  OLA_ASSERT_TRUE(pid.IsGetValid(1));
  OLA_ASSERT_TRUE(pid.IsGetValid(2));
  OLA_ASSERT_TRUE(pid.IsGetValid(511));
  OLA_ASSERT_TRUE(pid.IsGetValid(512));
  OLA_ASSERT_FALSE(pid.IsGetValid(513));
  OLA_ASSERT_FALSE(pid.IsGetValid(0xffff));
  OLA_ASSERT_TRUE(pid.IsSetValid(0));
  OLA_ASSERT_TRUE(pid.IsSetValid(1));
  OLA_ASSERT_TRUE(pid.IsSetValid(2));
  OLA_ASSERT_TRUE(pid.IsSetValid(511));
  OLA_ASSERT_TRUE(pid.IsSetValid(512));
  OLA_ASSERT_FALSE(pid.IsSetValid(513));
  OLA_ASSERT_TRUE(pid.IsSetValid(0xffff));
}


/**
 * Check the PidStore works.
 */
void PidStoreTest::testPidStore() {
  const PidDescriptor *foo_pid = new PidDescriptor(
      "foo", 0, NULL, NULL, NULL, NULL,
      PidDescriptor::NON_BROADCAST_SUB_DEVICE,
      PidDescriptor::ANY_SUB_DEVICE);
  const PidDescriptor *bar_pid = new PidDescriptor(
      "bar", 1, NULL, NULL, NULL, NULL,
      PidDescriptor::NON_BROADCAST_SUB_DEVICE,
      PidDescriptor::ANY_SUB_DEVICE);

  vector<const PidDescriptor*> pids;
  pids.push_back(foo_pid);
  pids.push_back(bar_pid);

  PidStore store(pids);

  // check value lookups
  OLA_ASSERT_EQ(foo_pid, store.LookupPID(0));
  OLA_ASSERT_EQ(bar_pid, store.LookupPID(1));
  OLA_ASSERT_EQ(static_cast<const PidDescriptor*>(NULL),
                       store.LookupPID(2));

  // check name lookups
  OLA_ASSERT_EQ(foo_pid, store.LookupPID("foo"));
  OLA_ASSERT_EQ(bar_pid, store.LookupPID("bar"));
  OLA_ASSERT_EQ(static_cast<const PidDescriptor*>(NULL),
                       store.LookupPID("baz"));

  // check all pids;
  vector<const PidDescriptor*> all_pids;
  store.AllPids(&all_pids);
  OLA_ASSERT_EQ(static_cast<size_t>(2), all_pids.size());
  OLA_ASSERT_EQ(foo_pid, all_pids[0]);
  OLA_ASSERT_EQ(bar_pid, all_pids[1]);
}


/**
 * Check we can load a PidStore from a string
 */
void PidStoreTest::testPidStoreLoad() {
  PidStoreLoader loader;
  // This is a stringstream not a ostringstream as the other end needs an
  // istream
  stringstream str;

  // check that this fails to load
  const RootPidStore *empty_root_store = loader.LoadFromStream(&str);
  OLA_ASSERT_EQ(static_cast<const RootPidStore*>(NULL),
                       empty_root_store);

  // now try a simple pid store config
  str.clear();
  str << "pid {" << endl <<
         "  name: \"PROXIED_DEVICES\"" << endl <<
         "  value: 16" << endl <<
         "  get_request {" << endl <<
         "  }" << endl <<
         "  get_response {" << endl <<
         "    field {" << endl <<
         "      type: GROUP" << endl <<
         "      name: \"uids\"" << endl <<
         "      field {" << endl <<
         "        type: UINT16" << endl <<
         "        name: \"manufacturer_id\"" << endl <<
         "      }" << endl <<
         "      field {" << endl <<
         "        type: UINT32" << endl <<
         "        name: \"device_id\"" << endl <<
         "      }" << endl <<
         "    }" << endl <<
         "  }" << endl <<
         "  get_sub_device_range: ROOT_DEVICE" << endl <<
         "}" << endl <<
         "manufacturer {" << endl <<
         "  manufacturer_id: 31344" << endl <<
         "  manufacturer_name: \"Open Lighting\"" << endl <<
         "}" << endl <<
         "version: 1" << endl;

  auto_ptr<const RootPidStore> root_store(loader.LoadFromStream(&str));
  OLA_ASSERT_TRUE(root_store.get());

  // check version
  OLA_ASSERT_EQ(static_cast<uint64_t>(1), root_store->Version());

  // check manufacturer pids
  const PidStore *open_lighting_store =
    root_store->ManufacturerStore(ola::OPEN_LIGHTING_ESTA_CODE);
  OLA_ASSERT_TRUE(open_lighting_store);
  OLA_ASSERT_EQ(0u, open_lighting_store->PidCount());

  // lookup by value
  OLA_ASSERT_TRUE(root_store->GetDescriptor(16));
  OLA_ASSERT_FALSE(root_store->GetDescriptor(17));
  OLA_ASSERT_TRUE(root_store->GetDescriptor(16, ola::OPEN_LIGHTING_ESTA_CODE));
  OLA_ASSERT_FALSE(root_store->GetDescriptor(17, ola::OPEN_LIGHTING_ESTA_CODE));

  // lookup by name
  OLA_ASSERT_TRUE(root_store->GetDescriptor("PROXIED_DEVICES"));
  OLA_ASSERT_FALSE(root_store->GetDescriptor("DEVICE_INFO"));
  OLA_ASSERT_TRUE(root_store->GetDescriptor("PROXIED_DEVICES",
                                            ola::OPEN_LIGHTING_ESTA_CODE));
  OLA_ASSERT_FALSE(root_store->GetDescriptor("DEVICE_INFO",
                                             ola::OPEN_LIGHTING_ESTA_CODE));

  // check lookups
  const PidStore *esta_store = root_store->EstaStore();
  OLA_ASSERT_TRUE(esta_store);

  const PidDescriptor *pid_descriptor = esta_store->LookupPID(16);
  OLA_ASSERT_TRUE(pid_descriptor);
  const PidDescriptor *pid_descriptor2 = esta_store->LookupPID(
      "PROXIED_DEVICES");
  OLA_ASSERT_TRUE(pid_descriptor2);
  OLA_ASSERT_EQ(pid_descriptor, pid_descriptor2);

  // check name and value
  OLA_ASSERT_EQ(static_cast<uint16_t>(16), pid_descriptor->Value());
  OLA_ASSERT_EQ(string("PROXIED_DEVICES"), pid_descriptor->Name());

  // check descriptors
  OLA_ASSERT_TRUE(pid_descriptor->GetRequest());
  OLA_ASSERT_TRUE(pid_descriptor->GetResponse());
  OLA_ASSERT_EQ(static_cast<const Descriptor*>(NULL),
                pid_descriptor->SetRequest());
  OLA_ASSERT_EQ(static_cast<const Descriptor*>(NULL),
                pid_descriptor->SetResponse());

  // check GET descriptors
  const Descriptor *get_request = pid_descriptor->GetRequest();
  OLA_ASSERT_EQ(0u, get_request->FieldCount());

  const Descriptor *get_response = pid_descriptor->GetResponse();
  OLA_ASSERT_EQ(1u, get_response->FieldCount());
  const FieldDescriptor *proxied_group = get_response->GetField(0);
  OLA_ASSERT_TRUE(proxied_group);

  // this is ugly but it's a test
  const FieldDescriptorGroup *group_descriptor =
    dynamic_cast<const FieldDescriptorGroup*>(proxied_group);
  OLA_ASSERT_TRUE(group_descriptor);

  // check all the group properties
  OLA_ASSERT_FALSE(group_descriptor->FixedSize());
  OLA_ASSERT_FALSE(group_descriptor->LimitedSize());
  OLA_ASSERT_EQ(0u, group_descriptor->MaxSize());
  OLA_ASSERT_EQ(2u, group_descriptor->FieldCount());
  OLA_ASSERT_TRUE(group_descriptor->FixedBlockSize());
  OLA_ASSERT_EQ(6u, group_descriptor->BlockSize());
  OLA_ASSERT_EQ(6u, group_descriptor->MaxBlockSize());
  OLA_ASSERT_EQ(static_cast<uint16_t>(0),
                       group_descriptor->MinBlocks());
  OLA_ASSERT_EQ(FieldDescriptorGroup::UNLIMITED_BLOCKS,
                       group_descriptor->MaxBlocks());
  OLA_ASSERT_FALSE(group_descriptor->FixedBlockCount());

  // Check this prints correctly
  ola::messaging::SchemaPrinter printer;
  get_response->Accept(&printer);
  string expected = (
      "uids {\n  manufacturer_id: uint16\n  device_id: uint32\n}\n");
  OLA_ASSERT_EQ(expected, printer.AsString());

  // check sub device ranges
  OLA_ASSERT_TRUE(pid_descriptor->IsGetValid(0));
  OLA_ASSERT_FALSE(pid_descriptor->IsGetValid(1));
  OLA_ASSERT_FALSE(pid_descriptor->IsGetValid(512));
  OLA_ASSERT_FALSE(pid_descriptor->IsGetValid(ola::rdm::ALL_RDM_SUBDEVICES));
  OLA_ASSERT_FALSE(pid_descriptor->IsSetValid(0));
  OLA_ASSERT_FALSE(pid_descriptor->IsSetValid(1));
  OLA_ASSERT_FALSE(pid_descriptor->IsSetValid(512));
  OLA_ASSERT_FALSE(pid_descriptor->IsSetValid(ola::rdm::ALL_RDM_SUBDEVICES));
}


/**
 * Check that loading from a file works
 */
void PidStoreTest::testPidStoreFileLoad() {
  PidStoreLoader loader;

  auto_ptr<const RootPidStore> root_store(
      loader.LoadFromFile(GetTestDataFile("test_pids.proto")));
  OLA_ASSERT_NOT_NULL(root_store.get());
  // check version
  OLA_ASSERT_EQ(static_cast<uint64_t>(1302986774), root_store->Version());

  // Check all the esta pids are there
  const PidStore *esta_store = root_store->EstaStore();
  OLA_ASSERT_NOT_NULL(esta_store);

  vector<const PidDescriptor*> all_pids;
  esta_store->AllPids(&all_pids);
  OLA_ASSERT_EQ(static_cast<size_t>(70), all_pids.size());

  // check for device info
  const PidDescriptor *device_info = esta_store->LookupPID("DEVICE_INFO");
  OLA_ASSERT_NOT_NULL(device_info);
  OLA_ASSERT_EQ(static_cast<uint16_t>(96), device_info->Value());
  OLA_ASSERT_EQ(string("DEVICE_INFO"), device_info->Name());

  // check descriptors
  OLA_ASSERT_TRUE(device_info->GetRequest());
  OLA_ASSERT_TRUE(device_info->GetResponse());
  OLA_ASSERT_EQ(static_cast<const Descriptor*>(NULL),
                       device_info->SetRequest());
  OLA_ASSERT_EQ(static_cast<const Descriptor*>(NULL),
                       device_info->SetResponse());

  ola::messaging::SchemaPrinter printer;
  device_info->GetResponse()->Accept(&printer);
  string expected = (
      "protocol_major: uint8\nprotocol_minor: uint8\ndevice_model: uint16\n"
      "product_category: uint16\nsoftware_version: uint32\n"
      "dmx_footprint: uint16\ncurrent_personality: uint8\n"
      "personality_count: uint8\ndmx_start_address: uint16\n"
      "sub_device_count: uint16\nsensor_count: uint8\n");
  OLA_ASSERT_EQ(expected, printer.AsString());

  // check manufacturer pids
  const PidStore *open_lighting_store =
    root_store->ManufacturerStore(ola::OPEN_LIGHTING_ESTA_CODE);
  OLA_ASSERT_NOT_NULL(open_lighting_store);
  OLA_ASSERT_EQ(1u, open_lighting_store->PidCount());

  const PidDescriptor *serial_number = open_lighting_store->LookupPID(
      "SERIAL_NUMBER");
  OLA_ASSERT_NOT_NULL(serial_number);
  OLA_ASSERT_EQ(static_cast<uint16_t>(32768), serial_number->Value());
  OLA_ASSERT_EQ(string("SERIAL_NUMBER"), serial_number->Name());

  // check descriptors
  OLA_ASSERT_EQ(static_cast<const Descriptor*>(NULL),
                serial_number->GetRequest());
  OLA_ASSERT_EQ(static_cast<const Descriptor*>(NULL),
                serial_number->GetResponse());
  OLA_ASSERT_TRUE(serial_number->SetRequest());
  OLA_ASSERT_TRUE(serial_number->SetResponse());

  printer.Reset();
  serial_number->SetRequest()->Accept(&printer);
  string expected2 = "serial_number: uint32\n";
  OLA_ASSERT_EQ(expected2, printer.AsString());
}


/**
 * Check that loading from a directory works. This also tests the override
 * mechanism and manufacturer names.
 */
void PidStoreTest::testPidStoreDirectoryLoad() {
  PidStoreLoader loader;

  auto_ptr<const RootPidStore> root_store(loader.LoadFromDirectory(
      GetTestDataFile("pids")));
  OLA_ASSERT_NOT_NULL(root_store.get());
  // check version
  OLA_ASSERT_EQ(static_cast<uint64_t>(1302986774), root_store->Version());

  // Check all the esta pids are there
  const PidStore *esta_store = root_store->EstaStore();
  OLA_ASSERT_NOT_NULL(esta_store);

  vector<const PidDescriptor*> all_pids;
  esta_store->AllPids(&all_pids);
  OLA_ASSERT_EQ(static_cast<size_t>(6), all_pids.size());

  // check manufacturer pids
  const PidStore *open_lighting_store =
    root_store->ManufacturerStore(ola::OPEN_LIGHTING_ESTA_CODE);
  OLA_ASSERT_NOT_NULL(open_lighting_store);
  OLA_ASSERT_EQ(1u, open_lighting_store->PidCount());

  // FOO_BAR in the overrides file replaces SERIAL_NUMBER.
  const PidDescriptor *serial_number = open_lighting_store->LookupPID(
      "SERIAL_NUMBER");
  OLA_ASSERT_NULL(serial_number);

  const PidDescriptor *foo_bar = open_lighting_store->LookupPID(
      "FOO_BAR");
  OLA_ASSERT_NOT_NULL(foo_bar);
  OLA_ASSERT_EQ(static_cast<uint16_t>(32768), foo_bar->Value());
  OLA_ASSERT_EQ(string("FOO_BAR"), foo_bar->Name());

  // check descriptors
  OLA_ASSERT_TRUE(foo_bar->GetRequest());
  OLA_ASSERT_TRUE(foo_bar->GetResponse());
  OLA_ASSERT_EQ(static_cast<const Descriptor*>(NULL),
                       foo_bar->SetRequest());
  OLA_ASSERT_EQ(static_cast<const Descriptor*>(NULL),
                       foo_bar->SetResponse());

  ola::messaging::SchemaPrinter printer;
  foo_bar->GetResponse()->Accept(&printer);
  string expected2 = "baz: uint32\n";
  OLA_ASSERT_EQ(expected2, printer.AsString());
}


/**
 * Check that loading a missing file fails.
 */
void PidStoreTest::testPidStoreLoadMissingFile() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      GetTestDataFile("missing_file_pids.proto"));
  OLA_ASSERT_NULL(root_store);
}


/**
 * Check that loading a file with duplicate manufacturers fails.
 */
void PidStoreTest::testPidStoreLoadDuplicateManufacturer() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      GetTestDataFile("duplicate_manufacturer.proto"));
  OLA_ASSERT_NULL(root_store);
}


/**
 * Check that loading file with duplicate pid values fails.
 */
void PidStoreTest::testPidStoreLoadDuplicateValue() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      GetTestDataFile("duplicate_pid_value.proto"));
  OLA_ASSERT_NULL(root_store);
}


/**
 * Check that loading a file with duplicate pid names fails.
 */
void PidStoreTest::testPidStoreLoadDuplicateName() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      GetTestDataFile("duplicate_pid_name.proto"));
  OLA_ASSERT_NULL(root_store);
}


/**
 * Check that loading a file with an out-of-range ESTA pid fails.
 */
void PidStoreTest::testPidStoreLoadInvalidEstaPid() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      GetTestDataFile("invalid_esta_pid.proto"));
  OLA_ASSERT_NULL(root_store);
}


/**
 * Check that loading a file with an inconsistent descriptor fails.
 */
void PidStoreTest::testInconsistentData() {
  PidStoreLoader loader;
  const RootPidStore *root_store = loader.LoadFromFile(
      GetTestDataFile("inconsistent_pid.proto"));
  OLA_ASSERT_NULL(root_store);
}
