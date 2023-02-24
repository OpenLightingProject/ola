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
 * RDMHelperTest.cpp
 * Test fixture for the RDM Helper code
 * Copyright (C) 2014 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RDMHelper.h"
#include "ola/testing/TestUtils.h"

using std::string;
using ola::rdm::StatusMessageIdToString;

class RDMHelperTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RDMHelperTest);
  CPPUNIT_TEST(testStatusMessageIdToString);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testStatusMessageIdToString();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RDMHelperTest);


/*
 * Test the StatusMessageIdToString function.
 */
void RDMHelperTest::testStatusMessageIdToString() {
  OLA_ASSERT_EQ(string("Slot 25 failed calibration"),
                StatusMessageIdToString(ola::rdm::STS_CAL_FAIL, 25, 0));

  OLA_ASSERT_EQ(string("Proxy Drop: PID 0xabcd at TN 1"),
                StatusMessageIdToString(ola::rdm::STS_PROXY_BROADCAST_DROPPED,
                                        0xABCD, 1));
  OLA_ASSERT_EQ(string("Proxy Drop: PID 0xff00 at TN 255"),
                StatusMessageIdToString(ola::rdm::STS_PROXY_BROADCAST_DROPPED,
                                        0xFF00, 255));
}
