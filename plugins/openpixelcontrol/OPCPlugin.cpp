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
 * OPCPlugin.cpp
 * The Open Pixel Control Plugin.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/openpixelcontrol/OPCPlugin.h"

#include <memory>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/SocketAddress.h"
#include "olad/Device.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/openpixelcontrol/OPCDevice.h"
#include "plugins/openpixelcontrol/OPCPluginDescription.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

using ola::network::IPV4SocketAddress;
using std::string;
using std::vector;

const char OPCPlugin::LISTEN_KEY[] = "listen";
const char OPCPlugin::PLUGIN_NAME[] = "Open Pixel Control";
const char OPCPlugin::PLUGIN_PREFIX[] = "openpixelcontrol";
const char OPCPlugin::TARGET_KEY[] = "target";

OPCPlugin::~OPCPlugin() {}

bool OPCPlugin::StartHook() {
  // Start Target (output) devices.
  AddDevices<OPCClientDevice>(TARGET_KEY);

  // Start listen (input) devices.
  AddDevices<OPCServerDevice>(LISTEN_KEY);
  return true;
}

bool OPCPlugin::StopHook() {
  OPCDevices::iterator iter = m_devices.begin();
  for (; iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    (*iter)->Stop();
    delete *iter;
  }
  m_devices.clear();
  return true;
}

string OPCPlugin::Description() const {
    return
"OPC Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates OPC Client and/or OPC Server Devices.\n"
"\n"
"--- Config file : ola-opc.conf ---\n"
"\n"
"target = <IP>:<port>\n"
"Create a Open Pixel Control client, connected to the IP:port.\n"
"Multiple targets can be specified and a device will be created\n"
"for each.\n"
"\n"
"listen = <IP>:<port>\n"
"Create an Open Pixel Control server, listening on IP:port.\n"
"To listen on any address use listen = 0.0.0.0.\n"
"Multiple listen keys can be specified and a device will be created\n"
"for each.\n"
"\n"
"target_<IP>:<port>_channel = <channel>\n"
"The Open Pixel Control channels to use for the specified device.\n"
"Multiple channels can be specified and an output port will be created\n"
"for each.\n"
"\n"
"listen_<IP>:<port>_channel = <channel>\n"
"The Open Pixel Control channels to use for the specified device.\n"
"Multiple channels can be specified and an input port will be created\n"
"for each.\n"
"\n";
}

bool OPCPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }
  // No defaults.
  return true;
}

template <typename DeviceClass>
void OPCPlugin::AddDevices(const std::string &key) {
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
      OLA_INFO << "Failed to start OPCDevice";
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
