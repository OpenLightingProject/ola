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
 * E133StatusHelperTest.cpp
 * Test fixture for the E133 Status Helper code
 * Copyright (C) 2024 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <string>

#include "ola/e133/E133Enums.h"
#include "ola/e133/E133StatusHelper.h"
#include "ola/testing/TestUtils.h"

using std::string;
using ola::e133::IntToStatusCode;
using ola::e133::IntToConnectStatusCode;
using ola::e133::IntToRPTStatusCode;
using ola::e133::RPTStatusCodeToRDMStatusCode;

class E133StatusHelperTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(E133StatusHelperTest);
  CPPUNIT_TEST(testIntToStatusCode);
  CPPUNIT_TEST(testIntToConnectStatusCode);
  CPPUNIT_TEST(testIntToRPTStatusCode);
  CPPUNIT_TEST(testRPTStatusCodeToRDMStatusCode);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testIntToStatusCode();
    void testIntToConnectStatusCode();
    void testIntToRPTStatusCode();
    void testRPTStatusCodeToRDMStatusCode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(E133StatusHelperTest);


/*
 * Test the IntToStatusCode function.
 */
void E133StatusHelperTest::testIntToStatusCode() {
  ola::e133::E133StatusCode value;
  OLA_ASSERT_TRUE(IntToStatusCode(0, &value));
  OLA_ASSERT_EQ(value, ola::e133::SC_E133_ACK);
  OLA_ASSERT_TRUE(IntToStatusCode(1, &value));
  OLA_ASSERT_EQ(value, ola::e133::SC_E133_RDM_TIMEOUT);
  OLA_ASSERT_TRUE(IntToStatusCode(9, &value));
  OLA_ASSERT_EQ(value, ola::e133::SC_E133_BROADCAST_COMPLETE);
  // Update this if additional entries are added to the enum
  OLA_ASSERT_FALSE(IntToStatusCode((ola::e133::SC_E133_BROADCAST_COMPLETE + 1), &value));
  // test null status code
  OLA_ASSERT_FALSE(IntToStatusCode(0, NULL));
}


/*
 * Test the IntToConnectStatusCode function.
 */
void E133StatusHelperTest::testIntToConnectStatusCode() {
  ola::e133::E133ConnectStatusCode value;
  OLA_ASSERT_TRUE(IntToConnectStatusCode(0, &value));
  OLA_ASSERT_EQ(value, ola::e133::CONNECT_OK);
  OLA_ASSERT_TRUE(IntToConnectStatusCode(1, &value));
  OLA_ASSERT_EQ(value, ola::e133::CONNECT_SCOPE_MISMATCH);
  OLA_ASSERT_TRUE(IntToConnectStatusCode(5, &value));
  OLA_ASSERT_EQ(value, ola::e133::CONNECT_INVALID_UID);
  // Update this if additional entries are added to the enum
  OLA_ASSERT_FALSE(IntToConnectStatusCode((ola::e133::CONNECT_INVALID_UID + 1), &value));
  // test null status code
  OLA_ASSERT_FALSE(IntToConnectStatusCode(0, NULL));
}


/*
 * Test the IntToRPTStatusCode function.
 */
void E133StatusHelperTest::testIntToRPTStatusCode() {
  ola::acn::RPTStatusVector value;
  OLA_ASSERT_TRUE(IntToRPTStatusCode(1, &value));
  OLA_ASSERT_EQ(value, ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RPT_UID);
  OLA_ASSERT_TRUE(IntToRPTStatusCode(2, &value));
  OLA_ASSERT_EQ(value, ola::acn::VECTOR_RPT_STATUS_RDM_TIMEOUT);
  OLA_ASSERT_TRUE(IntToRPTStatusCode(9, &value));
  OLA_ASSERT_EQ(value, ola::acn::VECTOR_RPT_STATUS_INVALID_COMMAND_CLASS);
  // Update this if additional entries are added to the enum
  OLA_ASSERT_FALSE(IntToRPTStatusCode((ola::acn::VECTOR_RPT_STATUS_INVALID_COMMAND_CLASS + 1), &value));
  // test null status code
  OLA_ASSERT_FALSE(IntToRPTStatusCode(1, NULL));
}


/*
 * Test the RPTStatusCodeToRDMStatusCode function.
 */
void E133StatusHelperTest::testRPTStatusCodeToRDMStatusCode() {
  ola::rdm::RDMStatusCode value;
  // Update this if earlier entries are added to the conversion
  OLA_ASSERT_TRUE(RPTStatusCodeToRDMStatusCode(ola::acn::VECTOR_RPT_STATUS_RDM_TIMEOUT, &value));
  OLA_ASSERT_EQ(value, ola::rdm::RDM_TIMEOUT);
  OLA_ASSERT_TRUE(RPTStatusCodeToRDMStatusCode(ola::acn::VECTOR_RPT_STATUS_RDM_INVALID_RESPONSE, &value));
  OLA_ASSERT_EQ(value, ola::rdm::RDM_INVALID_RESPONSE);
  OLA_ASSERT_TRUE(RPTStatusCodeToRDMStatusCode(ola::acn::VECTOR_RPT_STATUS_INVALID_COMMAND_CLASS, &value));
  OLA_ASSERT_EQ(value, ola::rdm::RDM_INVALID_COMMAND_CLASS);
  // Because the function accepts an enum as input, the compile traps any out of range source entry
  // Remove this once we provide suitable mapping for all statuses
  OLA_ASSERT_FALSE(RPTStatusCodeToRDMStatusCode(ola::acn::VECTOR_RPT_STATUS_UNKNOWN_RPT_UID, &value));
  // test null status code
  OLA_ASSERT_FALSE(RPTStatusCodeToRDMStatusCode(ola::acn::VECTOR_RPT_STATUS_RDM_TIMEOUT, NULL));
}
