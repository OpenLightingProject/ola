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
 * TokenBucketTest.cpp
 * Test fixture for the TokenBucket class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Clock.h"
#include "olad/TokenBucket.h"
#include "ola/Logging.h"


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

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(TokenBucketTest);


/*
 * Check that the Token Bucket works
 */
void TokenBucketTest::testTokenBucket() {
  TimeStamp now;
  TimeInterval ten_ms(10000);
  TimeInterval one_hundred_ms(100000);
  TimeInterval one_second(1000000);
  ola::Clock::CurrentTime(&now);
  TokenBucket bucket(0, 10, 10, now);  // one every 100ms
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bucket.Count(now));

  now += one_hundred_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, bucket.Count(now));
  now += ten_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, bucket.Count(now));
  now += ten_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, bucket.Count(now));
  now += one_hundred_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, bucket.Count(now));
  CPPUNIT_ASSERT(bucket.GetToken(now));
  CPPUNIT_ASSERT(bucket.GetToken(now));
  CPPUNIT_ASSERT(!bucket.GetToken(now));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bucket.Count(now));

  now += one_second;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 10, bucket.Count(now));
}


void TokenBucketTest::testTokenBucketTwo() {
  TimeStamp now;
  TimeInterval ten_ms(10000);
  TimeInterval one_hundred_ms(100000);
  TimeInterval one_second(1000000);
  TimeInterval five_minutes(5 * 60 * 1000000);
  ola::Clock::CurrentTime(&now);
  TokenBucket bucket(0, 40, 40, now);  // one every 25ms
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, bucket.Count(now));

  now += one_hundred_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4, bucket.Count(now));
  now += ten_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4, bucket.Count(now));
  now += ten_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 4, bucket.Count(now));
  now += ten_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 5, bucket.Count(now));
  now += ten_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 5, bucket.Count(now));
  now += one_hundred_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 9, bucket.Count(now));
  now += ten_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 10, bucket.Count(now));
  now += one_second;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 40, bucket.Count(now));

  // now try a very long duration
  now += five_minutes;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 40, bucket.Count(now));

  // take 10 tokens from the bucket
  for (unsigned int i = 0; i < 10; i++) {
    CPPUNIT_ASSERT(bucket.GetToken(now));
  }
  CPPUNIT_ASSERT_EQUAL((unsigned int) 30, bucket.Count(now));

  // add a bit of time
  now += ten_ms;
  CPPUNIT_ASSERT_EQUAL((unsigned int) 30, bucket.Count(now));
}
