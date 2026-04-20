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
 * NanoleafPlugin.cpp
 * The Nanoleaf plugin for OLA
 * Copyright (C) 2017 Peter Newman
 */

#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/network/IPV4Address.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/nanoleaf/NanoleafDevice.h"
#include "plugins/nanoleaf/NanoleafPlugin.h"
#include "plugins/nanoleaf/NanoleafPluginDescription.h"


namespace ola {
namespace plugin {
namespace nanoleaf {

using ola::network::IPV4Address;
using std::string;
using std::vector;

const char NanoleafPlugin::CONTROLLER_KEY[] = "controller";
const char NanoleafPlugin::PLUGIN_NAME[] = "Nanoleaf";
const char NanoleafPlugin::PLUGIN_PREFIX[] = "nanoleaf";

NanoleafPlugin::NanoleafPlugin(PluginAdaptor *plugin_adaptor)
    : Plugin(plugin_adaptor) {
}

NanoleafPlugin::~NanoleafPlugin() {}

/*
 * Start the plugin.
 */
bool NanoleafPlugin::StartHook() {
  vector<string> controllers_strings = m_preferences->GetMultipleValue(
      CONTROLLER_KEY);
  vector<string>::const_iterator iter = controllers_strings.begin();

  for (; iter != controllers_strings.end(); ++iter) {
    if (iter->empty()) {
      continue;
    }
    IPV4Address target;
    if (IPV4Address::FromString(*iter, &target)) {
      NanoleafDevice *device = new NanoleafDevice(this,
                                                  m_preferences,
                                                  m_plugin_adaptor,
                                                  target);

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
      OLA_WARN << "Invalid controller IP address : " << *iter;
    }
  }
  // TODO(Peter): Check we got at least one good device?
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool NanoleafPlugin::StopHook() {
  vector<NanoleafDevice*>::iterator iter = m_devices.begin();
  bool ok = true;
  for (; iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    ok &= (*iter)->Stop();
    delete *iter;
  }
  return ok;
}


/*
 * Return the description for this plugin.
 * @return a string description of the plugin
 */
string NanoleafPlugin::Description() const {
    return plugin_description;
}


/*
 * Set default preferences.
 */
bool NanoleafPlugin::SetDefaultPreferences() {
  bool save = false;

  if (!m_preferences) {
    return false;
  }

  save |= m_preferences->SetDefaultValue(CONTROLLER_KEY,
                                         StringValidator(true), "");

  if (save) {
    m_preferences->Save();
  }
  return true;
}
}  // namespace nanoleaf
}  // namespace plugin
}  // namespace ola
