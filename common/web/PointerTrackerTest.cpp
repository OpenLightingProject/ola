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
 * PointerTrackerTest.cpp
 * Unittest for the PointerTracker.
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "common/web/PointerTracker.h"

using ola::web::PointerTracker;
using std::string;

class PointerTrackerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PointerTrackerTest);
  CPPUNIT_TEST(testPointer);
  CPPUNIT_TEST(testErrorConditions);
  CPPUNIT_TEST(testEscaping);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testPointer();
  void testErrorConditions();
  void testEscaping();
};

CPPUNIT_TEST_SUITE_REGISTRATION(PointerTrackerTest);


void PointerTrackerTest::testPointer() {
  /*
   * The call sequence is based on the following JSON data:
   *
   * {
   *   "foo": [ 0, 1, { "bar": null}, true],
   *   "baz": { "bat" : null },
   *   "cat": [[0, 1], [], false],
   *  }
   */
  PointerTracker pointer;

  OLA_ASSERT_EQ(string(""), pointer.GetPointer());
  pointer.OpenObject();
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());
  pointer.SetProperty("foo");
  OLA_ASSERT_EQ(string("/foo"), pointer.GetPointer());
  pointer.OpenArray();
  OLA_ASSERT_EQ(string("/foo"), pointer.GetPointer());
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/foo/0"), pointer.GetPointer());
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/foo/1"), pointer.GetPointer());
  pointer.OpenObject();
  OLA_ASSERT_EQ(string("/foo/2"), pointer.GetPointer());
  pointer.SetProperty("bar");
  OLA_ASSERT_EQ(string("/foo/2/bar"), pointer.GetPointer());
  // No effect, but makes the implementation in the JsonHandler simpler.
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/foo/2/bar"), pointer.GetPointer());
  pointer.CloseObject();
  OLA_ASSERT_EQ(string("/foo/2"), pointer.GetPointer());
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/foo/3"), pointer.GetPointer());
  pointer.CloseArray();
  OLA_ASSERT_EQ(string("/foo"), pointer.GetPointer());
  pointer.SetProperty("baz");
  OLA_ASSERT_EQ(string("/baz"), pointer.GetPointer());
  pointer.OpenObject();
  OLA_ASSERT_EQ(string("/baz"), pointer.GetPointer());
  pointer.SetProperty("bat");
  OLA_ASSERT_EQ(string("/baz/bat"), pointer.GetPointer());
  // No effect, but makes the implementation in the JsonHandler simpler.
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/baz/bat"), pointer.GetPointer());
  pointer.CloseObject();
  OLA_ASSERT_EQ(string("/baz"), pointer.GetPointer());
  pointer.SetProperty("cat");
  OLA_ASSERT_EQ(string("/cat"), pointer.GetPointer());
  pointer.OpenArray();
  OLA_ASSERT_EQ(string("/cat"), pointer.GetPointer());
  pointer.OpenArray();
  OLA_ASSERT_EQ(string("/cat/0"), pointer.GetPointer());
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/cat/0/0"), pointer.GetPointer());
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/cat/0/1"), pointer.GetPointer());
  pointer.CloseArray();
  OLA_ASSERT_EQ(string("/cat/0"), pointer.GetPointer());
  pointer.OpenArray();
  OLA_ASSERT_EQ(string("/cat/1"), pointer.GetPointer());
  pointer.CloseArray();
  OLA_ASSERT_EQ(string("/cat/1"), pointer.GetPointer());
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/cat/2"), pointer.GetPointer());
  pointer.CloseArray();
  OLA_ASSERT_EQ(string("/cat"), pointer.GetPointer());
  pointer.CloseObject();
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());
}

void PointerTrackerTest::testErrorConditions() {
  PointerTracker pointer;

  // Close without Opens
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());
  pointer.CloseObject();
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());
  pointer.CloseArray();
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());

  // Mismatched open / close types.
  pointer.OpenObject();
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());
  pointer.SetProperty("foo");
  OLA_ASSERT_EQ(string("/foo"), pointer.GetPointer());
  pointer.CloseArray();
  OLA_ASSERT_EQ(string("/foo"), pointer.GetPointer());
  pointer.CloseObject();
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());

  // SetProperty while in an array
  pointer.OpenArray();
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());
  pointer.SetProperty("foo");
  OLA_ASSERT_EQ(string(""), pointer.GetPointer());
  pointer.IncrementIndex();
  OLA_ASSERT_EQ(string("/0"), pointer.GetPointer());
}

void PointerTrackerTest::testEscaping() {
  PointerTracker pointer;

  pointer.OpenObject();
  // Examples from RFC 6901
  pointer.SetProperty("");
  OLA_ASSERT_EQ(string("/"), pointer.GetPointer());
  pointer.SetProperty("a/b");
  OLA_ASSERT_EQ(string("/a~1b"), pointer.GetPointer());
  pointer.SetProperty("c%d");
  OLA_ASSERT_EQ(string("/c%d"), pointer.GetPointer());
  pointer.SetProperty("e^f");
  OLA_ASSERT_EQ(string("/e^f"), pointer.GetPointer());
  pointer.SetProperty("g|h");
  OLA_ASSERT_EQ(string("/g|h"), pointer.GetPointer());
  pointer.SetProperty("i\\\\j");
  OLA_ASSERT_EQ(string("/i\\\\j"), pointer.GetPointer());
  pointer.SetProperty("k\"l");
  OLA_ASSERT_EQ(string("/k\"l"), pointer.GetPointer());
  pointer.SetProperty(" ");
  OLA_ASSERT_EQ(string("/ "), pointer.GetPointer());
  pointer.SetProperty("m~n");
  OLA_ASSERT_EQ(string("/m~0n"), pointer.GetPointer());
}
