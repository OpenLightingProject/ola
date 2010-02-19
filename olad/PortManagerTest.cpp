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

#include "olad/DmxSource.h"
#include "olad/PortManager.h"
#include "olad/UniverseStore.h"
#include "olad/TestCommon.h"

using ola::DmxSource;
using ola::PortManager;
using ola::Port;
using ola::Universe;
using std::string;


class PortManagerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PortManagerTest);
  CPPUNIT_TEST(testPortPatching);
  CPPUNIT_TEST(testPortPatchingLoopMulti);
  CPPUNIT_TEST(testInputPortSetPriority);
  CPPUNIT_TEST(testOutputPortSetPriority);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testPortPatching();
    void testPortPatchingLoopMulti();
    void testInputPortSetPriority();
    void testOutputPortSetPriority();
};


CPPUNIT_TEST_SUITE_REGISTRATION(PortManagerTest);



/*
 * Check that we can patch ports correctly.
 */
void PortManagerTest::testPortPatching() {
  ola::UniverseStore uni_store(NULL, NULL);
  PortManager port_manager(&uni_store);

  // mock device, this doesn't allow looping or multiport patching
  MockDevice device1(NULL, "test_device_1");
  TestMockInputPort input_port(&device1, 1);
  TestMockInputPort input_port2(&device1, 2);
  TestMockOutputPort output_port(&device1, 1);
  TestMockOutputPort output_port2(&device1, 2);
  device1.AddPort(&input_port);
  device1.AddPort(&input_port2);
  device1.AddPort(&output_port);
  device1.AddPort(&output_port2);

  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL), input_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // simple patching
  CPPUNIT_ASSERT(port_manager.PatchPort(&input_port, 1));
  CPPUNIT_ASSERT(port_manager.PatchPort(&output_port, 2));
  CPPUNIT_ASSERT(input_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1,
                       input_port.GetUniverse()->UniverseId());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  CPPUNIT_ASSERT(output_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2,
                       output_port.GetUniverse()->UniverseId());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // test looping
  CPPUNIT_ASSERT(!port_manager.PatchPort(&input_port2, 2));
  CPPUNIT_ASSERT(!port_manager.PatchPort(&output_port2, 1));
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // test multiport
  CPPUNIT_ASSERT(!port_manager.PatchPort(&input_port2, 1));
  CPPUNIT_ASSERT(!port_manager.PatchPort(&output_port2, 2));
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // test repatching
  CPPUNIT_ASSERT(port_manager.PatchPort(&input_port, 3));
  CPPUNIT_ASSERT(port_manager.PatchPort(&output_port, 4));
  CPPUNIT_ASSERT(input_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 3,
                       input_port.GetUniverse()->UniverseId());
  CPPUNIT_ASSERT(output_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4,
                       output_port.GetUniverse()->UniverseId());

  // test unpatching
  CPPUNIT_ASSERT(port_manager.UnPatchPort(&input_port));
  CPPUNIT_ASSERT(port_manager.UnPatchPort(&input_port2));
  CPPUNIT_ASSERT(port_manager.UnPatchPort(&output_port));
  CPPUNIT_ASSERT(port_manager.UnPatchPort(&output_port2));
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL), input_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());
}


/*
 * test that patching works correctly for devices with looping and multiport
 * patching enabled.
 */
void PortManagerTest::testPortPatchingLoopMulti() {
  ola::UniverseStore uni_store(NULL, NULL);
  PortManager port_manager(&uni_store);

  // mock device that allows looping and multi port patching
  MockDeviceLoopAndMulti device1(NULL, "test_device_1");
  TestMockInputPort input_port(&device1, 1);
  TestMockInputPort input_port2(&device1, 2);
  TestMockOutputPort output_port(&device1, 1);
  TestMockOutputPort output_port2(&device1, 2);
  device1.AddPort(&input_port);
  device1.AddPort(&input_port2);
  device1.AddPort(&output_port);
  device1.AddPort(&output_port2);

  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL), input_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // simple patching
  CPPUNIT_ASSERT(port_manager.PatchPort(&input_port, 1));
  CPPUNIT_ASSERT(port_manager.PatchPort(&output_port, 2));
  CPPUNIT_ASSERT(input_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1,
                       input_port.GetUniverse()->UniverseId());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  CPPUNIT_ASSERT(output_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2,
                       output_port.GetUniverse()->UniverseId());
  CPPUNIT_ASSERT_EQUAL(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // test looping
  CPPUNIT_ASSERT(port_manager.PatchPort(&input_port2, 2));
  CPPUNIT_ASSERT(port_manager.PatchPort(&output_port2, 1));
  CPPUNIT_ASSERT(input_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2,
                       input_port2.GetUniverse()->UniverseId());
  CPPUNIT_ASSERT(output_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1,
                       output_port2.GetUniverse()->UniverseId());

  // test multiport
  CPPUNIT_ASSERT(port_manager.PatchPort(&input_port2, 1));
  CPPUNIT_ASSERT(port_manager.PatchPort(&output_port2, 2));
  CPPUNIT_ASSERT(input_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1,
                       input_port2.GetUniverse()->UniverseId());
  CPPUNIT_ASSERT(output_port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2,
                       output_port2.GetUniverse()->UniverseId());
}


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
                       DmxSource::PRIORITY_DEFAULT);

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
                       DmxSource::PRIORITY_DEFAULT);

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
                       DmxSource::PRIORITY_DEFAULT);

  // this port doesn't support priorities so these are all noops
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "1", "2"));
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "2", "2"));
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "2", "201"));
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "f00", "199"));
  CPPUNIT_ASSERT(!patcher.SetPriority(&output_port, "f00", "bar"));
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriority(),
                       DmxSource::PRIORITY_DEFAULT);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "f00", "40", false));
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriority(), DmxSource::PRIORITY_DEFAULT);

  // try with pedantic mode off
  CPPUNIT_ASSERT(patcher.SetPriority(&output_port, "1", "201", false));
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port.GetPriority(), DmxSource::PRIORITY_DEFAULT);

  // now test an output port that supports priorities
  TestMockPriorityOutputPort output_port2(NULL, 1);

  CPPUNIT_ASSERT_EQUAL(output_port2.PriorityCapability(),
                       ola::CAPABILITY_FULL);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  CPPUNIT_ASSERT_EQUAL(output_port2.GetPriority(),
                       DmxSource::PRIORITY_DEFAULT);

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
