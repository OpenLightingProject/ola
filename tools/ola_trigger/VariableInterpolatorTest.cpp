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
 * VariableInterpolatorTest.cpp
 * Test fixture for the ActionInterval class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <ola/Logging.h>
#include <string>

#include "tools/ola_trigger/Context.h"
#include "tools/ola_trigger/VariableInterpolator.h"


class VariableInterpolatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(VariableInterpolatorTest);
  CPPUNIT_TEST(testNoInterpolation);
  CPPUNIT_TEST(testSimpleInterpolation);
  CPPUNIT_TEST(testNestedInterpolation);
  CPPUNIT_TEST(testEscaping);
  CPPUNIT_TEST(testMissingVariables);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testNoInterpolation();
    void testSimpleInterpolation();
    void testNestedInterpolation();
    void testEscaping();
    void testMissingVariables();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(VariableInterpolatorTest);


/**
 * These strings don't require interpolation.
 */
void VariableInterpolatorTest::testNoInterpolation() {
  Context context;
  string result;

  CPPUNIT_ASSERT(InterpolateVariables("", &result, context));
  CPPUNIT_ASSERT_EQUAL(string(""), result);

  CPPUNIT_ASSERT(InterpolateVariables("foo bar baz", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("foo bar baz"), result);

  CPPUNIT_ASSERT(InterpolateVariables("{foo}", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("{foo}"), result);
}


/**
 * Test the simple case.
 */
void VariableInterpolatorTest::testSimpleInterpolation() {
  Context context;
  context.Update("one", "foo");
  context.Update("two", "bar");
  string result;

  CPPUNIT_ASSERT(InterpolateVariables("${one}", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("foo"), result);

  CPPUNIT_ASSERT(InterpolateVariables("foo ${two} baz", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("foo bar baz"), result);

  CPPUNIT_ASSERT(InterpolateVariables("${one} ${two}", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("foo bar"), result);

  CPPUNIT_ASSERT(InterpolateVariables("a${one}b${two}c", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("afoobbarc"), result);
}


/**
 * Test nested interpolation works. As we go right-to-left, we can nest
 * variables.
 */
void VariableInterpolatorTest::testNestedInterpolation() {
  Context context;
  context.Update("one", "1");
  context.Update("slot_1", "bar");
  string result;

  CPPUNIT_ASSERT(InterpolateVariables("${slot_${one}}", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("bar"), result);
}


/**
 * Test escaping works
 */
void VariableInterpolatorTest::testEscaping() {
  Context context;
  context.Update("one", "foo");
  string result;

  CPPUNIT_ASSERT(InterpolateVariables("\\${one\\}", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("${one}"), result);

  CPPUNIT_ASSERT(InterpolateVariables("${one} \\${one\\}", &result, context));
  CPPUNIT_ASSERT_EQUAL(string("foo ${one}"), result);
}


/**
 * Test for missing variables.
 */
void VariableInterpolatorTest::testMissingVariables() {
  Context context;
  string result;
  CPPUNIT_ASSERT(!InterpolateVariables("${one}", &result, context));
  CPPUNIT_ASSERT(!InterpolateVariables("${}", &result, context));
}
