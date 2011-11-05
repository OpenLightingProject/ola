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
 * TimeCodeTest.cpp
 * Test fixture for the TimeCode classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/timecode/TimeCode.h"
#include "ola/timecode/TimeCodeEnums.h"

using ola::timecode::TimeCode;
using std::string;

using ola::timecode::TIMECODE_FILM;
using ola::timecode::TIMECODE_EBU;
using ola::timecode::TIMECODE_DF;
using ola::timecode::TIMECODE_SMPTE;

class TimeCodeTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TimeCodeTest);
  CPPUNIT_TEST(testTimeCode);
  CPPUNIT_TEST(testIsValid);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testTimeCode();
    void testIsValid();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TimeCodeTest);


/*
 * Test the TimeCodes work.
 */
void TimeCodeTest::testTimeCode() {
  TimeCode t1(TIMECODE_FILM, 0, 0, 0, 0);
  CPPUNIT_ASSERT_EQUAL(TIMECODE_FILM, t1.Type());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0), t1.Hours());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0), t1.Minutes());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0), t1.Seconds());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(0), t1.Frames());
  CPPUNIT_ASSERT_EQUAL(string("00:00:00:00"), t1.AsString());
  CPPUNIT_ASSERT(t1.IsValid());

  TimeCode t2(t1);
  CPPUNIT_ASSERT_EQUAL(t1, t2);
  TimeCode t3(TIMECODE_SMPTE, 10, 9, 12, 14);
  CPPUNIT_ASSERT_EQUAL(string("10:09:12:14"), t3.AsString());
  CPPUNIT_ASSERT(t3.IsValid());
  CPPUNIT_ASSERT(t1 != t3);
  t3 = t1;
  CPPUNIT_ASSERT_EQUAL(t1, t3);
}

/**
 * test invalid codes
 */
void TimeCodeTest::testIsValid() {
  TimeCode t1(TIMECODE_FILM, 0, 0, 0, 24);
  CPPUNIT_ASSERT(!t1.IsValid());

  TimeCode t2(TIMECODE_EBU, 0, 0, 0, 25);
  CPPUNIT_ASSERT(!t2.IsValid());

  TimeCode t3(TIMECODE_DF, 0, 0, 0, 30);
  CPPUNIT_ASSERT(!t3.IsValid());

  TimeCode t4(TIMECODE_SMPTE, 0, 0, 0, 30);
  CPPUNIT_ASSERT(!t4.IsValid());
}
