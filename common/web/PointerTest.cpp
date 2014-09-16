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
 * PointerTest.cpp
 * Unittest for the Json Pointer.
 * Copyright (C) 2014 Simon Newton
 */

#include <string>

#include "ola/web/JsonPointer.h"
#include "ola/testing/TestUtils.h"

using ola::web::JsonPointer;
using std::string;

class JsonPointerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonPointerTest);
  CPPUNIT_TEST(testConstructionFrom);
  CPPUNIT_TEST(testEscaping);
  CPPUNIT_TEST(testIteration);
  CPPUNIT_TEST(testPrefix);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testConstructionFrom();
    void testEscaping();
    void testIteration();
    void testPrefix();
};


CPPUNIT_TEST_SUITE_REGISTRATION(JsonPointerTest);

void JsonPointerTest::testConstructionFrom() {
  JsonPointer pointer1("");
  OLA_ASSERT_TRUE(pointer1.IsValid());
  OLA_ASSERT_EQ(1u, pointer1.TokenCount());
  OLA_ASSERT_EQ(string(""), pointer1.TokenAt(0));
  OLA_ASSERT_EQ(string(""), pointer1.ToString());

  JsonPointer pointer2("/foo");
  OLA_ASSERT_TRUE(pointer2.IsValid());
  OLA_ASSERT_EQ(2u, pointer2.TokenCount());
  OLA_ASSERT_EQ(string("foo"), pointer2.TokenAt(0));
  OLA_ASSERT_EQ(string(""), pointer2.TokenAt(1));
  OLA_ASSERT_EQ(string(""), pointer2.TokenAt(2));
  OLA_ASSERT_EQ(string("/foo"), pointer2.ToString());

  JsonPointer pointer3("/foo/bar/1");
  OLA_ASSERT_TRUE(pointer3.IsValid());
  OLA_ASSERT_EQ(4u, pointer3.TokenCount());
  OLA_ASSERT_EQ(string("foo"), pointer3.TokenAt(0));
  OLA_ASSERT_EQ(string("bar"), pointer3.TokenAt(1));
  OLA_ASSERT_EQ(string("1"), pointer3.TokenAt(2));
  OLA_ASSERT_EQ(string(""), pointer3.TokenAt(3));
  OLA_ASSERT_EQ(string("/foo/bar/1"), pointer3.ToString());

  JsonPointer pointer4("/1");
  OLA_ASSERT_TRUE(pointer4.IsValid());
  OLA_ASSERT_EQ(2u, pointer4.TokenCount());
  OLA_ASSERT_EQ(string("1"), pointer4.TokenAt(0));
  OLA_ASSERT_EQ(string(""), pointer4.TokenAt(1));
  OLA_ASSERT_EQ(string("/1"), pointer4.ToString());

  JsonPointer pointer5("/-1");
  OLA_ASSERT_TRUE(pointer5.IsValid());
  OLA_ASSERT_EQ(2u, pointer5.TokenCount());
  OLA_ASSERT_EQ(string("-1"), pointer5.TokenAt(0));
  OLA_ASSERT_EQ(string(""), pointer5.TokenAt(1));
  OLA_ASSERT_EQ(string("/-1"), pointer5.ToString());

  JsonPointer invalid_pointer("foo");
  OLA_ASSERT_FALSE(invalid_pointer.IsValid());

  JsonPointer empty_pointer;
  OLA_ASSERT_TRUE(empty_pointer.IsValid());
  OLA_ASSERT_EQ(1u, empty_pointer.TokenCount());
  OLA_ASSERT_EQ(string(""), empty_pointer.TokenAt(0));
  OLA_ASSERT_EQ(string(""), empty_pointer.ToString());
}

void JsonPointerTest::testEscaping() {
  JsonPointer pointer1("/a~1b");
  OLA_ASSERT_TRUE(pointer1.IsValid());
  OLA_ASSERT_EQ(2u, pointer1.TokenCount());
  OLA_ASSERT_EQ(string("a/b"), pointer1.TokenAt(0));
  OLA_ASSERT_EQ(string(""), pointer1.TokenAt(1));
  OLA_ASSERT_EQ(string("/a~1b"), pointer1.ToString());

  JsonPointer pointer2("/m~0n");
  OLA_ASSERT_TRUE(pointer2.IsValid());
  OLA_ASSERT_EQ(2u, pointer2.TokenCount());
  OLA_ASSERT_EQ(string("m~n"), pointer2.TokenAt(0));
  OLA_ASSERT_EQ(string(""), pointer2.TokenAt(1));
  OLA_ASSERT_EQ(string("/m~0n"), pointer2.ToString());
}

void JsonPointerTest::testIteration() {
  JsonPointer pointer1("");
  JsonPointer::Iterator iter = pointer1.begin();

  OLA_ASSERT_TRUE(iter.IsValid());
  OLA_ASSERT_TRUE(iter.AtEnd());
  OLA_ASSERT_EQ(string(""), *iter);
  iter++;
  OLA_ASSERT_FALSE(iter.AtEnd());
  OLA_ASSERT_FALSE(iter.IsValid());

  JsonPointer pointer2("/foo");
  iter = pointer2.begin();
  OLA_ASSERT_TRUE(iter.IsValid());
  OLA_ASSERT_FALSE(iter.AtEnd());
  OLA_ASSERT_EQ(string("foo"), *iter);
  iter++;
  OLA_ASSERT_TRUE(iter.IsValid());
  OLA_ASSERT_TRUE(iter.AtEnd());
  OLA_ASSERT_EQ(string(""), *iter);
  iter++;
  OLA_ASSERT_FALSE(iter.IsValid());

  JsonPointer pointer3("/foo/bar/1/-1");
  iter = pointer3.begin();
  OLA_ASSERT_TRUE(iter.IsValid());
  OLA_ASSERT_FALSE(iter.AtEnd());
  OLA_ASSERT_EQ(string("foo"), *iter);
  iter++;
  OLA_ASSERT_TRUE(iter.IsValid());
  OLA_ASSERT_FALSE(iter.AtEnd());
  OLA_ASSERT_EQ(string("bar"), *iter);
  iter++;
  OLA_ASSERT_TRUE(iter.IsValid());
  OLA_ASSERT_FALSE(iter.AtEnd());
  OLA_ASSERT_EQ(string("1"), *iter);
  iter++;
  OLA_ASSERT_TRUE(iter.IsValid());
  OLA_ASSERT_FALSE(iter.AtEnd());
  OLA_ASSERT_EQ(string("-1"), *iter);
  iter++;
  OLA_ASSERT_TRUE(iter.IsValid());
  OLA_ASSERT_TRUE(iter.AtEnd());
  OLA_ASSERT_EQ(string(""), *iter);
  iter++;
  OLA_ASSERT_FALSE(iter.IsValid());
}

void JsonPointerTest::testPrefix() {
  JsonPointer invalid_pointer("foo");
  JsonPointer pointer1("/foo");
  JsonPointer pointer2("/foo/bar");
  JsonPointer pointer3("/baz");
  JsonPointer pointer4("");

  OLA_ASSERT_FALSE(invalid_pointer.IsPrefixOf(invalid_pointer));
  OLA_ASSERT_FALSE(invalid_pointer.IsPrefixOf(pointer1));
  OLA_ASSERT_FALSE(invalid_pointer.IsPrefixOf(pointer2));
  OLA_ASSERT_FALSE(invalid_pointer.IsPrefixOf(pointer3));

  OLA_ASSERT_TRUE(pointer1.IsPrefixOf(pointer2));
  OLA_ASSERT_TRUE(pointer4.IsPrefixOf(pointer1));
  OLA_ASSERT_TRUE(pointer4.IsPrefixOf(pointer3));

  OLA_ASSERT_FALSE(pointer1.IsPrefixOf(pointer1));
  OLA_ASSERT_FALSE(pointer1.IsPrefixOf(pointer3));
  OLA_ASSERT_FALSE(pointer1.IsPrefixOf(pointer4));
  OLA_ASSERT_FALSE(pointer2.IsPrefixOf(pointer1));
  OLA_ASSERT_FALSE(pointer2.IsPrefixOf(pointer2));
  OLA_ASSERT_FALSE(pointer2.IsPrefixOf(pointer3));
  OLA_ASSERT_FALSE(pointer2.IsPrefixOf(pointer4));
  OLA_ASSERT_FALSE(pointer3.IsPrefixOf(pointer4));
}
