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
#include <llad/Plugin.h>
#include <llad/Port.h>
#include <llad/Preferences.h>
#include "DeviceManager.h"
#include "UniverseStore.h"

#include <lla/Logging.h>

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


class DeviceTestMockPlugin: public Plugin {
  public:
    DeviceTestMockPlugin(const PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {}
    string Name() const { return "foo"; }
    string Description() const { return "bar"; }
    lla_plugin_id Id() const { return LLA_PLUGIN_ALL; }
    string PluginPrefix() const { return "test"; }
};

class DeviceTestMockDevice: public Device {
  public:
    DeviceTestMockDevice(AbstractPlugin *owner, const string &name):
      Device(owner, name) {}
    string DeviceId() const { return Name(); }
};


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
  string device_name = "test";
  DeviceTestMockDevice orphaned_device(NULL, device_name);

  CPPUNIT_ASSERT_EQUAL(device_name, orphaned_device.Name());
  CPPUNIT_ASSERT_EQUAL((AbstractPlugin*) NULL, orphaned_device.Owner());
  CPPUNIT_ASSERT_EQUAL(string(""), orphaned_device.UniqueId());

  // Non orphaned device
  DeviceTestMockPlugin plugin(NULL);
  DeviceTestMockDevice device(&plugin, device_name);
  CPPUNIT_ASSERT_EQUAL(device.Name(), device_name);
  CPPUNIT_ASSERT_EQUAL((AbstractPlugin*) &plugin, device.Owner());
  CPPUNIT_ASSERT_EQUAL(string("0-test"), device.UniqueId());

  // add some ports
  vector<AbstractPort*> ports = orphaned_device.Ports();
  CPPUNIT_ASSERT_EQUAL((size_t) 0, ports.size());
  DeviceTestMockPort port1(&orphaned_device, 1);
  DeviceTestMockPort port2(&device, 2);
  orphaned_device.AddPort(&port1);
  orphaned_device.AddPort(&port2);
  ports = orphaned_device.Ports();
  CPPUNIT_ASSERT_EQUAL((size_t) 2, ports.size());

  AbstractPort *port = orphaned_device.GetPort(0);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, port->PortId());
  port = orphaned_device.GetPort(1);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, port->PortId());
}


/*
 * Test that we can create universes and save their settings
 */
void DeviceTest::testDeviceManager() {

  DeviceManager manager(NULL, NULL);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, manager.DeviceCount());

  DeviceTestMockPlugin plugin(NULL);
  DeviceTestMockDevice orphaned_device(NULL, "orphaned device");
  DeviceTestMockDevice device1(&plugin, "test device 1");
  DeviceTestMockDevice device2(&plugin, "test device 2");

  // can't register NULL
  CPPUNIT_ASSERT(!manager.RegisterDevice(NULL));

  // Can't register a device with no unique id
  CPPUNIT_ASSERT(!manager.RegisterDevice(&orphaned_device));

  // register a device
  CPPUNIT_ASSERT(manager.RegisterDevice(&device1));
  // the second time must fail
  CPPUNIT_ASSERT(!manager.RegisterDevice(&device1));

  // register a second device
  CPPUNIT_ASSERT(manager.RegisterDevice(&device2));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, manager.DeviceCount());

  vector<device_alias_pair> devices = manager.Devices();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, devices[0].alias);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device1, devices[0].device);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, devices[1].alias);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device2, devices[1].device);

  // test fetching a device by alias
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device1, manager.GetDevice(1));
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device2, manager.GetDevice(2));
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) NULL, manager.GetDevice(3));

  // test fetching a device by id
  device_alias_pair result = manager.GetDevice(device1.UniqueId());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, result.alias);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device1, result.device);
  result = manager.GetDevice(device2.UniqueId());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, result.alias);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device2, result.device);
  result = manager.GetDevice("foo");
  CPPUNIT_ASSERT_EQUAL(lla::DeviceManager::MISSING_DEVICE_ALIAS, result.alias);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) NULL, result.device);
  result = manager.GetDevice("");
  CPPUNIT_ASSERT_EQUAL(lla::DeviceManager::MISSING_DEVICE_ALIAS, result.alias);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) NULL, result.device);

  // test unregistering null or non-registered device
  CPPUNIT_ASSERT(!manager.UnregisterDevice(NULL));
  CPPUNIT_ASSERT(!manager.UnregisterDevice(&orphaned_device));

  // unregistering the first device doesn't change the ID of the second
  CPPUNIT_ASSERT(manager.UnregisterDevice(&device1));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, manager.DeviceCount());
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) NULL, manager.GetDevice(1));
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device2, manager.GetDevice(2));

  // unregister by id
  CPPUNIT_ASSERT(!manager.UnregisterDevice(device1.UniqueId()));
  CPPUNIT_ASSERT(manager.UnregisterDevice(device2.UniqueId()));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, manager.DeviceCount());
  manager.UnregisterAllDevices();

  // add one back and check that ids reset
  CPPUNIT_ASSERT(manager.RegisterDevice(&device1));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, manager.DeviceCount());
  devices = manager.Devices();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, devices[0].alias);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device1, devices[0].device);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device1, manager.GetDevice(1));
  result = manager.GetDevice(device1.UniqueId());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, result.alias);
  CPPUNIT_ASSERT_EQUAL((AbstractDevice*) &device1, result.device);
}


/*
 * Check that we restore the port patchings
 */
void DeviceTest::testRestorePatchings() {
  MemoryPreferencesFactory prefs_factory;
  UniverseStore uni_store(NULL, NULL);
  DeviceManager manager(&prefs_factory, &uni_store);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, manager.DeviceCount());

  Preferences *prefs = prefs_factory.NewPreference("port");
  CPPUNIT_ASSERT(prefs);
  prefs->SetValue("1", "1");
  prefs->SetValue("2", "3");

  DeviceTestMockPlugin plugin(NULL);
  DeviceTestMockDevice device1(&plugin, "test device 1");
  DeviceTestMockPort port1(&device1, 1);
  DeviceTestMockPort port2(&device1, 2);
  device1.AddPort(&port1);
  device1.AddPort(&port2);

  CPPUNIT_ASSERT(manager.RegisterDevice(&device1));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, manager.DeviceCount());
  CPPUNIT_ASSERT(port1.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(port1.GetUniverse()->UniverseId(), (unsigned int) 1);
  CPPUNIT_ASSERT(port2.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(port2.GetUniverse()->UniverseId(), (unsigned int) 3);

  // Now check that patching a universe saves the settings
  Universe *uni = uni_store.GetUniverseOrCreate(10);
  CPPUNIT_ASSERT(uni);
  port1.SetUniverse(uni);

  // unregister all
  manager.UnregisterAllDevices();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, manager.DeviceCount());

  CPPUNIT_ASSERT_EQUAL(string("10"), prefs->GetValue("1"));
  CPPUNIT_ASSERT_EQUAL(string("3"), prefs->GetValue("2"));
}
