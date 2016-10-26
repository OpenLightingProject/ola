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
 * KaratePlugin.cpp
 * The karate plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/io/IOUtils.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/karate/KarateDevice.h"
#include "plugins/karate/KaratePlugin.h"
#include "plugins/karate/PluginDescription.cpp"

namespace ola {
namespace plugin {
namespace karate {

using ola::PluginAdaptor;
using std::string;
using std::vector;

const char KaratePlugin::KARATE_DEVICE_PATH[] = "/dev/kldmx0";
const char KaratePlugin::KARATE_DEVICE_NAME[] = "KarateLight Device";
const char KaratePlugin::PLUGIN_NAME[] = "KarateLight";
const char KaratePlugin::PLUGIN_PREFIX[] = "karate";
const char KaratePlugin::DEVICE_KEY[] = "device";

/**
 * @brief Start the plugin
 */
bool KaratePlugin::StartHook() {
  vector<string> devices = m_preferences->GetMultipleValue(DEVICE_KEY);
  vector<string>::const_iterator iter = devices.begin();

  // start counting device ids from 0
  unsigned int device_id = 0;

  for (; iter != devices.end(); ++iter) {
    // first check if it's there
    int fd;
    if (ola::io::Open(*iter, O_WRONLY, &fd)) {
      close(fd);
      KarateDevice *device = new KarateDevice(
          this,
          KARATE_DEVICE_NAME,
          *iter,
          device_id++);
      if (device->Start()) {
        m_devices.push_back(device);
        m_plugin_adaptor->RegisterDevice(device);
      } else {
        OLA_WARN << "Failed to start KarateLight for " << *iter;
        delete device;
      }
    } else {
      OLA_WARN << "Could not open " << *iter << " " << strerror(errno);
    }
  }
  return true;
}


/**
 * @brief Stop the plugin
 * @return true on success, false on failure
 */
bool KaratePlugin::StopHook() {
  bool ret = true;
  DeviceList::iterator iter = m_devices.begin();
  for (; iter != m_devices.end(); ++iter) {
    m_plugin_adaptor->UnregisterDevice(*iter);
    ret &= (*iter)->Stop();
    delete *iter;
  }
  m_devices.clear();
  return ret;
}


/**
 * @brief Return the description for this plugin
 */
string KaratePlugin::Description() const {
    return plugin_description;
}


/**
 * @brief Set default preferences.
 */
bool KaratePlugin::SetDefaultPreferences() {
  if (!m_preferences) {
    return false;
  }

  if (m_preferences->SetDefaultValue(DEVICE_KEY, StringValidator(),
                                     KARATE_DEVICE_PATH)) {
    m_preferences->Save();
  }

  // check if this saved correctly
  // we don't want to use it if null
  if (m_preferences->GetValue(DEVICE_KEY).empty()) {
    return false;
  }

  return true;
}
}  // namespace karate
}  // namespace plugin
}  // namespace ola
