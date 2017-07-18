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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OpcPlugin.cpp
 * The Open Pixel Control Plugin.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/openpixelcontrol/OpcPlugin.h"

#include <memory>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/SocketAddress.h"
#include "olad/Device.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/openpixelcontrol/OpcDevice.h"
#include "plugins/openpixelcontrol/OpcPluginDescription.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

using ola::network::IPV4SocketAddress;
using std::string;
using std::vector;

const char OpcPlugin::LISTEN_KEY[] = "listen";
const char OpcPlugin::PLUGIN_NAME[] = "Open Pixel Control";
const char OpcPlugin::PLUGIN_PREFIX[] = "openpixelcontrol";
const char OpcPlugin::TARGET_KEY[] = "target";

OpcPlugin::~OpcPlugin() {}

bool OpcPlugin::StartHook() {
  // Start Target (output) devices.
  AddDevices<OpcClientDevice>(TARGET_KEY);

  // Start listen (input) devices.
  AddDevices<OpcServerDevice>(LISTEN_KEY);
  return true;
}

bool OpcPlugin::StopHook() {
  OpcDevices::iterator iter = m_devices.begin();
  for (; iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    (*iter)->Stop();
    delete *iter;
  }
  m_devices.clear();
  return true;
}

string OpcPlugin::Description() const {
    return plugin_description;
}

bool OpcPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }
  // No defaults.
  return true;
}

template <typename DeviceClass>
void OpcPlugin::AddDevices(const std::string &key) {
  vector<string> addresses = m_preferences->GetMultipleValue(key);
  vector<string>::const_iterator iter = addresses.begin();
  for (; iter != addresses.end(); ++iter) {
    IPV4SocketAddress target;
    if (!IPV4SocketAddress::FromString(*iter, &target)) {
      OLA_WARN << "Invalid Open Pixel Control address: " << *iter;
      continue;
    }

    std::auto_ptr<DeviceClass> device(new DeviceClass(
        this, m_plugin_adaptor, m_preferences, target));

    if (!device->Start()) {
      OLA_INFO << "Failed to start OpcDevice";
      continue;
    }

    m_plugin_adaptor->RegisterDevice(device.get());
    OLA_INFO << "Added OPC device";
    m_devices.push_back(device.release());
  }
}
}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
