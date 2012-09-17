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
 * SlotTest.cpp
 * Test fixture for the Slot class.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <ola/Logging.h>
#include <sstream>
#include <string>

#include "ola/testing/TestUtils.h"

#include "tools/ola_trigger/Action.h"
#include "tools/ola_trigger/MockAction.h"


class SlotTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(SlotTest);
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


CPPUNIT_TEST_SUITE_REGISTRATION(SlotTest);


/**
 * Chech that we don't add Intervals which intersect
 */
void SlotTest::testIntersectingIntervalAddition() {
  Slot slot(0);
  OLA_ASSERT(slot.AddAction(ValueInterval(10, 20), NULL, NULL));

  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(10, 20), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(8, 10), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(10, 10), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(10, 11), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(10, 25), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(15, 25), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(19, 20), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(20, 20), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(20, 25), NULL, NULL));

  // now add another interval
  OLA_ASSERT(slot.AddAction(ValueInterval(30, 35), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(29, 30), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(30, 30), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(30, 35), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(34, 35), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(34, 36), NULL, NULL));

  // and another one
  OLA_ASSERT(slot.AddAction(ValueInterval(40, 45), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(29, 30), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(30, 30), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(30, 35), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(34, 35), NULL, NULL));
  OLA_ASSERT_FALSE(slot.AddAction(ValueInterval(34, 36), NULL, NULL));
}


/**
 * Check that adding intervals works
 */
void SlotTest::testIntervalAddition() {
  Slot slot(0);
  OLA_ASSERT(slot.AddAction(ValueInterval(10, 20), NULL, NULL));
  OLA_ASSERT_EQ(string("[10, 20]"), slot.IntervalsAsString());

  // add before the begining
  OLA_ASSERT(slot.AddAction(ValueInterval(5, 6), NULL, NULL));
  OLA_ASSERT_EQ(string("[5, 6], [10, 20]"),
                       slot.IntervalsAsString());

  // add at the end
  OLA_ASSERT(slot.AddAction(ValueInterval(100, 104), NULL, NULL));
  OLA_ASSERT_EQ(string("[5, 6], [10, 20], [100, 104]"),
                       slot.IntervalsAsString());

  // now try adding some in the middle
  OLA_ASSERT(slot.AddAction(ValueInterval(80, 82), NULL, NULL));
  OLA_ASSERT_EQ(string("[5, 6], [10, 20], [80, 82], [100, 104]"),
                       slot.IntervalsAsString());

  OLA_ASSERT(slot.AddAction(ValueInterval(76, 76), NULL, NULL));
  OLA_ASSERT_EQ(string("[5, 6], [10, 20], 76, [80, 82], [100, 104]"),
                       slot.IntervalsAsString());

  OLA_ASSERT(slot.AddAction(ValueInterval(70, 72), NULL, NULL));
  OLA_ASSERT_EQ(
      string("[5, 6], [10, 20], [70, 72], 76, [80, 82], [100, 104]"),
      slot.IntervalsAsString());

  OLA_ASSERT(slot.AddAction(ValueInterval(65, 69), NULL, NULL));
  OLA_ASSERT_EQ(
      string("[5, 6], [10, 20], [65, 69], [70, 72], 76, [80, 82], [100, 104]"),
      slot.IntervalsAsString());
}


/**
 * Check actions are matched correctly.
 */
void SlotTest::testActionMatching() {
  Slot slot(0);

  MockAction *rising_action1 = new MockAction();
  MockAction *falling_action1 = new MockAction();
  slot.AddAction(
      ValueInterval(10, 20), rising_action1, falling_action1);

  MockAction *default_rising_action = new MockAction();
  OLA_ASSERT_FALSE(slot.SetDefaultRisingAction(default_rising_action));
  MockAction *default_falling_action = new MockAction();
  OLA_ASSERT(
      !slot.SetDefaultFallingAction(default_falling_action));

  slot.TakeAction(NULL, 10);
  rising_action1->CheckForValue(__LINE__, 10);
  OLA_ASSERT(falling_action1->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());

  slot.TakeAction(NULL, 20);
  rising_action1->CheckForValue(__LINE__, 20);
  OLA_ASSERT(falling_action1->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());

  slot.TakeAction(NULL, 2);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  default_falling_action->CheckForValue(__LINE__, 2);

  slot.TakeAction(NULL, 9);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  default_rising_action->CheckForValue(__LINE__, 9);
  OLA_ASSERT(default_falling_action->NoCalls());

  slot.TakeAction(NULL, 21);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  default_rising_action->CheckForValue(__LINE__, 21);
  OLA_ASSERT(default_falling_action->NoCalls());

  // add another action
  MockAction *rising_action2 = new MockAction();
  MockAction *falling_action2 = new MockAction();
  OLA_ASSERT(slot.AddAction(
        ValueInterval(30, 40), rising_action2, falling_action2));

  slot.TakeAction(NULL, 30);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  rising_action2->CheckForValue(__LINE__, 30);
  OLA_ASSERT(falling_action2->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());

  slot.TakeAction(NULL, 35);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  rising_action2->CheckForValue(__LINE__, 35);
  OLA_ASSERT(falling_action2->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());

  slot.TakeAction(NULL, 40);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  rising_action2->CheckForValue(__LINE__, 40);
  OLA_ASSERT(falling_action2->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());

  // and another two actions
  MockAction *rising_action3 = new MockAction();
  OLA_ASSERT(
      slot.AddAction(ValueInterval(23, 27), rising_action3, NULL));

  MockAction *falling_action3 = new MockAction();
  OLA_ASSERT(
      slot.AddAction(ValueInterval(28, 29), NULL, falling_action3));

  slot.TakeAction(NULL, 28);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  OLA_ASSERT(rising_action2->NoCalls());
  OLA_ASSERT(falling_action2->NoCalls());
  OLA_ASSERT(rising_action3->NoCalls());
  falling_action3->CheckForValue(__LINE__, 28);
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());

  slot.TakeAction(NULL, 25);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  OLA_ASSERT(rising_action2->NoCalls());
  OLA_ASSERT(falling_action2->NoCalls());
  OLA_ASSERT(rising_action3->NoCalls());
  OLA_ASSERT(falling_action3->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  default_falling_action->CheckForValue(__LINE__, 25);

  slot.TakeAction(NULL, 27);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  OLA_ASSERT(rising_action2->NoCalls());
  OLA_ASSERT(falling_action2->NoCalls());
  rising_action3->CheckForValue(__LINE__, 27);
  OLA_ASSERT(falling_action3->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());

  // check the default case
  slot.TakeAction(NULL, 22);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  OLA_ASSERT(rising_action2->NoCalls());
  OLA_ASSERT(falling_action2->NoCalls());
  OLA_ASSERT(rising_action3->NoCalls());
  OLA_ASSERT(falling_action3->NoCalls());
  OLA_ASSERT(default_rising_action->NoCalls());
  default_falling_action->CheckForValue(__LINE__, 22);

  slot.TakeAction(NULL, 28);
  OLA_ASSERT(rising_action1->NoCalls());
  OLA_ASSERT(falling_action1->NoCalls());
  OLA_ASSERT(rising_action2->NoCalls());
  OLA_ASSERT(falling_action2->NoCalls());
  OLA_ASSERT(rising_action3->NoCalls());
  OLA_ASSERT(falling_action3->NoCalls());
  default_rising_action->CheckForValue(__LINE__, 28);
  OLA_ASSERT(default_falling_action->NoCalls());
}


/**
 * Check the default actions are called if no matches are found.
 */
void SlotTest::testDefaultAction() {
  Slot slot(1);

  MockAction *default_rising_action = new MockAction();
  MockAction *default_falling_action = new MockAction();
  OLA_ASSERT_FALSE(slot.SetDefaultRisingAction(default_rising_action));
  OLA_ASSERT(
      !slot.SetDefaultFallingAction(default_falling_action));

  // signal a rising edge
  slot.TakeAction(NULL, 100);
  default_rising_action->CheckForValue(__LINE__, 100);
  OLA_ASSERT(default_falling_action->NoCalls());

  // signal a falling edge
  slot.TakeAction(NULL, 99);
  OLA_ASSERT(default_rising_action->NoCalls());
  default_falling_action->CheckForValue(__LINE__, 99);

  // now try to add another default one, ref the existing actions otherwise
  // we'll delete them
  default_rising_action->Ref();
  default_falling_action->Ref();
  MockAction *default_rising_action2 = new MockAction();
  MockAction *default_falling_action2 = new MockAction();
  OLA_ASSERT(
      slot.SetDefaultRisingAction(default_rising_action2));
  OLA_ASSERT(
      slot.SetDefaultFallingAction(default_falling_action2));

  // make sure the new actions are used
  slot.TakeAction(NULL, 100);
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());
  default_rising_action2->CheckForValue(__LINE__, 100);
  OLA_ASSERT(default_falling_action2->NoCalls());

  // signal a falling edge
  slot.TakeAction(NULL, 99);
  OLA_ASSERT(default_rising_action->NoCalls());
  OLA_ASSERT(default_falling_action->NoCalls());
  OLA_ASSERT(default_rising_action2->NoCalls());
  default_falling_action2->CheckForValue(__LINE__, 99);

  default_rising_action->DeRef();
  default_falling_action->DeRef();
}
