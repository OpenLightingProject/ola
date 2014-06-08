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
 * PidDataTest.cpp
 * Ensure we can load the pid data.
 * Copyright (C) 2013 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/Logging.h"
#include "ola/rdm/PidStore.h"
#include "ola/testing/TestUtils.h"

using ola::rdm::PidStore;
using ola::rdm::RootPidStore;

class PidDataTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PidDataTest);
  CPPUNIT_TEST(testDataLoad);
  CPPUNIT_TEST_SUITE_END();

 public:
    void setUp();
    void testDataLoad();
};

CPPUNIT_TEST_SUITE_REGISTRATION(PidDataTest);

void PidDataTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
}

/*
 * Check we can load the data.
 */
void PidDataTest::testDataLoad() {
  std::auto_ptr<const RootPidStore> store(
      RootPidStore::LoadFromDirectory(DATADIR));
  OLA_ASSERT_NOT_NULL(store.get());

  const PidStore *esta_store = store->EstaStore();
  OLA_ASSERT_NOT_NULL(esta_store);
  OLA_ASSERT_NE(0, esta_store->PidCount());

  const PidStore *manufacturer_store = store->ManufacturerStore(0x00a1);
  OLA_ASSERT_NOT_NULL(manufacturer_store);
  OLA_ASSERT_NE(0, manufacturer_store->PidCount());
}
