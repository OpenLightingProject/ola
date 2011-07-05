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
 * PidStoreTest.cpp
 * Test fixture for the PidStore & Pid classes
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <sstream>
#include <string>
#include <vector>

#include "ola/messaging/Descriptor.h"
#include "ola/rdm/PidStore.h"


using ola::messaging::Descriptor;
using ola::rdm::PidDescriptor;
using std::string;
using std::vector;


class PidStoreTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(PidStoreTest);
  CPPUNIT_TEST(testPidDescriptor);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testPidDescriptor();
    void setUp() {}
    void tearDown() {}

  private:
};


CPPUNIT_TEST_SUITE_REGISTRATION(PidStoreTest);


/*
 * Test that the PidDescriptor works.
 */
void PidStoreTest::testPidDescriptor() {
  // just use empty fields for now
  vector<const class ola::messaging::FieldDescriptor*> fields;
  const Descriptor get_request_descriptor("GET Request", fields);
  const Descriptor get_response_descriptor("GET Response", fields);
  const Descriptor set_request_descriptor("SET Request", fields);
  const Descriptor set_response_descriptor("SET Response", fields);

  PidDescriptor pid("foo",
                    10,
                    &get_request_descriptor,
                    &get_response_descriptor,
                    &set_request_descriptor,
                    &set_response_descriptor,
                    PidDescriptor::NON_BROADCAST_SUB_DEVICE,
                    PidDescriptor::ANY_SUB_DEVICE);

  // basic checks
  CPPUNIT_ASSERT_EQUAL(string("foo"), pid.Name());
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(10), pid.Value());
  CPPUNIT_ASSERT_EQUAL(&get_request_descriptor, pid.GetRequest());
  CPPUNIT_ASSERT_EQUAL(&get_response_descriptor, pid.GetResponse());
  CPPUNIT_ASSERT_EQUAL(&set_request_descriptor, pid.SetRequest());
  CPPUNIT_ASSERT_EQUAL(&set_response_descriptor, pid.SetResponse());

  // check sub device constraints
  CPPUNIT_ASSERT(pid.IsGetValid(0));
  CPPUNIT_ASSERT(pid.IsGetValid(1));
  CPPUNIT_ASSERT(pid.IsGetValid(2));
  CPPUNIT_ASSERT(pid.IsGetValid(511));
  CPPUNIT_ASSERT(pid.IsGetValid(512));
  CPPUNIT_ASSERT(!pid.IsGetValid(513));
  CPPUNIT_ASSERT(!pid.IsGetValid(0xffff));
  CPPUNIT_ASSERT(pid.IsSetValid(0));
  CPPUNIT_ASSERT(pid.IsSetValid(1));
  CPPUNIT_ASSERT(pid.IsSetValid(2));
  CPPUNIT_ASSERT(pid.IsSetValid(511));
  CPPUNIT_ASSERT(pid.IsSetValid(512));
  CPPUNIT_ASSERT(!pid.IsSetValid(513));
  CPPUNIT_ASSERT(pid.IsSetValid(0xffff));
}
