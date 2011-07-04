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
 * DescriptorTest.cpp
 * Test fixture for the Descriptor classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/messaging/Descriptor.h"

using std::string;
using std::vector;


using ola::messaging::BoolFieldDescriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::GroupFieldDescriptor;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::UInt16FieldDescriptor;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt8FieldDescriptor;

class DescriptorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DescriptorTest);
  CPPUNIT_TEST(testFieldDescriptors);
  CPPUNIT_TEST_SUITE_END();

  public:
    DescriptorTest() {}
    void testFieldDescriptors();
};


CPPUNIT_TEST_SUITE_REGISTRATION(DescriptorTest);


/*
 * Test the FieldDescriptors
 */
void DescriptorTest::testFieldDescriptors() {
  // bool
  BoolFieldDescriptor bool_descriptor("bool");
  CPPUNIT_ASSERT_EQUAL(string("bool"), bool_descriptor.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(1), bool_descriptor.Size());
  CPPUNIT_ASSERT_EQUAL(true, bool_descriptor.FixedSize());

  // string
  StringFieldDescriptor string_descriptor("string", 10, 32);
  CPPUNIT_ASSERT_EQUAL(string("string"), string_descriptor.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(32),
                       string_descriptor.Size());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(10),
                       string_descriptor.MinSize());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(32),
                       string_descriptor.MaxSize());
  CPPUNIT_ASSERT_EQUAL(false, string_descriptor.FixedSize());

  // uint8_t
  UInt8FieldDescriptor uint8_descriptor("uint8", false, 10);
  CPPUNIT_ASSERT_EQUAL(string("uint8"), uint8_descriptor.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(1),
                       uint8_descriptor.Size());
  CPPUNIT_ASSERT_EQUAL(false, uint8_descriptor.IsLittleEndian());
  CPPUNIT_ASSERT_EQUAL(static_cast<int8_t>(10),
                       uint8_descriptor.Multiplier());
  CPPUNIT_ASSERT_EQUAL(true, uint8_descriptor.FixedSize());

  UInt8FieldDescriptor uint8_descriptor2("uint8", true, -1);
  CPPUNIT_ASSERT_EQUAL(string("uint8"), uint8_descriptor2.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(1),
                       uint8_descriptor2.Size());
  CPPUNIT_ASSERT_EQUAL(true, uint8_descriptor2.IsLittleEndian());
  CPPUNIT_ASSERT_EQUAL(static_cast<int8_t>(-1),
                       uint8_descriptor2.Multiplier());
  CPPUNIT_ASSERT_EQUAL(true, uint8_descriptor2.FixedSize());

  // uint16_t
  UInt16FieldDescriptor uint16_descriptor("uint16", false, 10);
  CPPUNIT_ASSERT_EQUAL(string("uint16"), uint16_descriptor.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(2),
                       uint16_descriptor.Size());
  CPPUNIT_ASSERT_EQUAL(false, uint16_descriptor.IsLittleEndian());
  CPPUNIT_ASSERT_EQUAL(static_cast<int8_t>(10),
                       uint16_descriptor.Multiplier());
  CPPUNIT_ASSERT_EQUAL(true, uint16_descriptor.FixedSize());

  UInt16FieldDescriptor uint16_descriptor2("uint16", true, -1);
  CPPUNIT_ASSERT_EQUAL(string("uint16"), uint16_descriptor2.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(2),
                       uint16_descriptor2.Size());
  CPPUNIT_ASSERT_EQUAL(true, uint16_descriptor2.IsLittleEndian());
  CPPUNIT_ASSERT_EQUAL(static_cast<int8_t>(-1),
                       uint16_descriptor2.Multiplier());
  CPPUNIT_ASSERT_EQUAL(true, uint16_descriptor2.FixedSize());

  // uint32_t
  UInt32FieldDescriptor uint32_descriptor("uint32", false, 10);
  CPPUNIT_ASSERT_EQUAL(string("uint32"), uint32_descriptor.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(4),
                       uint32_descriptor.Size());
  CPPUNIT_ASSERT_EQUAL(false, uint32_descriptor.IsLittleEndian());
  CPPUNIT_ASSERT_EQUAL(static_cast<int8_t>(10),
                       uint32_descriptor.Multiplier());
  CPPUNIT_ASSERT_EQUAL(true, uint32_descriptor.FixedSize());

  UInt32FieldDescriptor uint32_descriptor2("uint32", true, -1);
  CPPUNIT_ASSERT_EQUAL(string("uint32"), uint32_descriptor2.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(4),
                       uint32_descriptor2.Size());
  CPPUNIT_ASSERT_EQUAL(true, uint32_descriptor2.IsLittleEndian());
  CPPUNIT_ASSERT_EQUAL(static_cast<int8_t>(-1),
                       uint32_descriptor2.Multiplier());
  CPPUNIT_ASSERT_EQUAL(true, uint32_descriptor2.FixedSize());

  // group with a variable number of repeats
  std::vector<FieldDescriptor*> fields;
  fields.push_back(&bool_descriptor);
  fields.push_back(&uint8_descriptor);

  GroupFieldDescriptor group_descriptor("group", fields, 0, 3);
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(2),
                       group_descriptor.FieldCount());
  CPPUNIT_ASSERT_EQUAL(static_cast<const FieldDescriptor*>(&bool_descriptor),
                       group_descriptor.GetField(0));
  CPPUNIT_ASSERT_EQUAL(static_cast<const FieldDescriptor*>(&uint8_descriptor),
                       group_descriptor.GetField(1));
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(0),
                       group_descriptor.MinSize());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(3),
                       group_descriptor.MaxSize());
  CPPUNIT_ASSERT_EQUAL(false, group_descriptor.FixedSize());

  // A group with a fixed number of repeats.
  GroupFieldDescriptor group_descriptor2("group", fields, 2, 2);
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(2),
                       group_descriptor2.MinSize());
  CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(2),
                       group_descriptor2.MaxSize());
  CPPUNIT_ASSERT_EQUAL(true, group_descriptor2.FixedSize());
}
