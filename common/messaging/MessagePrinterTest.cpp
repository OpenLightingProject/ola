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
 * MessagePrinterTest.cpp
 * Test fixture for the MessagePrinter class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/messaging/Descriptor.h"
#include "ola/messaging/Message.h"
#include "ola/messaging/MessagePrinter.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "ola/testing/TestUtils.h"


using std::string;
using std::vector;
using ola::rdm::UID;
using ola::network::MACAddress;


using ola::messaging::BoolFieldDescriptor;
using ola::messaging::BoolMessageField;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::messaging::GenericMessagePrinter;
using ola::messaging::GroupMessageField;
using ola::messaging::Int16FieldDescriptor;
using ola::messaging::Int16MessageField;
using ola::messaging::Int8FieldDescriptor;
using ola::messaging::Int8MessageField;
using ola::messaging::IPV4FieldDescriptor;
using ola::messaging::IPV4MessageField;
using ola::messaging::MACFieldDescriptor;
using ola::messaging::MACMessageField;
using ola::messaging::Message;
using ola::messaging::MessageFieldInterface;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::StringMessageField;
using ola::messaging::UIDFieldDescriptor;
using ola::messaging::UIDMessageField;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt32MessageField;
using ola::messaging::UInt8FieldDescriptor;
using ola::messaging::UInt8MessageField;


class GenericMessagePrinterTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(GenericMessagePrinterTest);
  CPPUNIT_TEST(testSimplePrinter);
  CPPUNIT_TEST(testLabeledPrinter);
  CPPUNIT_TEST(testNestedPrinter);
  CPPUNIT_TEST_SUITE_END();

 public:
    GenericMessagePrinterTest() {}
    void testSimplePrinter();
    void testLabeledPrinter();
    void testNestedPrinter();

 private:
    GenericMessagePrinter m_printer;
};


CPPUNIT_TEST_SUITE_REGISTRATION(GenericMessagePrinterTest);


/*
 * Test the MessagePrinter
 */
void GenericMessagePrinterTest::testSimplePrinter() {
  // setup some fields
  BoolFieldDescriptor bool_descriptor("On/Off");
  IPV4FieldDescriptor ipv4_descriptor("ip");
  MACFieldDescriptor mac_descriptor("mac");
  UIDFieldDescriptor uid_descriptor("uid");
  StringFieldDescriptor string_descriptor("Name", 0, 32);
  UInt32FieldDescriptor uint32_descriptor("Id");
  UInt8FieldDescriptor uint8_descriptor("Count", false, -3);
  Int8FieldDescriptor int8_descriptor("Delta", false, 1);
  Int16FieldDescriptor int16_descriptor("Rate", false, -1);

  // try a simple print first
  vector<const ola::messaging::MessageFieldInterface*> fields;
  fields.push_back(new BoolMessageField(&bool_descriptor, false));
  fields.push_back(
      new IPV4MessageField(&ipv4_descriptor,
                           ola::network::HostToNetwork(0x0a000001)));
  fields.push_back(
      new MACMessageField(&mac_descriptor,
                          MACAddress::FromStringOrDie("01:23:45:67:89:ab")));
  fields.push_back(new UIDMessageField(&uid_descriptor, UID(0x7a70, 1)));
  fields.push_back(new StringMessageField(&string_descriptor, "foobar"));
  fields.push_back(new UInt32MessageField(&uint32_descriptor, 42));
  fields.push_back(new UInt8MessageField(&uint8_descriptor, 4));
  fields.push_back(new Int8MessageField(&int8_descriptor, 10));
  fields.push_back(new Int16MessageField(&int16_descriptor, 10));

  Message message(fields);
  string expected = (
      "On/Off: false\nip: 10.0.0.1\nmac: 01:23:45:67:89:ab\n"
      "uid: 7a70:00000001\nName: foobar\nId: 42\nCount: 4 x 10 ^ -3\n"
      "Delta: 10 x 10 ^ 1\nRate: 10 x 10 ^ -1\n");
  OLA_ASSERT_EQ(expected, m_printer.AsString(&message));
}


/**
 * Check that labels are added
 */
void GenericMessagePrinterTest::testLabeledPrinter() {
  UInt8FieldDescriptor::IntervalVector intervals;
  intervals.push_back(UInt8FieldDescriptor::Interval(0, 2));

  UInt8FieldDescriptor::LabeledValues labels;
  labels["off"] = 0;
  labels["on"] = 1;
  labels["auto"] = 2;

  UInt8FieldDescriptor uint8_descriptor("State", intervals, labels);

  vector<const ola::messaging::MessageFieldInterface*> fields;
  fields.push_back(new UInt8MessageField(&uint8_descriptor, 0));
  fields.push_back(new UInt8MessageField(&uint8_descriptor, 1));
  fields.push_back(new UInt8MessageField(&uint8_descriptor, 2));

  Message message(fields);
  string expected = "State: off\nState: on\nState: auto\n";
  OLA_ASSERT_EQ(expected, m_printer.AsString(&message));
}


void GenericMessagePrinterTest::testNestedPrinter() {
  // this holds some information on people
  StringFieldDescriptor *string_descriptor = new StringFieldDescriptor(
      "Name", 0, 32);
  BoolFieldDescriptor *bool_descriptor = new BoolFieldDescriptor("Female");
  UInt8FieldDescriptor *uint8_descriptor = new UInt8FieldDescriptor("Age");

  vector<const FieldDescriptor*> person_fields;
  person_fields.push_back(string_descriptor);
  person_fields.push_back(bool_descriptor);
  person_fields.push_back(uint8_descriptor);
  FieldDescriptorGroup group_descriptor("Person", person_fields, 0, 10);

  // setup the first person
  vector<const MessageFieldInterface*> person1;
  person1.push_back(new StringMessageField(string_descriptor, "Lisa"));
  person1.push_back(new BoolMessageField(bool_descriptor, true));
  person1.push_back(new UInt8MessageField(uint8_descriptor, 21));

  // setup the second person
  vector<const MessageFieldInterface*> person2;
  person2.push_back(new StringMessageField(string_descriptor, "Simon"));
  person2.push_back(new BoolMessageField(bool_descriptor, false));
  person2.push_back(new UInt8MessageField(uint8_descriptor, 26));

  vector<const ola::messaging::MessageFieldInterface*> messages;
  messages.push_back(new GroupMessageField(&group_descriptor, person1));
  messages.push_back(new GroupMessageField(&group_descriptor, person2));

  Message message(messages);

  string expected = (
      "Person {\n  Name: Lisa\n  Female: true\n  Age: 21\n}\n"
      "Person {\n  Name: Simon\n  Female: false\n  Age: 26\n}\n");
  OLA_ASSERT_EQ(expected, m_printer.AsString(&message));
}
