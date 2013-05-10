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
 * TestUtils.cpp
 * Functions used for unit testing.
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <iostream>
#include "ola/testing/TestUtils.h"
#include "ola/Logging.h"

namespace ola {
namespace testing {

/*
 * Assert that two blocks of data match.
 * @param line the line number of this assert
 * @param expected pointer to the expected data
 * @param expected_length the length of the expected data
 * @param actual point to the actual data
 * @param actual_length length of the actual data
 */
void ASSERT_DATA_EQUALS(unsigned int line,
                        const uint8_t *expected,
                        unsigned int expected_length,
                        const uint8_t *actual,
                        unsigned int actual_length) {
  std::stringstream str;
  str << "Line " << line;
  const string message = str.str();
  CPPUNIT_ASSERT_EQUAL_MESSAGE(message, expected_length, actual_length);

  bool data_matches = 0 == memcmp(expected, actual, expected_length);
  if (!data_matches) {
    for (unsigned int i = 0; i < expected_length; ++i) {
      str.str("");
      str << std::dec << i << ": 0x" << std::hex
          << static_cast<int>(expected[i]);
      str << ((expected[i] == actual[i]) ? " == " : "  != ");
      str << "0x" << static_cast<int>(actual[i]) << " (";
      str << ((expected[i] >= '!' && expected[i] <= '~') ? (char) expected[i] :
              ' ');
      str << ((expected[i] == actual[i]) ? " == " : "  != ");
      str << ((actual[i] >= '!' && actual[i] <= '~') ? (char) actual[i] : ' ');
      str << ")";

      if (expected[i] != actual[i])
        str << "  ## MISMATCH";
      OLA_INFO << str.str();
    }
  }
  CPPUNIT_ASSERT_MESSAGE(message, data_matches);
}
}  // namespace testing
}  // namespace ola
