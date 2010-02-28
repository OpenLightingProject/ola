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
#include "olad/Preferences.h"
#include "olad/UniverseStore.h"

using ola::Clock;
using ola::TimeStamp;

class PortTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PortTest);
  CPPUNIT_TEST(testOutputPortPriorities);
  CPPUNIT_TEST(testInputPortPriorities);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testOutputPortPriorities();
    void testInputPortPriorities();
};


CPPUNIT_TEST_SUITE_REGISTRATION(PortTest);


/*
 * Check that we can set the priority & mode of output ports
 */
void PortTest::testOutputPortPriorities() {
  TestMockOutputPort output_port(NULL, 1);

  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
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


/*
 * Test that we can set the priorities & modes of input ports
 */
void PortTest::testInputPortPriorities() {
  unsigned int universe_id = 1;
  ola::MemoryPreferences preferences("foo");
  ola::UniverseStore store(&preferences, NULL);
  ola::PortManager port_manager(&store);

  MockDevice device(NULL, "foo");
  TimeStamp time_stamp;
  // This port operates in static priority mode
  TestMockInputPort input_port(&device, 1, &time_stamp);
  port_manager.PatchPort(&input_port, universe_id);

  ola::DmxBuffer buffer("foo bar baz");
  Clock::CurrentTime(time_stamp);
  input_port.WriteDMX(buffer);
  input_port.DmxChanged();

  ola::Universe *universe = store.GetUniverseOrCreate(universe_id);
  CPPUNIT_ASSERT(universe);

  CPPUNIT_ASSERT_EQUAL(ola::DmxSource::PRIORITY_DEFAULT,
                       universe->ActivePriority());

  // change the priority
  uint8_t new_priority = 120;
  port_manager.SetPriority(&input_port, ola::PRIORITY_MODE_OVERRIDE,
                           new_priority);

  Clock::CurrentTime(time_stamp);
  input_port.WriteDMX(buffer);
  input_port.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(new_priority, universe->ActivePriority());

  new_priority = 0;
  port_manager.SetPriority(&input_port, ola::PRIORITY_MODE_OVERRIDE,
                           new_priority);

  Clock::CurrentTime(time_stamp);
  input_port.WriteDMX(buffer);
  input_port.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(new_priority, universe->ActivePriority());
  port_manager.UnPatchPort(&input_port);

  // now try a port that supported priorities
  TestMockPriorityInputPort input_port2(&device, 2, &time_stamp);
  port_manager.PatchPort(&input_port2, universe_id);

  // the default mode is inherit
  input_port2.SetInheritedPriority(99);
  Clock::CurrentTime(time_stamp);
  input_port2.WriteDMX(buffer);
  input_port2.DmxChanged();
  CPPUNIT_ASSERT_EQUAL((uint8_t) 99, universe->ActivePriority());

  input_port2.SetInheritedPriority(123);
  Clock::CurrentTime(time_stamp);
  input_port2.WriteDMX(buffer);
  input_port2.DmxChanged();
  CPPUNIT_ASSERT_EQUAL((uint8_t) 123, universe->ActivePriority());

  // now try override mode
  new_priority = 108;
  port_manager.SetPriority(&input_port2, ola::PRIORITY_MODE_OVERRIDE,
                           new_priority);
  Clock::CurrentTime(time_stamp);
  input_port2.WriteDMX(buffer);
  input_port2.DmxChanged();
  CPPUNIT_ASSERT_EQUAL(new_priority,  universe->ActivePriority());
}
