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

#include <arpa/inet.h>
#include <cppunit/extensions/HelperMacros.h>
#include <iomanip>
#include <iostream>
#include <vector>

#include "ola/network/InterfacePicker.h"
#include "ola/network/NetworkUtils.h"
#include "ola/Logging.h"

using ola::network::InterfacePicker;
using ola::network::Interface;
using ola::network::StringToAddress;
using std::vector;
using std::cout;
using std::endl;

class InterfacePickerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(InterfacePickerTest);
  CPPUNIT_TEST(testGetInterfaces);
  CPPUNIT_TEST(testChooseInterface);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testGetInterfaces();
    void testChooseInterface();
};

class MockPicker: public InterfacePicker {
  public:
    explicit MockPicker(const vector<Interface> &interfaces)
        : InterfacePicker(),
          m_interfaces(interfaces) {}
    std::vector<Interface> GetInterfaces() const { return m_interfaces; }
  private:
    const vector<Interface> &m_interfaces;
};

CPPUNIT_TEST_SUITE_REGISTRATION(InterfacePickerTest);


/*
 * Check that we find at least one candidate interface.
 */
void InterfacePickerTest::testGetInterfaces() {
  InterfacePicker *picker = InterfacePicker::NewPicker();
  vector<Interface> interfaces = picker->GetInterfaces();
  CPPUNIT_ASSERT(interfaces.size() > 0);

  vector<Interface>::iterator iter;
  cout << endl;
  for (iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
    cout << iter->name << endl;
    cout << " ip: " << inet_ntoa(iter->ip_address) << endl;
    cout << " bcast: " << inet_ntoa(iter->bcast_address) << endl;
    cout << " subnet: " << inet_ntoa(iter->subnet_address) << endl;
    cout << " hw_addr: ";
    for (unsigned int i = 0; i < ola::network::MAC_LENGTH; i++) {
      if (i)
        cout << ':';
      cout << std::setw(2) << std::setfill('0') << std::hex <<
        0 + (uint8_t) iter->hw_address[i];
    }
    cout << endl;
    cout << "---------------" << endl;
  }
  delete picker;
}


void InterfacePickerTest::testChooseInterface() {
  vector<Interface> interfaces;
  MockPicker picker(interfaces);

  // no interfaces
  Interface interface;
  CPPUNIT_ASSERT(!picker.ChooseInterface(&interface, ""));

  // now with one interface that doesn't match
  Interface iface1;
  iface1.name = "eth0";
  StringToAddress("10.0.0.1", iface1.ip_address);
  interfaces.push_back(iface1);
  CPPUNIT_ASSERT(picker.ChooseInterface(&interface, "192.168.1.1"));
  CPPUNIT_ASSERT(iface1 == interface);

  // check that preferred works
  Interface iface2;
  iface2.name = "eth1";
  StringToAddress("192.168.1.1", iface2.ip_address);
  interfaces.push_back(iface2);
  CPPUNIT_ASSERT(picker.ChooseInterface(&interface, "192.168.1.1"));
  CPPUNIT_ASSERT(iface2 == interface);

  // now check for iface name
  CPPUNIT_ASSERT(picker.ChooseInterface(&interface, "eth0"));
  CPPUNIT_ASSERT(iface1 == interface);

  CPPUNIT_ASSERT(picker.ChooseInterface(&interface, "eth1"));
  CPPUNIT_ASSERT(iface2 == interface);

  // a invalid address should return the first one
  CPPUNIT_ASSERT(picker.ChooseInterface(&interface, "foo"));
  CPPUNIT_ASSERT(iface1 == interface);
}
