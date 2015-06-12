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
 * SandNetPlugin.cpp
 * The SandNet plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#include <string>
#include "olad/Preferences.h"
#include "plugins/sandnet/SandNetDevice.h"
#include "plugins/sandnet/SandNetPlugin.h"


namespace ola {
namespace plugin {
namespace sandnet {

using std::string;

const char SandNetPlugin::SANDNET_NODE_NAME[] = "ola-SandNet";
const char SandNetPlugin::PLUGIN_NAME[] = "SandNet";
const char SandNetPlugin::PLUGIN_PREFIX[] = "sandnet";


/*
 * Start the plugin
 */
bool SandNetPlugin::StartHook() {
  m_device = new SandNetDevice(this,
                               m_preferences,
                               m_plugin_adaptor);

  if (!m_device) {
    return false;
  }

  if (!m_device->Start()) {
    delete m_device;
    return false;
  }
  m_plugin_adaptor->RegisterDevice(m_device);
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool SandNetPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


/*
 * Return the description for this plugin
 */
string SandNetPlugin::Description() const {
  return
"SandNet Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with 2 output and 8 input ports.\n"
"\n"
"The universe bindings are offset by one from those displayed in sandnet.\n"
"For example, SandNet universe 1 is OLA universe 0.\n"
"\n"
"--- Config file : ola-sandnet.conf ---\n"
"\n"
"ip = [a.b.c.d|<interface_name>]\n"
"The ip address or interface name to bind to. If not specified it will\n"
"use the first non-loopback interface.\n"
"\n"
"name = ola-SandNet\n"
"The name of the node.\n"
"\n";
}


/*
 * Assign default values
 */
bool SandNetPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = false;
  save |= m_preferences->SetDefaultValue(SandNetDevice::IP_KEY,
                                         StringValidator(true), "");
  save |= m_preferences->SetDefaultValue(SandNetDevice::NAME_KEY,
                                         StringValidator(), SANDNET_NODE_NAME);

  if (save) {
    m_preferences->Save();
  }

  if (m_preferences->GetValue(SandNetDevice::NAME_KEY).empty()) {
    return false;
  }
  return true;
}
}  // namespace sandnet
}  // namespace plugin
}  // namespace ola

