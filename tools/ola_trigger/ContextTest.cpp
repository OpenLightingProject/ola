/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ContextTest.cpp
 * Test fixture for the ActionContext class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "tools/ola_trigger/Context.h"
#include "ola/testing/TestUtils.h"



class ContextTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ContextTest);
  CPPUNIT_TEST(testContext);
  CPPUNIT_TEST(testSlotOffsetAndValue);
  CPPUNIT_TEST(testAsString);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testContext();
    void testSlotOffsetAndValue();
    void testAsString();
};


CPPUNIT_TEST_SUITE_REGISTRATION(ContextTest);


/**
 * Check that contexts work
 */
void ContextTest::testContext() {
  Context context;
  const string VARIABLE_ONE = "one";
  const string VARIABLE_TWO = "two";
  const string FOO_VALUE = "foo";
  const string BAR_VALUE = "bar";

  string value;
  OLA_ASSERT_FALSE(context.Lookup(VARIABLE_ONE, &value));
  OLA_ASSERT_FALSE(context.Lookup(VARIABLE_TWO, &value));

  // insert
  context.Update(VARIABLE_ONE, FOO_VALUE);
  OLA_ASSERT(context.Lookup(VARIABLE_ONE, &value));
  OLA_ASSERT_EQ(FOO_VALUE, value);
  OLA_ASSERT_FALSE(context.Lookup(VARIABLE_TWO, &value));

  // update
  context.Update(VARIABLE_ONE, BAR_VALUE);
  OLA_ASSERT(context.Lookup(VARIABLE_ONE, &value));
  OLA_ASSERT_EQ(BAR_VALUE, value);
  OLA_ASSERT_FALSE(context.Lookup(VARIABLE_TWO, &value));
}


/**
 * Check that the magic slot variables work.
 */
void ContextTest::testSlotOffsetAndValue() {
  Context context;
  string value;

  OLA_ASSERT_FALSE(context.Lookup(Context::SLOT_VALUE_VARIABLE, &value));
  OLA_ASSERT_FALSE(context.Lookup(Context::SLOT_OFFSET_VARIABLE, &value));

  context.SetSlotOffset(1);
  context.SetSlotValue(100);

  OLA_ASSERT(context.Lookup(Context::SLOT_OFFSET_VARIABLE, &value));
  OLA_ASSERT_EQ(string("1"), value);

  OLA_ASSERT(context.Lookup(Context::SLOT_VALUE_VARIABLE, &value));
  OLA_ASSERT_EQ(string("100"), value);
}


/**
 * Check we can convert to a string
 */
void ContextTest::testAsString() {
  Context context;
  const string VARIABLE_ONE = "one";
  const string VARIABLE_TWO = "two";
  const string FOO_VALUE = "foo";
  const string BAR_VALUE = "bar";

  context.Update(VARIABLE_ONE, FOO_VALUE);
  context.Update(VARIABLE_TWO, BAR_VALUE);

  OLA_ASSERT_EQ(string("one=foo, two=bar"), context.AsString());
}
