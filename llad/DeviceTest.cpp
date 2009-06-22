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
 * DeviceTest.cpp
 * Test fixture for the Device and DeviceManager classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include <lla/DmxBuffer.h>
#include <llad/Device.h>
#include <llad/Port.h>
#include <llad/Preferences.h>
#include "DeviceManager.h"
#include "UniverseStore.h"

using namespace lla;
using namespace std;


class DeviceTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DeviceTest);
  CPPUNIT_TEST(testDevice);
  CPPUNIT_TEST(testDeviceManager);
  CPPUNIT_TEST(testRestorePatchings);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testDevice();
    void testDeviceManager();
    void testRestorePatchings();
};


CPPUNIT_TEST_SUITE_REGISTRATION(DeviceTest);


class DeviceTestMockPort: public Port<AbstractDevice> {
  public:
    DeviceTestMockPort(AbstractDevice *parent, unsigned int id):
      Port<AbstractDevice>(parent, id) {}
    ~DeviceTestMockPort() {}
    bool WriteDMX(const DmxBuffer &buffer) {}
    const DmxBuffer &ReadDMX() const {}
    string UniqueId() const;
};

string DeviceTestMockPort::UniqueId() const {
  std::stringstream str;
  str << PortId();
  return str.str();
}


/*
 * Check that the base device class works correctly.
 */
void DeviceTest::testDevice() {
  string device_name = "test device";
  Device device(NULL, device_name);

  CPPUNIT_ASSERT_EQUAL(device.Name(), device_name);
  CPPUNIT_ASSERT_EQUAL(device.Owner(), (AbstractPlugin*) NULL);

  // set device id
  device.SetDeviceId(2);
  CPPUNIT_ASSERT_EQUAL(device.DeviceId(), (unsigned int) 2);
  device.SetDeviceId(3);
  CPPUNIT_ASSERT_EQUAL(device.DeviceId(), (unsigned int) 3);

  // ad some ports
  DeviceTestMockPort port1(&device, 1);
  DeviceTestMockPort port2(&device, 2);
  device.AddPort(&port1);
  device.AddPort(&port2);

  vector<AbstractPort*> ports = device.Ports();
  CPPUNIT_ASSERT_EQUAL(ports.size(), (size_t) 2);

  // TODO: unify port ids
  AbstractPort *port = device.GetPort(0);
  CPPUNIT_ASSERT_EQUAL(port->PortId(), (unsigned int) 1);
  port = device.GetPort(1);
  CPPUNIT_ASSERT_EQUAL(port->PortId(), (unsigned int) 2);
}


/*
 * Test that we can create universes and save their settings
 */
void DeviceTest::testDeviceManager() {

  DeviceManager manager(NULL, NULL);
  CPPUNIT_ASSERT_EQUAL(manager.DeviceCount(), (unsigned int) 0);

  Device device1(NULL, "test device 1");
  Device device2(NULL, "test device 2");

  manager.RegisterDevice(&device1);
  manager.RegisterDevice(&device2);
  CPPUNIT_ASSERT_EQUAL(manager.DeviceCount(), (unsigned int) 2);

  vector<AbstractDevice*> devices = manager.Devices();
  CPPUNIT_ASSERT_EQUAL(devices[0]->DeviceId(), (unsigned int) 1);
  CPPUNIT_ASSERT_EQUAL(devices[1]->DeviceId(), (unsigned int) 2);
  CPPUNIT_ASSERT_EQUAL(manager.GetDevice(1), (AbstractDevice*) &device1);
  CPPUNIT_ASSERT_EQUAL(manager.GetDevice(2), (AbstractDevice*) &device2);
  CPPUNIT_ASSERT_EQUAL(manager.GetDevice(3), (AbstractDevice*) NULL);

  // unregistering the first device doesn't change the ID of the second
  manager.UnregisterDevice(&device1);
  CPPUNIT_ASSERT_EQUAL(manager.DeviceCount(), (unsigned int) 1);
  CPPUNIT_ASSERT_EQUAL(manager.GetDevice(1), (AbstractDevice*) NULL);
  CPPUNIT_ASSERT_EQUAL(manager.GetDevice(2), (AbstractDevice*) &device2);

  // unregister all
  manager.UnregisterDevice(&device2);
  CPPUNIT_ASSERT_EQUAL(manager.DeviceCount(), (unsigned int) 0);
  manager.UnregisterAllDevices();

  // add one back
  manager.RegisterDevice(&device1);
  CPPUNIT_ASSERT_EQUAL(manager.DeviceCount(), (unsigned int) 1);
  devices = manager.Devices();
  CPPUNIT_ASSERT_EQUAL(devices[0]->DeviceId(), (unsigned int) 1);
  CPPUNIT_ASSERT_EQUAL(manager.GetDevice(1), (AbstractDevice*) &device1);
}


/*
 * Check that we restore the port patchings
 */
void DeviceTest::testRestorePatchings() {
  MemoryPreferencesFactory prefs_factory;
  UniverseStore uni_store(NULL, NULL);
  DeviceManager manager(&prefs_factory, &uni_store);
  CPPUNIT_ASSERT_EQUAL(manager.DeviceCount(), (unsigned int) 0);

  Preferences *prefs = prefs_factory.NewPreference("port");
  CPPUNIT_ASSERT(prefs);
  prefs->SetValue("1", "1");
  prefs->SetValue("2", "3");

  Device device1(NULL, "test device 1");
  DeviceTestMockPort port1(&device1, 1);
  DeviceTestMockPort port2(&device1, 2);
  device1.AddPort(&port1);
  device1.AddPort(&port2);

  manager.RegisterDevice(&device1);
  CPPUNIT_ASSERT_EQUAL(manager.DeviceCount(), (unsigned int) 1);
  CPPUNIT_ASSERT(port1.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, port1.GetUniverse()->UniverseId());
  CPPUNIT_ASSERT(port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 3, port2.GetUniverse()->UniverseId());

  // Now check that patching a universe saves the settings
  Universe *uni = uni_store.GetUniverseOrCreate(10);
  CPPUNIT_ASSERT(uni);
  port1.SetUniverse(uni);

  // unregister all
  manager.UnregisterAllDevices();
  CPPUNIT_ASSERT_EQUAL(manager.DeviceCount(), (unsigned int) 0);

  CPPUNIT_ASSERT_EQUAL(string("10"), prefs->GetValue("1"));
  CPPUNIT_ASSERT_EQUAL(string("3"), prefs->GetValue("2"));
}
