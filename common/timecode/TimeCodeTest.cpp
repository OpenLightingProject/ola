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
 * TimeCodeTest.cpp
 * Test fixture for the TimeCode classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/timecode/TimeCode.h"
#include "ola/timecode/TimeCodeEnums.h"
#include "ola/testing/TestUtils.h"


using ola::timecode::TimeCode;
using std::ostringstream;
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
  ostringstream str1;

  TimeCode t1(TIMECODE_FILM, 0, 0, 0, 0);
  OLA_ASSERT_EQ(TIMECODE_FILM, t1.Type());
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), t1.Hours());
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), t1.Minutes());
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), t1.Seconds());
  OLA_ASSERT_EQ(static_cast<uint8_t>(0), t1.Frames());
  OLA_ASSERT_EQ(string("00:00:00:00"), t1.AsString());
  str1 << t1;
  OLA_ASSERT_EQ(string("00:00:00:00"), str1.str());
  OLA_ASSERT_TRUE(t1.IsValid());

  ostringstream str3;

  TimeCode t2(t1);
  OLA_ASSERT_EQ(t1, t2);

  TimeCode t3(TIMECODE_SMPTE, 10, 9, 12, 14);
  OLA_ASSERT_EQ(string("10:09:12:14"), t3.AsString());
  str3 << t3;
  OLA_ASSERT_EQ(string("10:09:12:14"), str3.str());
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
