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
 * VariableFieldSizeCalculatorTest.cpp
 * Test fixture for the VariableFieldSizeCalculator.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include "ola/Logging.h"
#include "ola/messaging/Descriptor.h"
#include "common/rdm/VariableFieldSizeCalculator.h"


using ola::messaging::BoolFieldDescriptor;
using ola::messaging::IPV4FieldDescriptor;
using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using ola::messaging::FieldDescriptorGroup;
using ola::messaging::Int16FieldDescriptor;
using ola::messaging::Int32FieldDescriptor;
using ola::messaging::Int8FieldDescriptor;
using ola::messaging::StringFieldDescriptor;
using ola::messaging::UInt16FieldDescriptor;
using ola::messaging::UInt32FieldDescriptor;
using ola::messaging::UInt8FieldDescriptor;
using ola::rdm::VariableFieldSizeCalculator;
using std::vector;


class VariableFieldSizeCalculatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(VariableFieldSizeCalculatorTest);
  CPPUNIT_TEST(testFixedFields);
  CPPUNIT_TEST(testStringFields);
  CPPUNIT_TEST(testWithFixedGroups);
  CPPUNIT_TEST(testSingleVariableSizedGroup);
  CPPUNIT_TEST(testMultipleVariableSizedFields);
  CPPUNIT_TEST(testNestedVariableSizedGroups);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testFixedFields();
    void testStringFields();
    void testWithFixedGroups();
    void testSingleVariableSizedGroup();
    void testMultipleVariableSizedFields();
    void testNestedVariableSizedGroups();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }

  private:
    ola::rdm::VariableFieldSizeCalculator m_calculator;
};


CPPUNIT_TEST_SUITE_REGISTRATION(VariableFieldSizeCalculatorTest);


/**
 * Test that we can determine the token count for descriptors that contain only
 * fixed fields.
 */
void VariableFieldSizeCalculatorTest::testFixedFields() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new BoolFieldDescriptor("bool1"));
  fields.push_back(new IPV4FieldDescriptor("ip1"));
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  fields.push_back(new UInt16FieldDescriptor("uint16"));
  fields.push_back(new UInt32FieldDescriptor("uint32"));
  fields.push_back(new Int8FieldDescriptor("int8"));
  fields.push_back(new Int16FieldDescriptor("int16"));
  fields.push_back(new Int32FieldDescriptor("int32"));
  Descriptor descriptor("Test Descriptor", fields);

  unsigned int variable_field_size;

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_SMALL,
      m_calculator.CalculateFieldSize(
        0,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_SMALL,
      m_calculator.CalculateFieldSize(
        1,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_SMALL,
      m_calculator.CalculateFieldSize(
        14,
        &descriptor,
        &variable_field_size));

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::FIXED_SIZE,
      m_calculator.CalculateFieldSize(
        19,
        &descriptor,
        &variable_field_size));

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_LARGE,
      m_calculator.CalculateFieldSize(
        20,
        &descriptor,
        &variable_field_size));
}


/**
 * Check that we can work out the size for variable sized string fields
 */
void VariableFieldSizeCalculatorTest::testStringFields() {
  vector<const FieldDescriptor*> fields;
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  fields.push_back(new UInt16FieldDescriptor("uint16"));
  fields.push_back(new StringFieldDescriptor("str1", 0, 32));
  Descriptor descriptor("Test Descriptor", fields);

  // set this to something large so we can verify it's set correctly
  unsigned int variable_field_size = 2000;

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_SMALL,
      m_calculator.CalculateFieldSize(
        0,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_SMALL,
      m_calculator.CalculateFieldSize(
        2,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::VARIABLE_STRING,
      m_calculator.CalculateFieldSize(
        3,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(0u, variable_field_size);
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::VARIABLE_STRING,
      m_calculator.CalculateFieldSize(
        4,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(1u, variable_field_size);
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::VARIABLE_STRING,
      m_calculator.CalculateFieldSize(
        34,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(31u, variable_field_size);
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::VARIABLE_STRING,
      m_calculator.CalculateFieldSize(
        35,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(32u, variable_field_size);
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_LARGE,
      m_calculator.CalculateFieldSize(
        36,
        &descriptor,
        &variable_field_size));
}


/**
 * Check the calculators work with fixed groups.
 */
void VariableFieldSizeCalculatorTest::testWithFixedGroups() {
  vector<const FieldDescriptor*> group_fields, group_fields2;
  group_fields.push_back(new BoolFieldDescriptor("bool"));
  group_fields.push_back(new UInt8FieldDescriptor("uint8"));
  group_fields2.push_back(new Int16FieldDescriptor("int16"));
  group_fields2.push_back(new UInt16FieldDescriptor("uint16"));

  vector<const FieldDescriptor*> fields;
  fields.push_back(new FieldDescriptorGroup("", group_fields, 2, 2));
  fields.push_back(new FieldDescriptorGroup("", group_fields2, 2, 2));
  Descriptor descriptor("Test Descriptor", fields);
  unsigned int variable_field_size;

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_SMALL,
      m_calculator.CalculateFieldSize(
        11,
        &descriptor,
        &variable_field_size));

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::FIXED_SIZE,
      m_calculator.CalculateFieldSize(
        12,
        &descriptor,
        &variable_field_size));

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_LARGE,
      m_calculator.CalculateFieldSize(
        13,
        &descriptor,
        &variable_field_size));
}


/*
 * Test that a single variable-sized group passes
 */
void VariableFieldSizeCalculatorTest::testSingleVariableSizedGroup() {
  vector<const FieldDescriptor*> group_fields;
  group_fields.push_back(new BoolFieldDescriptor("bool"));
  group_fields.push_back(new UInt8FieldDescriptor("uint8"));

  vector<const FieldDescriptor*> fields;
  fields.push_back(new UInt8FieldDescriptor("uint8"));
  fields.push_back(new UInt16FieldDescriptor("uint16"));
  fields.push_back(new FieldDescriptorGroup("", group_fields, 0, 2));
  Descriptor descriptor("Test Descriptor", fields);

  unsigned int variable_field_size;
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_SMALL,
      m_calculator.CalculateFieldSize(
        2,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::VARIABLE_GROUP,
      m_calculator.CalculateFieldSize(
        3,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(0u, variable_field_size);
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::MISMATCHED_SIZE,
      m_calculator.CalculateFieldSize(
        4,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::VARIABLE_GROUP,
      m_calculator.CalculateFieldSize(
        5,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(1u, variable_field_size);
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::MISMATCHED_SIZE,
      m_calculator.CalculateFieldSize(
        6,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::VARIABLE_GROUP,
      m_calculator.CalculateFieldSize(
        7,
        &descriptor,
        &variable_field_size));
  CPPUNIT_ASSERT_EQUAL(2u, variable_field_size);

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::TOO_LARGE,
      m_calculator.CalculateFieldSize(
        8,
        &descriptor,
        &variable_field_size));
}


/*
 * Test that multiple variable-sized groups fail
 */
void VariableFieldSizeCalculatorTest::testMultipleVariableSizedFields() {
  // first try a descriptor with two strings
  vector<const FieldDescriptor*> fields;
  fields.push_back(new StringFieldDescriptor("str1", 0, 10));
  fields.push_back(new StringFieldDescriptor("str2", 0 , 10));

  Descriptor string_descriptor("Test Descriptor", fields);
  unsigned int variable_field_size;

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::MULTIPLE_VARIABLE_FIELDS,
      m_calculator.CalculateFieldSize(
        0,
        &string_descriptor,
        &variable_field_size));

  // now try a variable group and a variable string
  vector<const FieldDescriptor*> group_fields, fields2;
  group_fields.push_back(new BoolFieldDescriptor("bool"));
  group_fields.push_back(new UInt8FieldDescriptor("uint8"));

  fields2.push_back(new StringFieldDescriptor("str1", 0, 10));
  fields2.push_back(new FieldDescriptorGroup("", group_fields, 0, 2));
  Descriptor string_group_descriptor("Test Descriptor", fields2);

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::MULTIPLE_VARIABLE_FIELDS,
      m_calculator.CalculateFieldSize(
        0,
        &string_group_descriptor,
        &variable_field_size));

  // now do multiple variable sized groups
  vector<const FieldDescriptor*> group_fields1, group_fields2, fields3;
  group_fields1.push_back(new BoolFieldDescriptor("bool"));
  group_fields1.push_back(new UInt8FieldDescriptor("uint8"));
  group_fields2.push_back(new BoolFieldDescriptor("bool"));
  group_fields2.push_back(new UInt8FieldDescriptor("uint8"));

  fields3.push_back(new FieldDescriptorGroup("", group_fields1, 0, 2));
  fields3.push_back(new FieldDescriptorGroup("", group_fields2, 0, 2));
  Descriptor multi_group_descriptor("Test Descriptor", fields3);

  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::MULTIPLE_VARIABLE_FIELDS,
      m_calculator.CalculateFieldSize(
        0,
        &multi_group_descriptor,
        &variable_field_size));
}

/*
 * Test that a nested, variable sized groups fail
 */
void VariableFieldSizeCalculatorTest::testNestedVariableSizedGroups() {
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
  unsigned int variable_field_size;

  // now check the main calculator.
  CPPUNIT_ASSERT_EQUAL(
      VariableFieldSizeCalculator::NESTED_VARIABLE_GROUPS,
      m_calculator.CalculateFieldSize(
        10,
        &descriptor,
        &variable_field_size));
}
