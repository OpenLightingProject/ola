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
 * PortTest.cpp
 * Test fixture for the Port classes
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "olad/TestCommon.h"


class PortTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PortTest);
  CPPUNIT_TEST(testPortPriorities);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testPortPriorities();
};


CPPUNIT_TEST_SUITE_REGISTRATION(PortTest);


/*
 * Check that the base device class works correctly.
 */
void PortTest::testPortPriorities() {
  TestMockOutputPort output_port(NULL, 1);

  CPPUNIT_ASSERT_EQUAL(ola::Port::PORT_PRIORITY_DEFAULT,
                       output_port.GetPriority());
  CPPUNIT_ASSERT_EQUAL(ola::PRIORITY_MODE_INHERIT,
                       output_port.GetPriorityMode());

  // test the setting of priorities
  CPPUNIT_ASSERT(output_port.SetPriority(120));
  CPPUNIT_ASSERT_EQUAL((uint8_t) 120, output_port.GetPriority());
  CPPUNIT_ASSERT(!output_port.SetPriority(201));
  CPPUNIT_ASSERT_EQUAL((uint8_t) 120, output_port.GetPriority());
  CPPUNIT_ASSERT(output_port.SetPriority(0));
  CPPUNIT_ASSERT_EQUAL((uint8_t) 0, output_port.GetPriority());

  // test the setting of modes
  output_port.SetPriorityMode(ola::PRIORITY_MODE_OVERRIDE);
  CPPUNIT_ASSERT_EQUAL(ola::PRIORITY_MODE_OVERRIDE,
                       output_port.GetPriorityMode());

  output_port.SetPriorityMode(ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(ola::PRIORITY_MODE_INHERIT,
                       output_port.GetPriorityMode());
}
