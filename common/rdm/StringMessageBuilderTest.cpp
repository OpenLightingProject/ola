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
 * StringBuilderTest.cpp
 * Test fixture for the StringBuilder classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/messaging/Descriptor.h"
#include "ola/messaging/Message.h"
#include "ola/messaging/MessagePrinter.h"
#include "ola/rdm/StringMessageBuilder.h"


using ola::messaging::BoolFieldDescriptor;
using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::messaging::IPV4FieldDescriptor;
using ola::messaging::Int16FieldDescriptor;
using ola::messaging::Int32FieldDescriptor;
using ola::messaging::Int8FieldDescriptor;
using ola::messaging::Message;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::UInt16FieldDescriptor;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt8FieldDescriptor;
using ola::rdm::StringMessageBuilder;
using std::auto_ptr;
using std::string;
using std::vector;


class StringBuilderTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(StringBuilderTest);
  CPPUNIT_TEST(testSimpleBuilder);
  CPPUNIT_TEST(testBuilderWithLabels);
  CPPUNIT_TEST(testBuilderWithGroups);
  CPPUNIT_TEST(testBuilderWithNestedGroups);
  CPPUNIT_TEST(testBuilderWithVariableNestedGroups);
  CPPUNIT_TEST(testBoolFailure);
  CPPUNIT_TEST(testUIntFailure);
  CPPUNIT_TEST(testIntFailure);
  CPPUNIT_TEST(testStringFailure);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSimpleBuilder();
    void testBuilderWithLabels();
    void testBuilderWithGroups();
    void testBuilderWithNestedGroups();
    void testBuilderWithVariableNestedGroups();
    void testBoolFailure();
    void testUIntFailure();
    void testIntFailure();
    void testStringFailure();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }

  private:
    ola::messaging::GenericMessagePrinter m_printer;

    const Message *BuildMessage(const Descriptor &descriptor,
                                const vector<string> &inputs);
};


CPPUNIT_TEST_SUITE_REGISTRATION(StringBuilderTest);


/**
 * Build a message from a given set of inputs and return the string
 * representation of the message.
 */
const Message *StringBuilderTest::BuildMessage(
    const Descriptor &descriptor,
    const vector<string> &inputs) {
  StringMessageBuilder builder;
  const Message *message = builder.GetMessage(inputs, &descriptor);
  if (!message)
    OLA_WARN << "Error with field: " << builder.GetError();
  return message;
}


/**
 * Check the StringBuilder works.
 */
void StringBuilderTest::testSimpleBuilder() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new BoolFieldDescriptor("bool1"));
  fields.push_back(new BoolFieldDescriptor("bool2"));
  fields.push_back(new BoolFieldDescriptor("bool3"));
  fields.push_back(new BoolFieldDescriptor("bool4"));
  fields.push_back(new BoolFieldDescriptor("bool5"));
  fields.push_back(new BoolFieldDescriptor("bool6"));
  fields.push_back(new IPV4FieldDescriptor("ip1"));
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  fields.push_back(new UInt16FieldDescriptor("uint16"));
  fields.push_back(new UInt32FieldDescriptor("uint32"));
  fields.push_back(new Int8FieldDescriptor("int8"));
  fields.push_back(new Int16FieldDescriptor("int16"));
  fields.push_back(new Int32FieldDescriptor("int32"));
  fields.push_back(new StringFieldDescriptor("string", 0, 32));
  fields.push_back(new UInt16FieldDescriptor("hex uint16"));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  vector<string> inputs;
  inputs.push_back("true");
  inputs.push_back("false");
  inputs.push_back("1");
  inputs.push_back("0");
  inputs.push_back("TRUE");
  inputs.push_back("FALSE");
  inputs.push_back("10.0.0.1");
  inputs.push_back("255");
  inputs.push_back("300");
  inputs.push_back("66000");
  inputs.push_back("-128");
  inputs.push_back("-300");
  inputs.push_back("-66000");
  inputs.push_back("foo");
  inputs.push_back("0x400");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(fields.size()),
                       message->FieldCount());

  string expected = (
      "bool1: true\nbool2: false\nbool3: true\nbool4: false\nbool5: true\n"
      "bool6: false\nip1: 10.0.0.1\nuint8: 255\nuint16: 300\nuint32: 66000\n"
      "int8: -128\nint16: -300\nint32: -66000\nstring: foo\n"
      "hex uint16: 1024\n");
  CPPUNIT_ASSERT_EQUAL(expected, m_printer.AsString(message.get()));
}


/**
 * Check the builder accepts labels
 */
void StringBuilderTest::testBuilderWithLabels() {
  // build the descriptor
  UInt8FieldDescriptor::IntervalVector intervals;
  UInt8FieldDescriptor::LabeledValues labels;
  labels["dozen"] = 12;
  labels["bakers_dozen"] = 13;

  vector<const FieldDescriptor*> fields;
  fields.push_back(new UInt8FieldDescriptor("uint8", intervals, labels));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  vector<string> inputs;
  inputs.push_back("dozen");
  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(fields.size()),
                       message->FieldCount());

  string expected = "uint8: dozen\n";
  CPPUNIT_ASSERT_EQUAL(expected, m_printer.AsString(message.get()));
}


/**
 * Check the StringBuilder works with variable sized groups.
 */
void StringBuilderTest::testBuilderWithGroups() {
  // build the descriptor
  vector<const FieldDescriptor*> group_fields;
  group_fields.push_back(new BoolFieldDescriptor("bool"));
  group_fields.push_back(new UInt8FieldDescriptor("uint8"));

  vector<const FieldDescriptor*> fields;
  fields.push_back(new FieldDescriptorGroup("group", group_fields, 0, 3));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  vector<string> inputs;
  inputs.push_back("true");
  inputs.push_back("10");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(1), message->FieldCount());

  string expected = (
      "group {\n  bool: true\n  uint8: 10\n}\n");
  CPPUNIT_ASSERT_EQUAL(expected, m_printer.AsString(message.get()));

  // now do multiple groups
  vector<string> inputs2;
  inputs2.push_back("true");
  inputs2.push_back("10");
  inputs2.push_back("true");
  inputs2.push_back("42");
  inputs2.push_back("false");
  inputs2.push_back("240");

  auto_ptr<const Message> message2(BuildMessage(descriptor, inputs2));

  // verify
  CPPUNIT_ASSERT(message2.get());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(3), message2->FieldCount());

  string expected2 = (
      "group {\n  bool: true\n  uint8: 10\n}\n"
      "group {\n  bool: true\n  uint8: 42\n}\n"
      "group {\n  bool: false\n  uint8: 240\n}\n");
  CPPUNIT_ASSERT_EQUAL(expected2, m_printer.AsString(message2.get()));

  // now provide too many inputs
  inputs2.clear();
  inputs2.push_back("true");
  inputs2.push_back("10");
  inputs2.push_back("true");
  inputs2.push_back("42");
  inputs2.push_back("false");
  inputs2.push_back("240");
  inputs2.push_back("false");
  inputs2.push_back("53");

  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs2));
}


/**
 * test StringBuilder with nested fixed groups
 */
void StringBuilderTest::testBuilderWithNestedGroups() {
  vector<const FieldDescriptor*> fields, group_fields, group_fields2;
  group_fields.push_back(new BoolFieldDescriptor("bool"));

  group_fields2.push_back(new Int16FieldDescriptor("int16"));
  group_fields2.push_back(new FieldDescriptorGroup("bar", group_fields, 2, 2));

  const FieldDescriptorGroup *nested_group = new FieldDescriptorGroup(
      "", group_fields2, 0, 4);

  fields.push_back(nested_group);
  Descriptor descriptor("Test Descriptor", fields);

  vector<string> inputs;
  inputs.push_back("1");
  inputs.push_back("true");
  inputs.push_back("true");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(1), message->FieldCount());

  string expected = (
      " {\n  int16: 1\n  bar {\n    bool: true\n  }\n"
      "  bar {\n    bool: true\n  }\n}\n");
  CPPUNIT_ASSERT_EQUAL(expected, m_printer.AsString(message.get()));
}


/**
 * test StringBuilder with nested varible groups.
 */
void StringBuilderTest::testBuilderWithVariableNestedGroups() {
  vector<const FieldDescriptor*> fields, group_fields, group_fields2;
  group_fields.push_back(new BoolFieldDescriptor("bool"));
  group_fields.push_back(new UInt8FieldDescriptor("uint8"));

  group_fields2.push_back(new Int16FieldDescriptor("int16"));
  group_fields2.push_back(new FieldDescriptorGroup("", group_fields, 0, 2));

  const FieldDescriptorGroup *nested_variable_group = new FieldDescriptorGroup(
      "", group_fields2, 0, 4);

  fields.push_back(new Int16FieldDescriptor("int16"));
  fields.push_back(nested_variable_group);
  Descriptor descriptor("Test Descriptor", fields);

  vector<string> inputs;
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs));
}


/**
 * Test that the bool parsing fails with bad data.
 */
void StringBuilderTest::testBoolFailure() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new BoolFieldDescriptor("bool1"));
  Descriptor descriptor("Test Descriptor", fields);

  // bad string input
  vector<string> inputs;
  inputs.push_back("foo");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs));

  // bad int input
  vector<string> inputs2;
  inputs2.push_back("2");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs2));
}


/**
 * Test that the int parsing fails with bad data.
 */
void StringBuilderTest::testUIntFailure() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  Descriptor descriptor("Test Descriptor", fields);

  // bad uint8 input
  vector<string> inputs;
  inputs.push_back("a");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs));
  vector<string> inputs2;
  inputs2.push_back("-1");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs2));
  vector<string> inputs3;
  inputs3.push_back("256");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs3));
}


/**
 * Test that the int parsing fails with bad data.
 */
void StringBuilderTest::testIntFailure() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new Int8FieldDescriptor("int8"));
  Descriptor descriptor("Test Descriptor", fields);

  // bad uint8 input
  vector<string> inputs;
  inputs.push_back("a");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs));
  vector<string> inputs2;
  inputs2.push_back("-129");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs2));
  vector<string> inputs3;
  inputs3.push_back("128");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs3));
}


/**
 * Test that the int parsing fails with bad data.
 */
void StringBuilderTest::testStringFailure() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new StringFieldDescriptor("string", 0, 10));
  Descriptor descriptor("Test Descriptor", fields);

  // bad string input
  vector<string> inputs;
  inputs.push_back("this is a very long string");
  CPPUNIT_ASSERT(!BuildMessage(descriptor, inputs));
}
