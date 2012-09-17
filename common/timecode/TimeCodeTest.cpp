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

#include "ola/testing/TestUtils.h"

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
  OLA_ASSERT_EQ(TIMECODE_FILM, t1.Type());
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), t1.Hours());
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), t1.Minutes());
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), t1.Seconds());
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), t1.Frames());
  OLA_ASSERT_EQ(string("00:00:00:00"), t1.AsString());
  OLA_ASSERT_TRUE(t1.IsValid());

  TimeCode t2(t1);
  OLA_ASSERT_EQ(t1, t2);
  TimeCode t3(TIMECODE_SMPTE, 10, 9, 12, 14);
  OLA_ASSERT_EQ(string("10:09:12:14"), t3.AsString());
  OLA_ASSERT_TRUE(t3.IsValid());
  OLA_ASSERT_NE(t1, t3);
  t3 = t1;
  OLA_ASSERT_EQ(t1, t3);
}

/**
 * test invalid codes
 */
void TimeCodeTest::testIsValid() {
  TimeCode t1(TIMECODE_FILM, 0, 0, 0, 24);
  OLA_ASSERT_FALSE(t1.IsValid());

  TimeCode t2(TIMECODE_EBU, 0, 0, 0, 25);
  OLA_ASSERT_FALSE(t2.IsValid());

  TimeCode t3(TIMECODE_DF, 0, 0, 0, 30);
  OLA_ASSERT_FALSE(t3.IsValid());

  TimeCode t4(TIMECODE_SMPTE, 0, 0, 0, 30);
  OLA_ASSERT_FALSE(t4.IsValid());
}
