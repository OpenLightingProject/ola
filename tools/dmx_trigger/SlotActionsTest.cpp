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
 * SlotActionsTest.cpp
 * Test fixture for the SlotActions class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <ola/Logging.h>
#include <sstream>
#include <string>

#include "tools/dmx_trigger/Action.h"
#include "tools/dmx_trigger/MockAction.h"


class SlotActionsTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SlotActionsTest);
  CPPUNIT_TEST(testIntersectingIntervalAddition);
  CPPUNIT_TEST(testIntervalAddition);
  CPPUNIT_TEST(testActionMatching);
  CPPUNIT_TEST(testDefaultAction);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testIntersectingIntervalAddition();
    void testIntervalAddition();
    void testActionMatching();
    void testDefaultAction();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(SlotActionsTest);


/**
 * An action that should never be run.
 */
class BadAction: public Action {
  public:
    BadAction() : Action() {}

    void Execute(Context*, uint8_t slot_value) {
      std::stringstream str;
      str << "Incorrect action called for " << static_cast<int>(slot_value);
      CPPUNIT_FAIL(str.str());
    }
};


/**
 * Chech that we don't add Intervals which intersect
 */
void SlotActionsTest::testIntersectingIntervalAddition() {
  SlotActions slot_actions(0);
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(10, 20), NULL));

  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(10, 20), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(8, 10), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(10, 10), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(10, 11), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(10, 25), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(15, 25), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(19, 20), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(20, 20), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(20, 25), NULL));

  // now add another interval
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(30, 35), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(29, 30), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(30, 30), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(30, 35), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(34, 35), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(34, 36), NULL));

  // and another one
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(40, 45), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(29, 30), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(30, 30), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(30, 35), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(34, 35), NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(34, 36), NULL));
}


/**
 * Check that adding intervals works
 */
void SlotActionsTest::testIntervalAddition() {
  SlotActions slot_actions(0);
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(10, 20), NULL));
  CPPUNIT_ASSERT_EQUAL(string("[10, 20]"), slot_actions.IntervalsAsString());

  // add before the begining
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(5, 6), NULL));
  CPPUNIT_ASSERT_EQUAL(string("[5, 6], [10, 20]"),
                       slot_actions.IntervalsAsString());

  // add at the end
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(100, 104), NULL));
  CPPUNIT_ASSERT_EQUAL(string("[5, 6], [10, 20], [100, 104]"),
                       slot_actions.IntervalsAsString());

  // now try adding some in the middle
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(80, 82), NULL));
  CPPUNIT_ASSERT_EQUAL(string("[5, 6], [10, 20], [80, 82], [100, 104]"),
                       slot_actions.IntervalsAsString());

  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(76, 76), NULL));
  CPPUNIT_ASSERT_EQUAL(string("[5, 6], [10, 20], 76, [80, 82], [100, 104]"),
                       slot_actions.IntervalsAsString());

  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(70, 72), NULL));
  CPPUNIT_ASSERT_EQUAL(
      string("[5, 6], [10, 20], [70, 72], 76, [80, 82], [100, 104]"),
      slot_actions.IntervalsAsString());

  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(65, 69), NULL));
  CPPUNIT_ASSERT_EQUAL(
      string("[5, 6], [10, 20], [65, 69], [70, 72], 76, [80, 82], [100, 104]"),
      slot_actions.IntervalsAsString());
}


/**
 * Check actions are matched correctly.
 */
void SlotActionsTest::testActionMatching() {
  SlotActions slot_actions(0);

  MockAction *action1 = new MockAction();
  slot_actions.AddAction(ValueInterval(10, 20), action1);

  MockAction *default_action = new MockAction();
  slot_actions.SetDefaultAction(default_action);

  slot_actions.TakeAction(NULL, 10);
  action1->CheckForValue(__LINE__, 10);
  CPPUNIT_ASSERT(default_action->NoCalls());

  slot_actions.TakeAction(NULL, 20);
  action1->CheckForValue(__LINE__, 20);
  CPPUNIT_ASSERT(default_action->NoCalls());

  slot_actions.TakeAction(NULL, 0);
  CPPUNIT_ASSERT(action1->NoCalls());
  default_action->CheckForValue(__LINE__, 0);

  slot_actions.TakeAction(NULL, 9);
  CPPUNIT_ASSERT(action1->NoCalls());
  default_action->CheckForValue(__LINE__, 9);

  slot_actions.TakeAction(NULL, 21);
  CPPUNIT_ASSERT(action1->NoCalls());
  default_action->CheckForValue(__LINE__, 21);

  // add another action
  slot_actions.AddAction(ValueInterval(30, 40), action1);

  slot_actions.TakeAction(NULL, 30);
  action1->CheckForValue(__LINE__, 30);
  CPPUNIT_ASSERT(default_action->NoCalls());

  slot_actions.TakeAction(NULL, 35);
  action1->CheckForValue(__LINE__, 35);
  CPPUNIT_ASSERT(default_action->NoCalls());

  slot_actions.TakeAction(NULL, 40);
  action1->CheckForValue(__LINE__, 40);
  CPPUNIT_ASSERT(default_action->NoCalls());

  // and another
  slot_actions.AddAction(ValueInterval(23, 27), action1);

  slot_actions.TakeAction(NULL, 23);
  action1->CheckForValue(__LINE__, 23);
  CPPUNIT_ASSERT(default_action->NoCalls());

  slot_actions.TakeAction(NULL, 25);
  action1->CheckForValue(__LINE__, 25);
  CPPUNIT_ASSERT(default_action->NoCalls());

  slot_actions.TakeAction(NULL, 27);
  action1->CheckForValue(__LINE__, 27);
  CPPUNIT_ASSERT(default_action->NoCalls());

  // check the default case
  slot_actions.TakeAction(NULL, 22);
  CPPUNIT_ASSERT(action1->NoCalls());
  default_action->CheckForValue(__LINE__, 22);

  slot_actions.TakeAction(NULL, 28);
  CPPUNIT_ASSERT(action1->NoCalls());
  default_action->CheckForValue(__LINE__, 28);
}


/**
 * Check the default actions are called if no matches are found.
 */
void SlotActionsTest::testDefaultAction() {
  SlotActions slot_actions(1);

  MockAction *default_action = new MockAction();
  CPPUNIT_ASSERT(!slot_actions.SetDefaultAction(default_action));

  slot_actions.TakeAction(NULL, 100);
  default_action->CheckForValue(__LINE__, 100);

  // now try to add another default one
  CPPUNIT_ASSERT(slot_actions.SetDefaultAction(default_action));
}
