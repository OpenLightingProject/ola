/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * KaratePlugin.cpp
 * The Open DMX plugin for ola
 * Copyright (C) 2005-2008 Simon Newton
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
#include "plugins/karate/KarateDevice.h"
#include "plugins/karate/KaratePlugin.h"

namespace ola {
namespace plugin {
namespace karate {

using ola::PluginAdaptor;
using std::vector;

const char KaratePlugin::OPENDMX_DEVICE_PATH[] = "/dev/kldmx0";
const char KaratePlugin::OPENDMX_DEVICE_NAME[] = "KL USB Device using kl";
const char KaratePlugin::PLUGIN_NAME[] = "KL DMX";
const char KaratePlugin::PLUGIN_PREFIX[] = "karate";
const char KaratePlugin::DEVICE_KEY[] = "device";


/*
 * Start the plugin
 * TODO: scan /dev for devices?
 */
bool KaratePlugin::StartHook() {
  vector<string> devices = m_preferences->GetMultipleValue(DEVICE_KEY);
  vector<string>::const_iterator iter = devices.begin();

  // start counting device ids from 0
  unsigned int device_id = 0;

  for (; iter != devices.end(); ++iter) {
    // first check if it's there
    int fd = open(iter->c_str(), O_WRONLY);
    if (fd >= 0) {
      close(fd);
      KarateDevice *device = new KarateDevice(
          this,
          OPENDMX_DEVICE_NAME,
          *iter,
          device_id++);
      if (device->Start()) {
        m_devices.push_back(device);
        m_plugin_adaptor->RegisterDevice(device);
      } else {
        OLA_WARN << "Failed to start KLDMX for " << *iter;
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


/*
 * Return the description for this plugin
 */
string KaratePlugin::Description() const {
    return
"KL Open DMX Plugin - Version 0.3\n"
"----------------------------\n"
"\n"
"The plugin creates a single device with one output port using the Enttec\n"
"Open DMX USB widget.\n\n"
"--- Config file : ola-karate.conf ---\n"
"\n"
"device = /dev/dmx0\n"
"The path to the open dmx usb device. Multiple entries are supported.\n";
}


/*
 * Set default preferences.
 */
bool KaratePlugin::SetDefaultPreferences() {
  if (!m_preferences)
    return false;

  if (m_preferences->SetDefaultValue(DEVICE_KEY, StringValidator(),
                                     OPENDMX_DEVICE_PATH))
    m_preferences->Save();

  // check if this save correctly
  // we don't want to use it if null
  if (m_preferences->GetValue(DEVICE_KEY).empty())
    return false;

  return true;
}
}  // karate
}  // plugins
}  // ola
