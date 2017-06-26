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
 * OlaClientWrapperTest.cpp
 * Basic test fixture for the OlaClientWrapper class
 * Copyright (C) 2016 Peter Newman
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <memory>

#include "ola/client/ClientWrapper.h"
#include "ola/testing/TestUtils.h"

using ola::client::OlaClientWrapper;

class OlaClientWrapperTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(OlaClientWrapperTest);
  CPPUNIT_TEST(testNoAutoStartNoOlad);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testNoAutoStartNoOlad();
};


CPPUNIT_TEST_SUITE_REGISTRATION(OlaClientWrapperTest);


/*
 * Check that the client works correctly with no auto start and without olad
 * running.
 */
void OlaClientWrapperTest::testNoAutoStartNoOlad() {
  // Setup the client
  OlaClientWrapper ola_client(false);

  // Setup the client, this tried to connect to the server
  OLA_ASSERT_FALSE_MSG(ola_client.Setup(),
                       "Check for another instance of olad running");
  // Try it again to make sure it still fails
  OLA_ASSERT_FALSE_MSG(ola_client.Setup(),
                       "Check for another instance of olad running");

  // This should be a NOOP
  OLA_ASSERT_TRUE(ola_client.Cleanup());

  // Try for a third time to start
  OLA_ASSERT_FALSE_MSG(ola_client.Setup(),
                       "Check for another instance of olad running");
}
