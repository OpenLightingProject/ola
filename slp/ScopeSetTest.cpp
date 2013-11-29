/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ScopeSetTest.cpp
 * Test fixture for the ScopeSet class.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <set>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "slp/ScopeSet.h"

using ola::slp::ScopeSet;
using std::set;
using std::string;
using std::vector;


class ScopeSetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ScopeSetTest);
  CPPUNIT_TEST(testConstruction);
  CPPUNIT_TEST(testIntersection);
  CPPUNIT_TEST(testDifference);
  CPPUNIT_TEST(testUpdate);
  CPPUNIT_TEST(testSuperSet);
  CPPUNIT_TEST(testStringEscaping);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testConstruction();
    void testIntersection();
    void testDifference();
    void testUpdate();
    void testSuperSet();
    void testStringEscaping();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION(ScopeSetTest);


/**
 * Check the various constructors
 */
void ScopeSetTest::testConstruction() {
  // empty constructor
  ScopeSet empty_scope_set;
  OLA_ASSERT_TRUE(empty_scope_set.empty());
  OLA_ASSERT_EQ(0u, empty_scope_set.size());

  set<string> set_of_scopes, set_of_scopes2;
  set_of_scopes.insert("one");
  set_of_scopes.insert("two");

  ScopeSet scopes1(set_of_scopes);
  OLA_ASSERT_FALSE(scopes1.empty());
  OLA_ASSERT_EQ(2u, scopes1.size());
  OLA_ASSERT_EQ(string("one,two"), scopes1.ToString());
  OLA_ASSERT_TRUE(scopes1.Contains("one"));
  OLA_ASSERT_TRUE(scopes1.Contains("two"));
  OLA_ASSERT_TRUE(scopes1.Contains("ONe"));
  OLA_ASSERT_TRUE(scopes1.Contains("Two"));
  OLA_ASSERT_FALSE(scopes1.Contains("three"));

  // copy constructor
  ScopeSet scopes2 = scopes1;
  OLA_ASSERT_FALSE(scopes2.empty());
  OLA_ASSERT_EQ(2u, scopes2.size());
  OLA_ASSERT_EQ(string("one,two"), scopes2.ToString());
  OLA_ASSERT_EQ(scopes1, scopes2);

  // test canonicalization of the scope strings
  set_of_scopes2.insert(" OnE ");
  set_of_scopes2.insert(" tWo");
  ScopeSet scopes3(set_of_scopes2);
  OLA_ASSERT_FALSE(scopes3.empty());
  OLA_ASSERT_EQ(2u, scopes3.size());
  OLA_ASSERT_EQ(string("one,two"), scopes3.ToString());
  OLA_ASSERT_EQ(scopes1, scopes3);

  // test a set with duplicate entries
  set_of_scopes2.insert("one");
  set_of_scopes2.insert("two");
  OLA_ASSERT_EQ((size_t) 4, set_of_scopes2.size());

  ScopeSet scopes4(set_of_scopes2);
  OLA_ASSERT_FALSE(scopes4.empty());
  OLA_ASSERT_EQ(2u, scopes4.size());
  OLA_ASSERT_EQ(string("one,two"), scopes4.ToString());
  OLA_ASSERT_EQ(scopes1, scopes4);

  // test the string constructors
  ScopeSet scopes5("one,two");
  OLA_ASSERT_FALSE(scopes4.empty());
  OLA_ASSERT_EQ(2u, scopes4.size());
  OLA_ASSERT_EQ(string("one,two"), scopes4.ToString());
  OLA_ASSERT_EQ(scopes1, scopes4);

  // test string parsing
  ScopeSet scopes6("one,TwO,OnE,two");
  OLA_ASSERT_EQ(scopes1, scopes6);

  ScopeSet scopes7("one,,TwO,OnE,two,");
  OLA_ASSERT_EQ(scopes1, scopes7);

  ScopeSet scopes8("one");
  OLA_ASSERT_FALSE(scopes8.empty());
  OLA_ASSERT_EQ(1u, scopes8.size());
  OLA_ASSERT_EQ(string("one"), scopes8.ToString());
  OLA_ASSERT_NE(scopes1, scopes8);
}

/**
 * Test that the intersection methods works
 */
void ScopeSetTest::testIntersection() {
  ScopeSet scopes1("one,two");
  ScopeSet scopes2("two,three");
  ScopeSet scopes3("three,four");
  // test Intersects
  OLA_ASSERT_TRUE(scopes1.Intersects(scopes2));
  OLA_ASSERT_TRUE(scopes2.Intersects(scopes3));
  OLA_ASSERT_FALSE(scopes1.Intersects(scopes3));

  // test Intersection
  ScopeSet intersection = scopes1.Intersection(scopes2);
  OLA_ASSERT_EQ(1u, intersection.size());
  OLA_ASSERT_EQ(string("two"), intersection.ToString());

  intersection = scopes2.Intersection(scopes3);
  OLA_ASSERT_EQ(1u, intersection.size());
  OLA_ASSERT_EQ(string("three"), intersection.ToString());

  intersection = scopes1.Intersection(scopes3);
  OLA_ASSERT_TRUE(intersection.empty());

  // test IntersectionCount
  OLA_ASSERT_EQ(1u, scopes1.IntersectionCount(scopes2));
  OLA_ASSERT_EQ(1u, scopes2.IntersectionCount(scopes3));
  OLA_ASSERT_EQ(0u, scopes1.IntersectionCount(scopes3));
}


/**
 * test the difference methods
 */
void ScopeSetTest::testDifference() {
  ScopeSet scopes1("one,two,three");
  ScopeSet scopes2("two,three");
  ScopeSet scopes3("one,four");
  ScopeSet scopes4("four,five,six");

  ScopeSet difference = scopes1.Difference(scopes2);
  OLA_ASSERT_EQ(1u, difference.size());
  OLA_ASSERT_EQ(string("one"), difference.ToString());

  difference = scopes2.Difference(scopes1);
  OLA_ASSERT_TRUE(difference.empty());
  OLA_ASSERT_EQ(0u, difference.size());
  OLA_ASSERT_EQ(string(""), difference.ToString());

  // difference update
  ScopeSet removed = scopes4.DifferenceUpdate(scopes3);
  OLA_ASSERT_FALSE(removed.empty());
  OLA_ASSERT_EQ(1u, removed.size());
  OLA_ASSERT_EQ(string("four"), removed.ToString());

  OLA_ASSERT_FALSE(scopes4.empty());
  OLA_ASSERT_EQ(2u, scopes4.size());
  OLA_ASSERT_EQ(string("five,six"), scopes4.ToString());
}


/**
 * test the update method
 */
void ScopeSetTest::testUpdate() {
  ScopeSet scopes1("one,two,three");
  ScopeSet scopes2("two,three");
  ScopeSet scopes3("one,four");
  ScopeSet scopes4("four,five,six");

  scopes1.Update(scopes2);
  OLA_ASSERT_EQ(3u, scopes1.size());
  // alphabetical sorting
  OLA_ASSERT_EQ(string("one,three,two"), scopes1.ToString());

  scopes1.Update(scopes3);
  OLA_ASSERT_EQ(4u, scopes1.size());
  OLA_ASSERT_EQ(string("four,one,three,two"), scopes1.ToString());
}


/**
 * test the IsSuperSet method
 */
void ScopeSetTest::testSuperSet() {
  ScopeSet scopes("one,two,three");
  ScopeSet subset1("two,three");
  ScopeSet subset2("one,two");
  ScopeSet subset3("one");
  ScopeSet non_subset1("four,five,six");
  ScopeSet non_subset2("four");
  ScopeSet empty_set;

  OLA_ASSERT_TRUE(scopes.IsSuperSet(scopes));
  OLA_ASSERT_TRUE(scopes.IsSuperSet(subset1));
  OLA_ASSERT_TRUE(scopes.IsSuperSet(subset2));
  OLA_ASSERT_TRUE(scopes.IsSuperSet(subset3));
  OLA_ASSERT_FALSE(scopes.IsSuperSet(non_subset1));
  OLA_ASSERT_FALSE(scopes.IsSuperSet(non_subset2));
  OLA_ASSERT_TRUE(scopes.IsSuperSet(empty_set));
}

/**
 * Test that SLP escaping works
 */
void ScopeSetTest::testStringEscaping() {
  ScopeSet scopes1("one\\2c,two");
  OLA_ASSERT_FALSE(scopes1.empty());
  OLA_ASSERT_EQ(2u, scopes1.size());
  OLA_ASSERT_TRUE(scopes1.Contains("one,"));
  OLA_ASSERT_TRUE(scopes1.Contains("two"));
  OLA_ASSERT_EQ(string("one\\2c,two"), scopes1.AsEscapedString());

  ScopeSet scopes2("one\\5c,two");
  OLA_ASSERT_FALSE(scopes2.empty());
  OLA_ASSERT_EQ(2u, scopes2.size());
  OLA_ASSERT_TRUE(scopes2.Contains("one\\"));
  OLA_ASSERT_TRUE(scopes2.Contains("two"));
  OLA_ASSERT_EQ(string("one\\5c,two"), scopes2.AsEscapedString());
}
