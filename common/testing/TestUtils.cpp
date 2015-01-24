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
 * TestUtils.cpp
 * Functions used for unit testing.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <iostream>
#include <string>
#include "ola/testing/TestUtils.h"
#include "ola/Logging.h"

namespace ola {
namespace testing {

using std::string;

/*
 * Assert that two blocks of data match.
 * @param source_line the file name and line number of this assert
 * @param expected pointer to the expected data
 * @param expected_length the length of the expected data
 * @param actual point to the actual data
 * @param actual_length length of the actual data
 */
void ASSERT_DATA_EQUALS(const SourceLine &source_line,
                        const uint8_t *expected,
                        unsigned int expected_length,
                        const uint8_t *actual,
                        unsigned int actual_length) {
  _AssertEquals(source_line, expected_length, actual_length,
                "Data lengths differ");

  bool data_matches = (0 == memcmp(expected, actual, expected_length));
  if (!data_matches) {
    std::ostringstream str;
    for (unsigned int i = 0; i < expected_length; ++i) {
      str.str("");
      str << std::dec << i << ": 0x" << std::hex
          << static_cast<int>(expected[i]);
      str << ((expected[i] == actual[i]) ? " == " : " != ");
      str << "0x" << static_cast<int>(actual[i]) << " (";
      str << ((expected[i] >= '!' && expected[i] <= '~') ?
              static_cast<char>(expected[i]) : ' ');
      str << ((expected[i] == actual[i]) ? " == " : " != ");
      str << ((actual[i] >= '!' && actual[i] <= '~') ?
              static_cast<char>(actual[i]) : ' ');
      str << ")";

      if (expected[i] != actual[i]) {
        str << "  ## MISMATCH";
      }
      OLA_INFO << str.str();
    }
  }
  CPPUNIT_NS::Asserter::failIf(!data_matches, "Data differs", source_line);
}

void ASSERT_DATA_EQUALS(const SourceLine &source_line,
                        const char *expected,
                        unsigned int expected_length,
                        const char *actual,
                        unsigned int actual_length) {
  ASSERT_DATA_EQUALS(source_line, reinterpret_cast<const uint8_t*>(expected),
                     expected_length,
                     reinterpret_cast<const uint8_t*>(actual),
                     actual_length);
}
}  // namespace testing
}  // namespace ola
