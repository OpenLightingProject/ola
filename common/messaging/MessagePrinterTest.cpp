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

using std::string;
using std::vector;


using ola::messaging::BoolFieldDescriptor;
using ola::messaging::BoolMessageField;
using ola::messaging::FieldDescriptor;
using ola::messaging::GroupFieldDescriptor;
using ola::messaging::GroupMessageField;
using ola::messaging::Int8FieldDescriptor;
using ola::messaging::Int16FieldDescriptor;
using ola::messaging::Int8MessageField;
using ola::messaging::Int16MessageField;
using ola::messaging::Message;
using ola::messaging::MessageFieldInterface;
using ola::messaging::MessagePrinter;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::StringMessageField;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt32MessageField;
using ola::messaging::UInt8FieldDescriptor;
using ola::messaging::UInt8MessageField;


class MessagePrinterTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(MessagePrinterTest);
  CPPUNIT_TEST(testSimplePrinter);
  CPPUNIT_TEST(testLabeledPrinter);
  CPPUNIT_TEST(testNestedPrinter);
  CPPUNIT_TEST_SUITE_END();

  public:
    MessagePrinterTest() {}
    void testSimplePrinter();
    void testLabeledPrinter();
    void testNestedPrinter();
};


CPPUNIT_TEST_SUITE_REGISTRATION(MessagePrinterTest);


/*
 * Test the MessagePrinter
 */
void MessagePrinterTest::testSimplePrinter() {
  // setup some fields
  BoolFieldDescriptor bool_descriptor("On/Off");
  BoolMessageField bool_message(&bool_descriptor, false);

  StringFieldDescriptor string_descriptor("Name", 0, 32);
  StringMessageField string_message(&string_descriptor, "foobar");

  UInt32FieldDescriptor uint32_descriptor("Id");
  UInt32MessageField uint32_message(&uint32_descriptor, 42);

  UInt8FieldDescriptor uint8_descriptor("Count", false, -3);
  UInt8MessageField uint8_message(&uint8_descriptor, 4);

  Int8FieldDescriptor int8_descriptor("Delta", false, 1);
  Int8MessageField int8_message(&int8_descriptor, 10);

  Int16FieldDescriptor int16_descriptor("Rate", false, -1);
  Int16MessageField int16_message(&int16_descriptor, 10);

  // try a simple print first
  vector<const ola::messaging::MessageFieldInterface*> fields;
  fields.push_back(&bool_message);
  fields.push_back(&string_message);
  fields.push_back(&uint32_message);
  fields.push_back(&uint8_message);
  fields.push_back(&int8_message);
  fields.push_back(&int16_message);

  Message message(fields);
  MessagePrinter printer;
  message.Accept(printer);

  string expected = (
      "On/Off: false\nName: foobar\nId: 42\nCount: 4 x 10 ^ -3\n"
      "Delta: 10 x 10 ^ 1\nRate: 10 x 10 ^ -1\n");
  CPPUNIT_ASSERT_EQUAL(expected, printer.AsString());
}


/**
 * Check that labels are added
 */
void MessagePrinterTest::testLabeledPrinter() {
  UInt8FieldDescriptor::IntervalVector intervals;
  intervals.push_back(UInt8FieldDescriptor::Interval(0, 2));

  UInt8FieldDescriptor::LabeledValues labels;
  labels["off"] = 0;
  labels["on"] = 1;
  labels["auto"] = 2;

  UInt8FieldDescriptor uint8_descriptor("State", intervals, labels);
  UInt8MessageField uint8_message_off(&uint8_descriptor, 0);
  UInt8MessageField uint8_message_on(&uint8_descriptor, 1);
  UInt8MessageField uint8_message_auto(&uint8_descriptor, 2);

  vector<const ola::messaging::MessageFieldInterface*> fields;
  fields.push_back(&uint8_message_off);
  fields.push_back(&uint8_message_on);
  fields.push_back(&uint8_message_auto);

  Message message(fields);
  MessagePrinter printer;
  message.Accept(printer);

  string expected = "State: off\nState: on\nState: auto\n";
  CPPUNIT_ASSERT_EQUAL(expected, printer.AsString());
}


void MessagePrinterTest::testNestedPrinter() {
  // this holds some information on people
  StringFieldDescriptor string_descriptor("Name", 0, 32);
  BoolFieldDescriptor bool_descriptor("Female");
  UInt8FieldDescriptor uint8_descriptor("Age");

  vector<const FieldDescriptor*> person_fields;
  person_fields.push_back(&string_descriptor);
  person_fields.push_back(&bool_descriptor);
  person_fields.push_back(&uint8_descriptor);
  GroupFieldDescriptor group_descriptor("Person", person_fields, 0, 10);

  // setup the first person
  StringMessageField string_message1(&string_descriptor, "Lisa");
  BoolMessageField bool_message1(&bool_descriptor, true);
  UInt8MessageField uint8_message1(&uint8_descriptor, 21);
  vector<const MessageFieldInterface*> person1;
  person1.push_back(&string_message1);
  person1.push_back(&bool_message1);
  person1.push_back(&uint8_message1);
  GroupMessageField group_message1(&group_descriptor, person1);

  // setup the second person
  StringMessageField string_message2(&string_descriptor, "Simon");
  BoolMessageField bool_message2(&bool_descriptor, false);
  UInt8MessageField uint8_message2(&uint8_descriptor, 26);
  vector<const MessageFieldInterface*> person2;
  person2.push_back(&string_message2);
  person2.push_back(&bool_message2);
  person2.push_back(&uint8_message2);
  GroupMessageField group_message2(&group_descriptor, person2);

  vector<const ola::messaging::MessageFieldInterface*> messages;
  messages.push_back(&group_message1);
  messages.push_back(&group_message2);

  Message message(messages);
  MessagePrinter printer;
  message.Accept(printer);

  string expected = (
      "Person {\n  Name: Lisa\n  Female: true\n  Age: 21\n}\n"
      "Person {\n  Name: Simon\n  Female: false\n  Age: 26\n}\n");
  CPPUNIT_ASSERT_EQUAL(expected, printer.AsString());
}
