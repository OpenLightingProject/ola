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
 * MessageDeserializerTest.cpp
 * Test fixture for the MessageDeserializer classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/messaging/Descriptor.h"
#include "ola/messaging/Message.h"
#include "ola/messaging/MessagePrinter.h"
#include "ola/rdm/MessageSerializer.h"
#include "ola/rdm/MessageDeserializer.h"
#include "ola/testing/TestUtils.h"



using ola::messaging::BoolFieldDescriptor;
using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::messaging::Int16FieldDescriptor;
using ola::messaging::Int32FieldDescriptor;
using ola::messaging::Int64FieldDescriptor;
using ola::messaging::Int8FieldDescriptor;
using ola::messaging::IPV4FieldDescriptor;
using ola::messaging::IPV6FieldDescriptor;
using ola::messaging::MACFieldDescriptor;
using ola::messaging::Message;
using ola::messaging::GenericMessagePrinter;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::UInt16FieldDescriptor;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt64FieldDescriptor;
using ola::messaging::UInt8FieldDescriptor;
using ola::messaging::UIDFieldDescriptor;
using ola::rdm::MessageDeserializer;
using std::auto_ptr;
using std::string;
using std::vector;


class MessageDeserializerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(MessageDeserializerTest);
  CPPUNIT_TEST(testEmpty);
  CPPUNIT_TEST(testSimpleBigEndian);
  CPPUNIT_TEST(testSimpleLittleEndian);
  CPPUNIT_TEST(testIPV4);
  CPPUNIT_TEST(testIPV6);
  CPPUNIT_TEST(testMAC);
  CPPUNIT_TEST(testString);
  CPPUNIT_TEST(testUID);
  CPPUNIT_TEST(testWithGroups);
  CPPUNIT_TEST(testWithNestedFixedGroups);
  CPPUNIT_TEST(testWithNestedVariableGroups);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testEmpty();
    void testSimpleBigEndian();
    void testSimpleLittleEndian();
    void testIPV4();
    void testIPV6();
    void testMAC();
    void testString();
    void testUID();
    void testWithGroups();
    void testWithNestedFixedGroups();
    void testWithNestedVariableGroups();

 private:
    MessageDeserializer m_deserializer;
    GenericMessagePrinter m_printer;
};


CPPUNIT_TEST_SUITE_REGISTRATION(MessageDeserializerTest);


/**
 * Check that empty messages work.
 */
void MessageDeserializerTest::testEmpty() {
  vector<const FieldDescriptor*> fields;
  Descriptor descriptor("Empty Descriptor", fields);

  auto_ptr<const Message> empty_message(m_deserializer.InflateMessage(
      &descriptor,
      NULL,
      0));
  OLA_ASSERT_NOT_NULL(empty_message.get());
  OLA_ASSERT_EQ(0u, empty_message->FieldCount());

  // now and try to pass in too much data
  const uint8_t data[] = {0, 1, 2};
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      sizeof(data)));
}


/**
 * Test that simple (no variable sized fields) work big endian style.
 */
void MessageDeserializerTest::testSimpleBigEndian() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  // All multi-byte fields default to big endian
  fields.push_back(new BoolFieldDescriptor("bool"));
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  fields.push_back(new Int8FieldDescriptor("int8"));
  fields.push_back(new UInt16FieldDescriptor("uint16"));
  fields.push_back(new Int16FieldDescriptor("int16"));
  fields.push_back(new UInt32FieldDescriptor("uint32"));
  fields.push_back(new Int32FieldDescriptor("int32"));
  fields.push_back(new UInt64FieldDescriptor("uint64"));
  fields.push_back(new Int64FieldDescriptor("int64"));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the data
  const uint8_t big_endian_data[] = {
    0, 10, 246, 1, 0x2c, 0xfe, 10,
    1, 2, 3, 4, 0xfe, 6, 7, 8,
    0, 0, 0, 17, 237, 142, 194, 0,
    255, 255, 255, 238, 18, 113, 62, 0
};

  // try to inflate with no data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      NULL,
      0));

  // now inflate with too little data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      big_endian_data,
      1));

  // now inflate with too much data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      big_endian_data,
      sizeof(big_endian_data) + 1));

  // now the correct amount & verify
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      big_endian_data,
      sizeof(big_endian_data)));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(9u, message->FieldCount());

  const string expected = (
      "bool: false\nuint8: 10\nint8: -10\nuint16: 300\nint16: -502\n"
      "uint32: 16909060\nint32: -33159416\n"
      "uint64: 77000000000\nint64: -77000000000\n");
  OLA_ASSERT_EQ(expected, m_printer.AsString(message.get()));
}


/**
 * Test that simple (no variable sized fields) work little endian style.
 */
void MessageDeserializerTest::testSimpleLittleEndian() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new BoolFieldDescriptor("bool"));
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  fields.push_back(new Int8FieldDescriptor("int8"));
  fields.push_back(new UInt16FieldDescriptor("uint16", true));
  fields.push_back(new Int16FieldDescriptor("int16", true));
  fields.push_back(new UInt32FieldDescriptor("uint32", true));
  fields.push_back(new Int32FieldDescriptor("int32", true));
  fields.push_back(new UInt64FieldDescriptor("uint64", true));
  fields.push_back(new Int64FieldDescriptor("int64", true));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the data
  const uint8_t little_endian_data[] = {
    1, 10, 246, 0x2c, 1, 10, 0xfe,
    4, 3, 2, 1, 8, 7, 6, 0xfe,
    0, 194, 142, 237, 17, 0, 0, 0,
    0, 62, 113, 18, 238, 255, 255, 255
};

  // try to inflate with no data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      NULL,
      0));

  // now inflate with too little data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      little_endian_data,
      1));

  // now inflate with too much data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      little_endian_data,
      sizeof(little_endian_data) + 1));

  // now the correct amount & verify
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      little_endian_data,
      sizeof(little_endian_data)));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(9u, message->FieldCount());

  const string expected = (
      "bool: true\nuint8: 10\nint8: -10\nuint16: 300\nint16: -502\n"
      "uint32: 16909060\nint32: -33159416\n"
      "uint64: 77000000000\nint64: -77000000000\n");
  OLA_ASSERT_EQ(expected, m_printer.AsString(message.get()));
}


/**
 * Test IPV4 inflation.
 */
void MessageDeserializerTest::testIPV4() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new IPV4FieldDescriptor("Address"));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the data
  const uint8_t big_endian_data[] = {10, 0, 0, 1};

  // now the correct amount & verify
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      big_endian_data,
      sizeof(big_endian_data)));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(1u, message->FieldCount());

  const string expected = "Address: 10.0.0.1\n";
  OLA_ASSERT_EQ(expected, m_printer.AsString(message.get()));
}


/**
 * Test IPV6 inflation.
 */
void MessageDeserializerTest::testIPV6() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new IPV6FieldDescriptor("Addressv6"));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the data
  const uint8_t big_endian_data[] = {0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 255, 255, 10, 0, 0, 1};

  // now the correct amount & verify
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      big_endian_data,
      sizeof(big_endian_data)));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(1u, message->FieldCount());

  const string expected = "Addressv6: ::ffff:10.0.0.1\n";
  OLA_ASSERT_EQ(expected, m_printer.AsString(message.get()));
}


/**
 * Test MAC inflation.
 */
void MessageDeserializerTest::testMAC() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new MACFieldDescriptor("Address"));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the data
  const uint8_t big_endian_data[] = {1, 35, 69, 103, 137, 171};

  // now the correct amount & verify
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      big_endian_data,
      sizeof(big_endian_data)));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(1u, message->FieldCount());

  const string expected = "Address: 01:23:45:67:89:ab\n";
  OLA_ASSERT_EQ(expected, m_printer.AsString(message.get()));
}


/**
 * Check that strings do the right thing
 */
void MessageDeserializerTest::testString() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new StringFieldDescriptor("string", 10, 10));
  fields.push_back(new StringFieldDescriptor("string", 0, 32));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the data
  const uint8_t data[] = "0123456789this is a longer string";

  // try to inflate with too little
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      0));
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      9));

  // try to inflat with too much data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      43));

  // now to the right amount
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      data,
      sizeof(data)));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(2u, message->FieldCount());

  const string expected = (
      "string: 0123456789\nstring: this is a longer string\n");
  OLA_ASSERT_EQ(expected, m_printer.AsString(message.get()));

  // now try with different sizes
  auto_ptr<const Message> message2(m_deserializer.InflateMessage(
      &descriptor,
      data,
      19));
  OLA_ASSERT_NOT_NULL(message2.get());
  OLA_ASSERT_EQ(2u, message2->FieldCount());

  const string expected2 = (
      "string: 0123456789\nstring: this is a\n");
  OLA_ASSERT_EQ(expected2, m_printer.AsString(message2.get()));
}


/**
 * Test UID inflation.
 */
void MessageDeserializerTest::testUID() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new UIDFieldDescriptor("Address"));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the data
  const uint8_t big_endian_data[] = {0x70, 0x7a, 0, 0, 0, 1};

  // now the correct amount & verify
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      big_endian_data,
      sizeof(big_endian_data)));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(1u, message->FieldCount());

  const string expected = "Address: 707a:00000001\n";
  OLA_ASSERT_EQ(expected, m_printer.AsString(message.get()));
}


/*
 * Check the MessageSerializer works with variable sized groups.
 */
void MessageDeserializerTest::testWithGroups() {
  // build the descriptor
  vector<const FieldDescriptor*> group_fields;
  group_fields.push_back(new BoolFieldDescriptor("bool"));
  group_fields.push_back(new UInt8FieldDescriptor("uint8"));

  vector<const FieldDescriptor*> fields;
  fields.push_back(new FieldDescriptorGroup("group", group_fields, 0, 3));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  const uint8_t data[] = {0, 10, 1, 3, 0, 20, 1, 40};

  // an empty message
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      data,
      0));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(0u, message->FieldCount());

  // message with not enough data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      1));

  // a single instance of a group
  auto_ptr<const Message> message2(m_deserializer.InflateMessage(
      &descriptor,
      data,
      2));
  OLA_ASSERT_NOT_NULL(message2.get());
  OLA_ASSERT_EQ(1u,
                       message2->FieldCount());

  const string expected = "group {\n  bool: false\n  uint8: 10\n}\n";
  OLA_ASSERT_EQ(expected, m_printer.AsString(message2.get()));

  // another message with not enough data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      3));

  // two instances of the group
  auto_ptr<const Message> message3(m_deserializer.InflateMessage(
      &descriptor,
      data,
      4));
  OLA_ASSERT_NOT_NULL(message3.get());
  OLA_ASSERT_EQ(2u, message3->FieldCount());

  const string expected2 = (
      "group {\n  bool: false\n  uint8: 10\n}\n"
      "group {\n  bool: true\n  uint8: 3\n}\n");
  OLA_ASSERT_EQ(expected2, m_printer.AsString(message3.get()));

  // trhee instances of the group
  auto_ptr<const Message> message4(m_deserializer.InflateMessage(
      &descriptor,
      data,
      6));
  OLA_ASSERT_NOT_NULL(message4.get());
  OLA_ASSERT_EQ(3u,
                       message4->FieldCount());

  const string expected3 = (
      "group {\n  bool: false\n  uint8: 10\n}\n"
      "group {\n  bool: true\n  uint8: 3\n}\n"
      "group {\n  bool: false\n  uint8: 20\n}\n");
  OLA_ASSERT_EQ(expected3, m_printer.AsString(message4.get()));

  // message with too much data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      sizeof(data)));
}


/*
 * Test MessageSerializer with nested fixed groups.
 */
void MessageDeserializerTest::testWithNestedFixedGroups() {
  vector<const FieldDescriptor*> fields, group_fields, group_fields2;
  group_fields.push_back(new BoolFieldDescriptor("bool"));
  group_fields2.push_back(new UInt8FieldDescriptor("uint8"));
  group_fields2.push_back(new FieldDescriptorGroup("bar", group_fields, 2, 2));
  fields.push_back(new FieldDescriptorGroup("", group_fields2, 0, 4));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  const uint8_t data[] = {0, 0, 0, 1, 0, 1, 2, 1, 0, 3, 1, 1};

  // an empty message
  auto_ptr<const Message> message(m_deserializer.InflateMessage(
      &descriptor,
      data,
      0));
  OLA_ASSERT_NOT_NULL(message.get());
  OLA_ASSERT_EQ(0u, message->FieldCount());

  // message with not enough data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      1));
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      2));

  // a single instance of a group
  auto_ptr<const Message> message2(m_deserializer.InflateMessage(
      &descriptor,
      data,
      3));
  OLA_ASSERT_NOT_NULL(message2.get());
  OLA_ASSERT_EQ(1u, message2->FieldCount());

  const string expected = (
      " {\n  uint8: 0\n  bar {\n    bool: false\n  }\n  bar {\n"
      "    bool: false\n  }\n}\n");
  OLA_ASSERT_EQ(expected, m_printer.AsString(message2.get()));

  // four instances
  auto_ptr<const Message> message3(m_deserializer.InflateMessage(
      &descriptor,
      data,
      sizeof(data)));
  OLA_ASSERT_NOT_NULL(message3.get());
  OLA_ASSERT_EQ(4u, message3->FieldCount());

  const string expected2 = (
      " {\n  uint8: 0\n  bar {\n    bool: false\n  }\n  bar {\n"
      "    bool: false\n  }\n}\n"
      " {\n  uint8: 1\n  bar {\n    bool: false\n  }\n  bar {\n"
      "    bool: true\n  }\n}\n"
      " {\n  uint8: 2\n  bar {\n    bool: true\n  }\n  bar {\n"
      "    bool: false\n  }\n}\n"
      " {\n  uint8: 3\n  bar {\n    bool: true\n  }\n  bar {\n"
      "    bool: true\n  }\n}\n");
  OLA_ASSERT_EQ(expected2, m_printer.AsString(message3.get()));

  // too much data
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      sizeof(data) + 1));
}


/*
 * Test MessageSerializer with nested variable groups, this should never
 * extract.
 */
void MessageDeserializerTest::testWithNestedVariableGroups() {
  vector<const FieldDescriptor*> fields, group_fields, group_fields2;
  group_fields.push_back(new BoolFieldDescriptor("bool"));
  group_fields2.push_back(new Int16FieldDescriptor("uint16"));
  group_fields2.push_back(new FieldDescriptorGroup("bar", group_fields, 0, 2));
  fields.push_back(new FieldDescriptorGroup("", group_fields2, 0, 4));
  Descriptor descriptor("Test Descriptor", fields);

  // an empty message would be valid.
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      NULL,
      0));

  const uint8_t data[] = {0, 1, 0, 1};
  // none of these are valid
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      1));
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      2));
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      3));
  OLA_ASSERT_NULL(m_deserializer.InflateMessage(
      &descriptor,
      data,
      4));
}
