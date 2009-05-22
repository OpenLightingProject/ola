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
 * InterfacePickerTest.cpp
 * Test fixture for the InterfacePicker class
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>

#include <lla/network/InterfacePicker.h>
#include <lla/Logging.h>

using namespace lla::network;
using std::vector;

class InterfacePickerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(InterfacePickerTest);
  CPPUNIT_TEST(testPickInterfaces);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testPickInterfaces();
};


CPPUNIT_TEST_SUITE_REGISTRATION(InterfacePickerTest);


/*
 * Check that we find at least one candidate interface.
 */
void InterfacePickerTest::testPickInterfaces() {
  lla::InitLogging(lla::LLA_LOG_INFO, lla::LLA_LOG_STDERR);
  InterfacePicker picker;
  vector<Interface> interfaces = picker.GetInterfaces();
  CPPUNIT_ASSERT(interfaces.size() > 0);
}
