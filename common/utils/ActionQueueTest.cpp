/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ActionQueueTest.cpp
 * Test fixture for the ActionQueue class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/ActionQueue.h"
#include "ola/Callback.h"
#include "ola/testing/TestUtils.h"


using ola::Action;
using ola::ActionQueue;
using ola::SingleUseCallback0;

class ActionQueueTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ActionQueueTest);
  CPPUNIT_TEST(testEmptyQueue);
  CPPUNIT_TEST(testSimpleQueue);
  CPPUNIT_TEST(testFailedQueue);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void CommandsComplete(ActionQueue *queue);
    void testEmptyQueue();
    void testSimpleQueue();
    void testFailedQueue();

  private:
    ActionQueue *m_received_queue;
};


/*
 * A basic test action
 */
class MockAction: public Action {
  public:
    MockAction(bool fatal = false, bool fail = false):
      Action(),
      m_fatal(fatal),
      m_failed(fail),
      m_executed(false) {}

    bool IsFatal() const { return m_fatal; }
    bool Failed() const { return m_failed; }
    bool Executed() const { return m_executed; }

    void Perform(SingleUseCallback0<void> *on_done);

  private:
    bool m_fatal;
    bool m_failed;
    bool m_executed;
};


void MockAction::Perform(SingleUseCallback0<void> *on_done) {
  m_executed = true;
  on_done->Run();
}


CPPUNIT_TEST_SUITE_REGISTRATION(ActionQueueTest);


void ActionQueueTest::setUp() {
  m_received_queue = NULL;
}


void ActionQueueTest::CommandsComplete(ActionQueue *queue) {
  m_received_queue = queue;
}


/*
 * Check that an empty queue works.
 */
void ActionQueueTest::testEmptyQueue() {
  ActionQueue queue(
      NewSingleCallback(this, &ActionQueueTest::CommandsComplete));
  queue.NextAction();
  OLA_ASSERT_EQ(&queue, m_received_queue);
  OLA_ASSERT_TRUE(queue.WasSuccessful());

  // try calling next item to make sure nothing happens
  queue.NextAction();
}


/*
 * Test that a simple queue works
 */
void ActionQueueTest::testSimpleQueue() {
  ActionQueue queue(
      NewSingleCallback(this, &ActionQueueTest::CommandsComplete));

  MockAction *action1 = new MockAction();
  queue.AddAction(action1);
  MockAction *action2 = new MockAction();
  queue.AddAction(action2);

  queue.NextAction();
  OLA_ASSERT_EQ(&queue, m_received_queue);
  OLA_ASSERT_TRUE(queue.WasSuccessful());
  OLA_ASSERT_TRUE(action1->Executed());
  OLA_ASSERT_TRUE(action2->Executed());

  // try calling next item to make sure nothing happens
  queue.NextAction();
}


/*
 * Test that a simple queue works
 */
void ActionQueueTest::testFailedQueue() {
  ActionQueue queue(
      NewSingleCallback(this, &ActionQueueTest::CommandsComplete));

  MockAction *action1 = new MockAction(false, true);
  queue.AddAction(action1);
  MockAction *action2 = new MockAction(true, true);
  queue.AddAction(action2);
  MockAction *action3 = new MockAction();
  queue.AddAction(action3);

  queue.NextAction();
  OLA_ASSERT_EQ(&queue, m_received_queue);
  OLA_ASSERT_FALSE(queue.WasSuccessful());
  OLA_ASSERT_TRUE(action1->Executed());
  OLA_ASSERT_TRUE(action2->Executed());
  OLA_ASSERT_FALSE(action3->Executed());

  // try calling next item to make sure nothing happens
  queue.NextAction();
}

