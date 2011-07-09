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
  CPPUNIT_TEST(testStringBuilder);
  CPPUNIT_TEST(testBoolFailure);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testStringBuilder();
    void testBoolFailure();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }

  private:
    const Message *BuildMessage(const Descriptor &descriptor,
                                           const vector<string> &inputs);

    const string MessageToString(const Message *message);
};


CPPUNIT_TEST_SUITE_REGISTRATION(StringBuilderTest);


/**
 * Build a message from a given set of inputs and return the string
 * representation of the message.
 */
const Message *StringBuilderTest::BuildMessage(
    const Descriptor &descriptor,
    const vector<string> &inputs) {
  StringMessageBuilder builder(inputs);
  descriptor.Accept(builder);
  const Message *message = builder.GetMessage();
  if (!message)
    OLA_WARN << "Error with field: " << builder.GetError();
  return message;
}


/**
 * Convert a message to a string
 */
const string StringBuilderTest::MessageToString(const Message *message) {
  ola::messaging::MessagePrinter printer;
  message->Accept(printer);
  return printer.AsString();
}


/**
 * Check the StringBuilder works.
 */
void StringBuilderTest::testStringBuilder() {
  // build the descriptor
  vector<const FieldDescriptor*> fields;
  fields.push_back(new BoolFieldDescriptor("bool1"));
  fields.push_back(new BoolFieldDescriptor("bool2"));
  fields.push_back(new BoolFieldDescriptor("bool3"));
  fields.push_back(new BoolFieldDescriptor("bool4"));
  fields.push_back(new BoolFieldDescriptor("bool5"));
  fields.push_back(new BoolFieldDescriptor("bool6"));
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  fields.push_back(new UInt16FieldDescriptor("uint16"));
  fields.push_back(new UInt32FieldDescriptor("uint32"));
  fields.push_back(new Int8FieldDescriptor("int8"));
  fields.push_back(new Int16FieldDescriptor("int16"));
  fields.push_back(new Int32FieldDescriptor("int32"));
  fields.push_back(new StringFieldDescriptor("string", 0, 32));
  Descriptor descriptor("Test Descriptor", fields);

  // now setup the inputs
  vector<string> inputs;
  inputs.push_back("true");
  inputs.push_back("false");
  inputs.push_back("1");
  inputs.push_back("0");
  inputs.push_back("TRUE");
  inputs.push_back("FALSE");
  inputs.push_back("255");
  inputs.push_back("300");
  inputs.push_back("66000");
  inputs.push_back("-128");
  inputs.push_back("-300");
  inputs.push_back("-66000");
  inputs.push_back("foo");

  auto_ptr<const Message> message(BuildMessage(descriptor, inputs));

  // verify
  CPPUNIT_ASSERT(message.get());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(fields.size()),
                       message->FieldCount());

  string expected = (
      "bool1: true\nbool2: false\nbool3: true\nbool4: false\nbool5: true\n"
      "bool6: false\nuint8: 255\nuint16: 300\nuint32: 66000\n"
      "int8: -128\nint16: -300\nint32: -66000\nstring: foo\n");
  CPPUNIT_ASSERT_EQUAL(expected, MessageToString(message.get()));
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
  StringMessageBuilder builder(inputs);
  descriptor.Accept(builder);
  CPPUNIT_ASSERT(!builder.GetMessage());

  // bad int input
  vector<string> inputs2;
  inputs2.push_back("2");
  StringMessageBuilder builder2(inputs2);
  descriptor.Accept(builder2);
  CPPUNIT_ASSERT(!builder2.GetMessage());
}
