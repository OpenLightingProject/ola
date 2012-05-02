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
#include "ola/testing/TestUtils.h"

using ola::io::IOQueue;
using ola::testing::ASSERT_DATA_EQUALS;
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

  private:
    auto_ptr<IOQueue> m_buffer;

    unsigned int SumLengthOfIOVec(const struct iovec *iov, int iocnt);
};


CPPUNIT_TEST_SUITE_REGISTRATION(IOQueueTest);


void IOQueueTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_buffer.reset(new IOQueue());
}


/**
 * Sum up the length of data in a iovec
 */
unsigned int IOQueueTest::SumLengthOfIOVec(const struct iovec *iov,
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
  CPPUNIT_ASSERT_EQUAL(0u, m_buffer->Size());
  uint8_t data1[] = {0, 1, 2, 3, 4};

  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(5u, m_buffer->Size());

  m_buffer->Pop(1);
  CPPUNIT_ASSERT_EQUAL(4u, m_buffer->Size());

  m_buffer->Pop(4);
  CPPUNIT_ASSERT_EQUAL(0u, m_buffer->Size());
}


/*
 * Check that overflowing blocks works
 */
void IOQueueTest::testBlockOverflow() {
  // block size of 4
  m_buffer.reset(new IOQueue(4));
  uint8_t data1[] = {0, 1, 2, 3, 4};
  uint8_t data2[] = {5, 6, 7, 8, 9};
  uint8_t data3[] = {0xa, 0xb, 0xc, 0xd, 0xe};

  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(5u, m_buffer->Size());

  m_buffer->Write(data2, sizeof(data2));
  CPPUNIT_ASSERT_EQUAL(10u, m_buffer->Size());

  m_buffer->Write(data3, sizeof(data3));
  CPPUNIT_ASSERT_EQUAL(15u, m_buffer->Size());

  OLA_INFO << "pop";
  m_buffer->Pop(9);
  CPPUNIT_ASSERT_EQUAL(6u, m_buffer->Size());

  // append some more data
  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(11u, m_buffer->Size());
  m_buffer->Write(data2, sizeof(data2));
  CPPUNIT_ASSERT_EQUAL(16u, m_buffer->Size());

  // test what happens when we peek too much
}


/**
 * Test that Pop behaves
 */
void IOQueueTest::testPop() {
  uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  m_buffer->Pop(9);
  CPPUNIT_ASSERT_EQUAL(0u, m_buffer->Size());
  CPPUNIT_ASSERT(m_buffer->Empty());

  // try to pop off more data
  m_buffer->Pop(1);
  CPPUNIT_ASSERT_EQUAL(0u, m_buffer->Size());
  CPPUNIT_ASSERT(m_buffer->Empty());

  // add the data back, then try to pop off more than we have
  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  m_buffer->Pop(10);
  CPPUNIT_ASSERT_EQUAL(0u, m_buffer->Size());
  CPPUNIT_ASSERT(m_buffer->Empty());

  // one more time
  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());

  // Now try a buffer with smaller blocks
  OLA_INFO << "new";
  m_buffer.reset(new IOQueue(4));
  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());

  // pop the same amount as the first block size
  OLA_INFO << "pop";
  m_buffer->Pop(4);
  CPPUNIT_ASSERT_EQUAL(5u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  // now pop more than the buffer size
  m_buffer->Pop(6);
  CPPUNIT_ASSERT_EQUAL(0u, m_buffer->Size());
  CPPUNIT_ASSERT(m_buffer->Empty());
}


/**
 * Test that Peek behaves
 */
void IOQueueTest::testPeek() {
  uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  const unsigned int DATA_SIZE = 20;
  uint8_t *output_data = new uint8_t[DATA_SIZE];

  // peek at the first four bytes
  unsigned int output_size = m_buffer->Peek(output_data, 4);
  ASSERT_DATA_EQUALS(__LINE__, data1, 4, output_data, output_size);
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());

  // peek at the first 9 bytes
  output_size = m_buffer->Peek(output_data, 9);
  ASSERT_DATA_EQUALS(__LINE__, data1, 9, output_data, output_size);
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());

  // peek at more bytes that exist in the buffer
  output_size = m_buffer->Peek(output_data, DATA_SIZE);
  CPPUNIT_ASSERT_EQUAL(9u, output_size);
  ASSERT_DATA_EQUALS(__LINE__, data1, sizeof(data1), output_data, output_size);
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());

  // Now try a buffer with smaller blocks
  m_buffer.reset(new IOQueue(4));
  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());

  // peek at he same amount as the first block size
  output_size = m_buffer->Peek(output_data, 4);
  ASSERT_DATA_EQUALS(__LINE__, data1, 4, output_data, output_size);
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  // peek at data from more than one block
  output_size = m_buffer->Peek(output_data, 6);
  ASSERT_DATA_EQUALS(__LINE__, data1, 6, output_data, output_size);
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  // peek at data on the two block boundry
  output_size = m_buffer->Peek(output_data, 8);
  ASSERT_DATA_EQUALS(__LINE__, data1, 8, output_data, output_size);
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  // peek at all the data
  output_size = m_buffer->Peek(output_data, 9);
  ASSERT_DATA_EQUALS(__LINE__, data1, 9, output_data, output_size);
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  // peek at more data than what exists
  output_size = m_buffer->Peek(output_data, DATA_SIZE);
  CPPUNIT_ASSERT_EQUAL(9u, output_size);
  ASSERT_DATA_EQUALS(__LINE__, data1, 9, output_data, output_size);
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  delete[] output_data;
}


/**
 * Test getting / setting iovecs work.
 */
void IOQueueTest::testIOVec() {
  uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());
  CPPUNIT_ASSERT(!m_buffer->Empty());

  int iocnt;
  const struct iovec *vector = m_buffer->AsIOVec(&iocnt);
  CPPUNIT_ASSERT_EQUAL(9u, SumLengthOfIOVec(vector, iocnt));
  CPPUNIT_ASSERT_EQUAL(1, iocnt);
  m_buffer->FreeIOVec(vector);

  // try a smaller block size
  m_buffer.reset(new IOQueue(4));
  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());

  OLA_INFO << "as iovec";
  vector = m_buffer->AsIOVec(&iocnt);
  CPPUNIT_ASSERT_EQUAL(3, iocnt);
  CPPUNIT_ASSERT_EQUAL(9u, SumLengthOfIOVec(vector, iocnt));

  // test append
  IOQueue target_buffer;
  target_buffer.AppendIOVec(vector, iocnt);
  CPPUNIT_ASSERT_EQUAL(9u, target_buffer.Size());

  // test both buffers have the same data
  uint8_t *output1 = new uint8_t[10];
  uint8_t *output2 = new uint8_t[10];
  unsigned int output1_size = m_buffer->Peek(output1, 10);
  unsigned int output2_size = target_buffer.Peek(output2, 10);

  ASSERT_DATA_EQUALS(__LINE__, output1, output1_size, output2, output2_size);
  delete[] output1;
  delete[] output2;

  m_buffer->FreeIOVec(vector);
}


/**
 * Test dumping to a ostream works
 */
void IOQueueTest::testDump() {
  m_buffer.reset(new IOQueue(4));
  uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

  m_buffer->Write(data1, sizeof(data1));
  CPPUNIT_ASSERT_EQUAL(9u, m_buffer->Size());

  std::stringstream str;
  m_buffer->Dump(&str);
  CPPUNIT_ASSERT_EQUAL(
      string("00 01 02 03 04 05 06 07  ........\n"
             "08                       .\n"),
      str.str());
}
