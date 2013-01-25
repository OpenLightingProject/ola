/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * XIDAllocatorTest.cpp
 * Test fixture for the XIDAllocator classes
 * Copyright (C) 2012 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include "ola/testing/TestUtils.h"
#include "tools/slp/SLPPacketConstants.h"
#include "tools/slp/XIDAllocator.h"

using ola::slp::XIDAllocator;
using ola::slp::xid_t;


class XIDAllocatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(XIDAllocatorTest);
  CPPUNIT_TEST(testXIDAllocator);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testXIDAllocator();
};


CPPUNIT_TEST_SUITE_REGISTRATION(XIDAllocatorTest);


/*
 * Check XID allocation.
 */
void XIDAllocatorTest::testXIDAllocator() {
  XIDAllocator allocator1;
  OLA_ASSERT_EQ((xid_t) 0, allocator1.Next());
  OLA_ASSERT_EQ((xid_t) 1, allocator1.Next());
  OLA_ASSERT_EQ((xid_t) 2, allocator1.Next());

  // test wrapping
  XIDAllocator allocator2(65534);
  OLA_ASSERT_EQ((xid_t) 65534, allocator2.Next());
  OLA_ASSERT_EQ((xid_t) 65535, allocator2.Next());
  OLA_ASSERT_EQ((xid_t) 0, allocator2.Next());
}
