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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FlagsTest.cpp
 * Test the flags parsing code.
 * Copyright (C) 2013 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Logging.h"
#include "ola/base/Flags.h"
#include "ola/testing/TestUtils.h"

DEFINE_bool(default_false, false, "Default False");
DEFINE_bool(default_true, true, "Default True");
DEFINE_int8(f_int8, -10, "Default -10");
DEFINE_uint8(f_uint8, 10, "Default 10");
DEFINE_int16(f_int16, -1000, "Default -1000");
DEFINE_uint16(f_uint16, 1000, "Default 1000");
DEFINE_int32(f_int32, -2000, "Default -2000");
DEFINE_uint32(f_uint32, 2000, "Default 2000");
DEFINE_string(f_str, "foo", "Test String");

// now flags with short options
DEFINE_s_bool(s_default_false, a, false, "Default False");
DEFINE_s_bool(s_default_true, b, true, "Default True");
DEFINE_s_int8(s_int8, c, -10, "Default -10");
DEFINE_s_uint8(s_uint8, d, 10, "Default 10");
DEFINE_s_int16(s_int16, e, -1000, "Default -1000");
DEFINE_s_uint16(s_uint16, f, 1000, "Default 1000");
DEFINE_s_int32(s_int32, g, -2000, "Default -2000");
DEFINE_s_uint32(s_uint32, i, 2000, "Default 2000");
DEFINE_s_string(s_str, j, "bar", "Test String");

using std::string;

class FlagsTest: public CppUnit::TestFixture {
  public:
    CPPUNIT_TEST_SUITE(FlagsTest);
    CPPUNIT_TEST(testDefaults);
    CPPUNIT_TEST(testSetting);
    CPPUNIT_TEST(testBoolFlags);
    CPPUNIT_TEST(testIntFlags);
    CPPUNIT_TEST(testStringFlags);
    CPPUNIT_TEST_SUITE_END();

  public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
    void testDefaults();
    void testSetting();
    void testBoolFlags();
    void testIntFlags();
    void testStringFlags();
};


CPPUNIT_TEST_SUITE_REGISTRATION(FlagsTest);

/**
 * Check the defaults are correct.
 */
void FlagsTest::testDefaults() {
  OLA_ASSERT_EQ(false, static_cast<bool>(FLAGS_default_false));
  OLA_ASSERT_EQ(true, static_cast<bool>(FLAGS_default_true));
  OLA_ASSERT_EQ(static_cast<int8_t>(-10), static_cast<int8_t>(FLAGS_f_int8));
  OLA_ASSERT_EQ(static_cast<uint8_t>(10), static_cast<uint8_t>(FLAGS_f_uint8));
  OLA_ASSERT_EQ(static_cast<int16_t>(-1000),
                static_cast<int16_t>(FLAGS_f_int16));
  OLA_ASSERT_EQ(static_cast<uint16_t>(1000),
                static_cast<uint16_t>(FLAGS_f_uint16));
  OLA_ASSERT_EQ(static_cast<int32_t>(-2000),
                static_cast<int32_t>(FLAGS_f_int32));
  OLA_ASSERT_EQ(static_cast<uint32_t>(2000),
                static_cast<uint32_t>(FLAGS_f_uint32));

  OLA_ASSERT_EQ(string("foo"), string(FLAGS_f_str));

  OLA_ASSERT_EQ(static_cast<int8_t>(-10), static_cast<int8_t>(FLAGS_s_int8));
  OLA_ASSERT_EQ(static_cast<uint8_t>(10), static_cast<uint8_t>(FLAGS_s_uint8));
  OLA_ASSERT_EQ(static_cast<int16_t>(-1000),
                static_cast<int16_t>(FLAGS_s_int16));
  OLA_ASSERT_EQ(static_cast<uint16_t>(1000),
                static_cast<uint16_t>(FLAGS_s_uint16));
  OLA_ASSERT_EQ(static_cast<int32_t>(-2000),
                static_cast<int32_t>(FLAGS_s_int32));
  OLA_ASSERT_EQ(static_cast<uint32_t>(2000),
                static_cast<uint32_t>(FLAGS_s_uint32));

  OLA_ASSERT_EQ(string("bar"), string(FLAGS_s_str));
}

/**
 * Check that we can set the flags
 */
void FlagsTest::testSetting() {
  OLA_ASSERT_EQ(false, static_cast<bool>(FLAGS_default_false));
  OLA_ASSERT_EQ(true, static_cast<bool>(FLAGS_default_true));
  OLA_ASSERT_EQ(static_cast<int8_t>(-10), static_cast<int8_t>(FLAGS_f_int8));
  OLA_ASSERT_EQ(static_cast<uint8_t>(10), static_cast<uint8_t>(FLAGS_f_uint8));
  OLA_ASSERT_EQ(static_cast<int16_t>(-1000),
                static_cast<int16_t>(FLAGS_f_int16));
  OLA_ASSERT_EQ(static_cast<uint16_t>(1000),
                static_cast<uint16_t>(FLAGS_f_uint16));
  OLA_ASSERT_EQ(static_cast<int32_t>(-2000),
                static_cast<int32_t>(FLAGS_f_int32));
  OLA_ASSERT_EQ(static_cast<uint32_t>(2000),
                static_cast<uint32_t>(FLAGS_f_uint32));
  OLA_ASSERT_EQ(string("foo"), string(FLAGS_f_str));

  FLAGS_default_false = true;
  FLAGS_default_true = false;
  FLAGS_f_int8 = -20;
  FLAGS_f_uint8 = 20;
  FLAGS_f_int16 = -2000;
  FLAGS_f_uint16 = 2000;
  FLAGS_f_int32 = -4000;
  FLAGS_f_uint32 = 4000;

  FLAGS_f_str = "hello";

  OLA_ASSERT_EQ(true, static_cast<bool>(FLAGS_default_false));
  OLA_ASSERT_EQ(false, static_cast<bool>(FLAGS_default_true));
  OLA_ASSERT_EQ(static_cast<int8_t>(-20), static_cast<int8_t>(FLAGS_f_int8));
  OLA_ASSERT_EQ(static_cast<uint8_t>(20), static_cast<uint8_t>(FLAGS_f_uint8));
  OLA_ASSERT_EQ(static_cast<int16_t>(-2000),
                static_cast<int16_t>(FLAGS_f_int16));
  OLA_ASSERT_EQ(static_cast<uint16_t>(2000),
                static_cast<uint16_t>(FLAGS_f_uint16));
  OLA_ASSERT_EQ(static_cast<int32_t>(-4000),
                static_cast<int32_t>(FLAGS_f_int32));
  OLA_ASSERT_EQ(static_cast<uint32_t>(4000),
                static_cast<uint32_t>(FLAGS_f_uint32));
  OLA_ASSERT_EQ(string("hello"), string(FLAGS_f_str));
}


/**
 * Check bool flags
 */
void FlagsTest::testBoolFlags() {
  char bin_name[] = "foo";
  char opt1[] = "--default-false";
  char opt2[] = "--nodefault-true";

  char *argv[] = {bin_name, opt1, opt2};
  int argc = sizeof(argv) / sizeof(argv[0]);
  ola::ParseFlags(&argc, argv);

  OLA_ASSERT_EQ(1, argc);
  OLA_ASSERT_EQ(string(bin_name), string(argv[0]));

  OLA_ASSERT_EQ(true, static_cast<bool>(FLAGS_default_false));
  OLA_ASSERT_EQ(false, static_cast<bool>(FLAGS_default_true));

  // now try the short option versions
  char opt3[] = "-a";
  char opt4[] = "-b";

  char *argv2[] = {bin_name, opt3, opt4};
  argc = sizeof(argv2) / sizeof(argv[0]);
  ola::ParseFlags(&argc, argv2);

  OLA_ASSERT_EQ(1, argc);
  OLA_ASSERT_EQ(string(bin_name), string(argv2[0]));

  OLA_ASSERT_EQ(true, static_cast<bool>(FLAGS_s_default_false));
  OLA_ASSERT_EQ(false, static_cast<bool>(FLAGS_s_default_true));
}

/**
 * Check the int flags
 */
void FlagsTest::testIntFlags() {
  char bin_name[] = "foo";
  char opt1[] = "--f-int8";
  char opt2[] = "-20";
  char opt3[] = "--f-uint8";
  char opt4[] = "20";
  char opt5[] = "--f-int16";
  char opt6[] = "-2000";
  char opt7[] = "--f-uint16";
  char opt8[] = "2000";
  char opt9[] = "--f-int32=-4000";
  char opt10[] = "--f-uint32=4000";

  char *argv[] = {bin_name, opt1, opt2, opt3, opt4, opt5, opt6, opt7, opt8,
                  opt9, opt10};
  int argc = sizeof(argv) / sizeof(argv[0]);
  ola::ParseFlags(&argc, argv);

  OLA_ASSERT_EQ(1, argc);
  OLA_ASSERT_EQ(string(bin_name), string(argv[0]));

  OLA_ASSERT_EQ(static_cast<int8_t>(-20), static_cast<int8_t>(FLAGS_f_int8));
  OLA_ASSERT_EQ(static_cast<uint8_t>(20), static_cast<uint8_t>(FLAGS_f_uint8));
  OLA_ASSERT_EQ(static_cast<int16_t>(-2000),
                static_cast<int16_t>(FLAGS_f_int16));
  OLA_ASSERT_EQ(static_cast<uint16_t>(2000),
                static_cast<uint16_t>(FLAGS_f_uint16));
  OLA_ASSERT_EQ(static_cast<int32_t>(-4000),
                static_cast<int32_t>(FLAGS_f_int32));
  OLA_ASSERT_EQ(static_cast<uint32_t>(4000),
                static_cast<uint32_t>(FLAGS_f_uint32));

  // now try the short versions
  char sopt1[] = "-c-20";
  char sopt2[] = "-d";
  char sopt3[] = "20";
  char sopt4[] = "-e";
  char sopt5[] = "-2000";
  char sopt6[] = "-f";
  char sopt7[] = "2000";
  char sopt8[] = "-g-4000";
  char sopt9[] = "-i4000";

  char *argv2[] = {bin_name, sopt1, sopt2, sopt3, sopt4, sopt5, sopt6, sopt7,
                  sopt8, sopt9};
  argc = sizeof(argv2) / sizeof(argv[0]);
  ola::ParseFlags(&argc, argv2);

  OLA_ASSERT_EQ(1, argc);
  OLA_ASSERT_EQ(string(bin_name), string(argv[0]));

  OLA_ASSERT_EQ(static_cast<int8_t>(-20), static_cast<int8_t>(FLAGS_s_int8));
  OLA_ASSERT_EQ(static_cast<uint8_t>(20), static_cast<uint8_t>(FLAGS_s_uint8));
  OLA_ASSERT_EQ(static_cast<int16_t>(-2000),
                static_cast<int16_t>(FLAGS_s_int16));
  OLA_ASSERT_EQ(static_cast<uint16_t>(2000),
                static_cast<uint16_t>(FLAGS_s_uint16));
  OLA_ASSERT_EQ(static_cast<int32_t>(-4000),
                static_cast<int32_t>(FLAGS_s_int32));
  OLA_ASSERT_EQ(static_cast<uint32_t>(4000),
                static_cast<uint32_t>(FLAGS_s_uint32));
}

/**
 * Check string flags
 */
void FlagsTest::testStringFlags() {
  char bin_name[] = "a.out";
  char opt1[] = "--f-str";
  char opt2[] = "data";
  char opt3[] = "extra arg";

  char *argv[] = {bin_name, opt1, opt2, opt3};
  int argc = sizeof(argv) / sizeof(argv[0]);

  ola::ParseFlags(&argc, argv);
  OLA_ASSERT_EQ(2, argc);
  OLA_ASSERT_EQ(string(bin_name), string(argv[0]));
  OLA_ASSERT_EQ(string(opt3), string(argv[1]));
  OLA_ASSERT_EQ(string(opt2), string(FLAGS_f_str));

  // try the --foo=bar version
  char opt4[] = "--f-str=data2";
  char *argv2[] = {bin_name, opt4};
  argc = sizeof(argv2) / sizeof(argv[0]);

  ola::ParseFlags(&argc, argv2);
  OLA_ASSERT_EQ(1, argc);
  OLA_ASSERT_EQ(string(bin_name), string(argv[0]));
  OLA_ASSERT_EQ(string("data2"), string(FLAGS_f_str));

  // try the short version
  char opt5[] = "-j";
  char opt6[] = "data3";
  char *argv3[] = {bin_name, opt5, opt6};
  argc = sizeof(argv3) / sizeof(argv[0]);

  ola::ParseFlags(&argc, argv3);
  OLA_ASSERT_EQ(1, argc);
  OLA_ASSERT_EQ(string(bin_name), string(argv[0]));
  OLA_ASSERT_EQ(string("data3"), string(FLAGS_s_str));
}
