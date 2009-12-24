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

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/DmxBuffer.h"
#include "olad/Device.h"
#include "olad/DeviceManager.h"
#include "olad/Plugin.h"
#include "olad/Port.h"
#include "olad/PortPatcher.h"
#include "olad/Preferences.h"
#include "olad/TestCommon.h"
#include "olad/UniverseStore.h"

using ola::AbstractDevice;
using ola::AbstractPlugin;
using ola::DeviceManager;
using ola::DmxBuffer;
using ola::PortPatcher;
using ola::Universe;
using ola::UniverseStore;
using std::string;
using std::vector;


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

  private:
    void AddPortsToDeviceAndCheck(ola::Device *device);
};


CPPUNIT_TEST_SUITE_REGISTRATION(DeviceTest);


class DeviceTestMockDevice: public ola::Device {
  public:
    DeviceTestMockDevice(ola::AbstractPlugin *owner, const string &name):
      Device(owner, name) {}
    string DeviceId() const { return Name(); }
    bool AllowLooping() const { return false; }
    bool AllowMultiPortPatching() const { return false; }
};


/*
 * Check that the base device class works correctly.
 */
void DeviceTest::testDevice() {
  string device_name = "test";
  DeviceTestMockDevice orphaned_device(NULL, device_name);

  CPPUNIT_ASSERT_EQUAL(device_name, orphaned_device.Name());
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractPlugin*>(NULL),
                       orphaned_device.Owner());
  CPPUNIT_ASSERT_EQUAL(string(""), orphaned_device.UniqueId());
  AddPortsToDeviceAndCheck(&orphaned_device);

  // Non orphaned device
  TestMockPlugin plugin(NULL);
  DeviceTestMockDevice device(&plugin, device_name);
  CPPUNIT_ASSERT_EQUAL(device.Name(), device_name);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractPlugin*>(&plugin),
                       device.Owner());
  CPPUNIT_ASSERT_EQUAL(string("0-test"), device.UniqueId());
  AddPortsToDeviceAndCheck(&device);
}


/*
 * Test that we can create universes and save their settings
 */
void DeviceTest::testDeviceManager() {
  DeviceManager manager(NULL, NULL);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, manager.DeviceCount());

  TestMockPlugin plugin(NULL);
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

  vector<ola::device_alias_pair> devices = manager.Devices();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, devices[0].alias);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device1),
                       devices[0].device);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, devices[1].alias);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device2),
                       devices[1].device);

  // test fetching a device by alias
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device1),
                      manager.GetDevice(1));
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device2),
                       manager.GetDevice(2));
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(NULL),
                       manager.GetDevice(3));

  // test fetching a device by id
  ola::device_alias_pair result = manager.GetDevice(device1.UniqueId());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, result.alias);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device1),
                       result.device);
  result = manager.GetDevice(device2.UniqueId());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, result.alias);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device2),
                       result.device);
  result = manager.GetDevice("foo");
  CPPUNIT_ASSERT_EQUAL(DeviceManager::MISSING_DEVICE_ALIAS, result.alias);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(NULL), result.device);
  result = manager.GetDevice("");
  CPPUNIT_ASSERT_EQUAL(DeviceManager::MISSING_DEVICE_ALIAS, result.alias);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(NULL), result.device);

  // test unregistering null or non-registered device
  CPPUNIT_ASSERT(!manager.UnregisterDevice(NULL));
  CPPUNIT_ASSERT(!manager.UnregisterDevice(&orphaned_device));

  // unregistering the first device doesn't change the ID of the second
  CPPUNIT_ASSERT(manager.UnregisterDevice(&device1));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, manager.DeviceCount());
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(NULL),
                       manager.GetDevice(1));
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device2),
                       manager.GetDevice(2));

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
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device1),
                       devices[0].device);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device1),
                       manager.GetDevice(1));
  result = manager.GetDevice(device1.UniqueId());
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, result.alias);
  CPPUNIT_ASSERT_EQUAL(reinterpret_cast<AbstractDevice*>(&device1),
                       result.device);
}


/*
 * Check that we restore the port patchings
 */
void DeviceTest::testRestorePatchings() {
  ola::MemoryPreferencesFactory prefs_factory;
  UniverseStore uni_store(NULL, NULL);
  PortPatcher port_patcher(&uni_store);
  DeviceManager manager(&prefs_factory, &port_patcher);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, manager.DeviceCount());

  ola::Preferences *prefs = prefs_factory.NewPreference("port");
  CPPUNIT_ASSERT(prefs);
  prefs->SetValue("0-test_device_1-I-1", "1");
  prefs->SetValue("0-test_device_1-O-1", "3");

  TestMockPlugin plugin(NULL);
  DeviceTestMockDevice device1(&plugin, "test_device_1");
  TestMockInputPort input_port(&device1, 1);
  TestMockOutputPort output_port(&device1, 1);
  device1.AddPort(&input_port);
  device1.AddPort(&output_port);

  CPPUNIT_ASSERT(manager.RegisterDevice(&device1));
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, manager.DeviceCount());
  CPPUNIT_ASSERT(input_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(input_port.GetUniverse()->UniverseId(),
                       (unsigned int) 1);
  CPPUNIT_ASSERT(output_port.GetUniverse());
  CPPUNIT_ASSERT_EQUAL(output_port.GetUniverse()->UniverseId(),
                       (unsigned int) 3);

  // Now check that patching a universe saves the settings
  Universe *uni = uni_store.GetUniverseOrCreate(10);
  CPPUNIT_ASSERT(uni);
  input_port.SetUniverse(uni);

  // unregister all
  manager.UnregisterAllDevices();
  CPPUNIT_ASSERT_EQUAL((unsigned int) 0, manager.DeviceCount());

  CPPUNIT_ASSERT_EQUAL(string("10"), prefs->GetValue("0-test_device_1-I-1"));
  CPPUNIT_ASSERT_EQUAL(string("3"), prefs->GetValue("0-test_device_1-O-1"));
}


void DeviceTest::AddPortsToDeviceAndCheck(ola::Device *device) {
  // check we don't have any ports yet.
  vector<InputPort*> input_ports;
  device->InputPorts(&input_ports);
  CPPUNIT_ASSERT_EQUAL((size_t) 0, input_ports.size());
  vector<OutputPort*> output_ports;
  device->OutputPorts(&output_ports);
  CPPUNIT_ASSERT_EQUAL((size_t) 0, output_ports.size());

  // add two input ports and an output port
  TestMockInputPort input_port1(device, 1);
  TestMockInputPort input_port2(device, 2);
  TestMockOutputPort output_port1(device, 1);
  device->AddPort(&input_port1);
  device->AddPort(&input_port2);
  device->AddPort(&output_port1);

  device->InputPorts(&input_ports);
  CPPUNIT_ASSERT_EQUAL((size_t) 2, input_ports.size());
  device->OutputPorts(&output_ports);
  CPPUNIT_ASSERT_EQUAL((size_t) 1, output_ports.size());

  InputPort *input_port = device->GetInputPort(1);
  CPPUNIT_ASSERT(input_port);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, input_port->PortId());
  input_port = device->GetInputPort(2);
  CPPUNIT_ASSERT(input_port);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 2, input_port->PortId());
  OutputPort *output_port = device->GetOutputPort(1);
  CPPUNIT_ASSERT(output_port);
  CPPUNIT_ASSERT_EQUAL((unsigned int) 1, output_port->PortId());
}
