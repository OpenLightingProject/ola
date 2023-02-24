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
 * RDMFrameTest.cpp
 * Test fixture for the RDMFrame.
 * Copyright (C) 2015 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/base/Array.h"
#include "ola/io/ByteString.h"
#include "ola/rdm/RDMFrame.h"
#include "ola/testing/TestUtils.h"

using ola::io::ByteString;
using ola::rdm::RDMFrame;

class RDMFrameTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMFrameTest);
  CPPUNIT_TEST(testRDMFrame);
  CPPUNIT_TEST(testPrependStartCode);
  CPPUNIT_TEST_SUITE_END();

 public:
  void testRDMFrame();
  void testPrependStartCode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMFrameTest);

void RDMFrameTest::testRDMFrame() {
  const uint8_t raw_data[] = {1, 2, 3, 4, 5};

  RDMFrame frame(raw_data, arraysize(raw_data));
  OLA_ASSERT_DATA_EQUALS(raw_data, arraysize(raw_data), frame.data.data(),
                         frame.data.size());

  OLA_ASSERT_EQ(0u, frame.timing.response_time);
  OLA_ASSERT_EQ(0u, frame.timing.break_time);
  OLA_ASSERT_EQ(0u, frame.timing.mark_time);
  OLA_ASSERT_EQ(0u, frame.timing.data_time);

  ByteString input_data(raw_data, arraysize(raw_data));
  RDMFrame frame2(input_data);

  OLA_ASSERT_DATA_EQUALS(input_data.data(), input_data.size(),
                         frame.data.data(), frame.data.size());
  OLA_ASSERT_EQ(0u, frame2.timing.response_time);
  OLA_ASSERT_EQ(0u, frame2.timing.break_time);
  OLA_ASSERT_EQ(0u, frame2.timing.mark_time);
  OLA_ASSERT_EQ(0u, frame2.timing.data_time);
}

void RDMFrameTest::testPrependStartCode() {
  const uint8_t raw_data[] = {1, 2, 3, 4, 5};

  RDMFrame::Options options(true);
  RDMFrame frame(raw_data, arraysize(raw_data), options);

  ByteString expected_data(1, 0xcc);
  expected_data.append(raw_data, arraysize(raw_data));
  OLA_ASSERT_DATA_EQUALS(expected_data.data(), expected_data.size(),
                         frame.data.data(), frame.data.size());

  ByteString input_data(raw_data, arraysize(raw_data));
  RDMFrame frame2(input_data, options);
  OLA_ASSERT_DATA_EQUALS(expected_data.data(), expected_data.size(),
                         frame2.data.data(), frame2.data.size());
}
