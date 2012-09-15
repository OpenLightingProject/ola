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
 * ScopedSLPStoreTest.cpp
 * Test fixture for the ScopedSLPStore class
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"
#include "tools/slp/ScopedSLPStore.h"
#include "tools/slp/SLPStore.h"

using ola::slp::ScopedSLPStore;
using ola::slp::SLPStore;


class ScopedSLPStoreTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ScopedSLPStoreTest);
  CPPUNIT_TEST(testLookups);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testLookups();

    void setUp() {
      ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
    }

  private:
    ScopedSLPStore m_store;
};


CPPUNIT_TEST_SUITE_REGISTRATION(ScopedSLPStoreTest);

/*
 * Check that we can lookup SLPStores.
 */
void ScopedSLPStoreTest::testLookups() {
  const string default_scope = "default";
  const string upper_default_scope = "DEFAULT";
  const string acn_scope = "ACN";
  const string other_scope = "other";

  OLA_ASSERT_NULL(m_store.Lookup(default_scope));
  OLA_ASSERT_NULL(m_store.Lookup(upper_default_scope));
  OLA_ASSERT_NULL(m_store.Lookup(acn_scope));
  OLA_ASSERT_NULL(m_store.Lookup(other_scope));

  // test LookupOrCreate
  SLPStore *default_store = m_store.LookupOrCreate(default_scope);
  OLA_ASSERT_NOT_NULL(default_store);
  OLA_ASSERT_EQ(default_store, m_store.LookupOrCreate(default_scope));
  OLA_ASSERT_EQ(default_store, m_store.LookupOrCreate(upper_default_scope));
  OLA_ASSERT_NE(default_store, m_store.LookupOrCreate(acn_scope));

  // now test Lookup again
  OLA_ASSERT_EQ(default_store, m_store.Lookup(default_scope));
  OLA_ASSERT_EQ(default_store, m_store.Lookup(upper_default_scope));
  OLA_ASSERT_NE(default_store, m_store.Lookup(acn_scope));
  OLA_ASSERT_NULL(m_store.Lookup(other_scope));
}
