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
#include "plugins/kinet/KiNetNode.h"
#include "plugins/kinet/KiNetPlugin.h"
#include "plugins/kinet/KiNetPluginDescription.h"


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
  m_node = new KiNetNode(m_plugin_adaptor);

  if (!m_node->Start()) {
    delete m_node;
    m_node = NULL;
    return false;
  }

  vector<string> power_supplies_strings = m_preferences->GetMultipleValue(
      POWER_SUPPLY_KEY);
  vector<string>::const_iterator iter = power_supplies_strings.begin();

  // TODO(Peter): De-duplicate PSU IPs (via a set?)
  for (; iter != power_supplies_strings.end(); ++iter) {
    if (iter->empty()) {
      continue;
    }
    IPV4Address target;
    if (IPV4Address::FromString(*iter, &target)) {
      string mode = m_preferences->GetValue(KiNetDevice::ModeKey(target));
      OLA_DEBUG << "Got mode " << mode << " for " << target;
      KiNetDevice *device = NULL;
      if (mode.compare(KiNetDevice::PORTOUT_MODE) == 0) {
        device = new KiNetPortOutDevice(this,
                                        target,
                                        m_plugin_adaptor,
                                        m_node,
                                        m_preferences);
      } else {
        device = new KiNetDmxOutDevice(this,
                                       target,
                                       m_plugin_adaptor,
                                       m_node,
                                       m_preferences);
      }

      if (!device) {
        continue;
      }

      if (!device->Start()) {
        delete device;
        continue;
      }
      m_devices.push_back(device);
      m_plugin_adaptor->RegisterDevice(device);
    } else {
      OLA_WARN << "Invalid power supply IP address: " << *iter;
    }
  }
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool KiNetPlugin::StopHook() {
  vector<KiNetDevice*>::iterator iter = m_devices.begin();
  bool ok = true;
  for (; iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    ok &= (*iter)->Stop();
    delete *iter;
  }

  ok &= m_node->Stop();
  delete m_node;
  m_node = NULL;

  return ok;
}


/*
 * Return the description for this plugin.
 * @return a string description of the plugin
 */
string KiNetPlugin::Description() const {
    return plugin_description;
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
