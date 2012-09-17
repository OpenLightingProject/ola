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
 * RunLengthDecoderTest.cpp
 * Test fixture for the RunLengthDecoder class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/testing/TestUtils.h"

#include <ola/DmxBuffer.h>
#include "plugins/espnet/RunLengthDecoder.h"

class RunLengthDecoderTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RunLengthDecoderTest);
  CPPUNIT_TEST(testDecode);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testDecode();
  private:
};


CPPUNIT_TEST_SUITE_REGISTRATION(RunLengthDecoderTest);


/*
 * Check that we can decode DMX data
 */
void RunLengthDecoderTest::testDecode() {
  ola::plugin::espnet::RunLengthDecoder decoder;
  uint8_t data[] = {0x78, 0x56, 0x74, 0xFE, 0x5, 0x10, 0x41, 0x78, 0xFD, 0xFE,
                    0x36, 0xFD, 0xFD};
  uint8_t expected_data[] = {0x78, 0x56, 0x74, 0x10, 0x10, 0x10, 0x10, 0x10,
                             0x41, 0x78, 0xFE, 0x36, 0xFD};
  ola::DmxBuffer buffer;
  ola::DmxBuffer expected(expected_data, sizeof(expected_data));

  buffer.Blackout();
  buffer.Reset();
  OLA_ASSERT_EQ((unsigned int) 0, buffer.Size());
  decoder.Decode(&buffer, data, sizeof(data));
  OLA_ASSERT(buffer == expected);
}
