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
 * DeviceManagerTest.cpp
 * Test fixture for the DeviceManager class.
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/DmxBuffer.h"
#include "olad/DeviceManager.h"
#include "olad/Plugin.h"
#include "olad/Port.h"
#include "olad/PortBroker.h"
#include "olad/PortManager.h"
#include "olad/Preferences.h"
#include "olad/TestCommon.h"
#include "olad/UniverseStore.h"
#include "ola/testing/TestUtils.h"


using ola::AbstractDevice;
using ola::AbstractPlugin;
using ola::DeviceManager;
using ola::DmxBuffer;
using ola::Port;
using ola::PortManager;
using ola::Universe;
using ola::UniverseStore;
using std::string;
using std::vector;


class DeviceManagerTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DeviceManagerTest);
  CPPUNIT_TEST(testDeviceManager);
  CPPUNIT_TEST(testRestorePatchings);
  CPPUNIT_TEST(testRestorePriorities);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testDeviceManager();
    void testRestorePatchings();
    void testRestorePriorities();
};


CPPUNIT_TEST_SUITE_REGISTRATION(DeviceManagerTest);


/*
 * Test that we can register devices and retrieve them.
 */
void DeviceManagerTest::testDeviceManager() {
  DeviceManager manager(NULL, NULL);
  OLA_ASSERT_EQ((unsigned int) 0, manager.DeviceCount());

  TestMockPlugin plugin(NULL, ola::OLA_PLUGIN_ARTNET);
  MockDevice orphaned_device(NULL, "orphaned device");
  MockDevice device1(&plugin, "test device 1");
  MockDevice device2(&plugin, "test device 2");

  // can't register NULL
  OLA_ASSERT_FALSE(manager.RegisterDevice(NULL));

  // Can't register a device with no unique id
  OLA_ASSERT_FALSE(manager.RegisterDevice(&orphaned_device));

  // register a device
  OLA_ASSERT(manager.RegisterDevice(&device1));
  // the second time must fail
  OLA_ASSERT_FALSE(manager.RegisterDevice(&device1));

  // register a second device
  OLA_ASSERT(manager.RegisterDevice(&device2));
  OLA_ASSERT_EQ((unsigned int) 2, manager.DeviceCount());

  vector<ola::device_alias_pair> devices = manager.Devices();
  OLA_ASSERT_EQ((unsigned int) 1, devices[0].alias);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device1),
                       devices[0].device);
  OLA_ASSERT_EQ((unsigned int) 2, devices[1].alias);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device2),
                       devices[1].device);

  // test fetching a device by alias
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device1),
                       manager.GetDevice(1));
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device2),
                       manager.GetDevice(2));
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(NULL),
                       manager.GetDevice(3));

  // test fetching a device by id
  ola::device_alias_pair result = manager.GetDevice(device1.UniqueId());
  OLA_ASSERT_EQ((unsigned int) 1, result.alias);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device1), result.device);
  result = manager.GetDevice(device2.UniqueId());
  OLA_ASSERT_EQ((unsigned int) 2, result.alias);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device2), result.device);
  result = manager.GetDevice("foo");
  OLA_ASSERT_EQ(DeviceManager::MISSING_DEVICE_ALIAS, result.alias);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(NULL), result.device);
  result = manager.GetDevice("");
  OLA_ASSERT_EQ(DeviceManager::MISSING_DEVICE_ALIAS, result.alias);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(NULL), result.device);

  // test unregistering null or non-registered device
  OLA_ASSERT_FALSE(manager.UnregisterDevice(NULL));
  OLA_ASSERT_FALSE(manager.UnregisterDevice(&orphaned_device));

  // unregistering the first device doesn't change the ID of the second
  OLA_ASSERT(manager.UnregisterDevice(&device1));
  OLA_ASSERT_EQ((unsigned int) 1, manager.DeviceCount());
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(NULL),
                       manager.GetDevice(1));
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device2),
                       manager.GetDevice(2));

  // unregister by id
  OLA_ASSERT_FALSE(manager.UnregisterDevice(device1.UniqueId()));
  OLA_ASSERT(manager.UnregisterDevice(device2.UniqueId()));
  OLA_ASSERT_EQ((unsigned int) 0, manager.DeviceCount());
  manager.UnregisterAllDevices();

  // add one back and check that ids reset
  OLA_ASSERT(manager.RegisterDevice(&device1));
  OLA_ASSERT_EQ((unsigned int) 1, manager.DeviceCount());
  devices = manager.Devices();
  OLA_ASSERT_EQ((unsigned int) 1, devices[0].alias);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device1),
                       devices[0].device);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device1),
                       manager.GetDevice(1));
  result = manager.GetDevice(device1.UniqueId());
  OLA_ASSERT_EQ((unsigned int) 1, result.alias);
  OLA_ASSERT_EQ(static_cast<AbstractDevice*>(&device1),
                       result.device);
}


/*
 * Check that we restore the port patchings
 */
void DeviceManagerTest::testRestorePatchings() {
  ola::MemoryPreferencesFactory prefs_factory;
  UniverseStore uni_store(NULL, NULL);
  ola::PortBroker broker;
  PortManager port_manager(&uni_store, &broker);
  DeviceManager manager(&prefs_factory, &port_manager);
  OLA_ASSERT_EQ((unsigned int) 0, manager.DeviceCount());

  ola::Preferences *prefs = prefs_factory.NewPreference("port");
  OLA_ASSERT(prefs);
  prefs->SetValue("2-test_device_1-I-1", "1");
  prefs->SetValue("2-test_device_1-O-1", "3");

  TestMockPlugin plugin(NULL, ola::OLA_PLUGIN_ARTNET);
  MockDevice device1(&plugin, "test_device_1");
  TestMockInputPort input_port(&device1, 1, NULL);
  TestMockOutputPort output_port(&device1, 1);
  device1.AddPort(&input_port);
  device1.AddPort(&output_port);

  OLA_ASSERT(manager.RegisterDevice(&device1));
  OLA_ASSERT_EQ((unsigned int) 1, manager.DeviceCount());
  OLA_ASSERT(input_port.GetUniverse());
  OLA_ASSERT_EQ(input_port.GetUniverse()->UniverseId(),
                       (unsigned int) 1);
  OLA_ASSERT(output_port.GetUniverse());
  OLA_ASSERT_EQ(output_port.GetUniverse()->UniverseId(),
                       (unsigned int) 3);

  // Now check that patching a universe saves the settings
  Universe *uni = uni_store.GetUniverseOrCreate(10);
  OLA_ASSERT(uni);
  input_port.SetUniverse(uni);

  // unregister all
  manager.UnregisterAllDevices();
  OLA_ASSERT_EQ((unsigned int) 0, manager.DeviceCount());

  OLA_ASSERT_EQ(string("10"), prefs->GetValue("2-test_device_1-I-1"));
  OLA_ASSERT_EQ(string("3"), prefs->GetValue("2-test_device_1-O-1"));
}



/*
 * Test that port priorities are restored correctly.
 */
void DeviceManagerTest::testRestorePriorities() {
  ola::MemoryPreferencesFactory prefs_factory;
  UniverseStore uni_store(NULL, NULL);
  ola::PortBroker broker;
  PortManager port_manager(&uni_store, &broker);
  DeviceManager manager(&prefs_factory, &port_manager);
  OLA_ASSERT_EQ((unsigned int) 0, manager.DeviceCount());

  ola::Preferences *prefs = prefs_factory.NewPreference("port");
  OLA_ASSERT(prefs);
  prefs->SetValue("2-test_device_1-I-1_priority_mode", "0");
  prefs->SetValue("2-test_device_1-I-1_priority_value", "120");
  prefs->SetValue("2-test_device_1-O-1_priority_mode", "0");
  prefs->SetValue("2-test_device_1-O-1_priority_value", "140");
  prefs->SetValue("2-test_device_1-I-2_priority_mode", "1");  // override mode
  prefs->SetValue("2-test_device_1-I-2_priority_value", "160");
  prefs->SetValue("2-test_device_1-O-2_priority_mode", "1");  // override mode
  prefs->SetValue("2-test_device_1-O-2_priority_value", "180");
  prefs->SetValue("2-test_device_1-I-3_priority_mode", "0");  // inherit mode
  // invalid priority
  prefs->SetValue("2-test_device_1-I-3_priority_value", "210");
  prefs->SetValue("2-test_device_1-O-3_priority_mode", "0");  // inherit mode
  prefs->SetValue("2-test_device_1-O-3_priority_value", "180");

  TestMockPlugin plugin(NULL, ola::OLA_PLUGIN_ARTNET);
  MockDevice device1(&plugin, "test_device_1");
  // these ports don't support priorities.
  TestMockInputPort input_port(&device1, 1, NULL);
  TestMockOutputPort output_port(&device1, 1);
  // these devices support priorities
  TestMockPriorityInputPort input_port2(&device1, 2, NULL);
  TestMockPriorityOutputPort output_port2(&device1, 2);
  TestMockPriorityInputPort input_port3(&device1, 3, NULL);
  TestMockPriorityOutputPort output_port3(&device1, 3);
  device1.AddPort(&input_port);
  device1.AddPort(&output_port);
  device1.AddPort(&input_port2);
  device1.AddPort(&output_port2);
  device1.AddPort(&input_port3);
  device1.AddPort(&output_port3);

  OLA_ASSERT(manager.RegisterDevice(&device1));
  OLA_ASSERT_EQ((unsigned int) 1, manager.DeviceCount());
  OLA_ASSERT_EQ(ola::CAPABILITY_STATIC,
                       input_port.PriorityCapability());
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_INHERIT,
                       input_port.GetPriorityMode());
  OLA_ASSERT_EQ((uint8_t) 120, input_port.GetPriority());
  OLA_ASSERT_EQ(ola::CAPABILITY_NONE,
                       output_port.PriorityCapability());
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_INHERIT,
                       output_port.GetPriorityMode());
  OLA_ASSERT_EQ((uint8_t) 100, output_port.GetPriority());

  // these ports support priorities
  OLA_ASSERT_EQ(ola::CAPABILITY_FULL,
                       input_port2.PriorityCapability());
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_OVERRIDE,
                       input_port2.GetPriorityMode());
  OLA_ASSERT_EQ((uint8_t) 160, input_port2.GetPriority());
  OLA_ASSERT_EQ(ola::CAPABILITY_FULL,
                       output_port2.PriorityCapability());
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_OVERRIDE,
                       output_port2.GetPriorityMode());
  OLA_ASSERT_EQ((uint8_t) 180, output_port2.GetPriority());
  OLA_ASSERT_EQ(ola::CAPABILITY_FULL,
                       input_port3.PriorityCapability());
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_INHERIT,
                       input_port3.GetPriorityMode());
  OLA_ASSERT_EQ((uint8_t) 200, input_port3.GetPriority());
  OLA_ASSERT_EQ(ola::CAPABILITY_FULL,
                       output_port3.PriorityCapability());
  OLA_ASSERT_EQ(ola::PRIORITY_MODE_INHERIT,
                       output_port3.GetPriorityMode());
  OLA_ASSERT_EQ((uint8_t) 180, output_port3.GetPriority());

  // Now make some changes
  input_port2.SetPriorityMode(ola::PRIORITY_MODE_INHERIT);
  output_port2.SetPriorityMode(ola::PRIORITY_MODE_INHERIT);
  input_port3.SetPriorityMode(ola::PRIORITY_MODE_OVERRIDE);
  input_port3.SetPriority(40);
  output_port3.SetPriorityMode(ola::PRIORITY_MODE_OVERRIDE);
  output_port3.SetPriority(60);

  // unregister all
  manager.UnregisterAllDevices();
  OLA_ASSERT_EQ((unsigned int) 0, manager.DeviceCount());

  OLA_ASSERT_EQ(string("0"),
                       prefs->GetValue("2-test_device_1-I-2_priority_mode"));
  OLA_ASSERT_EQ(string("0"),
                       prefs->GetValue("2-test_device_1-O-2_priority_mode"));
  OLA_ASSERT_EQ(string("1"),
                       prefs->GetValue("2-test_device_1-I-3_priority_mode"));
  OLA_ASSERT_EQ(string("40"),
                       prefs->GetValue("2-test_device_1-I-3_priority_value"));
  OLA_ASSERT_EQ(string("1"),
                       prefs->GetValue("2-test_device_1-O-3_priority_mode"));
  OLA_ASSERT_EQ(string("60"),
                       prefs->GetValue("2-test_device_1-O-3_priority_value"));
}
