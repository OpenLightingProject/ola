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
 * UIDAllocatorTest.cpp
 * Test fixture for the UIDAllocator
 * Copyright (C) 2013 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/rdm/UID.h"
#include "ola/rdm/UIDAllocator.h"
#include "ola/testing/TestUtils.h"


using ola::rdm::UID;
using ola::rdm::UIDAllocator;
using std::auto_ptr;

class UIDAllocatorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UIDAllocatorTest);
  CPPUNIT_TEST(testAllocator);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testAllocator();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UIDAllocatorTest);


/*
 * Test the Allocator works.
 */
void UIDAllocatorTest::testAllocator() {
  UID uid(1, 0xffffff00);
  UIDAllocator allocator(uid);

  for (unsigned int i = 0xffffff00; i < 0xffffffff; i++) {
    auto_ptr<UID> uid(allocator.AllocateNext());
    OLA_ASSERT_NOT_NULL(uid.get());
    OLA_ASSERT_EQ(i, uid->DeviceId());
  }

  OLA_ASSERT_NULL(allocator.AllocateNext());
  OLA_ASSERT_NULL(allocator.AllocateNext());
  OLA_ASSERT_NULL(allocator.AllocateNext());

  // try another allocator that has a custom upper bound
  UIDAllocator bounded_allocator(uid, 0xffffff10);
  for (unsigned int i = 0xffffff00; i <= 0xffffff10; i++) {
    auto_ptr<UID> uid(bounded_allocator.AllocateNext());
    OLA_ASSERT_NOT_NULL(uid.get());
    OLA_ASSERT_EQ(i, uid->DeviceId());
  }

  OLA_ASSERT_NULL(bounded_allocator.AllocateNext());
  OLA_ASSERT_NULL(bounded_allocator.AllocateNext());

  // confirm we never hand out the broadcast id
  UID uid2(1, 0xfffffff0);
  UIDAllocator bounded_allocator2(uid2, 0xffffffff);
  for (unsigned int i = 0xfffffff0; i < 0xffffffff; i++) {
    auto_ptr<UID> uid(bounded_allocator2.AllocateNext());
    OLA_ASSERT_NOT_NULL(uid.get());
    OLA_ASSERT_EQ(i, uid->DeviceId());
  }
  OLA_ASSERT_NULL(bounded_allocator2.AllocateNext());
}
