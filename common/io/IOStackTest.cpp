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
 * IOStackTest.cpp
 * Test fixture for the IOStack class.
 * Copyright (C) 2013 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <iostream>
#include <string>

#include "ola/Logging.h"
#include "ola/io/IOStack.h"
#include "ola/io/IOQueue.h"
#include "ola/io/MemoryBlockPool.h"
#include "ola/testing/TestUtils.h"

using ola::io::IOStack;
using ola::io::IOQueue;
using ola::io::IOVec;
using ola::io::MemoryBlockPool;
using std::string;


class IOStackTest: public CppUnit::TestFixture {
 public:
    CPPUNIT_TEST_SUITE(IOStackTest);
    CPPUNIT_TEST(testBasicWrite);
    CPPUNIT_TEST(testBlockOverflow);
    CPPUNIT_TEST(testIOVec);
    CPPUNIT_TEST(testAppendToQueue);
    CPPUNIT_TEST(testBlockReuse);
    CPPUNIT_TEST_SUITE_END();

 public:
    void testBasicWrite();
    void testBlockOverflow();
    void testIOVec();
    void testAppendToQueue();
    void testBlockReuse();

 private:
    unsigned int SumLengthOfIOVec(const struct IOVec *iov, int iocnt);
};


CPPUNIT_TEST_SUITE_REGISTRATION(IOStackTest);


/**
 * Sum up the length of data in a IOVec
 */
unsigned int IOStackTest::SumLengthOfIOVec(const struct IOVec *iov,
                                            int iocnt) {
  unsigned int sum = 0;
  for (int i = 0; i < iocnt; iov++, i++)
    sum += iov->iov_len;
  return sum;
}


/*
 * Check that basic prepending works.
 */
void IOStackTest::testBasicWrite() {
  IOStack stack;
  OLA_ASSERT_EQ(0u, stack.Size());
  OLA_ASSERT_TRUE(stack.Empty());
  uint8_t data1[] = {0, 1};
  uint8_t data2[] = {2};
  uint8_t data3[] = {3, 4};

  stack.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(2u, stack.Size());
  stack.Write(data2, sizeof(data2));
  OLA_ASSERT_EQ(3u, stack.Size());
  stack.Write(data3, sizeof(data3));
  OLA_ASSERT_EQ(5u, stack.Size());

  std::ostringstream str;
  stack.Dump(&str);
  OLA_ASSERT_EQ(
      string("03 04 02 00 01           .....\n"),
      str.str());

  unsigned int data_size = stack.Size();
  uint8_t output[data_size];
  OLA_ASSERT_EQ(data_size, stack.Read(output, data_size));

  const uint8_t expected_data[] = {3, 4, 2, 0, 1};
  OLA_ASSERT_DATA_EQUALS(expected_data, sizeof(expected_data), output,
                         data_size);
}


/*
 * Check that overflowing blocks works
 */
void IOStackTest::testBlockOverflow() {
  // block size of 4
  MemoryBlockPool pool(4);
  IOStack stack(&pool);
  uint8_t data1[] = {0, 1, 2, 3, 4};
  uint8_t data2[] = {5, 6, 7, 8, 9};
  uint8_t data3[] = {0xa, 0xb, 0xc, 0xd, 0xe};

  stack.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(5u, stack.Size());

  stack.Write(data2, sizeof(data2));
  OLA_ASSERT_EQ(10u, stack.Size());

  stack.Write(data3, sizeof(data3));
  OLA_ASSERT_EQ(15u, stack.Size());

  unsigned int data_size = stack.Size();
  uint8_t output[data_size];
  OLA_ASSERT_EQ(data_size, stack.Read(output, data_size));

  const uint8_t expected_data[] = {0xa, 0xb, 0xc, 0xd, 0xe, 5, 6, 7, 8, 9,
                                   0, 1, 2, 3, 4};
  OLA_ASSERT_DATA_EQUALS(expected_data, sizeof(expected_data), output,
                         data_size);
}


/**
 * Test getting / setting IOVec work.
 */
void IOStackTest::testIOVec() {
  MemoryBlockPool pool(4);
  IOStack stack(&pool);
  uint8_t data1[] = {5, 6, 7, 8};
  uint8_t data2[] = {0, 1, 2, 3, 4};

  stack.Write(data1, sizeof(data1));
  stack.Write(data2, sizeof(data2));
  OLA_ASSERT_EQ(9u, stack.Size());
  OLA_ASSERT_FALSE(stack.Empty());

  int iocnt;
  const struct IOVec *vector = stack.AsIOVec(&iocnt);
  OLA_ASSERT_EQ(9u, SumLengthOfIOVec(vector, iocnt));
  OLA_ASSERT_EQ(3, iocnt);
  stack.FreeIOVec(vector);
}


/**
 * Test appending IOStacks to an IOQueue works.
 */
void IOStackTest::testAppendToQueue() {
  MemoryBlockPool pool(4);
  IOStack stack(&pool);

  uint8_t data1[] = {2, 1, 0};
  uint8_t data2[] = {6, 5, 4, 3};
  stack.Write(data1, sizeof(data1));
  stack.Write(data2, sizeof(data2));
  OLA_ASSERT_EQ(7u, stack.Size());

  IOQueue queue(&pool);
  stack.MoveToIOQueue(&queue);
  OLA_ASSERT_TRUE(stack.Empty());
  OLA_ASSERT_EQ(7u, queue.Size());

  uint8_t expected_data[] = {6, 5, 4, 3, 2, 1, 0};
  uint8_t tmp_data[100];
  unsigned int queue_size = queue.Peek(tmp_data, sizeof(tmp_data));
  OLA_ASSERT_EQ(7u, queue_size);
  OLA_ASSERT_DATA_EQUALS(tmp_data, queue_size, expected_data,
                         sizeof(expected_data));

  // now add a second stack
  uint8_t data3[] = {0xb, 0xa};
  uint8_t data4[] = {0xf, 0xe, 0xd, 0xc};
  stack.Write(data3, sizeof(data3));
  stack.Write(data4, sizeof(data4));
  OLA_ASSERT_EQ(6u, stack.Size());

  stack.MoveToIOQueue(&queue);
  OLA_ASSERT_TRUE(stack.Empty());
  OLA_ASSERT_EQ(13u, queue.Size());

  uint8_t expected_data2[] = {6, 5, 4, 3, 2, 1, 0, 0xf, 0xe, 0xd, 0xc, 0xb,
                              0xa};
  queue_size = queue.Peek(tmp_data, sizeof(tmp_data));
  OLA_ASSERT_EQ(13u, queue_size);
  OLA_ASSERT_DATA_EQUALS(tmp_data, queue_size, expected_data2,
                         sizeof(expected_data2));

  OLA_ASSERT_EQ(4u, pool.BlocksAllocated());
}

/**
 * Confirm we reuse blocks
 */
void IOStackTest::testBlockReuse() {
  MemoryBlockPool pool(4);

  {
    IOStack stack(&pool);
    uint8_t data1[] = {6, 5, 3, 3, 2, 1, 0};
    stack.Write(data1, sizeof(data1));
    OLA_ASSERT_EQ(7u, stack.Size());
  }

  {
    IOStack stack(&pool);
    uint8_t data1[] = {0xf, 0xe, 0xd, 0xc, 0xb, 0xa};
    stack.Write(data1, sizeof(data1));
    OLA_ASSERT_EQ(6u, stack.Size());
  }

  OLA_ASSERT_EQ(2u, pool.BlocksAllocated());
  pool.Purge();
  OLA_ASSERT_EQ(0u, pool.BlocksAllocated());
}
