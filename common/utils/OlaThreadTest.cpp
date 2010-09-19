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
 * OlaThreadTest.cpp
 * Test fixture for the OlaThread class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/OlaThread.h"

using ola::OlaThread;

class OlaThreadTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OlaThreadTest);
  CPPUNIT_TEST(testOlaThread);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testOlaThread();
};

class MockThread: public OlaThread {
  public:
    MockThread()
        : OlaThread(),
          thread_ran(false) {
    }
    ~MockThread() {}

    void *Run() {
      thread_ran = true;
      return NULL;
    }

    bool thread_ran;
};


CPPUNIT_TEST_SUITE_REGISTRATION(OlaThreadTest);


/*
 * Check that basic thread functionality works.
 */
void OlaThreadTest::testOlaThread() {
  MockThread thread;
  CPPUNIT_ASSERT(!thread.thread_ran);
  thread.Start();
  CPPUNIT_ASSERT(thread.IsRunning());
  CPPUNIT_ASSERT(thread.Join());
  CPPUNIT_ASSERT(thread.thread_ran);
}
