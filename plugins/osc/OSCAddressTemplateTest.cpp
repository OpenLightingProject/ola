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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OSCAddressTemplateTest.cpp
 * Test fixture for the OSCAddressTemplate function.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/testing/TestUtils.h"
#include "plugins/osc/OSCAddressTemplate.h"

using ola::plugin::osc::ExpandTemplate;
using std::string;

class OSCAddressTemplateTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OSCAddressTemplateTest);
  // The tests to run.
  CPPUNIT_TEST(testExpand);
  CPPUNIT_TEST_SUITE_END();

 public:
    // The test method
    void testExpand();
};

// Register this test class
CPPUNIT_TEST_SUITE_REGISTRATION(OSCAddressTemplateTest);

/**
 * Check that ExpandTemplate() works.
 */
void OSCAddressTemplateTest::testExpand() {
  OLA_ASSERT_EQ(string(""), ExpandTemplate("", 0));
  OLA_ASSERT_EQ(string("foo"), ExpandTemplate("foo", 0));
  OLA_ASSERT_EQ(string("/dmx/universe/0"),
                ExpandTemplate("/dmx/universe/%d", 0));
  OLA_ASSERT_EQ(string("0"), ExpandTemplate("%d", 0));
  OLA_ASSERT_EQ(string("port_1"), ExpandTemplate("port_%d", 1));
  OLA_ASSERT_EQ(string("1_%d"), ExpandTemplate("%d_%d", 1));
}
