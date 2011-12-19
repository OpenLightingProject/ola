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
 * ContextTest.cpp
 * Test fixture for the ActionContext class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "tools/dmx_trigger/Context.h"


class ContextTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ContextTest);
  CPPUNIT_TEST(testContext);
  CPPUNIT_TEST(testAsString);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testContext();
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
  CPPUNIT_ASSERT(!context.Lookup(VARIABLE_ONE, &value));
  CPPUNIT_ASSERT(!context.Lookup(VARIABLE_TWO, &value));

  // insert
  context.Update(VARIABLE_ONE, FOO_VALUE);
  CPPUNIT_ASSERT(context.Lookup(VARIABLE_ONE, &value));
  CPPUNIT_ASSERT_EQUAL(FOO_VALUE, value);
  CPPUNIT_ASSERT(!context.Lookup(VARIABLE_TWO, &value));

  // update
  context.Update(VARIABLE_ONE, BAR_VALUE);
  CPPUNIT_ASSERT(context.Lookup(VARIABLE_ONE, &value));
  CPPUNIT_ASSERT_EQUAL(BAR_VALUE, value);
  CPPUNIT_ASSERT(!context.Lookup(VARIABLE_TWO, &value));
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

  CPPUNIT_ASSERT_EQUAL(string("one=foo, two=bar"), context.AsString());
}
