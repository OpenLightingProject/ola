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
 * PortTest.cpp
 * Test fixture for the Port classes
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "olad/PluginAdaptor.h"
#include "olad/PortBroker.h"
#include "olad/Preferences.h"
#include "olad/plugin_api/TestCommon.h"
#include "olad/plugin_api/UniverseStore.h"
#include "ola/testing/TestUtils.h"


using ola::Clock;
using ola::TimeStamp;
using std::string;

class PortTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PortTest);
  CPPUNIT_TEST(testOutputPortPriorities);
  CPPUNIT_TEST(testInputPortPriorities);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testOutputPortPriorities();
    void testInputPortPriorities();

 private:
    Clock m_clock;
};


CPPUNIT_TEST_SUITE_REGISTRATION(PortTest);


/*
 * Check that we can set the priority & mode of output ports
 */
void PortTest::testOutputPortPriorities() {
  TestMockOutputPort output_port(NULL, 1);

  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, output_port.GetPriority());
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_INHERIT, output_port.GetPriorityMode());

  // test the setting of priorities
  OLA_ASSERT(output_port.SetPriority(120));
  OLA_ASSERT_EQ((uint8_t) 120, output_port.GetPriority());
  OLA_ASSERT_FALSE(output_port.SetPriority(201));
  OLA_ASSERT_EQ((uint8_t) 120, output_port.GetPriority());
  OLA_ASSERT(output_port.SetPriority(0));
  OLA_ASSERT_EQ((uint8_t) 0, output_port.GetPriority());

  // test the setting of modes
  output_port.SetPriorityMode(ola::PRIORITY_MODE_STATIC);
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_STATIC, output_port.GetPriorityMode());

  output_port.SetPriorityMode(ola::PRIORITY_MODE_INHERIT);
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_INHERIT, output_port.GetPriorityMode());
}


/*
 * Test that we can set the priorities & modes of input ports
 */
void PortTest::testInputPortPriorities() {
  unsigned int universe_id = 1;
  ola::MemoryPreferences preferences("foo");
  ola::UniverseStore store(&preferences, NULL);
  ola::PortBroker broker;
  ola::PortManager port_manager(&store, &broker);

  MockDevice device(NULL, "foo");
  TimeStamp time_stamp;
  MockSelectServer ss(&time_stamp);
  ola::PluginAdaptor plugin_adaptor(NULL, &ss, NULL, NULL, NULL, NULL, NULL);
  // This port operates in static priority mode
  TestMockInputPort input_port(&device, 1, &plugin_adaptor);
  port_manager.PatchPort(&input_port, universe_id);

  ola::DmxBuffer buffer("foo bar baz");
  m_clock.CurrentMonotonicTime(&time_stamp);
  input_port.WriteDMX(buffer);
  input_port.DmxChanged();

  ola::Universe *universe = store.GetUniverseOrCreate(universe_id);
  OLA_ASSERT(universe);

  OLA_ASSERT_EQ(ola::dmx::SOURCE_PRIORITY_DEFAULT, universe->ActivePriority());

  // change the priority
  uint8_t new_priority = 120;
  port_manager.SetPriorityStatic(&input_port, new_priority);

  m_clock.CurrentMonotonicTime(&time_stamp);
  input_port.WriteDMX(buffer);
  input_port.DmxChanged();
  OLA_ASSERT_EQ(new_priority, universe->ActivePriority());

  new_priority = 0;
  port_manager.SetPriorityStatic(&input_port, new_priority);

  m_clock.CurrentMonotonicTime(&time_stamp);
  input_port.WriteDMX(buffer);
  input_port.DmxChanged();
  OLA_ASSERT_EQ(new_priority, universe->ActivePriority());
  port_manager.UnPatchPort(&input_port);

  // now try a port that supported priorities
  TestMockPriorityInputPort input_port2(&device, 2, &plugin_adaptor);
  port_manager.PatchPort(&input_port2, universe_id);

  // the default mode is static, lets change it to inherit
  input_port2.SetPriorityMode(ola::PRIORITY_MODE_INHERIT);
  input_port2.SetInheritedPriority(99);
  m_clock.CurrentMonotonicTime(&time_stamp);
  input_port2.WriteDMX(buffer);
  input_port2.DmxChanged();
  OLA_ASSERT_EQ((uint8_t) 99, universe->ActivePriority());

  input_port2.SetInheritedPriority(123);
  m_clock.CurrentMonotonicTime(&time_stamp);
  input_port2.WriteDMX(buffer);
  input_port2.DmxChanged();
  OLA_ASSERT_EQ((uint8_t) 123, universe->ActivePriority());

  // now try static mode
  new_priority = 108;
  port_manager.SetPriorityStatic(&input_port2, new_priority);
  m_clock.CurrentMonotonicTime(&time_stamp);
  input_port2.WriteDMX(buffer);
  input_port2.DmxChanged();
  OLA_ASSERT_EQ(new_priority,  universe->ActivePriority());
}
