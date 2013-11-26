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
 * RenardPlugin.cpp
 * The Renard plugin for ola
 * Copyright (C) 2013 Hakan Lindestaf
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "olad/PluginAdaptor.h"
#include "olad/Preferences.h"
#include "plugins/renard/RenardDevice.h"
#include "plugins/renard/RenardPlugin.h"

namespace ola {
namespace plugin {
namespace renard {

using ola::PluginAdaptor;
using std::vector;

const char RenardPlugin::RENARD_DEVICE_PATH[] = "/dev/ttyUSB0";
const char RenardPlugin::RENARD_DEVICE_NAME[] = "Renard Serial Device";
const char RenardPlugin::PLUGIN_NAME[] = "Renard";
const char RenardPlugin::PLUGIN_PREFIX[] = "renard";
const char RenardPlugin::DEVICE_KEY[] = "device";


/*
 * Start the plugin
 * TODO: scan /dev for devices?
 */
bool RenardPlugin::StartHook() {
  vector<string> devices = m_preferences->GetMultipleValue(DEVICE_KEY);
  vector<string>::const_iterator iter = devices.begin();

  // start counting device ids from 0
  unsigned int device_id = 0;

  for (; iter != devices.end(); ++iter) {
    // first check if it's there
    int fd = open(iter->c_str(), O_WRONLY);
    if (fd >= 0) {
      close(fd);
      RenardDevice *device = new RenardDevice(
          this,
          RENARD_DEVICE_NAME,
          *iter,
          device_id++);
      if (device->Start()) {
        m_devices.push_back(device);
        m_plugin_adaptor->RegisterDevice(device);
      } else {
        OLA_WARN << "Failed to start RenardDevice for " << *iter;
        delete device;
      }
    } else {
      OLA_WARN << "Could not open " << *iter << " " << strerror(errno);
    }
  }
  return true;
}


/*
 * Stop the plugin
 * @return true on success, false on failure
 */
bool RenardPlugin::StopHook() {
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


/*
 * Return the description for this plugin
 */
string RenardPlugin::Description() const {
    return
"Renard Plugin\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with one output port using the Renard\n"
"widget.\n"
"\n"
"--- Config file : ola-renard.conf ---\n"
"\n"
"device = /dev/ttyUSB0\n"
"The path to the serial (usb) device that's connected to the renard board (SS24, etc). Multiple entries are supported.\n"
"\n";
}


/*
 * Set default preferences.
 */
bool RenardPlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  if (m_preferences->SetDefaultValue(DEVICE_KEY, StringValidator(),
                                     RENARD_DEVICE_PATH))
    m_preferences->Save();

  // check if this save correctly
  // we don't want to use it if null
  if (m_preferences->GetValue(DEVICE_KEY).empty())
    return false;

  return true;
}
}  // namespace renard
}  // namespace plugin
}  // namespace ola
