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
 * PortManagerTest.cpp
 * Test fixture for the PortManager classes
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "olad/PortManager.h"
#include "olad/UniverseStore.h"
#include "olad/TestCommon.h"

using ola::PortManager;
using ola::Port;
using std::string;


class PortManagerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PortManagerTest);
  CPPUNIT_TEST(testInputPortSetPriority);
  CPPUNIT_TEST(testOutputPortSetPriority);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testInputPortSetPriority();
    void testOutputPortSetPriority();
};


CPPUNIT_TEST_SUITE_REGISTRATION(PortManagerTest);


/*
 * Check that we can set priorities on an input port
 */
void PortManagerTest::testInputPortSetPriority() {
  PortManager patcher(NULL);  // we're not testing patching so pass NULL here

  // Input port that doesn't support priorities
  TestMockInputPort input_port(NULL, 0);

  CPPUNIT_ASSERT_EQUAL(input_port.PriorityCapability(),
                       ola::CAPABILITY_STATIC);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriority(),
                       Port::PORT_PRIORITY_DEFAULT);

  // this port doesn't support priorities so the mode is a noop
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port, "1", "2"));
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriority(), (uint8_t) 2);

  // try an invalid mode, this shouldn't fail for the reasons above
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port, "2", "2"));
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriority(), (uint8_t) 2);

  // try an invalid priority
  CPPUNIT_ASSERT(!patcher.SetPriority(&input_port, "2", "201"));
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriority(), (uint8_t) 2);

  // try invalid mode string, this should set the priority
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port, "f00", "199"));
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriority(), (uint8_t) 199);

  // test invalid priority string, this should fail
  CPPUNIT_ASSERT(!patcher.SetPriority(&input_port, "f00", "bar"));
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriority(), (uint8_t) 199);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port, "f00", "40", false));
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriority(), (uint8_t) 40);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port, "1", "201", false));
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port.GetPriority(), (uint8_t) 200);

  // Now test an input port that does support priorities
  TestMockPriorityInputPort input_port2(NULL, 1);
  CPPUNIT_ASSERT_EQUAL(input_port2.PriorityCapability(),
                       ola::CAPABILITY_FULL);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(),
                       Port::PORT_PRIORITY_DEFAULT);

  // try changing to static mode
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port2, "1", "2"));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 2);

  // bump priority
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port2, "1", "180"));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 180);

  // change back to inherit mode
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port2, "0", "180"));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 180);

  // try an invalid mode
  CPPUNIT_ASSERT(!patcher.SetPriority(&input_port2, "2", "180"));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 180);

  // try an invalid priority
  CPPUNIT_ASSERT(!patcher.SetPriority(&input_port2, "1", "201"));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 180);

  // try an invalid mode string
  CPPUNIT_ASSERT(!patcher.SetPriority(&input_port2, "foo", "180"));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 180);

  // try an invalid prioity string
  CPPUNIT_ASSERT(!patcher.SetPriority(&input_port2, "0", "bar"));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 180);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port2, "f00", "40", false));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 40);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&input_port2, "1", "201", false));
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  CPPUNIT_ASSERT_EQUAL(input_port2.GetPriority(), (uint8_t) 200);
}


/*
 * Check that we can set priorities on an Output port
 */
void PortManagerTest::testOutputPortSetPriority() {
  PortManager patcher(NULL);  // we're not testing patching so pass NULL here

  // Input port that doesn't support priorities
  TestMockOutputPort output_port(NULL, 0);

  CPPUNIT_ASSERT_EQUAL(output_port.PriorityCapability(),
                       ola::CAPABILITY_NONE);
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriority(),
                       Port::PORT_PRIORITY_DEFAULT);

  // this port doesn't support priorities so these are all noops
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "1", "2"));
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "2", "2"));
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "2", "201"));
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "f00", "199"));
  CPPUNIT_ASSERT(!patcher.SetPriority(&output_port, "f00", "bar"));
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriority(),
                       Port::PORT_PRIORITY_DEFAULT);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "f00", "40", false));
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriority(), Port::PORT_PRIORITY_DEFAULT);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "1", "201", false));
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriority(), Port::PORT_PRIORITY_DEFAULT);

  // now test an output port that supports priorities
  TestMockPriorityOutputPort output_port2(NULL, 1);

  CPPUNIT_ASSERT_EQUAL(output_port2.PriorityCapability(),
                       ola::CAPABILITY_FULL);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(),
                       Port::PORT_PRIORITY_DEFAULT);

  // try changing to static mode
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port2, "1", "2"));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 2);

  // bump priority
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port2, "1", "180"));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 180);

  // change back to inherit mode
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port2, "0", "180"));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 180);

  // try an invalid mode
  CPPUNIT_ASSERT(!patcher.SetPriority(&output_port2, "2", "180"));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 180);

  // try an invalid priority
  CPPUNIT_ASSERT(!patcher.SetPriority(&output_port2, "1", "201"));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 180);

  // try an invalid mode string
  CPPUNIT_ASSERT(!patcher.SetPriority(&output_port2, "foo", "180"));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 180);

  // try an invalid prioity string
  CPPUNIT_ASSERT(!patcher.SetPriority(&output_port2, "0", "bar"));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 180);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port2, "f00", "40", false));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 40);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port2, "1", "201", false));
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(), (uint8_t) 200);
}
