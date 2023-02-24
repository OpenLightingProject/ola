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
 * KiNetPlugin.cpp
 * The KiNet plugin for ola
 * Copyright (C) 2013 Simon Newton
 */

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/kinet/KiNetDevice.h"
#include "plugins/kinet/KiNetPlugin.h"


namespace ola {
namespace plugin {
namespace kinet {

using ola::network::IPV4Address;
using std::string;
using std::vector;

const char KiNetPlugin::POWER_SUPPLY_KEY[] = "power_supply";
const char KiNetPlugin::PLUGIN_NAME[] = "KiNET";
const char KiNetPlugin::PLUGIN_PREFIX[] = "kinet";

KiNetPlugin::KiNetPlugin(PluginAdaptor *plugin_adaptor)
    : Plugin(plugin_adaptor) {
}

KiNetPlugin::~KiNetPlugin() {}

/*
 * Start the plugin.
 */
bool KiNetPlugin::StartHook() {
  vector<string> power_supplies_strings = m_preferences->GetMultipleValue(
      POWER_SUPPLY_KEY);
  vector<string>::const_iterator iter = power_supplies_strings.begin();
  vector<IPV4Address> power_supplies;

  for (; iter != power_supplies_strings.end(); ++iter) {
    if (iter->empty()) {
      continue;
    }
    IPV4Address target;
    if (IPV4Address::FromString(*iter, &target)) {
      power_supplies.push_back(target);
    } else {
      OLA_WARN << "Invalid power supply IP address : " << *iter;
    }
  }
  m_device.reset(new KiNetDevice(this, power_supplies, m_plugin_adaptor));

  if (!m_device->Start()) {
    m_device.reset();
    return false;
  }
  m_plugin_adaptor->RegisterDevice(m_device.get());
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool KiNetPlugin::StopHook() {
  if (m_device.get()) {
    // stop the device
    m_plugin_adaptor->UnregisterDevice(m_device.get());
    bool ret = m_device->Stop();
    m_device.reset();
    return ret;
  }
  return true;
}


/*
 * Return the description for this plugin.
 * @return a string description of the plugin
 */
string KiNetPlugin::Description() const {
    return
"KiNET Plugin\n"
"----------------------------\n"
"\n"
"This plugin creates a single device with multiple output ports. Each port\n"
"represents a power supply. This plugin uses the V1 DMX-Out version of the\n"
"KiNET protocol\n"
"\n"
"--- Config file : ola-kinet.conf ---\n"
"\n"
"power_supply = <ip>\n"
"The IP of the power supply to send to. You can communicate with more than\n"
"one power supply by adding multiple power_supply = lines\n"
"\n";
}


/*
 * Set default preferences.
 */
bool KiNetPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences) {
    return false;
  }

  save |= m_preferences->SetDefaultValue(POWER_SUPPLY_KEY,
                                         StringValidator(true), "");

  if (save) {
    m_preferences->Save();
  }
  return true;
}
}  // namespace kinet
}  // namespace plugin
}  // namespace ola
