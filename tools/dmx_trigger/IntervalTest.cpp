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
 * IntervalTest.cpp
 * Test fixture for the ActionInterval class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "tools/dmx_trigger/Action.h"


class IntervalTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(IntervalTest);
  CPPUNIT_TEST(testLowerUpper);
  CPPUNIT_TEST(testContains);
  CPPUNIT_TEST(testIntersects);
  CPPUNIT_TEST(testLessThan);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testLowerUpper();
    void testContains();
    void testIntersects();
    void testLessThan();
};


CPPUNIT_TEST_SUITE_REGISTRATION(IntervalTest);


/**
 * Check that lower and upper work.
 */
void IntervalTest::testLowerUpper() {
  ActionInterval interval(0, 10, NULL);
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0), interval.Lower());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(10), interval.Upper());
}


/**
 * Check that contains works
 */
void IntervalTest::testContains() {
  ActionInterval interval(0, 10, NULL);
  for (uint8_t i = 0; i <= 10; i++)
    CPPUNIT_ASSERT(interval.Contains(i));
  CPPUNIT_ASSERT(!interval.Contains(11));

  ActionInterval interval2(10, 10, NULL);
  CPPUNIT_ASSERT(!interval2.Contains(0));
  CPPUNIT_ASSERT(!interval2.Contains(9));
  CPPUNIT_ASSERT(interval2.Contains(10));
  CPPUNIT_ASSERT(!interval2.Contains(11));

  ActionInterval interval3(234, 255, NULL);
  CPPUNIT_ASSERT(!interval3.Contains(0));
  CPPUNIT_ASSERT(!interval3.Contains(233));
  for (uint8_t i = 234; i != 0; i++)
    CPPUNIT_ASSERT(interval3.Contains(i));
}


/**
 * Check that Intersects works
 */
void IntervalTest::testIntersects() {
  ActionInterval interval(0, 10, NULL);
  ActionInterval interval2(10, 10, NULL);
  ActionInterval interval3(5, 6, NULL);
  ActionInterval interval4(11, 20, NULL);
  CPPUNIT_ASSERT(interval.Intersects(interval2));
  CPPUNIT_ASSERT(interval2.Intersects(interval));
  CPPUNIT_ASSERT(interval.Intersects(interval3));
  CPPUNIT_ASSERT(interval3.Intersects(interval));
  CPPUNIT_ASSERT(!interval2.Intersects(interval3));
  CPPUNIT_ASSERT(!interval2.Intersects(interval4));
  CPPUNIT_ASSERT(!interval.Intersects(interval4));

  CPPUNIT_ASSERT(interval.Intersects(interval));
  CPPUNIT_ASSERT(interval2.Intersects(interval2));
  CPPUNIT_ASSERT(interval3.Intersects(interval3));
  CPPUNIT_ASSERT(interval4.Intersects(interval4));
}


/**
 * Check the less than operator works.
 */
void IntervalTest::testLessThan() {
  ActionInterval interval1(0, 10, NULL);
  ActionInterval interval2(11, 12, NULL);
  ActionInterval interval3(14, 15, NULL);

  CPPUNIT_ASSERT(interval1 < interval2);
  CPPUNIT_ASSERT(interval1 < interval3);
  CPPUNIT_ASSERT(interval2 < interval3);

  CPPUNIT_ASSERT(!(interval2 < interval1));
  CPPUNIT_ASSERT(!(interval3 < interval2));
  CPPUNIT_ASSERT(!(interval3 < interval1));
}
