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
 * ShowNetPlugin.cpp
 * The ShowNet plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#include <string>
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/shownet/ShowNetDevice.h"
#include "plugins/shownet/ShowNetPlugin.h"
#include "plugins/shownet/ShowNetPluginDescription.h"


namespace ola {
namespace plugin {
namespace shownet {

using std::string;

const char ShowNetPlugin::SHOWNET_NODE_NAME[] = "ola-ShowNet";
const char ShowNetPlugin::PLUGIN_NAME[] = "ShowNet";
const char ShowNetPlugin::PLUGIN_PREFIX[] = "shownet";
const char ShowNetPlugin::SHOWNET_NAME_KEY[] = "name";

/*
 * Start the plugin
 */
bool ShowNetPlugin::StartHook() {
  m_device = new ShowNetDevice(this, m_preferences, m_plugin_adaptor);

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
bool ShowNetPlugin::StopHook() {
  if (m_device) {
    m_plugin_adaptor->UnregisterDevice(m_device);
    bool ret = m_device->Stop();
    delete m_device;
    return ret;
  }
  return true;
}


/*
 * return the description for this plugin
 *
 */
string ShowNetPlugin::Description() const {
  return plugin_description;
}


/*
 * Set default preferences
 */
bool ShowNetPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = false;

  save |= m_preferences->SetDefaultValue(ShowNetDevice::IP_KEY,
                                         StringValidator(true), "");
  save |= m_preferences->SetDefaultValue(SHOWNET_NAME_KEY, StringValidator(),
                                         SHOWNET_NODE_NAME);

  if (save) {
    m_preferences->Save();
  }

  if (m_preferences->GetValue(SHOWNET_NAME_KEY).empty()) {
    return false;
  }
  return true;
}
}  // namespace shownet
}  // namespace plugin
}  // namespace ola
