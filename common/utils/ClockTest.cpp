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
 * ClockTest.cpp
 * Unittest for String functions.
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include "ola/Clock.h"


class ClockTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ClockTest);
  CPPUNIT_TEST(testTimeStamp);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testTimeStamp();
};


CPPUNIT_TEST_SUITE_REGISTRATION(ClockTest);

using ola::TimeStamp;
using ola::TimeInterval;
using std::string;


/*
 * Test the TimeStamp class
 */
void ClockTest::testTimeStamp() {
  TimeStamp timestamp, timestamp2;
  CPPUNIT_ASSERT(!timestamp.IsSet());
  CPPUNIT_ASSERT(!timestamp2.IsSet());

  // test assignment & copy constructor
  timestamp.SetToCurrentTime();
  CPPUNIT_ASSERT(timestamp.IsSet());
  timestamp2 = timestamp;
  CPPUNIT_ASSERT(timestamp2.IsSet());
  TimeStamp timestamp3(timestamp);
  CPPUNIT_ASSERT(timestamp3.IsSet());
  CPPUNIT_ASSERT_EQUAL(timestamp, timestamp2);
  CPPUNIT_ASSERT_EQUAL(timestamp, timestamp3);

  // test equalities
  timestamp3.SetToCurrentTime();
  CPPUNIT_ASSERT(timestamp3 != timestamp);
  CPPUNIT_ASSERT(timestamp3 > timestamp);
  CPPUNIT_ASSERT(timestamp < timestamp3);

  // test intervals
  TimeInterval interval = timestamp3 - timestamp;
  TimeInterval interval2 = interval;
  TimeInterval interval3(interval);
  CPPUNIT_ASSERT_EQUAL(interval, interval2);
  CPPUNIT_ASSERT_EQUAL(interval, interval3);

  // test subtraction / addition
  timestamp2 = timestamp + interval;
  CPPUNIT_ASSERT_EQUAL(timestamp2, timestamp3);
  timestamp2 -= interval;
  CPPUNIT_ASSERT_EQUAL(timestamp, timestamp2);

  // test toString and AsInt
  TimeInterval one_point_five_seconds(1500);
  CPPUNIT_ASSERT_EQUAL(string("1.500000"), one_point_five_seconds.ToString());
  CPPUNIT_ASSERT_EQUAL(static_cast<int64_t>(1500000),
                       one_point_five_seconds.AsInt());
}
