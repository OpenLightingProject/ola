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
 * HMACTest.cpp
 * Unittest for HMAC Crypto functions.
 * Copyright (C) 2026 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/crypto/HMAC.h"
#include "ola/testing/TestUtils.h"

using ola::crypto::HMACSHA256;

class HMACTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(HMACTest);
  CPPUNIT_TEST(testHMACSHA256);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testHMACSHA256();
};


CPPUNIT_TEST_SUITE_REGISTRATION(HMACTest);

/*
 * Test the HMACSHA256 function
 */
void HMACTest::testHMACSHA256() {
  // RFC 4231 - Test Case 1
  // uint8_t hmac[EVP_MAX_MD_SIZE];
  uint8_t hmac[100];
  unsigned int len = 0;

  const uint8_t key[] = {
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b
  };

  const uint8_t data[] = { 'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e' };

  uint8_t expected_hmac[32] = {
    0xB0, 0x34, 0x4C, 0x61, 0xD8, 0xDB, 0x38, 0x53,
    0x5C, 0xA8, 0xAF, 0xCE, 0xAF, 0x0B, 0xF1, 0x2B,
    0x88, 0x1D, 0xC2, 0x00, 0xC9, 0x83, 0x3D, 0xA7,
    0x26, 0xE9, 0x37, 0x6C, 0x2E, 0x32, 0xCF, 0xF7
  };

  OLA_ASSERT_TRUE(HMACSHA256(key, sizeof(key), data, sizeof(data), hmac, &len));
  OLA_ASSERT_DATA_EQUALS(expected_hmac, sizeof(expected_hmac),
                         hmac, len);
}
