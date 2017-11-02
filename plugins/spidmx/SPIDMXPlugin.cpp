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
 * SPIDMXPlugin.cpp
 * This looks for possible SPI devices to instantiate and is managed by OLAD.
 * Copyright (C) 2017 Florian Edelmann
 */

#include <fcntl.h>
#include <errno.h>

#include <memory>
#include <string>
#include <vector>

#include "ola/StringUtils.h"
#include "ola/io/IOUtils.h"
#include "ola/file/Util.h"
#include "olad/Preferences.h"
#include "olad/PluginAdaptor.h"
#include "plugins/spidmx/SPIDMXPlugin.h"
#include "plugins/spidmx/SPIDMXPluginDescription.h"
#include "plugins/spidmx/SPIDMXDevice.h"

namespace ola {
namespace plugin {
namespace spidmx {

using std::string;
using std::vector;

const char SPIDMXPlugin::PLUGIN_NAME[] = "SPI native DMX";
const char SPIDMXPlugin::PLUGIN_PREFIX[] = "spidmx";
const char SPIDMXPlugin::PREF_DEVICE_PREFIX_DEFAULT[] = "spidev";
const char SPIDMXPlugin::PREF_DEVICE_PREFIX_KEY[] = "device_prefix";

/*
 * Start the plugin by finding all SPI devices.
 */
bool SPIDMXPlugin::StartHook() {
  vector<string> spi_devices;
  vector<string> spi_prefixes = m_preferences->GetMultipleValue(
      PREF_DEVICE_PREFIX_KEY);
  if (!ola::file::FindMatchingFiles("/dev", spi_prefixes, &spi_devices)) {
    return false;
  }

  vector<string>::const_iterator iter;  // iterate over devices

  for (iter = spi_devices.begin(); iter != spi_devices.end(); ++iter) {
    SPIDMXDevice *device = new SPIDMXDevice(this, m_preferences,
                                            m_plugin_adaptor, PLUGIN_NAME,
                                            *iter);

    if (!device) {
      continue;
    }

    if (!device->Start()) {
      delete device;
      continue;
    }

    m_devices.push_back(device);
    m_plugin_adaptor->RegisterDevice(device);
  }

  return true;
}

/**
 * Stop all the devices.
 */
bool SPIDMXPlugin::StopHook() {
  SPIDMXDeviceVector::iterator iter;
  for (iter = m_devices.begin(); iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    (*iter)->Stop();
    delete *iter;
  }
  m_devices.clear();
  return true;
}


/**
 * Return a description for this plugin.
 */
string SPIDMXPlugin::Description() const {
  return plugin_description;
}


/*
 * Load the plugin prefs and default to sensible values
 */
bool SPIDMXPlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  bool save = m_preferences->SetDefaultValue(PREF_DEVICE_PREFIX_KEY,
                                             StringValidator(),
                                             PREF_DEVICE_PREFIX_DEFAULT);
  if (save) {
    m_preferences->Save();
  }

  if (m_preferences->GetValue(PREF_DEVICE_PREFIX_KEY).empty()) {
    return false;
  }

  return true;
}

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
