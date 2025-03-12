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
 * SchemaPrinterTest.cpp
 * Test fixture for the SchemaPrinter class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/messaging/Descriptor.h"
#include "ola/messaging/SchemaPrinter.h"
#include "ola/testing/TestUtils.h"


using std::string;
using std::vector;


using ola::messaging::BoolFieldDescriptor;
using ola::messaging::IPV4FieldDescriptor;
using ola::messaging::IPV6FieldDescriptor;
using ola::messaging::MACFieldDescriptor;
using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::messaging::SchemaPrinter;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::UInt16FieldDescriptor;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt64FieldDescriptor;
using ola::messaging::UInt8FieldDescriptor;
using ola::messaging::Int16FieldDescriptor;
using ola::messaging::Int32FieldDescriptor;
using ola::messaging::Int64FieldDescriptor;
using ola::messaging::Int8FieldDescriptor;
using ola::messaging::UIDFieldDescriptor;

class SchemaPrinterTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SchemaPrinterTest);
  CPPUNIT_TEST(testPrinter);
  CPPUNIT_TEST(testGroupPrinter);
  CPPUNIT_TEST(testLabels);
  CPPUNIT_TEST(testIntervalsAndLabels);
  CPPUNIT_TEST(testIntervalTypes);
  CPPUNIT_TEST_SUITE_END();

 public:
    SchemaPrinterTest() {}
    void testPrinter();
    void testGroupPrinter();
    void testLabels();
    void testIntervalsAndLabels();
    void testIntervalTypes();

 private:
    template<typename field_descriptor_class, typename int_type>
    string GenerateIntervalString(int_type min, int_type max);
};


CPPUNIT_TEST_SUITE_REGISTRATION(SchemaPrinterTest);


/*
 * Test the SchemaPrinter
 */
void SchemaPrinterTest::testPrinter() {
  // setup some fields
  BoolFieldDescriptor *bool_descriptor = new BoolFieldDescriptor("On/Off");
  StringFieldDescriptor *string_descriptor = new StringFieldDescriptor(
      "Name", 0, 32);
  UInt8FieldDescriptor *uint8_descriptor = new UInt8FieldDescriptor(
      "Count", false, 10);
  IPV4FieldDescriptor *ipv4_descriptor = new IPV4FieldDescriptor(
      "Address");
  IPV6FieldDescriptor *ipv6_descriptor = new IPV6FieldDescriptor(
      "v6 Address");
  MACFieldDescriptor *mac_descriptor = new MACFieldDescriptor(
      "MAC Address");
  UIDFieldDescriptor *uid_descriptor = new UIDFieldDescriptor("Device");

  // try a simple print first
  vector<const FieldDescriptor*> fields;
  fields.push_back(bool_descriptor);
  fields.push_back(string_descriptor);
  fields.push_back(uint8_descriptor);
  fields.push_back(ipv4_descriptor);
  fields.push_back(ipv6_descriptor);
  fields.push_back(mac_descriptor);
  fields.push_back(uid_descriptor);

  Descriptor test_descriptor("Test Descriptor", fields);
  SchemaPrinter printer(false, false);
  test_descriptor.Accept(&printer);

  string expected = (
      "On/Off: bool\nName: string [0, 32]\nCount: uint8\n"
      "Address: IPv4 address\nv6 Address: IPv6 address\nMAC Address: MAC\n"
      "Device: UID\n");
  OLA_ASSERT_EQ(expected, printer.AsString());
}


void SchemaPrinterTest::testGroupPrinter() {
  BoolFieldDescriptor *bool_descriptor = new BoolFieldDescriptor("On/Off");
  StringFieldDescriptor *string_descriptor = new StringFieldDescriptor(
      "Name", 0, 32);
  StringFieldDescriptor *string_descriptor2 = new StringFieldDescriptor(
      "Device", 0, 32);
  UInt8FieldDescriptor *uint8_descriptor = new UInt8FieldDescriptor(
      "Count", false, 10);
  UInt32FieldDescriptor *uint32_descriptor = new UInt32FieldDescriptor("Id");

  vector<const FieldDescriptor*> fields;
  fields.push_back(bool_descriptor);
  fields.push_back(string_descriptor);
  fields.push_back(uint8_descriptor);

  // now do a descriptor which contains a GroupDescriptor
  FieldDescriptorGroup *group_descriptor = new FieldDescriptorGroup(
      "Group 1", fields, 0, 2);
  vector<const FieldDescriptor*> fields2;
  fields2.push_back(string_descriptor2);
  fields2.push_back(uint32_descriptor);
  fields2.push_back(group_descriptor);
  Descriptor test_descriptor("Test Descriptor2", fields2);

  SchemaPrinter printer(false, false);
  test_descriptor.Accept(&printer);

  string expected = "Device: string [0, 32]\nId: uint32\nGroup 1 {\n"
    "  On/Off: bool\n  Name: string [0, 32]\n  Count: uint8\n}\n";
  OLA_ASSERT_EQ(expected, printer.AsString());
}


void SchemaPrinterTest::testLabels() {
  UInt16FieldDescriptor::IntervalVector intervals;
  intervals.push_back(UInt16FieldDescriptor::Interval(12, 12));
  intervals.push_back(UInt16FieldDescriptor::Interval(13, 13));
  UInt16FieldDescriptor::LabeledValues labels;
  labels["dozen"] = 12;
  labels["bakers_dozen"] = 13;

  UInt16FieldDescriptor *uint16_descriptor = new UInt16FieldDescriptor(
      "Count", intervals, labels);
  vector<const FieldDescriptor*> fields;
  fields.push_back(uint16_descriptor);
  Descriptor test_descriptor("Test Descriptor", fields);

  SchemaPrinter interval_printer(true, false);
  test_descriptor.Accept(&interval_printer);
  string expected = "Count: uint16: 12, 13\n";
  OLA_ASSERT_EQ(expected, interval_printer.AsString());
}


void SchemaPrinterTest::testIntervalsAndLabels() {
  UInt16FieldDescriptor::IntervalVector intervals;
  intervals.push_back(UInt16FieldDescriptor::Interval(2, 8));
  intervals.push_back(UInt16FieldDescriptor::Interval(12, 14));

  UInt16FieldDescriptor::LabeledValues labels;
  labels["dozen"] = 12;
  labels["bakers_dozen"] = 13;

  UInt16FieldDescriptor *uint16_descriptor = new UInt16FieldDescriptor(
      "Count", intervals, labels);
  vector<const FieldDescriptor*> fields;
  fields.push_back(uint16_descriptor);
  Descriptor test_descriptor("Test Descriptor", fields);

  SchemaPrinter interval_printer(true, false);
  test_descriptor.Accept(&interval_printer);
  string expected = "Count: uint16: (2, 8), (12, 14)\n";
  OLA_ASSERT_EQ(expected, interval_printer.AsString());

  SchemaPrinter label_printer(false, true);
  test_descriptor.Accept(&label_printer);
  string expected2 = "Count: uint16\n  bakers_dozen: 13\n  dozen: 12\n";
  OLA_ASSERT_EQ(expected2, label_printer.AsString());

  SchemaPrinter interval_label_printer(true, true);
  test_descriptor.Accept(&interval_label_printer);
  string expected3 = (
      "Count: uint16: (2, 8), (12, 14)\n  bakers_dozen: 13\n  dozen: 12\n");
  OLA_ASSERT_EQ(expected3, interval_label_printer.AsString());
}


template<typename field_descriptor_class, typename int_type>
string SchemaPrinterTest::GenerateIntervalString(int_type min, int_type max) {
  typename field_descriptor_class::IntervalVector intervals;
  intervals.push_back(typename field_descriptor_class::Interval(min, max));
  typename field_descriptor_class::LabeledValues labels;

  vector<const FieldDescriptor*> fields;
  fields.push_back(new field_descriptor_class("Count", intervals, labels));
  Descriptor test_descriptor("Test Descriptor", fields);

  SchemaPrinter interval_printer(true, false);
  test_descriptor.Accept(&interval_printer);

  return interval_printer.AsString();
}


void SchemaPrinterTest::testIntervalTypes() {
  OLA_ASSERT_EQ(string("Count: uint8: (2, 8)\n"),
                GenerateIntervalString<UInt8FieldDescriptor>(2, 8));
  OLA_ASSERT_EQ(string("Count: uint16: (2, 8256)\n"),
                GenerateIntervalString<UInt16FieldDescriptor>(2, 8256));
  OLA_ASSERT_EQ(
      string("Count: uint32: (2, 82560)\n"),
      GenerateIntervalString<UInt32FieldDescriptor>(2, 82560));

  OLA_ASSERT_EQ(string("Count: int8: (-2, 8)\n"),
                       GenerateIntervalString<Int8FieldDescriptor>(-2, 8));
  OLA_ASSERT_EQ(
      string("Count: int16: (-300, 8256)\n"),
      GenerateIntervalString<Int16FieldDescriptor>(-300, 8256));
  OLA_ASSERT_EQ(
      string("Count: int32: (-70000, 82560)\n"),
      GenerateIntervalString<Int32FieldDescriptor>(-70000, 82560));
  OLA_ASSERT_EQ(
      string("Count: int64: (-7000000000, 8256123456)\n"),
      GenerateIntervalString<Int64FieldDescriptor>(-7000000000, 8256123456));
}

