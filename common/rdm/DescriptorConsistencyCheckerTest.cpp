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
 * DescriptorConsistencyCheckerTest.cpp
 * Test fixture for the DescriptorConsistencyChecker class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include "ola/Logging.h"
#include "ola/messaging/Descriptor.h"
#include "common/rdm/DescriptorConsistencyChecker.h"


using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using ola::rdm::DescriptorConsistencyChecker;
using std::vector;


class DescriptorConsistencyCheckerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DescriptorConsistencyCheckerTest);
  CPPUNIT_TEST(testOkDescriptors);
  CPPUNIT_TEST(testDuplicateStrings);
  CPPUNIT_TEST(testNestedGroups);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testOkDescriptors();
    void testDuplicateStrings();
    void testNestedGroups();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_DEBUG, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(DescriptorConsistencyCheckerTest);


/*
 * Test the simple Descriptor cases
 */
void DescriptorConsistencyCheckerTest::testOkDescriptors() {
  DescriptorConsistencyChecker checker;

  // test the empty descriptor
  vector<const class FieldDescriptor*> fields;
  const Descriptor empty_descriptor("Empty", fields);
  CPPUNIT_ASSERT(checker.CheckConsistency(&empty_descriptor));

  // now a simple multi-field descriptor
  vector<const class FieldDescriptor*> fields2;
  fields2.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  fields2.push_back(new ola::messaging::BoolFieldDescriptor("bool"));
  const Descriptor simple_descriptor("Simple", fields2);
  CPPUNIT_ASSERT(checker.CheckConsistency(&simple_descriptor));

  // now a multi-field descriptor with a variable string
  vector<const class FieldDescriptor*> fields3;
  fields3.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  fields3.push_back(
      new ola::messaging::StringFieldDescriptor("string1", 0, 32));
  const Descriptor simple_string_descriptor("Simple", fields3);
  CPPUNIT_ASSERT(checker.CheckConsistency(&simple_string_descriptor));
}


/*
 * Verify that the check fails if the descriptor contains mutiple, variable
 * length strings. Also check that it passes if there are multiple, fixed
 * length strings.
 */
void DescriptorConsistencyCheckerTest::testDuplicateStrings() {
  DescriptorConsistencyChecker checker;

  // test fixed length strings
  vector<const class FieldDescriptor*> fields;
  fields.push_back(new ola::messaging::StringFieldDescriptor("string1", 4, 4));
  fields.push_back(new ola::messaging::StringFieldDescriptor("string2", 4, 4));
  const Descriptor fixed_length_descriptor("Fixed", fields);
  CPPUNIT_ASSERT(checker.CheckConsistency(&fixed_length_descriptor));

  // variable length strings
  vector<const class FieldDescriptor*> fields2;
  fields2.push_back(
      new ola::messaging::StringFieldDescriptor("string1", 4, 32));
  fields2.push_back(
      new ola::messaging::StringFieldDescriptor("string2", 4, 32));
  const Descriptor variable_length_descriptor("Variable", fields2);
  CPPUNIT_ASSERT(!checker.CheckConsistency(&variable_length_descriptor));
}


/**
 * Verify that groups produce the correct results
 */
void DescriptorConsistencyCheckerTest::testNestedGroups() {
  DescriptorConsistencyChecker checker;

  // test a single, fixed sized group
  vector<const class FieldDescriptor*> fields, group_fields;
  group_fields.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  fields.push_back(
      new ola::messaging::FieldDescriptorGroup("group", group_fields, 2, 2));

  const Descriptor fixed_length_descriptor("SingleFixed", fields);
  CPPUNIT_ASSERT(checker.CheckConsistency(&fixed_length_descriptor));

  // test multiple, fixed size groups
  vector<const class FieldDescriptor*> fields2, group_fields2, group_fields3;
  group_fields2.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  group_fields3.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  fields2.push_back(
      new ola::messaging::FieldDescriptorGroup("group1", group_fields2, 2, 2));
  fields2.push_back(
      new ola::messaging::FieldDescriptorGroup("group2", group_fields3, 2, 2));

  const Descriptor multiple_fixed_descriptor("MuiltpleFixed", fields2);
  CPPUNIT_ASSERT(checker.CheckConsistency(&multiple_fixed_descriptor));

  // test a fixed size group, and a variable-sized group
  vector<const class FieldDescriptor*> fields3, group_fields4, group_fields5;
  group_fields4.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  group_fields5.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  fields3.push_back(
      new ola::messaging::FieldDescriptorGroup("group1", group_fields4, 2, 2));
  fields3.push_back(
      new ola::messaging::FieldDescriptorGroup("group2", group_fields5, 2, 8));

  const Descriptor fixed_and_variable_descriptor("Fixed", fields3);
  CPPUNIT_ASSERT(checker.CheckConsistency(&fixed_and_variable_descriptor));

  // test a variable sized group
  vector<const class FieldDescriptor*> fields4, group_fields6;
  group_fields6.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  fields4.push_back(
      new ola::messaging::FieldDescriptorGroup("group1", group_fields6, 2, 8));

  const Descriptor variable_descriptor("Variable", fields4);
  CPPUNIT_ASSERT(checker.CheckConsistency(&variable_descriptor));

  // test a multiple variable sized groups
  vector<const class FieldDescriptor*> fields5, group_fields7, group_fields8;
  group_fields7.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  group_fields8.push_back(new ola::messaging::UInt8FieldDescriptor("uint8"));
  fields5.push_back(
      new ola::messaging::FieldDescriptorGroup("group1", group_fields7, 2, 8));
  fields5.push_back(
      new ola::messaging::FieldDescriptorGroup("group1", group_fields8, 2, 8));

  const Descriptor multiple_variable_descriptor("Variable", fields5);
  CPPUNIT_ASSERT(!checker.CheckConsistency(&multiple_variable_descriptor));
}
