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
 * MemoryBlockTest.cpp
 * Test fixture for the IOQueue class.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <iostream>

#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/io/MemoryBlock.h"
#include "ola/testing/TestUtils.h"

using ola::io::MemoryBlock;

class MemoryBlockTest: public CppUnit::TestFixture {
 public:
  CPPUNIT_TEST_SUITE(MemoryBlockTest);
  CPPUNIT_TEST(testAppend);
  CPPUNIT_TEST(testPrepend);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testAppend();
  void testPrepend();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MemoryBlockTest);


/*
 * Check that basic appending works.
 */
void MemoryBlockTest::testAppend() {
  unsigned int size = 100;
  uint8_t *data = new uint8_t[size];
  MemoryBlock block(data, size);

  OLA_ASSERT_EQ(0u, block.Size());
  OLA_ASSERT_EQ(size, block.Capacity());
  OLA_ASSERT_EQ(size, block.Remaining());
  OLA_ASSERT_TRUE(block.Empty());
  OLA_ASSERT_EQ(data, block.Data());

  // append 4 bytes
  const uint8_t data1[] = {1, 2, 3, 4};
  block.Append(data1, arraysize(data1));
  OLA_ASSERT_EQ(static_cast<unsigned int>(arraysize(data1)), block.Size());
  OLA_ASSERT_EQ(size - static_cast<unsigned int>(arraysize(data1)),
                block.Remaining());
  OLA_ASSERT_FALSE(block.Empty());
  OLA_ASSERT_DATA_EQUALS(data1, arraysize(data1), block.Data(), block.Size());

  // pop 1 byte
  OLA_ASSERT_EQ(1u, block.PopFront(1));
  OLA_ASSERT_EQ(3u, block.Size());
  // popping doesn't change the location of the data in the memory block.
  OLA_ASSERT_EQ(96u, block.Remaining());
  OLA_ASSERT_FALSE(block.Empty());
  OLA_ASSERT_DATA_EQUALS(data1 + 1, arraysize(data1) - 1, block.Data(),
                         block.Size());

  // try to pop more data than exists
  OLA_ASSERT_EQ(3u, block.PopFront(5));
  OLA_ASSERT_EQ(0u, block.Size());
  // now that all data is removed, the block should reset
  OLA_ASSERT_EQ(100u, block.Remaining());
  OLA_ASSERT_TRUE(block.Empty());
}

/*
 * Check that basic prepending works.
 */
void MemoryBlockTest::testPrepend() {
  unsigned int size = 100;
  uint8_t *data = new uint8_t[size];
  MemoryBlock block(data, size);

  // by default the insertion point is at the begining
  const uint8_t data1[] = {1, 2, 3, 4};
  OLA_ASSERT_EQ(0u, block.Prepend(data1, arraysize(data1)));

  // seek
  block.SeekBack();
  OLA_ASSERT_EQ(4u, block.Prepend(data1, arraysize(data1)));

  OLA_ASSERT_EQ(4u, block.Size());
  OLA_ASSERT_EQ(0u, block.Remaining());
  OLA_ASSERT_FALSE(block.Empty());
  OLA_ASSERT_DATA_EQUALS(data1, arraysize(data1), block.Data(), block.Size());

  // pop
  OLA_ASSERT_EQ(1u, block.PopFront(1));
  OLA_ASSERT_EQ(3u, block.Size());
  // popping doesn't change the location of the data in the memory block.
  OLA_ASSERT_EQ(0u, block.Remaining());
  OLA_ASSERT_FALSE(block.Empty());
  OLA_ASSERT_DATA_EQUALS(data1 + 1, arraysize(data1) - 1, block.Data(),
                         block.Size());

  // try to pop more data than exists
  OLA_ASSERT_EQ(3u, block.PopFront(5));
  OLA_ASSERT_EQ(0u, block.Size());
  // now that all data is removed, the block should reset
  OLA_ASSERT_EQ(100u, block.Remaining());
}
