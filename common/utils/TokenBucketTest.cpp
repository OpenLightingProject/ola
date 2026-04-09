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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * TokenBucketTest.cpp
 * Test fixture for the TokenBucket class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Clock.h"
#include "olad/TokenBucket.h"
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"



using ola::Clock;
using ola::TimeInterval;
using ola::TimeStamp;
using ola::TokenBucket;


class TokenBucketTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TokenBucketTest);
  CPPUNIT_TEST(testTokenBucket);
  CPPUNIT_TEST(testTokenBucketTwo);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testTokenBucket();
    void testTokenBucketTwo();
};


CPPUNIT_TEST_SUITE_REGISTRATION(TokenBucketTest);


/*
 * Check that the Token Bucket works
 */
void TokenBucketTest::testTokenBucket() {
  TimeStamp now;
  Clock clock;
  TimeInterval ten_ms(10000);
  TimeInterval one_hundred_ms(100000);
  TimeInterval one_second(1000000);
  clock.CurrentMonotonicTime(&now);
  TokenBucket bucket(0, 10, 10, now);  // one every 100ms
  OLA_ASSERT_EQ(0u, bucket.Count(now));

  now += one_hundred_ms;
  OLA_ASSERT_EQ(1u, bucket.Count(now));
  now += ten_ms;
  OLA_ASSERT_EQ(1u, bucket.Count(now));
  now += ten_ms;
  OLA_ASSERT_EQ(1u, bucket.Count(now));
  now += one_hundred_ms;
  OLA_ASSERT_EQ(2u, bucket.Count(now));
  OLA_ASSERT_TRUE(bucket.GetToken(now));
  OLA_ASSERT_TRUE(bucket.GetToken(now));
  OLA_ASSERT_FALSE(bucket.GetToken(now));
  OLA_ASSERT_EQ(0u, bucket.Count(now));

  now += one_second;
  OLA_ASSERT_EQ(10u, bucket.Count(now));
}


void TokenBucketTest::testTokenBucketTwo() {
  TimeStamp now;
  Clock clock;
  TimeInterval ten_ms(10000);
  TimeInterval one_hundred_ms(100000);
  TimeInterval one_second(1000000);
  TimeInterval five_minutes(5 * 60 * 1000000);
  clock.CurrentMonotonicTime(&now);
  TokenBucket bucket(0, 40, 40, now);  // one every 25ms
  OLA_ASSERT_EQ(0u, bucket.Count(now));

  now += one_hundred_ms;
  OLA_ASSERT_EQ(4u, bucket.Count(now));
  now += ten_ms;
  OLA_ASSERT_EQ(4u, bucket.Count(now));
  now += ten_ms;
  OLA_ASSERT_EQ(4u, bucket.Count(now));
  now += ten_ms;
  OLA_ASSERT_EQ(5u, bucket.Count(now));
  now += ten_ms;
  OLA_ASSERT_EQ(5u, bucket.Count(now));
  now += one_hundred_ms;
  OLA_ASSERT_EQ(9u, bucket.Count(now));
  now += ten_ms;
  OLA_ASSERT_EQ(10u, bucket.Count(now));
  now += one_second;
  OLA_ASSERT_EQ(40u, bucket.Count(now));

  // now try a very long duration
  now += five_minutes;
  OLA_ASSERT_EQ(40u, bucket.Count(now));

  // take 10 tokens from the bucket
  for (unsigned int i = 0; i < 10; i++) {
    OLA_ASSERT_TRUE(bucket.GetToken(now));
  }
  OLA_ASSERT_EQ(30u, bucket.Count(now));

  // add a bit of time
  now += ten_ms;
  OLA_ASSERT_EQ(30u, bucket.Count(now));
}
