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
 * BackoffTest.cpp
 * Test fixture for the TCPConnector class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <string>

#include "ola/Callback.h"
#include "ola/Clock.h"
#include "ola/util/Backoff.h"
#include "ola/testing/TestUtils.h"


using ola::BackoffGenerator;
using ola::ExponentialBackoffPolicy;
using ola::LinearBackoffPolicy;
using ola::TimeInterval;

class BackoffTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(BackoffTest);

  CPPUNIT_TEST(testLinearBackoffPolicy);
  CPPUNIT_TEST(testExponentialBackoffPolicy);
  CPPUNIT_TEST(testBackoffGenerator);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testLinearBackoffPolicy();
    void testExponentialBackoffPolicy();
    void testBackoffGenerator();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BackoffTest);


/**
 * Test the linear backoff policy.
 */
void BackoffTest::testLinearBackoffPolicy() {
  // 5 per attempt, up to a max of 30
  LinearBackoffPolicy policy(TimeInterval(5, 0), TimeInterval(30, 0));

  OLA_ASSERT_EQ(TimeInterval(5, 0), policy.BackOffTime(1));
  OLA_ASSERT_EQ(TimeInterval(10, 0), policy.BackOffTime(2));
  OLA_ASSERT_EQ(TimeInterval(15, 0), policy.BackOffTime(3));

  OLA_ASSERT_EQ(TimeInterval(30, 0), policy.BackOffTime(6));
  OLA_ASSERT_EQ(TimeInterval(30, 0), policy.BackOffTime(7));
}


/**
 * Test the exponential backoff policy.
 */
void BackoffTest::testExponentialBackoffPolicy() {
  // start with 10s, up to 170s.
  ExponentialBackoffPolicy policy(TimeInterval(10, 0), TimeInterval(170, 0));

  OLA_ASSERT_EQ(TimeInterval(10, 0), policy.BackOffTime(1));
  OLA_ASSERT_EQ(TimeInterval(20, 0), policy.BackOffTime(2));
  OLA_ASSERT_EQ(TimeInterval(40, 0), policy.BackOffTime(3));
  OLA_ASSERT_EQ(TimeInterval(80, 0), policy.BackOffTime(4));
  OLA_ASSERT_EQ(TimeInterval(160, 0), policy.BackOffTime(5));
  OLA_ASSERT_EQ(TimeInterval(170, 0), policy.BackOffTime(6));
  OLA_ASSERT_EQ(TimeInterval(170, 0), policy.BackOffTime(7));
}


/**
 * Test the BackoffGenerator
 */
void BackoffTest::testBackoffGenerator() {
  BackoffGenerator generator(
      new LinearBackoffPolicy(TimeInterval(5, 0), TimeInterval(30, 0)));

  OLA_ASSERT_EQ(TimeInterval(5, 0), generator.Next());
  OLA_ASSERT_EQ(TimeInterval(10, 0), generator.Next());
  OLA_ASSERT_EQ(TimeInterval(15, 0), generator.Next());
  OLA_ASSERT_EQ(TimeInterval(20, 0), generator.Next());
  OLA_ASSERT_EQ(TimeInterval(25, 0), generator.Next());
  OLA_ASSERT_EQ(TimeInterval(30, 0), generator.Next());

  generator.Reset();
  OLA_ASSERT_EQ(TimeInterval(5, 0), generator.Next());
  OLA_ASSERT_EQ(TimeInterval(10, 0), generator.Next());
  OLA_ASSERT_EQ(TimeInterval(15, 0), generator.Next());
}
