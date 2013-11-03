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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * PortManagerTest.cpp
 * Test fixture for the PortManager classes
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "olad/DmxSource.h"
#include "olad/PortBroker.h"
#include "olad/PortManager.h"
#include "olad/UniverseStore.h"
#include "olad/TestCommon.h"
#include "ola/testing/TestUtils.h"


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
  ola::PortBroker broker;
  ola::PortManager port_manager(&uni_store, &broker);

  // mock device, this doesn't allow looping or multiport patching
  MockDevice device1(NULL, "test_device_1");
  TestMockInputPort input_port(&device1, 1, NULL);
  TestMockInputPort input_port2(&device1, 2, NULL);
  TestMockOutputPort output_port(&device1, 1);
  TestMockOutputPort output_port2(&device1, 2);
  device1.AddPort(&input_port);
  device1.AddPort(&input_port2);
  device1.AddPort(&output_port);
  device1.AddPort(&output_port2);

  OLA_ASSERT_EQ(static_cast<Universe*>(NULL), input_port.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // simple patching
  OLA_ASSERT(port_manager.PatchPort(&input_port, 1));
  OLA_ASSERT(port_manager.PatchPort(&output_port, 2));
  OLA_ASSERT(input_port.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 1,
                       input_port.GetUniverse()->UniverseId());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  OLA_ASSERT(output_port.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 2,
                       output_port.GetUniverse()->UniverseId());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // test looping
  OLA_ASSERT_FALSE(port_manager.PatchPort(&input_port2, 2));
  OLA_ASSERT_FALSE(port_manager.PatchPort(&output_port2, 1));
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // test multiport
  OLA_ASSERT_FALSE(port_manager.PatchPort(&input_port2, 1));
  OLA_ASSERT_FALSE(port_manager.PatchPort(&output_port2, 2));
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // test repatching
  OLA_ASSERT(port_manager.PatchPort(&input_port, 3));
  OLA_ASSERT(port_manager.PatchPort(&output_port, 4));
  OLA_ASSERT(input_port.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 3,
                       input_port.GetUniverse()->UniverseId());
  OLA_ASSERT(output_port.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 4,
                       output_port.GetUniverse()->UniverseId());

  // test unpatching
  OLA_ASSERT(port_manager.UnPatchPort(&input_port));
  OLA_ASSERT(port_manager.UnPatchPort(&input_port2));
  OLA_ASSERT(port_manager.UnPatchPort(&output_port));
  OLA_ASSERT(port_manager.UnPatchPort(&output_port2));
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL), input_port.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());
}


/*
 * test that patching works correctly for devices with looping and multiport
 * patching enabled.
 */
void PortManagerTest::testPortPatchingLoopMulti() {
  ola::UniverseStore uni_store(NULL, NULL);
  ola::PortBroker broker;
  ola::PortManager port_manager(&uni_store, &broker);

  // mock device that allows looping and multi port patching
  MockDeviceLoopAndMulti device1(NULL, "test_device_1");
  TestMockInputPort input_port(&device1, 1, NULL);
  TestMockInputPort input_port2(&device1, 2, NULL);
  TestMockOutputPort output_port(&device1, 1);
  TestMockOutputPort output_port2(&device1, 2);
  device1.AddPort(&input_port);
  device1.AddPort(&input_port2);
  device1.AddPort(&output_port);
  device1.AddPort(&output_port2);

  OLA_ASSERT_EQ(static_cast<Universe*>(NULL), input_port.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port.GetUniverse());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // simple patching
  OLA_ASSERT(port_manager.PatchPort(&input_port, 1));
  OLA_ASSERT(port_manager.PatchPort(&output_port, 2));
  OLA_ASSERT(input_port.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 1,
                       input_port.GetUniverse()->UniverseId());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       input_port2.GetUniverse());
  OLA_ASSERT(output_port.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 2,
                       output_port.GetUniverse()->UniverseId());
  OLA_ASSERT_EQ(static_cast<Universe*>(NULL),
                       output_port2.GetUniverse());

  // test looping
  OLA_ASSERT(port_manager.PatchPort(&input_port2, 2));
  OLA_ASSERT(port_manager.PatchPort(&output_port2, 1));
  OLA_ASSERT(input_port2.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 2,
                       input_port2.GetUniverse()->UniverseId());
  OLA_ASSERT(output_port2.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 1,
                       output_port2.GetUniverse()->UniverseId());

  // test multiport
  OLA_ASSERT(port_manager.PatchPort(&input_port2, 1));
  OLA_ASSERT(port_manager.PatchPort(&output_port2, 2));
  OLA_ASSERT(input_port2.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 1,
                       input_port2.GetUniverse()->UniverseId());
  OLA_ASSERT(output_port2.GetUniverse());
  OLA_ASSERT_EQ((unsigned int) 2,
                       output_port2.GetUniverse()->UniverseId());
}


/*
 * Check that we can set priorities on an input port
 */
void PortManagerTest::testInputPortSetPriority() {
  // we're not testing patching so pass NULL here
  PortManager patcher(NULL, NULL);

  // Input port that doesn't support priorities
  TestMockInputPort input_port(NULL, 0, NULL);

  OLA_ASSERT_EQ(input_port.PriorityCapability(),
                       ola::CAPABILITY_STATIC);
  OLA_ASSERT_EQ(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(input_port.GetPriority(), ola::dmx::SOURCE_PRIORITY_DEFAULT);

  // this port doesn't support priorities so this is a noop
  OLA_ASSERT(patcher.SetPriorityInherit(&input_port));
  OLA_ASSERT_EQ(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(input_port.GetPriority(), ola::dmx::SOURCE_PRIORITY_DEFAULT);

  // set the static priority to 20
  OLA_ASSERT(patcher.SetPriorityOverride(&input_port, 20));
  OLA_ASSERT_EQ(input_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(input_port.GetPriority(), (uint8_t) 20);

  // Now test an input port that does support priorities
  TestMockPriorityInputPort input_port2(NULL, 1, NULL);
  OLA_ASSERT_EQ(input_port2.PriorityCapability(),
                       ola::CAPABILITY_FULL);
  OLA_ASSERT_EQ(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(input_port2.GetPriority(), ola::dmx::SOURCE_PRIORITY_DEFAULT);

  // try changing to override mode
  OLA_ASSERT(patcher.SetPriorityOverride(&input_port2, 20));
  OLA_ASSERT_EQ(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  OLA_ASSERT_EQ(input_port2.GetPriority(), (uint8_t) 20);

  // bump priority
  OLA_ASSERT(patcher.SetPriorityOverride(&input_port2, 180));
  OLA_ASSERT_EQ(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  OLA_ASSERT_EQ(input_port2.GetPriority(), (uint8_t) 180);

  // change back to inherit mode
  OLA_ASSERT(patcher.SetPriorityInherit(&input_port2));
  OLA_ASSERT_EQ(input_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(input_port2.GetPriority(), (uint8_t) 180);
}


/*
 * Check that we can set priorities on an Output port
 */
void PortManagerTest::testOutputPortSetPriority() {
  // we're not testing patching so pass NULL here
  PortManager patcher(NULL, NULL);

  // Input port that doesn't support priorities
  TestMockOutputPort output_port(NULL, 0);

  OLA_ASSERT_EQ(output_port.PriorityCapability(),
                       ola::CAPABILITY_NONE);
  OLA_ASSERT_EQ(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(output_port.GetPriority(), ola::dmx::SOURCE_PRIORITY_DEFAULT);

  // this port doesn't support priorities so these are all noops
  OLA_ASSERT(patcher.SetPriorityInherit(&output_port));
  OLA_ASSERT(patcher.SetPriorityOverride(&output_port, 20));
  OLA_ASSERT_EQ(output_port.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(output_port.GetPriority(), ola::dmx::SOURCE_PRIORITY_DEFAULT);

  // now test an output port that supports priorities
  TestMockPriorityOutputPort output_port2(NULL, 1);

  OLA_ASSERT_EQ(output_port2.PriorityCapability(),
                       ola::CAPABILITY_FULL);
  OLA_ASSERT_EQ(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(output_port2.GetPriority(), ola::dmx::SOURCE_PRIORITY_DEFAULT);

  // try changing to static mode
  OLA_ASSERT(patcher.SetPriorityOverride(&output_port2, 20));
  OLA_ASSERT_EQ(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  OLA_ASSERT_EQ(output_port2.GetPriority(), (uint8_t) 20);

  // bump priority
  OLA_ASSERT(patcher.SetPriorityOverride(&output_port2, 180));
  OLA_ASSERT_EQ(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_OVERRIDE);
  OLA_ASSERT_EQ(output_port2.GetPriority(), (uint8_t) 180);

  // change back to inherit mode
  OLA_ASSERT(patcher.SetPriorityInherit(&output_port2));
  OLA_ASSERT_EQ(output_port2.GetPriorityMode(),
                       ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(output_port2.GetPriority(), (uint8_t) 180);
}
