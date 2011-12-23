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
 * DMXTriggerTest.cpp
 * Test fixture for the DMXTrigger class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <ola/Logging.h>
#include <ola/DmxBuffer.h>
#include <vector>
#include "tools/dmx_trigger/Action.h"
#include "tools/dmx_trigger/DmxTrigger.h"
#include "tools/dmx_trigger/MockAction.h"

using std::vector;
using ola::DmxBuffer;


class DMXTriggerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DMXTriggerTest);
  CPPUNIT_TEST(testTrigger);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testTrigger();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(DMXTriggerTest);


/**
 * Check that triggering works correctly.
 */
void DMXTriggerTest::testTrigger() {
  // setup the actions
  vector<SlotActions*> slots;
  SlotActions slot_actions(2);
  MockAction *action = new MockAction();
  ValueInterval interval(10, 20);
  slot_actions.AddAction(interval, action);
  slots.push_back(&slot_actions);

  Context context;
  DMXTrigger trigger(&context, slots);
  DmxBuffer buffer;

  // this shouldn't trigger
  buffer.SetFromString("0,0,0");
  trigger.NewDMX(buffer);
  CPPUNIT_ASSERT(action->NoCalls());

  // trigger
  buffer.SetFromString("0,0,10");
  trigger.NewDMX(buffer);
  action->CheckForValue(__LINE__, 10);

  // now send the same again
  trigger.NewDMX(buffer);
  CPPUNIT_ASSERT(action->NoCalls());

  // shorten the frame
  buffer.SetFromString("0,0");
  trigger.NewDMX(buffer);
  CPPUNIT_ASSERT(action->NoCalls());

  // lengthen again
  buffer.SetFromString("0,0,10,0");
  trigger.NewDMX(buffer);
  action->CheckForValue(__LINE__, 10);

  // change everything else
  buffer.SetFromString("10,100,10,20");
  trigger.NewDMX(buffer);
  CPPUNIT_ASSERT(action->NoCalls());
}
