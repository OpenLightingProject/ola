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

#include "ola/testing/TestUtils.h"

#include "tools/ola_trigger/Action.h"
#include "tools/ola_trigger/DMXTrigger.h"
#include "tools/ola_trigger/MockAction.h"

using std::vector;
using ola::DmxBuffer;


class DMXTriggerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DMXTriggerTest);
  CPPUNIT_TEST(testRisingEdgeTrigger);
  CPPUNIT_TEST(testFallingEdgeTrigger);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testRisingEdgeTrigger();
    void testFallingEdgeTrigger();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(DMXTriggerTest);


/**
 * Check that triggering on a rising edge works correctly.
 */
void DMXTriggerTest::testRisingEdgeTrigger() {
  // setup the actions
  vector<Slot*> slots;
  Slot slot(2);
  MockAction *action = new MockAction();
  BadAction *bad_action = new BadAction();
  ValueInterval interval(10, 20);
  slot.AddAction(interval, action, bad_action);
  slots.push_back(&slot);

  Context context;
  DMXTrigger trigger(&context, slots);
  DmxBuffer buffer;

  // this shouldn't trigger
  buffer.SetFromString("0,0,0");
  trigger.NewDMX(buffer);
  OLA_ASSERT(action->NoCalls());

  // trigger rising edge
  buffer.SetFromString("0,0,10");
  trigger.NewDMX(buffer);
  action->CheckForValue(__LINE__, 10);

  // now send the same again
  OLA_ASSERT(action->NoCalls());
  trigger.NewDMX(buffer);
  OLA_ASSERT(action->NoCalls());

  // shorten the frame
  buffer.SetFromString("0,0");
  trigger.NewDMX(buffer);
  OLA_ASSERT(action->NoCalls());

  // lengthen again
  buffer.SetFromString("0,0,10,0");
  trigger.NewDMX(buffer);
  OLA_ASSERT(action->NoCalls());

  // change everything else
  buffer.SetFromString("10,100,10,20");
  trigger.NewDMX(buffer);
  OLA_ASSERT(action->NoCalls());
}


/**
 * Test that falling edges trigger
 */
void DMXTriggerTest::testFallingEdgeTrigger() {
  // setup the actions
  vector<Slot*> slots;
  Slot slot(2);
  MockAction *rising_action = new MockAction();
  MockAction *falling_action = new MockAction();
  ValueInterval interval(10, 20);
  slot.AddAction(interval, rising_action, falling_action);
  slots.push_back(&slot);

  Context context;
  DMXTrigger trigger(&context, slots);
  DmxBuffer buffer;

  // trigger
  buffer.SetFromString("0,0,20");
  trigger.NewDMX(buffer);
  rising_action->CheckForValue(__LINE__, 20);
  OLA_ASSERT(falling_action->NoCalls());

  // trigger a falling edge
  buffer.SetFromString("0,0,19");
  trigger.NewDMX(buffer);
  OLA_ASSERT(rising_action->NoCalls());
  falling_action->CheckForValue(__LINE__, 19);

  // now send the same again
  trigger.NewDMX(buffer);
  OLA_ASSERT(rising_action->NoCalls());
  OLA_ASSERT(falling_action->NoCalls());

  // shorten the frame
  buffer.SetFromString("0,0");
  trigger.NewDMX(buffer);
  OLA_ASSERT(rising_action->NoCalls());
  OLA_ASSERT(falling_action->NoCalls());

  // lengthen again
  buffer.SetFromString("0,0,19,0");
  trigger.NewDMX(buffer);
  OLA_ASSERT(rising_action->NoCalls());
  OLA_ASSERT(falling_action->NoCalls());

  // change everything else
  buffer.SetFromString("10,100,19,20");
  trigger.NewDMX(buffer);
  OLA_ASSERT(rising_action->NoCalls());
  OLA_ASSERT(falling_action->NoCalls());

  // change once more
  buffer.SetFromString("10,100,20,20");
  trigger.NewDMX(buffer);
  rising_action->CheckForValue(__LINE__, 20);
  OLA_ASSERT(falling_action->NoCalls());
}
