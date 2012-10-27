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
 * MultiCallbackTest.cpp
 * Unittest for MultiCallback class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>
#include <string>

#include "ola/Callback.h"
#include "ola/MultiCallback.h"
#include "ola/testing/TestUtils.h"


using std::string;

class MultiCallbackTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(MultiCallbackTest);
  CPPUNIT_TEST(testMultiCallback);
  CPPUNIT_TEST(testZeroLimit);
  CPPUNIT_TEST(testSingleLimit);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testMultiCallback();
    void testZeroLimit();
    void testSingleLimit();

    void CallbackMethod() {
      m_callback_count++;
    }

    void setUp() {
      m_callback_count = 0;
    }

  private:
    int m_callback_count;
};

CPPUNIT_TEST_SUITE_REGISTRATION(MultiCallbackTest);

using ola::BaseCallback0;
using ola::NewSingleCallback;
using ola::NewMultiCallback;


/**
 * Test the MultiCallback class.
 */
void MultiCallbackTest::testMultiCallback() {
  BaseCallback0<void> *callback = NewSingleCallback(
      this,
      &MultiCallbackTest::CallbackMethod);

  OLA_ASSERT_EQ(0, m_callback_count);
  BaseCallback0<void> *multi_callback = NewMultiCallback(3, callback);
  OLA_ASSERT_EQ(0, m_callback_count);

  multi_callback->Run();
  OLA_ASSERT_EQ(0, m_callback_count);

  multi_callback->Run();
  OLA_ASSERT_EQ(0, m_callback_count);

  multi_callback->Run();
  OLA_ASSERT_EQ(1, m_callback_count);
}


/**
 * Test the MultiCallback works with a 0 limit
 */
void MultiCallbackTest::testZeroLimit() {
  BaseCallback0<void> *callback = NewSingleCallback(
      this,
      &MultiCallbackTest::CallbackMethod);

  OLA_ASSERT_EQ(0, m_callback_count);
  BaseCallback0<void> *multi_callback = NewMultiCallback(0, callback);
  OLA_ASSERT_EQ(1, m_callback_count);

  (void) multi_callback;
}


/**
 * Test the MultiCallback works with a single limit
 */
void MultiCallbackTest::testSingleLimit() {
  BaseCallback0<void> *callback = NewSingleCallback(
      this,
      &MultiCallbackTest::CallbackMethod);

  OLA_ASSERT_EQ(0, m_callback_count);
  BaseCallback0<void> *multi_callback = NewMultiCallback(1, callback);
  OLA_ASSERT_EQ(0, m_callback_count);

  multi_callback->Run();
  OLA_ASSERT_EQ(1, m_callback_count);
}
