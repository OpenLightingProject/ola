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
 * PointerTrackerTest.cpp
 * Unittest for the PointerTracker.
 * Copyright (C) 2014 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>
#include <vector>

#include "ola/testing/TestUtils.h"
#include "ola/web/JsonPointer.h"
#include "common/web/PointerTracker.h"

using ola::web::JsonPointer;
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
  JsonPointer pointer;
  PointerTracker tracker(&pointer);

  // Basic tests first
  // {}
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.CloseObject();

  // []
  tracker.OpenArray();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.CloseArray();
  OLA_ASSERT_EQ(string(""), pointer.ToString());

  // [ {}, {} ]
  tracker.OpenArray();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string("/0"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string("/0"), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string("/1"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string("/1"), pointer.ToString());
  tracker.CloseArray();
  OLA_ASSERT_EQ(string(""), pointer.ToString());

  // {"foo": {}}
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.SetProperty("foo");
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.CloseObject();

  // {"foo": {"bar": {} } }
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.SetProperty("foo");
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.SetProperty("bar");
  OLA_ASSERT_EQ(string("/foo/bar"), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string("/foo/bar"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string("/foo/bar"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());


  /*
   * The call sequence is based on the following JSON data:
   *
   * {
   *   "foo": [ 0, 1, { "bar": null}, true],
   *   "baz": { "bat" : null },
   *   "cat": [[0, 1], [], false],
   *  }
   */

  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.SetProperty("foo");
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.OpenArray();
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/foo/0"), pointer.ToString());
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/foo/1"), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string("/foo/2"), pointer.ToString());
  tracker.SetProperty("bar");
  OLA_ASSERT_EQ(string("/foo/2/bar"), pointer.ToString());
  // No effect, but makes the implementation in the JsonHandler simpler.
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/foo/2/bar"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string("/foo/2"), pointer.ToString());
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/foo/3"), pointer.ToString());
  tracker.CloseArray();
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.SetProperty("baz");
  OLA_ASSERT_EQ(string("/baz"), pointer.ToString());
  tracker.OpenObject();
  OLA_ASSERT_EQ(string("/baz"), pointer.ToString());
  tracker.SetProperty("bat");
  OLA_ASSERT_EQ(string("/baz/bat"), pointer.ToString());
  // No effect, but makes the implementation in the JsonHandler simpler.
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/baz/bat"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string("/baz"), pointer.ToString());
  tracker.SetProperty("cat");
  OLA_ASSERT_EQ(string("/cat"), pointer.ToString());
  tracker.OpenArray();
  OLA_ASSERT_EQ(string("/cat"), pointer.ToString());
  tracker.OpenArray();
  OLA_ASSERT_EQ(string("/cat/0"), pointer.ToString());
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/cat/0/0"), pointer.ToString());
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/cat/0/1"), pointer.ToString());
  tracker.CloseArray();
  OLA_ASSERT_EQ(string("/cat/0"), pointer.ToString());
  tracker.OpenArray();
  OLA_ASSERT_EQ(string("/cat/1"), pointer.ToString());
  tracker.CloseArray();
  OLA_ASSERT_EQ(string("/cat/1"), pointer.ToString());
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/cat/2"), pointer.ToString());
  tracker.CloseArray();
  OLA_ASSERT_EQ(string("/cat"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
}

void PointerTrackerTest::testErrorConditions() {
  JsonPointer pointer;
  PointerTracker tracker(&pointer);

  // Close without Opens
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.CloseArray();
  OLA_ASSERT_EQ(string(""), pointer.ToString());

  // Mismatched open / close types.
  tracker.OpenObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.SetProperty("foo");
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.CloseArray();
  OLA_ASSERT_EQ(string("/foo"), pointer.ToString());
  tracker.CloseObject();
  OLA_ASSERT_EQ(string(""), pointer.ToString());

  // SetProperty while in an array
  tracker.OpenArray();
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.SetProperty("foo");
  OLA_ASSERT_EQ(string(""), pointer.ToString());
  tracker.IncrementIndex();
  OLA_ASSERT_EQ(string("/0"), pointer.ToString());
}

void PointerTrackerTest::testEscaping() {
  JsonPointer pointer;
  PointerTracker tracker(&pointer);

  tracker.OpenObject();
  // Examples from RFC 6901
  tracker.SetProperty("");
  OLA_ASSERT_EQ(string("/"), pointer.ToString());
  tracker.SetProperty("a/b");
  OLA_ASSERT_EQ(string("/a~1b"), pointer.ToString());
  tracker.SetProperty("c%d");
  OLA_ASSERT_EQ(string("/c%d"), pointer.ToString());
  tracker.SetProperty("e^f");
  OLA_ASSERT_EQ(string("/e^f"), pointer.ToString());
  tracker.SetProperty("g|h");
  OLA_ASSERT_EQ(string("/g|h"), pointer.ToString());
  tracker.SetProperty("i\\\\j");
  OLA_ASSERT_EQ(string("/i\\\\j"), pointer.ToString());
  tracker.SetProperty("k\"l");
  OLA_ASSERT_EQ(string("/k\"l"), pointer.ToString());
  tracker.SetProperty(" ");
  OLA_ASSERT_EQ(string("/ "), pointer.ToString());
  tracker.SetProperty("m~n");
  OLA_ASSERT_EQ(string("/m~0n"), pointer.ToString());
}
