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
 * ResponderTagSetTest.cpp
 * Test fixture for the TagSet class
 * Copyright (C) 2025 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/rdm/ResponderTagSet.h"
#include "ola/testing/TestUtils.h"


using std::string;
using ola::rdm::TagSet;

class TagSetTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TagSetTest);
  CPPUNIT_TEST(testTagSet);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testTagSet();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TagSetTest);


/*
 * Test the TagSet
 */
void TagSetTest::testTagSet() {
  TagSet set1;
  OLA_ASSERT_EQ(0u, set1.Size());

  // now test the packing
  unsigned int buffer_size1 = 10;
  unsigned int packed_size1 = buffer_size1;
  uint8_t *buffer1 = new uint8_t[buffer_size1];
  memset(buffer1, '#', buffer_size1);  // Set the buffer to non-null data
  OLA_ASSERT_TRUE(set1.Pack(buffer1, &packed_size1));

  uint8_t expected1[] = {};
  OLA_ASSERT_DATA_EQUALS(expected1, sizeof(expected1), buffer1, packed_size1);

  string tag("foo");
  string tag2("bar");
  set1.AddTag(tag);
  OLA_ASSERT_EQ(1u, set1.Size());
  OLA_ASSERT_EQ(string("foo"), set1.ToString());
  OLA_ASSERT_TRUE(set1.Contains(tag));
  OLA_ASSERT_FALSE(set1.Contains(tag2));
  set1.AddTag(tag);
  OLA_ASSERT_TRUE(set1.Contains(tag));
  OLA_ASSERT_EQ(1u, set1.Size());

  // now test the packing
  unsigned int buffer_size2 = 32 + 1;
  unsigned int packed_size2 = buffer_size2;
  uint8_t *buffer2 = new uint8_t[buffer_size2];
  memset(buffer2, '#', buffer_size2);  // Set the buffer to non-null data
  // Technically exactly the right size, but the function requires
  // (32 + 1) * set size
  unsigned int packed_size2_undersized = 4;
  OLA_ASSERT_FALSE(set1.Pack(buffer2, &packed_size2_undersized));
  OLA_ASSERT_TRUE(set1.Pack(buffer2, &packed_size2));

  uint8_t expected2[] = {'f', 'o', 'o', 0};
  OLA_ASSERT_DATA_EQUALS(expected2, sizeof(expected2), buffer2, packed_size2);

  set1.AddTag(tag2);
  OLA_ASSERT_EQ(2u, set1.Size());
  OLA_ASSERT_EQ(string("bar,foo"), set1.ToString());
  OLA_ASSERT_TRUE(set1.Contains(tag));
  OLA_ASSERT_TRUE(set1.Contains(tag2));

  // now test the packing
  unsigned int buffer_size3 = 100;
  unsigned int packed_size3 = buffer_size3;
  uint8_t *buffer3 = new uint8_t[buffer_size3];
  memset(buffer3, '#', buffer_size3);  // Set the buffer to non-null data
  OLA_ASSERT_TRUE(set1.Pack(buffer3, &packed_size3));

  uint8_t expected3[] = {'b', 'a', 'r', 0, 'f', 'o', 'o', 0};
  OLA_ASSERT_DATA_EQUALS(expected3, sizeof(expected3), buffer3, packed_size3);

  TagSet set2(set1);
  OLA_ASSERT_EQ(set1, set2);
  TagSet set3;
  OLA_ASSERT_EQ(0u, set3.Size());
  set3 = set2;
  OLA_ASSERT_EQ(set1, set2);

  set3.RemoveTag(tag2);
  OLA_ASSERT_EQ(1u, set3.Size());
  OLA_ASSERT_EQ(string("foo"), set3.ToString());
}
