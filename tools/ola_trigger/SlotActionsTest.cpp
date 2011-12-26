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

#include "tools/ola_trigger/Action.h"
#include "tools/ola_trigger/MockAction.h"


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
 * Chech that we don't add Intervals which intersect
 */
void SlotActionsTest::testIntersectingIntervalAddition() {
  SlotActions slot_actions(0);
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(10, 20), NULL, NULL));

  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(10, 20), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(8, 10), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(10, 10), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(10, 11), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(10, 25), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(15, 25), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(19, 20), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(20, 20), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(20, 25), NULL, NULL));

  // now add another interval
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(30, 35), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(29, 30), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(30, 30), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(30, 35), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(34, 35), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(34, 36), NULL, NULL));

  // and another one
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(40, 45), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(29, 30), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(30, 30), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(30, 35), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(34, 35), NULL, NULL));
  CPPUNIT_ASSERT(!slot_actions.AddAction(ValueInterval(34, 36), NULL, NULL));
}


/**
 * Check that adding intervals works
 */
void SlotActionsTest::testIntervalAddition() {
  SlotActions slot_actions(0);
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(10, 20), NULL, NULL));
  CPPUNIT_ASSERT_EQUAL(string("[10, 20]"), slot_actions.IntervalsAsString());

  // add before the begining
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(5, 6), NULL, NULL));
  CPPUNIT_ASSERT_EQUAL(string("[5, 6], [10, 20]"),
                       slot_actions.IntervalsAsString());

  // add at the end
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(100, 104), NULL, NULL));
  CPPUNIT_ASSERT_EQUAL(string("[5, 6], [10, 20], [100, 104]"),
                       slot_actions.IntervalsAsString());

  // now try adding some in the middle
  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(80, 82), NULL, NULL));
  CPPUNIT_ASSERT_EQUAL(string("[5, 6], [10, 20], [80, 82], [100, 104]"),
                       slot_actions.IntervalsAsString());

  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(76, 76), NULL, NULL));
  CPPUNIT_ASSERT_EQUAL(string("[5, 6], [10, 20], 76, [80, 82], [100, 104]"),
                       slot_actions.IntervalsAsString());

  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(70, 72), NULL, NULL));
  CPPUNIT_ASSERT_EQUAL(
      string("[5, 6], [10, 20], [70, 72], 76, [80, 82], [100, 104]"),
      slot_actions.IntervalsAsString());

  CPPUNIT_ASSERT(slot_actions.AddAction(ValueInterval(65, 69), NULL, NULL));
  CPPUNIT_ASSERT_EQUAL(
      string("[5, 6], [10, 20], [65, 69], [70, 72], 76, [80, 82], [100, 104]"),
      slot_actions.IntervalsAsString());
}


/**
 * Check actions are matched correctly.
 */
void SlotActionsTest::testActionMatching() {
  SlotActions slot_actions(0);

  MockAction *rising_action1 = new MockAction();
  MockAction *falling_action1 = new MockAction();
  slot_actions.AddAction(
      ValueInterval(10, 20), rising_action1, falling_action1);

  MockAction *default_rising_action = new MockAction();
  CPPUNIT_ASSERT(!slot_actions.SetDefaultRisingAction(default_rising_action));
  MockAction *default_falling_action = new MockAction();
  CPPUNIT_ASSERT(
      !slot_actions.SetDefaultFallingAction(default_falling_action));

  slot_actions.TakeAction(NULL, 10, SlotActions::RISING);
  rising_action1->CheckForValue(__LINE__, 10);
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  slot_actions.TakeAction(NULL, 20, SlotActions::RISING);
  rising_action1->CheckForValue(__LINE__, 20);
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  slot_actions.TakeAction(NULL, 2, SlotActions::FALLING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  default_falling_action->CheckForValue(__LINE__, 2);

  slot_actions.TakeAction(NULL, 9, SlotActions::RISING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  default_rising_action->CheckForValue(__LINE__, 9);
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  slot_actions.TakeAction(NULL, 21, SlotActions::RISING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  default_rising_action->CheckForValue(__LINE__, 21);
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  // add another action
  MockAction *rising_action2 = new MockAction();
  MockAction *falling_action2 = new MockAction();
  CPPUNIT_ASSERT(slot_actions.AddAction(
        ValueInterval(30, 40), rising_action2, falling_action2));

  slot_actions.TakeAction(NULL, 30, SlotActions::RISING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  rising_action2->CheckForValue(__LINE__, 30);
  CPPUNIT_ASSERT(falling_action2->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  slot_actions.TakeAction(NULL, 35, SlotActions::RISING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  rising_action2->CheckForValue(__LINE__, 35);
  CPPUNIT_ASSERT(falling_action2->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  slot_actions.TakeAction(NULL, 40, SlotActions::RISING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  rising_action2->CheckForValue(__LINE__, 40);
  CPPUNIT_ASSERT(falling_action2->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  // and another two actions
  MockAction *rising_action3 = new MockAction();
  CPPUNIT_ASSERT(
      slot_actions.AddAction(ValueInterval(23, 27), rising_action3, NULL));

  MockAction *falling_action3 = new MockAction();
  CPPUNIT_ASSERT(
      slot_actions.AddAction(ValueInterval(28, 29), NULL, falling_action3));

  slot_actions.TakeAction(NULL, 28, SlotActions::FALLING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  CPPUNIT_ASSERT(rising_action2->NoCalls());
  CPPUNIT_ASSERT(falling_action2->NoCalls());
  CPPUNIT_ASSERT(rising_action3->NoCalls());
  falling_action3->CheckForValue(__LINE__, 28);
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  slot_actions.TakeAction(NULL, 25, SlotActions::FALLING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  CPPUNIT_ASSERT(rising_action2->NoCalls());
  CPPUNIT_ASSERT(falling_action2->NoCalls());
  CPPUNIT_ASSERT(rising_action3->NoCalls());
  CPPUNIT_ASSERT(falling_action3->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  default_falling_action->CheckForValue(__LINE__, 25);

  slot_actions.TakeAction(NULL, 27, SlotActions::RISING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  CPPUNIT_ASSERT(rising_action2->NoCalls());
  CPPUNIT_ASSERT(falling_action2->NoCalls());
  rising_action3->CheckForValue(__LINE__, 27);
  CPPUNIT_ASSERT(falling_action3->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  // check the default case
  slot_actions.TakeAction(NULL, 22, SlotActions::FALLING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  CPPUNIT_ASSERT(rising_action2->NoCalls());
  CPPUNIT_ASSERT(falling_action2->NoCalls());
  CPPUNIT_ASSERT(rising_action3->NoCalls());
  CPPUNIT_ASSERT(falling_action3->NoCalls());
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  default_falling_action->CheckForValue(__LINE__, 22);

  slot_actions.TakeAction(NULL, 28, SlotActions::RISING);
  CPPUNIT_ASSERT(rising_action1->NoCalls());
  CPPUNIT_ASSERT(falling_action1->NoCalls());
  CPPUNIT_ASSERT(rising_action2->NoCalls());
  CPPUNIT_ASSERT(falling_action2->NoCalls());
  CPPUNIT_ASSERT(rising_action3->NoCalls());
  CPPUNIT_ASSERT(falling_action3->NoCalls());
  default_rising_action->CheckForValue(__LINE__, 28);
  CPPUNIT_ASSERT(default_falling_action->NoCalls());
}


/**
 * Check the default actions are called if no matches are found.
 */
void SlotActionsTest::testDefaultAction() {
  SlotActions slot_actions(1);

  MockAction *default_rising_action = new MockAction();
  MockAction *default_falling_action = new MockAction();
  CPPUNIT_ASSERT(!slot_actions.SetDefaultRisingAction(default_rising_action));
  CPPUNIT_ASSERT(
      !slot_actions.SetDefaultFallingAction(default_falling_action));

  // signal a rising edge
  slot_actions.TakeAction(NULL, 100, SlotActions::RISING);
  default_rising_action->CheckForValue(__LINE__, 100);
  CPPUNIT_ASSERT(default_falling_action->NoCalls());

  // signal a falling edge
  slot_actions.TakeAction(NULL, 100, SlotActions::FALLING);
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  default_falling_action->CheckForValue(__LINE__, 100);

  // now try to add another default one, ref the existing actions otherwise
  // we'll delete them
  default_rising_action->Ref();
  default_falling_action->Ref();
  MockAction *default_rising_action2 = new MockAction();
  MockAction *default_falling_action2 = new MockAction();
  CPPUNIT_ASSERT(
      slot_actions.SetDefaultRisingAction(default_rising_action2));
  CPPUNIT_ASSERT(
      slot_actions.SetDefaultFallingAction(default_falling_action2));

  // make sure the new actions are used
  slot_actions.TakeAction(NULL, 100, SlotActions::RISING);
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());
  default_rising_action2->CheckForValue(__LINE__, 100);
  CPPUNIT_ASSERT(default_falling_action2->NoCalls());

  // signal a falling edge
  slot_actions.TakeAction(NULL, 100, SlotActions::FALLING);
  CPPUNIT_ASSERT(default_rising_action->NoCalls());
  CPPUNIT_ASSERT(default_falling_action->NoCalls());
  CPPUNIT_ASSERT(default_rising_action2->NoCalls());
  default_falling_action2->CheckForValue(__LINE__, 100);

  default_rising_action->DeRef();
  default_falling_action->DeRef();
}
