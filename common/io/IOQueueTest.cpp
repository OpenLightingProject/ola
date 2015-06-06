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
 * IOQueueTest.cpp
 * Test fixture for the IOQueue class.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <iostream>
#include <string>

#include "ola/Logging.h"
#include "ola/io/IOQueue.h"
#include "ola/io/MemoryBlockPool.h"
#include "ola/testing/TestUtils.h"

using ola::io::IOQueue;
using ola::io::IOVec;
using ola::io::MemoryBlockPool;
using std::auto_ptr;
using std::string;


class IOQueueTest: public CppUnit::TestFixture {
 public:
    CPPUNIT_TEST_SUITE(IOQueueTest);
    CPPUNIT_TEST(testBasicWrite);
    CPPUNIT_TEST(testBlockOverflow);
    CPPUNIT_TEST(testPop);
    CPPUNIT_TEST(testPeek);
    CPPUNIT_TEST(testIOVec);
    CPPUNIT_TEST(testDump);
    CPPUNIT_TEST(testStringRead);
    CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void tearDown() {}
    void testBasicWrite();
    void testBlockOverflow();
    void testPop();
    void testPeek();
    void testIOVec();
    void testDump();
    void testStringRead();

 private:
    auto_ptr<IOQueue> m_buffer;

    unsigned int SumLengthOfIOVec(const struct IOVec *iov, int iocnt);
};


CPPUNIT_TEST_SUITE_REGISTRATION(IOQueueTest);


void IOQueueTest::setUp() {
  m_buffer.reset(new IOQueue());
}


/**
 * Sum up the length of data in a IOVec
 */
unsigned int IOQueueTest::SumLengthOfIOVec(const struct IOVec *iov,
                                            int iocnt) {
  unsigned int sum = 0;
  for (int i = 0; i < iocnt; iov++, i++)
    sum += iov->iov_len;
  return sum;
}


/*
 * Check that basic appending works.
 */
void IOQueueTest::testBasicWrite() {
  OLA_ASSERT_EQ(0u, m_buffer->Size());
  uint8_t data1[] = {0, 1, 2, 3, 4};

  m_buffer->Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(5u, m_buffer->Size());

  m_buffer->Pop(1);
  OLA_ASSERT_EQ(4u, m_buffer->Size());

  m_buffer->Pop(4);
  OLA_ASSERT_EQ(0u, m_buffer->Size());
}


/*
 * Check that overflowing blocks works
 */
void IOQueueTest::testBlockOverflow() {
  // block size of 4
  MemoryBlockPool pool(4);
  IOQueue queue(&pool);
  uint8_t data1[] = {0, 1, 2, 3, 4};
  uint8_t data2[] = {5, 6, 7, 8, 9};
  uint8_t data3[] = {0xa, 0xb, 0xc, 0xd, 0xe};

  queue.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(5u, queue.Size());

  queue.Write(data2, sizeof(data2));
  OLA_ASSERT_EQ(10u, queue.Size());

  queue.Write(data3, sizeof(data3));
  OLA_ASSERT_EQ(15u, queue.Size());

  queue.Pop(9);
  OLA_ASSERT_EQ(6u, queue.Size());

  // append some more data
  queue.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(11u, queue.Size());
  queue.Write(data2, sizeof(data2));
  OLA_ASSERT_EQ(16u, queue.Size());
}


/**
 * Test that Pop behaves
 */
void IOQueueTest::testPop() {
  uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

  m_buffer->Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, m_buffer->Size());
  OLA_ASSERT_FALSE(m_buffer->Empty());

  m_buffer->Pop(9);
  OLA_ASSERT_EQ(0u, m_buffer->Size());
  OLA_ASSERT_TRUE(m_buffer->Empty());

  // try to pop off more data
  m_buffer->Pop(1);
  OLA_ASSERT_EQ(0u, m_buffer->Size());
  OLA_ASSERT_TRUE(m_buffer->Empty());

  // add the data back, then try to pop off more than we have
  m_buffer->Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, m_buffer->Size());
  OLA_ASSERT_FALSE(m_buffer->Empty());

  m_buffer->Pop(10);
  OLA_ASSERT_EQ(0u, m_buffer->Size());
  OLA_ASSERT_TRUE(m_buffer->Empty());

  // one more time
  m_buffer->Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, m_buffer->Size());

  // Now try a buffer with smaller blocks
  MemoryBlockPool pool(4);
  IOQueue queue(&pool);
  queue.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, queue.Size());

  // pop the same amount as the first block size
  queue.Pop(4);
  OLA_ASSERT_EQ(5u, queue.Size());
  OLA_ASSERT_FALSE(queue.Empty());

  // now pop more than the buffer size
  queue.Pop(6);
  OLA_ASSERT_EQ(0u, queue.Size());
  OLA_ASSERT_TRUE(queue.Empty());

  // test the block boundry
  uint8_t *output_data = new uint8_t[4];
  m_buffer.reset(new IOQueue(&pool));
  queue.Write(data1, 4);
  OLA_ASSERT_EQ(4u, queue.Size());
  unsigned int output_size = queue.Peek(output_data, 4);
  OLA_ASSERT_DATA_EQUALS(data1, 4, output_data, output_size);
  queue.Pop(4);
  OLA_ASSERT_TRUE(queue.Empty());

  // now add some more data
  queue.Write(data1 + 4, 4);
  OLA_ASSERT_EQ(4u, queue.Size());
  output_size = queue.Peek(output_data, 4);
  OLA_ASSERT_DATA_EQUALS(data1 + 4, 4, output_data, output_size);
  queue.Pop(4);
  OLA_ASSERT_TRUE(queue.Empty());

  delete[] output_data;
}


/**
 * Test that Peek behaves
 */
void IOQueueTest::testPeek() {
  uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

  m_buffer->Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, m_buffer->Size());
  OLA_ASSERT_FALSE(m_buffer->Empty());

  const unsigned int DATA_SIZE = 20;
  uint8_t *output_data = new uint8_t[DATA_SIZE];

  // peek at the first four bytes
  unsigned int output_size = m_buffer->Peek(output_data, 4);
  OLA_ASSERT_DATA_EQUALS(data1, 4, output_data, output_size);
  OLA_ASSERT_EQ(9u, m_buffer->Size());

  // peek at the first 9 bytes
  output_size = m_buffer->Peek(output_data, 9);
  OLA_ASSERT_DATA_EQUALS(data1, 9, output_data, output_size);
  OLA_ASSERT_EQ(9u, m_buffer->Size());

  // peek at more bytes that exist in the buffer
  output_size = m_buffer->Peek(output_data, DATA_SIZE);
  OLA_ASSERT_EQ(9u, output_size);
  OLA_ASSERT_DATA_EQUALS(data1, sizeof(data1), output_data, output_size);
  OLA_ASSERT_EQ(9u, m_buffer->Size());

  // Now try a buffer with smaller blocks
  MemoryBlockPool pool(4);
  IOQueue queue(&pool);
  queue.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, queue.Size());

  // peek at he same amount as the first block size
  output_size = queue.Peek(output_data, 4);
  OLA_ASSERT_DATA_EQUALS(data1, 4, output_data, output_size);
  OLA_ASSERT_EQ(9u, queue.Size());
  OLA_ASSERT_FALSE(queue.Empty());

  // peek at data from more than one block
  output_size = queue.Peek(output_data, 6);
  OLA_ASSERT_DATA_EQUALS(data1, 6, output_data, output_size);
  OLA_ASSERT_EQ(9u, queue.Size());
  OLA_ASSERT_FALSE(queue.Empty());

  // peek at data on the two block boundry
  output_size = queue.Peek(output_data, 8);
  OLA_ASSERT_DATA_EQUALS(data1, 8, output_data, output_size);
  OLA_ASSERT_EQ(9u, queue.Size());
  OLA_ASSERT_FALSE(queue.Empty());

  // peek at all the data
  output_size = queue.Peek(output_data, 9);
  OLA_ASSERT_DATA_EQUALS(data1, 9, output_data, output_size);
  OLA_ASSERT_EQ(9u, queue.Size());
  OLA_ASSERT_FALSE(queue.Empty());

  // peek at more data than what exists
  output_size = queue.Peek(output_data, DATA_SIZE);
  OLA_ASSERT_EQ(9u, output_size);
  OLA_ASSERT_DATA_EQUALS(data1, 9, output_data, output_size);
  OLA_ASSERT_EQ(9u, queue.Size());
  OLA_ASSERT_FALSE(queue.Empty());

  delete[] output_data;
}


/**
 * Test getting / setting IOVec work.
 */
void IOQueueTest::testIOVec() {
  uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

  m_buffer->Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, m_buffer->Size());
  OLA_ASSERT_FALSE(m_buffer->Empty());

  int iocnt;
  const struct IOVec *vector = m_buffer->AsIOVec(&iocnt);
  OLA_ASSERT_EQ(9u, SumLengthOfIOVec(vector, iocnt));
  OLA_ASSERT_EQ(1, iocnt);
  m_buffer->FreeIOVec(vector);

  // try a smaller block size
  MemoryBlockPool pool(4);
  IOQueue queue(&pool);
  m_buffer.reset(new IOQueue(&pool));
  queue.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, queue.Size());

  vector = queue.AsIOVec(&iocnt);
  OLA_ASSERT_EQ(3, iocnt);
  OLA_ASSERT_EQ(9u, SumLengthOfIOVec(vector, iocnt));
  queue.FreeIOVec(vector);
}


/**
 * Test dumping to a ostream works
 */
void IOQueueTest::testDump() {
  MemoryBlockPool pool(4);
  IOQueue queue(&pool);
  uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

  queue.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, queue.Size());

  std::ostringstream str;
  queue.Dump(&str);
  OLA_ASSERT_EQ(
      string("00 01 02 03 04 05 06 07  ........\n"
             "08                       .\n"),
      str.str());
}


/**
 * Test reading to a string works.
 */
void IOQueueTest::testStringRead() {
  MemoryBlockPool pool(4);
  IOQueue queue(&pool);
  uint8_t data1[] = {'a', 'b', 'c', 'd', '1', '2', '3', '4', ' '};

  queue.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(9u, queue.Size());

  string output;
  OLA_ASSERT_EQ(9u, queue.Read(&output, 9u));
  OLA_ASSERT_EQ(string("abcd1234 "), output);
}
