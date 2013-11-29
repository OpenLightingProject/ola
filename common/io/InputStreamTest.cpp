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
 * StreamTest.cpp
 * Test fixture for the InputStream classes.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/Logging.h"
#include "ola/io/MemoryBuffer.h"
#include "ola/io/BigEndianStream.h"
#include "ola/testing/TestUtils.h"

using ola::io::BigEndianInputStream;
using ola::io::MemoryBuffer;


class InputStreamTest: public CppUnit::TestFixture {
 public:
    CPPUNIT_TEST_SUITE(InputStreamTest);
    CPPUNIT_TEST(testRead);
    CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void tearDown() {}
    void testRead();
};


CPPUNIT_TEST_SUITE_REGISTRATION(InputStreamTest);


void InputStreamTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
}


/*
 * Confirm that reading works.
 */
void InputStreamTest::testRead() {
  uint8_t data[] = {
    0x80,
    0x81,
    0x80, 0x00,
    0x83, 0x01,
    0x87, 0x65, 0x43, 0x21,
    0x12, 0x34, 0x56, 0x78,
  };

  MemoryBuffer buffer(data, sizeof(data));
  BigEndianInputStream stream(&buffer);

  int8_t int8;
  OLA_ASSERT_TRUE(stream >> int8);
  OLA_ASSERT_EQ(static_cast<int8_t>(-128), int8);

  uint8_t uint8;
  OLA_ASSERT_TRUE(stream >> uint8);
  OLA_ASSERT_EQ(static_cast<uint8_t>(129), uint8);

  int16_t int16;
  OLA_ASSERT_TRUE(stream >> int16);
  OLA_ASSERT_EQ(static_cast<int16_t>(-32768), int16);

  uint16_t uint16;
  OLA_ASSERT_TRUE(stream >> uint16);
  OLA_ASSERT_EQ(static_cast<uint16_t>(33537), uint16);

  int32_t int32;
  OLA_ASSERT_TRUE(stream >> int32);
  OLA_ASSERT_EQ(static_cast<int32_t>(-2023406815), int32);

  uint32_t uint32;
  OLA_ASSERT_TRUE(stream >> uint32);
  OLA_ASSERT_EQ(static_cast<uint32_t>(305419896), uint32);

  OLA_ASSERT_FALSE(stream >> uint16);
}
