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
 * MessageSerializerTest.cpp
 * Test fixture for the MessageSerializer classes
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
#include "ola/rdm/StringMessageBuilder.h"
#include "ola/rdm/MessageSerializer.h"


using ola::messaging::BoolFieldDescriptor;
using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::messaging::Int16FieldDescriptor;
using ola::messaging::Int32FieldDescriptor;
using ola::messaging::Int8FieldDescriptor;
using ola::messaging::Message;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::UInt16FieldDescriptor;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt8FieldDescriptor;
using ola::messaging::UIDFieldDescriptor;
using ola::rdm::StringMessageBuilder;
using ola::rdm::MessageSerializer;
using std::auto_ptr;
using std::string;
using std::vector;


class MessageSerializerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(MessageSerializerTest);
  CPPUNIT_TEST(testSimple);
  CPPUNIT_TEST(testString);
  CPPUNIT_TEST(testUID);
  CPPUNIT_TEST(testLittleEndian);
  CPPUNIT_TEST(testWithGroups);
  CPPUNIT_TEST(testWithNestedGroups);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testSimple();
    void testString();
    void testUID();
    void testLittleEndian();
    void testWithGroups();
    void testWithNestedGroups();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }

  private:
    const Message *BuildMessage(const Descriptor &descriptor,
                                const vector<string> &inputs);

    void ConfirmData(unsigned long line,
                     const uint8_t *expected,
                     unsigned int expected_length,
                     const uint8_t *actual,
                     unsigned int actual_length);
};


CPPUNIT_TEST_SUITE_REGISTRATION(MessageSerializerTest);


/**
 * Confirm the data matches what we expected
 */
void MessageSerializerTest::ConfirmData(unsigned long line,
                                        const uint8_t *expected,
                                        unsigned int expected_length,
                                        const uint8_t *actual,
                                        unsigned int actual_length) {
  std::stringstream str;
  str << "Line " << line;
  CPPUNIT_ASSERT_EQUAL_MESSAGE(str.str(), expected_length, actual_length);
  for (unsigned int i = 0; i < expected_length; ++i) {
    str.str("");
    str << "line " << line << ", offset " << i << ": " <<
      static_cast<unsigned int>(expected[i]) <<
      " != " << static_cast<unsigned int>(actual[i]);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(str.str(), expected[i], actual[i]);
  }
}


/**
 * Build a message from a given set of inputs and return the string
 * representation of the message.
 */
const Message *MessageSerializerTest::BuildMessage(
    const Descriptor &descriptor,
    const vector<string> &inputs) {
  StringMessageBuilder builder;
  const Message *message = builder.GetMessage(inputs, &descriptor);
  if (!message)
    OLA_WARN << "Error with field: " << builder.GetError();
  return message;
}


/**
 * Check the MessageSerializer works.
 */
void MessageSerializerTest::testSimple() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new BoolFieldDescriptor("bool1"));
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  fields.push_back(new Int8FieldDescriptor("int8"));
  fields.push_back(new UInt16FieldDescriptor("uint16"));
  fields.push_back(new Int16FieldDescriptor("int16"));
  fields.push_back(new UInt32FieldDescriptor("uint32"));
  fields.push_back(new Int32FieldDescriptor("int32"));
  fields.push_back(new StringFieldDescriptor("string", 0, 32));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  vector<string> inputs;
  inputs.push_back("true");
  inputs.push_back("1");
  inputs.push_back("-3");
  inputs.push_back("300");
  inputs.push_back("-400");
  inputs.push_back("66000");
  inputs.push_back("-66000");
  inputs.push_back("foo");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  MessageSerializer serializer;
  unsigned int packed_length;
  const uint8_t *data = serializer.SerializeMessage(message.get(),
                                                    &packed_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(18u, packed_length);

  uint8_t expected[] = {
    1, 1, 253, 1, 44, 254, 112,
    0, 1, 1, 208, 255, 254, 254, 48,
    'f', 'o', 'o'};
  ConfirmData(__LINE__,
              expected,
              sizeof(expected),
              data,
              packed_length);
}


/**
 * Check that strings do the right thing
 */
void MessageSerializerTest::testString() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new StringFieldDescriptor("string", 10, 10));
  fields.push_back(new StringFieldDescriptor("string", 0, 32));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  vector<string> inputs;
  inputs.push_back("foo bar");  // this is shorter than the min size
  inputs.push_back("long long foo bar baz");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  MessageSerializer serializer;
  unsigned int packed_length;
  const uint8_t *data = serializer.SerializeMessage(message.get(),
                                                    &packed_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(31u, packed_length);

  uint8_t expected[] = "foo bar\0\0\0long long foo bar baz";
  ConfirmData(__LINE__,
              expected,
              sizeof(expected) - 1,  // ignore the trailing \0
              data,
              packed_length);
}


/**
 * Check that UIDs work.
 */
void MessageSerializerTest::testUID() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new UIDFieldDescriptor("Address"));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  vector<string> inputs;
  inputs.push_back("7a70:00000001");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  MessageSerializer serializer;
  unsigned int packed_length;
  const uint8_t *data = serializer.SerializeMessage(message.get(),
                                                    &packed_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(6u, packed_length);

  uint8_t expected[] = {0x7a, 0x70, 0, 0, 0, 1};
  ConfirmData(__LINE__, expected, sizeof(expected), data, packed_length);
}


/**
 * Check the MessageSerializer works with little endian fields.
 */
void MessageSerializerTest::testLittleEndian() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new UInt8FieldDescriptor("uint8", true));
  fields.push_back(new Int8FieldDescriptor("int8", true));
  fields.push_back(new UInt16FieldDescriptor("uint16", true));
  fields.push_back(new Int16FieldDescriptor("int16", true));
  fields.push_back(new UInt32FieldDescriptor("uint32", true));
  fields.push_back(new Int32FieldDescriptor("int32", true));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  vector<string> inputs;
  inputs.push_back("1");
  inputs.push_back("-3");
  inputs.push_back("300");
  inputs.push_back("-400");
  inputs.push_back("66000");
  inputs.push_back("-66000");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  MessageSerializer serializer;
  unsigned int packed_length;
  const uint8_t *data = serializer.SerializeMessage(message.get(),
                                                    &packed_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(14u, packed_length);

  uint8_t expected[] = {
    1, 253, 44, 1, 112, 254,
    208, 1, 1, 0, 48, 254, 254, 255};
  ConfirmData(__LINE__,
              expected,
              sizeof(expected),
              data,
              packed_length);
}


/**
 * Check the MessageSerializer works with variable sized groups.
 */
void MessageSerializerTest::testWithGroups() {
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
  MessageSerializer serializer;

  unsigned int packed_length;
  const uint8_t *data = serializer.SerializeMessage(message.get(),
                                                    &packed_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(2u, packed_length);
  uint8_t expected[] = {1, 10};
  ConfirmData(__LINE__,
              expected,
              sizeof(expected),
              data,
              packed_length);

  // now do multiple groups
  vector<string> inputs2;
  inputs2.push_back("true");
  inputs2.push_back("10");
  inputs2.push_back("true");
  inputs2.push_back("42");
  inputs2.push_back("false");
  inputs2.push_back("240");

  auto_ptr<const Message> message2(BuildMessage(descriptor, inputs2));
  data = serializer.SerializeMessage(message2.get(), &packed_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(6u, packed_length);
  uint8_t expected2[] = {1, 10, 1, 42, 0, 240};
  ConfirmData(__LINE__,
              expected2,
              sizeof(expected2),
              data,
              packed_length);
}


/**
 * test MessageSerializer with nested fixed groups
 */
void MessageSerializerTest::testWithNestedGroups() {
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
  inputs.push_back("2");
  inputs.push_back("true");
  inputs.push_back("false");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));
  CPPUNIT_ASSERT(message.get());
  MessageSerializer serializer;

  unsigned int packed_length;
  const uint8_t *data = serializer.SerializeMessage(message.get(),
                                                    &packed_length);
  CPPUNIT_ASSERT(data);
  CPPUNIT_ASSERT_EQUAL(8u, packed_length);
  uint8_t expected[] = {0, 1, 1, 1, 0, 2, 1, 0};
  ConfirmData(__LINE__,
              expected,
              sizeof(expected),
              data,
              packed_length);
}
