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
 * OutputStream.cpp
 * Test fixture for the OutputStream class.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <iostream>
#include <string>

#include "ola/Logging.h"
#include "ola/io/BigEndianStream.h"
#include "ola/io/IOQueue.h"
#include "ola/network/NetworkUtils.h"
#include "ola/testing/TestUtils.h"

using ola::io::IOQueue;
using ola::io::BigEndianOutputStream;
using ola::network::HostToNetwork;
using ola::testing::ASSERT_DATA_EQUALS;
using std::auto_ptr;
using std::string;


class OutputStreamTest: public CppUnit::TestFixture {
  public:
    CPPUNIT_TEST_SUITE(OutputStreamTest);
    CPPUNIT_TEST(testBasicWrite);
    CPPUNIT_TEST(testWritePrimatives);
    CPPUNIT_TEST_SUITE_END();

  public:
    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }
    void testBasicWrite();
    void testWritePrimatives();

  private:
    IOQueue m_buffer;

    unsigned int SumLengthOfIOVec(const struct iovec *iov, int iocnt);
};


CPPUNIT_TEST_SUITE_REGISTRATION(OutputStreamTest);


/*
 * Check that basic appending works.
 */
void OutputStreamTest::testBasicWrite() {
  BigEndianOutputStream stream(&m_buffer);
  OLA_ASSERT_EQ(0u, m_buffer.Size());
  uint8_t data1[] = {0, 1, 2, 3, 4};

  stream.Write(data1, sizeof(data1));
  OLA_ASSERT_EQ(5u, m_buffer.Size());

  m_buffer.Pop(1);
  OLA_ASSERT_EQ(4u, m_buffer.Size());

  m_buffer.Pop(4);
  OLA_ASSERT_EQ(0u, m_buffer.Size());
}


/*
 * Check that the << operators work
 */
void OutputStreamTest::testWritePrimatives() {
  BigEndianOutputStream stream(&m_buffer);
  OLA_ASSERT_EQ(0u, m_buffer.Size());

  stream << 4;
  OLA_ASSERT_EQ(4u, m_buffer.Size());

  stream << (1u << 31);
  OLA_ASSERT_EQ(8u, m_buffer.Size());

  stream << static_cast<uint8_t>(10) << static_cast<uint16_t>(2400);
  OLA_ASSERT_EQ(11u, m_buffer.Size());

  // confirm this matches what we expect
  const unsigned int DATA_SIZE = 20;
  uint8_t *output_data = new uint8_t[DATA_SIZE];

  uint8_t data1[] = {0, 0, 0, 4, 0x80, 0, 0, 0, 0xa, 0x9, 0x60};
  unsigned int output_size = m_buffer.Peek(output_data, m_buffer.Size());
  ASSERT_DATA_EQUALS(__LINE__, data1, sizeof(data1), output_data, output_size);
  delete[] output_data;
}
