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
 * DeviceTest.cpp
 * Test fixture for the Device class.
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <vector>

#include "olad/Device.h"
#include "olad/Plugin.h"
#include "olad/Port.h"
#include "olad/Preferences.h"
#include "olad/TestCommon.h"
#include "ola/testing/TestUtils.h"


using ola::AbstractDevice;
using ola::AbstractPlugin;
using ola::InputPort;
using ola::OutputPort;
using std::string;
using std::vector;


class DeviceTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(DeviceTest);
  CPPUNIT_TEST(testDevice);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testDevice();

  private:
    void AddPortsToDeviceAndCheck(ola::Device *device);
};


CPPUNIT_TEST_SUITE_REGISTRATION(DeviceTest);


/*
 * Check that the base device class works correctly.
 */
void DeviceTest::testDevice() {
  string device_name = "test";
  MockDevice orphaned_device(NULL, device_name);

  OLA_ASSERT_EQ(device_name, orphaned_device.Name());
  OLA_ASSERT_EQ(static_cast<AbstractPlugin*>(NULL),
                       orphaned_device.Owner());
  OLA_ASSERT_EQ(string(""), orphaned_device.UniqueId());
  AddPortsToDeviceAndCheck(&orphaned_device);

  // Non orphaned device
  TestMockPlugin plugin(NULL, ola::OLA_PLUGIN_ARTNET);
  MockDevice device(&plugin, device_name);
  OLA_ASSERT_EQ(device.Name(), device_name);
  OLA_ASSERT_EQ(static_cast<AbstractPlugin*>(&plugin), device.Owner());
  OLA_ASSERT_EQ(string("2-test"), device.UniqueId());
  AddPortsToDeviceAndCheck(&device);
}


void DeviceTest::AddPortsToDeviceAndCheck(ola::Device *device) {
  // check we don't have any ports yet.
  vector<InputPort*> input_ports;
  device->InputPorts(&input_ports);
  OLA_ASSERT_EQ((size_t) 0, input_ports.size());
  vector<OutputPort*> output_ports;
  device->OutputPorts(&output_ports);
  OLA_ASSERT_EQ((size_t) 0, output_ports.size());

  // add two input ports and an output port
  TestMockInputPort input_port1(device, 1, NULL);
  TestMockInputPort input_port2(device, 2, NULL);
  TestMockOutputPort output_port1(device, 1);
  device->AddPort(&input_port1);
  device->AddPort(&input_port2);
  device->AddPort(&output_port1);

  device->InputPorts(&input_ports);
  OLA_ASSERT_EQ((size_t) 2, input_ports.size());
  device->OutputPorts(&output_ports);
  OLA_ASSERT_EQ((size_t) 1, output_ports.size());

  InputPort *input_port = device->GetInputPort(1);
  OLA_ASSERT(input_port);
  OLA_ASSERT_EQ((unsigned int) 1, input_port->PortId());
  input_port = device->GetInputPort(2);
  OLA_ASSERT(input_port);
  OLA_ASSERT_EQ((unsigned int) 2, input_port->PortId());
  OutputPort *output_port = device->GetOutputPort(1);
  OLA_ASSERT(output_port);
  OLA_ASSERT_EQ((unsigned int) 1, output_port->PortId());
}
