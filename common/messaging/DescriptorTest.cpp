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
 * DescriptorTest.cpp
 * Test fixture for the Descriptor classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/messaging/Descriptor.h"
#include "ola/testing/TestUtils.h"


using std::string;
using std::vector;


using ola::messaging::BoolFieldDescriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::messaging::IPV4FieldDescriptor;
using ola::messaging::MACFieldDescriptor;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::UIDFieldDescriptor;
using ola::messaging::UInt16FieldDescriptor;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt8FieldDescriptor;

class DescriptorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DescriptorTest);
  CPPUNIT_TEST(testFieldDescriptors);
  CPPUNIT_TEST(testFieldDescriptorGroup);
  CPPUNIT_TEST(testIntervalsAndLabels);
  CPPUNIT_TEST_SUITE_END();

 public:
    DescriptorTest() {}
    void testFieldDescriptors();
    void testFieldDescriptorGroup();
    void testIntervalsAndLabels();
};


CPPUNIT_TEST_SUITE_REGISTRATION(DescriptorTest);


/*
 * Test the FieldDescriptors
 */
void DescriptorTest::testFieldDescriptors() {
  // bool
  BoolFieldDescriptor bool_descriptor("bool");
  OLA_ASSERT_EQ(string("bool"), bool_descriptor.Name());
  OLA_ASSERT_TRUE(bool_descriptor.FixedSize());
  OLA_ASSERT_TRUE(bool_descriptor.LimitedSize());
  OLA_ASSERT_EQ(1u, bool_descriptor.MaxSize());

  // IPv4 address
  IPV4FieldDescriptor ipv4_descriptor("ipv4");
  OLA_ASSERT_EQ(string("ipv4"), ipv4_descriptor.Name());
  OLA_ASSERT_TRUE(ipv4_descriptor.FixedSize());
  OLA_ASSERT_TRUE(ipv4_descriptor.LimitedSize());
  OLA_ASSERT_EQ(4u, ipv4_descriptor.MaxSize());

  // MAC address
  MACFieldDescriptor mac_descriptor("mac");
  OLA_ASSERT_EQ(string("mac"), mac_descriptor.Name());
  OLA_ASSERT_TRUE(mac_descriptor.FixedSize());
  OLA_ASSERT_TRUE(mac_descriptor.LimitedSize());
  OLA_ASSERT_EQ(6u, mac_descriptor.MaxSize());

  // UID
  UIDFieldDescriptor uid_descriptor("uid");
  OLA_ASSERT_EQ(string("uid"), uid_descriptor.Name());
  OLA_ASSERT_TRUE(uid_descriptor.FixedSize());
  OLA_ASSERT_TRUE(uid_descriptor.LimitedSize());
  OLA_ASSERT_EQ(6u, uid_descriptor.MaxSize());

  // string
  StringFieldDescriptor string_descriptor("string", 10, 32);
  OLA_ASSERT_EQ(string("string"), string_descriptor.Name());
  OLA_ASSERT_EQ(10u, string_descriptor.MinSize());
  OLA_ASSERT_EQ(32u, string_descriptor.MaxSize());
  OLA_ASSERT_FALSE(string_descriptor.FixedSize());
  OLA_ASSERT_TRUE(string_descriptor.LimitedSize());

  // uint8_t
  UInt8FieldDescriptor uint8_descriptor("uint8", false, 10);
  OLA_ASSERT_EQ(string("uint8"), uint8_descriptor.Name());
  OLA_ASSERT_EQ(1u, uint8_descriptor.MaxSize());
  OLA_ASSERT_FALSE(uint8_descriptor.IsLittleEndian());
  OLA_ASSERT_EQ(static_cast<int8_t>(10),
                       uint8_descriptor.Multiplier());
  OLA_ASSERT_TRUE(uint8_descriptor.FixedSize());
  OLA_ASSERT_TRUE(uint8_descriptor.LimitedSize());

  UInt8FieldDescriptor uint8_descriptor2("uint8", true, -1);
  OLA_ASSERT_EQ(string("uint8"), uint8_descriptor2.Name());
  OLA_ASSERT_EQ(1u, uint8_descriptor2.MaxSize());
  OLA_ASSERT_TRUE(uint8_descriptor2.IsLittleEndian());
  OLA_ASSERT_EQ(static_cast<int8_t>(-1),
                       uint8_descriptor2.Multiplier());
  OLA_ASSERT_TRUE(uint8_descriptor2.FixedSize());
  OLA_ASSERT_TRUE(uint8_descriptor2.LimitedSize());

  // uint16_t
  UInt16FieldDescriptor uint16_descriptor("uint16", false, 10);
  OLA_ASSERT_EQ(string("uint16"), uint16_descriptor.Name());
  OLA_ASSERT_EQ(2u, uint16_descriptor.MaxSize());
  OLA_ASSERT_FALSE(uint16_descriptor.IsLittleEndian());
  OLA_ASSERT_EQ(static_cast<int8_t>(10),
                       uint16_descriptor.Multiplier());
  OLA_ASSERT_TRUE(uint16_descriptor.FixedSize());
  OLA_ASSERT_TRUE(uint16_descriptor.LimitedSize());

  UInt16FieldDescriptor uint16_descriptor2("uint16", true, -1);
  OLA_ASSERT_EQ(string("uint16"), uint16_descriptor2.Name());
  OLA_ASSERT_EQ(2u, uint16_descriptor2.MaxSize());
  OLA_ASSERT_TRUE(uint16_descriptor2.IsLittleEndian());
  OLA_ASSERT_EQ(static_cast<int8_t>(-1),
                       uint16_descriptor2.Multiplier());
  OLA_ASSERT_TRUE(uint16_descriptor2.FixedSize());
  OLA_ASSERT_TRUE(uint16_descriptor2.LimitedSize());

  // uint32_t
  UInt32FieldDescriptor uint32_descriptor("uint32", false, 10);
  OLA_ASSERT_EQ(string("uint32"), uint32_descriptor.Name());
  OLA_ASSERT_EQ(4u, uint32_descriptor.MaxSize());
  OLA_ASSERT_FALSE(uint32_descriptor.IsLittleEndian());
  OLA_ASSERT_EQ(static_cast<int8_t>(10),
                       uint32_descriptor.Multiplier());
  OLA_ASSERT_TRUE(uint32_descriptor.FixedSize());
  OLA_ASSERT_TRUE(uint32_descriptor.LimitedSize());

  UInt32FieldDescriptor uint32_descriptor2("uint32", true, -1);
  OLA_ASSERT_EQ(string("uint32"), uint32_descriptor2.Name());
  OLA_ASSERT_EQ(4u, uint32_descriptor2.MaxSize());
  OLA_ASSERT_TRUE(uint32_descriptor2.IsLittleEndian());
  OLA_ASSERT_EQ(static_cast<int8_t>(-1),
                       uint32_descriptor2.Multiplier());
  OLA_ASSERT_TRUE(uint32_descriptor2.FixedSize());
  OLA_ASSERT_TRUE(uint32_descriptor2.LimitedSize());
}


/**
 * Check FieldDescriptorGroup
 */
void DescriptorTest::testFieldDescriptorGroup() {
  // first try a group where the fields are all a fixed size
  BoolFieldDescriptor *bool_descriptor = new BoolFieldDescriptor("bool");
  UInt8FieldDescriptor *uint8_descriptor = new UInt8FieldDescriptor(
      "uint8", false, 10);

  // group with a variable number of repeats
  vector<const FieldDescriptor*> fields;
  fields.push_back(bool_descriptor);
  fields.push_back(uint8_descriptor);

  FieldDescriptorGroup group_descriptor("group", fields, 0, 3);
  OLA_ASSERT_FALSE(group_descriptor.FixedSize());
  OLA_ASSERT_TRUE(group_descriptor.LimitedSize());
  OLA_ASSERT_EQ(6u, group_descriptor.MaxSize());
  OLA_ASSERT_EQ(2u, group_descriptor.FieldCount());
  OLA_ASSERT_TRUE(group_descriptor.FixedBlockSize());
  OLA_ASSERT_EQ(2u, group_descriptor.BlockSize());
  OLA_ASSERT_EQ(2u, group_descriptor.MaxBlockSize());
  OLA_ASSERT_EQ(static_cast<uint16_t>(0),
                       group_descriptor.MinBlocks());
  OLA_ASSERT_EQ(static_cast<int16_t>(3),
                       group_descriptor.MaxBlocks());
  OLA_ASSERT_FALSE(group_descriptor.FixedBlockCount());

  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(bool_descriptor),
                       group_descriptor.GetField(0));
  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(uint8_descriptor),
                       group_descriptor.GetField(1));

  // A group with a fixed number of repeats and fixed size fields
  BoolFieldDescriptor *bool_descriptor2 = new BoolFieldDescriptor("bool");
  UInt8FieldDescriptor *uint8_descriptor2 = new UInt8FieldDescriptor(
      "uint8", false, 10);
  UInt16FieldDescriptor *uint16_descriptor2 = new UInt16FieldDescriptor(
      "uint16", false, 10);

  vector<const FieldDescriptor*> fields2;
  fields2.push_back(bool_descriptor2);
  fields2.push_back(uint8_descriptor2);
  fields2.push_back(uint16_descriptor2);

  FieldDescriptorGroup group_descriptor2("group", fields2, 2, 2);
  OLA_ASSERT_TRUE(group_descriptor2.FixedSize());
  OLA_ASSERT_TRUE(group_descriptor2.LimitedSize());
  OLA_ASSERT_EQ(8u, group_descriptor2.MaxSize());
  OLA_ASSERT_EQ(3u, group_descriptor2.FieldCount());
  OLA_ASSERT_TRUE(group_descriptor2.FixedBlockSize());
  OLA_ASSERT_EQ(4u, group_descriptor2.BlockSize());
  OLA_ASSERT_EQ(4u, group_descriptor2.MaxBlockSize());
  OLA_ASSERT_EQ(static_cast<uint16_t>(2),
                       group_descriptor2.MinBlocks());
  OLA_ASSERT_EQ(static_cast<int16_t>(2),
                       group_descriptor2.MaxBlocks());
  OLA_ASSERT_TRUE(group_descriptor2.FixedBlockCount());

  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(bool_descriptor2),
                       group_descriptor2.GetField(0));
  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(uint8_descriptor2),
                       group_descriptor2.GetField(1));
  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(uint16_descriptor2),
                       group_descriptor2.GetField(2));

  // now check a group with variable sized fields
  BoolFieldDescriptor *bool_descriptor3 = new BoolFieldDescriptor("bool");
  StringFieldDescriptor *string_descriptor2 =
    new StringFieldDescriptor("string", 0, 32);
  vector<const FieldDescriptor*> fields3;
  fields3.push_back(bool_descriptor3);
  fields3.push_back(string_descriptor2);

  FieldDescriptorGroup group_descriptor3("group", fields3, 0, 2);
  OLA_ASSERT_FALSE(group_descriptor3.FixedSize());
  OLA_ASSERT_TRUE(group_descriptor3.LimitedSize());
  OLA_ASSERT_EQ(66u, group_descriptor3.MaxSize());
  OLA_ASSERT_EQ(2u, group_descriptor3.FieldCount());
  OLA_ASSERT_FALSE(group_descriptor3.FixedBlockSize());
  OLA_ASSERT_EQ(0u, group_descriptor3.BlockSize());
  OLA_ASSERT_EQ(33u, group_descriptor3.MaxBlockSize());
  OLA_ASSERT_EQ(static_cast<uint16_t>(0),
                       group_descriptor3.MinBlocks());
  OLA_ASSERT_EQ(static_cast<int16_t>(2),
                       group_descriptor3.MaxBlocks());
  OLA_ASSERT_FALSE(group_descriptor3.FixedBlockCount());
  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(bool_descriptor3),
                       group_descriptor3.GetField(0));
  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(string_descriptor2),
                       group_descriptor3.GetField(1));

  // now check a group with variable sized fields but a fixed block count
  BoolFieldDescriptor *bool_descriptor4 = new BoolFieldDescriptor("bool");
  StringFieldDescriptor *string_descriptor3 =
    new StringFieldDescriptor("string", 0, 32);
  vector<const FieldDescriptor*> fields4;
  fields4.push_back(bool_descriptor4);
  fields4.push_back(string_descriptor3);

  FieldDescriptorGroup group_descriptor4("group", fields4, 2, 2);
  OLA_ASSERT_FALSE(group_descriptor4.FixedSize());
  OLA_ASSERT_TRUE(group_descriptor4.LimitedSize());
  OLA_ASSERT_EQ(66u, group_descriptor4.MaxSize());
  OLA_ASSERT_EQ(2u, group_descriptor4.FieldCount());
  OLA_ASSERT_FALSE(group_descriptor4.FixedBlockSize());
  OLA_ASSERT_EQ(0u, group_descriptor4.BlockSize());
  OLA_ASSERT_EQ(33u, group_descriptor4.MaxBlockSize());
  OLA_ASSERT_EQ(static_cast<uint16_t>(2),
                       group_descriptor4.MinBlocks());
  OLA_ASSERT_EQ(static_cast<int16_t>(2),
                       group_descriptor4.MaxBlocks());
  OLA_ASSERT_TRUE(group_descriptor4.FixedBlockCount());
  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(bool_descriptor4),
                       group_descriptor4.GetField(0));
  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(string_descriptor3),
                       group_descriptor4.GetField(1));

  // now check a group with an unlimited block count
  BoolFieldDescriptor *bool_descriptor5 = new BoolFieldDescriptor("bool");
  vector<const FieldDescriptor*> fields5;
  fields5.push_back(bool_descriptor5);

  FieldDescriptorGroup group_descriptor5(
    "group",
    fields5,
    0,
    FieldDescriptorGroup::UNLIMITED_BLOCKS);
  OLA_ASSERT_FALSE(group_descriptor5.FixedSize());
  OLA_ASSERT_FALSE(group_descriptor5.LimitedSize());
  OLA_ASSERT_EQ(0u, group_descriptor5.MaxSize());
  OLA_ASSERT_EQ(1u, group_descriptor5.FieldCount());
  OLA_ASSERT_TRUE(group_descriptor5.FixedBlockSize());
  OLA_ASSERT_EQ(1u, group_descriptor5.BlockSize());
  OLA_ASSERT_EQ(1u, group_descriptor5.MaxBlockSize());
  OLA_ASSERT_EQ(static_cast<uint16_t>(0),
                       group_descriptor5.MinBlocks());
  OLA_ASSERT_EQ(
      static_cast<int16_t>(FieldDescriptorGroup::UNLIMITED_BLOCKS),
      group_descriptor5.MaxBlocks());
  OLA_ASSERT_FALSE(group_descriptor5.FixedBlockCount());
  OLA_ASSERT_EQ(static_cast<const FieldDescriptor*>(bool_descriptor5),
                       group_descriptor5.GetField(0));
}


/**
 * Check that intervals and labels work
 */
void DescriptorTest::testIntervalsAndLabels() {
  UInt16FieldDescriptor::IntervalVector intervals;
  intervals.push_back(UInt16FieldDescriptor::Interval(2, 8));
  intervals.push_back(UInt16FieldDescriptor::Interval(12, 14));

  UInt16FieldDescriptor::LabeledValues labels;
  labels["dozen"] = 12;
  labels["bakers_dozen"] = 13;

  UInt16FieldDescriptor uint16_descriptor("uint16", intervals, labels);

  // check IsValid()
  OLA_ASSERT_FALSE(uint16_descriptor.IsValid(0));
  OLA_ASSERT_FALSE(uint16_descriptor.IsValid(1));
  OLA_ASSERT_TRUE(uint16_descriptor.IsValid(2));
  OLA_ASSERT_TRUE(uint16_descriptor.IsValid(8));
  OLA_ASSERT_FALSE(uint16_descriptor.IsValid(9));
  OLA_ASSERT_FALSE(uint16_descriptor.IsValid(11));
  OLA_ASSERT_TRUE(uint16_descriptor.IsValid(12));
  OLA_ASSERT_TRUE(uint16_descriptor.IsValid(13));
  OLA_ASSERT_TRUE(uint16_descriptor.IsValid(14));
  OLA_ASSERT_FALSE(uint16_descriptor.IsValid(15));
  OLA_ASSERT_FALSE(uint16_descriptor.IsValid(255));
  OLA_ASSERT_FALSE(uint16_descriptor.IsValid(65535));

  // check LookupLabel()
  uint16_t value;
  OLA_ASSERT_FALSE(uint16_descriptor.LookupLabel("one", &value));
  OLA_ASSERT_TRUE(uint16_descriptor.LookupLabel("dozen", &value));
  OLA_ASSERT_EQ(static_cast<uint16_t>(12), value);
  OLA_ASSERT_TRUE(uint16_descriptor.LookupLabel("bakers_dozen", &value));
  OLA_ASSERT_EQ(static_cast<uint16_t>(13), value);
  OLA_ASSERT_FALSE(uint16_descriptor.LookupLabel("twenty", &value));

  // check LookupValue
  OLA_ASSERT_EQ(string(""), uint16_descriptor.LookupValue(0));
  OLA_ASSERT_EQ(string("dozen"), uint16_descriptor.LookupValue(12));
  OLA_ASSERT_EQ(string("bakers_dozen"),
                       uint16_descriptor.LookupValue(13));

  // a Descriptor with no labels or intervals
  UInt16FieldDescriptor::IntervalVector intervals2;
  UInt16FieldDescriptor::LabeledValues labels2;
  UInt16FieldDescriptor uint16_descriptor2("uint16", intervals2, labels2);
  OLA_ASSERT_TRUE(uint16_descriptor2.IsValid(0));
  OLA_ASSERT_TRUE(uint16_descriptor2.IsValid(255));
  OLA_ASSERT_TRUE(uint16_descriptor2.IsValid(65535));
}
