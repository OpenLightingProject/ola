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
 * DmxUtilsTest.cpp
 * Unittest for String functions.
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <lla/DmxUtils.h>

using namespace lla;
using namespace std;

class DmxUtilsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DmxUtilsTest);
  CPPUNIT_TEST(testStringToDmx);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testStringToDmx();
    void runStringToDmx(const string &input, dmx_t *expected,
                        size_t expected_length);
};


CPPUNIT_TEST_SUITE_REGISTRATION(DmxUtilsTest);

/*
 * Run the StringToDmxTest
 * @param input the string to parse
 * @param expected the expected result
 */
void DmxUtilsTest::runStringToDmx(const string &input, dmx_t *expected,
                                  size_t expected_length) {

  dmx_t dmx_data[DMX_UNIVERSE_SIZE];
  unsigned int length = StringToDmx(input, dmx_data, DMX_UNIVERSE_SIZE);
  CPPUNIT_ASSERT_EQUAL((unsigned int) expected_length, length);
  CPPUNIT_ASSERT(!memcmp(dmx_data, expected, expected_length));
}

/*
 * Test the StringToDmx function
 */
void DmxUtilsTest::testStringToDmx() {
  string input = "1,2,3,4";
  dmx_t expected1[] = {1, 2, 3, 4};
  runStringToDmx(input, expected1, sizeof(expected1));

  input = "a,b,c,d";
  dmx_t expected2[] = {0, 0, 0, 0};
  runStringToDmx(input, expected2, sizeof(expected2));

  input = "a,b,c,";
  dmx_t expected3[] = {0, 0, 0, 0};
  runStringToDmx(input, expected3, sizeof(expected3));

  input = "255,,,";
  dmx_t expected4[] = {255, 0, 0, 0};
  runStringToDmx(input, expected4, sizeof(expected4));

  input = "255,,,10";
  dmx_t expected5[] = {255, 0, 0, 10};
  runStringToDmx(input, expected5, sizeof(expected5));

  input = " 266,,,10  ";
  dmx_t expected6[] = {266, 0, 0, 10};
  runStringToDmx(input, expected6, sizeof(expected6));

  input = "";
  dmx_t expected7[] = {};
  runStringToDmx(input, expected7, sizeof(expected7));
}
